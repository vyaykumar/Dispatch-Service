#include "client_state_machine.h"

ClientState step (const state::InitSocket&, exec_ctx& e_ctx) {
    LOG_SCOPE("Initialising Socket");
    auto& sock = e_ctx.sock;
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == transport::kInvalidSocket) {
        const std::string error = "Socket creation failed.";
        logging::Event(error);
        e_ctx.result.error = error;
        return state::RetryDecision {};
    }

    logging::Event("Socket created.");
    return state::ConfigureAddress {};
}

ClientState step (const state::ConfigureAddress&, exec_ctx& e_ctx) {
    LOG_SCOPE("Configuring Address");
    auto& addr = e_ctx.addr;
    auto& ctx = e_ctx.ctx;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(ctx.port);
    inet_pton(AF_INET, ctx.serverAddr.c_str(), &addr.sin_addr);

    logging::Event("Address configured.");
    return state::ConfigureTimeout {};
}

ClientState step (const state::ConfigureTimeout&, exec_ctx& e_ctx) {
    LOG_SCOPE("Configuring Timeout");
    const auto& sock = e_ctx.sock;
    auto& ctx = e_ctx.ctx;

    const auto timeout_ms = ctx.timeout.count();

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
        reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms)) != 0) {
        const std::string error = "Timeout configured unsuccessfully.";
        logging::Event(error);
        e_ctx.result.error = error;
        return state::RetryDecision {};
    }
    logging::Event("Timeout configured successfully.");
    return state::Connect {};
}

ClientState step (const state::Connect&, exec_ctx& e_ctx) {
    LOG_SCOPE("Connecting");
    const auto& sock = e_ctx.sock;
    auto& addr = e_ctx.addr;

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        const std::string error = "Connection failed.";
        logging::Event(error);
        e_ctx.result.error = error;
        return state::RetryDecision {};
    }
    logging::Event("Connected.");
    return state::SubmitTask {};
}

ClientState step (const state::SubmitTask&, exec_ctx& e_ctx) {
    LOG_SCOPE("Submitting Task");
    const auto& sock = e_ctx.sock;

    const protocol::TaskSubmit submit{
        .taskId = e_ctx.ctx.task_id,
        .idempotencyKey = e_ctx.ctx.task_id+e_ctx.ctx.client_id,
        .payload = e_ctx.ctx.w_conf.payload,
    };

    if (!protocol::SendTaskSubmit(sock, submit)) {
        const std::string error = "Task submitted unsuccessfully.";
        logging::Event(error);
        e_ctx.result.error = error;
        return state::RetryDecision {};
    }

    logging::Event("Task submitted.");
    return state::WaitAck {};
}

ClientState step (const state::WaitAck&, exec_ctx& e_ctx) {
    LOG_SCOPE("Awaiting ACK");
    const auto& sock = e_ctx.sock;

    if (const auto ackMsg = protocol::ReceiveMessage(sock); !ackMsg or ackMsg->type != protocol::MessageType::kTaskAck) {
        const std::string error = "ACK validation failed.";
        logging::Event(error);
        e_ctx.result.error = error;
        return state::RetryDecision {};
    }

    logging::Event("ACK received.");
    return state::WaitResult {};
}

ClientState step (const state::WaitResult&, exec_ctx& e_ctx) {
    LOG_SCOPE("Awaiting result");
    const auto& sock = e_ctx.sock;

    const auto message = protocol::ReceiveMessage(sock);
    if (!message) {
        const std::string error = "No message received.";
        logging::Event(error);
        e_ctx.result.error = error;
        return state::RetryDecision {};
    }

    if (message->type != protocol::MessageType::kTaskResult) {
        const std::string error = "Malformed result.";
        logging::Event(error);
        e_ctx.result.error = error;
        return state::RetryDecision {};
    }

    logging::Event("Received result.");

    const std::string resultText(message->result.payload.begin(), message->result.payload.end());
    e_ctx.result.payload = resultText;
    e_ctx.result.status = message->result.status;

    if (message->result.status == protocol::TaskStatus::kSucceeded)
        return state::Success {};

    e_ctx.result.error = resultText;
    logging::Event("Task failed: " + e_ctx.result.error);
    return state::Failure {};
}

ClientState step (const state::RetryDecision&, exec_ctx& e_ctx) {
    LOG_SCOPE("Retry Handler");

    if (std::chrono::steady_clock::now() >= e_ctx.deadline) {
        const std::string error = "We are out of time.";
        logging::Event(error);
        e_ctx.result.error += " " + error;
        return state::Failure {};
    }

    if (e_ctx.retries_remaining == 0) {
        const std::string error = "No retries left.";
        logging::Event(error);
        e_ctx.result.error += " " + error;
        return state::Failure {};
    }

    --e_ctx.retries_remaining;
    ++e_ctx.result.retry_count;

    // logging::Event("Retrying cause of:" + std::to_string(e_ctx.result));
    logging::Event("Retries left: " + std::to_string(e_ctx.retries_remaining));

    return state::CloseSocket {};
}

ClientState step (const state::CloseSocket&, exec_ctx& e_ctx) {
    LOG_SCOPE("Closing socket.");
    transport::CloseSocket(e_ctx.sock);
    logging::Event("Socket closed.");
    return state::BackOff {};
}

ClientState step (const state::BackOff&, exec_ctx& e_ctx) {
    const auto delay = rando::Delay(
        e_ctx.ctx.w_conf.retry_backoff_ms.first,
        e_ctx.ctx.w_conf.retry_backoff_ms.second );
    LOG_SCOPE("Backing Off for " + std::to_string(delay.count()) + "ms.");
    std::this_thread::sleep_for(delay);
    logging::Event("Back again.");
    return state::InitSocket {};
}

void step (const state::Success&, exec_ctx& e_ctx) {
    LOG_SCOPE("Succeeded");
    transport::CloseSocket(e_ctx.sock);
    logging::Event("Socket closed.");

    e_ctx.result.success = true;
    e_ctx.result.error.clear();
}

void step (const state::Failure&, exec_ctx& e_ctx){
    LOG_SCOPE("Failed");
    transport::CloseSocket(e_ctx.sock);
    logging::Event("Socket closed.");

    e_ctx.result.success = false;
}

Result RunStateMachine(const Context& ctx)
{
    LOG_SCOPE(ctx.client_id);
    exec_ctx e_ctx {
        .ctx = ctx,
        .result {
            .task_id = ctx.task_id,
            .client_id = ctx.client_id
        },
        .retries_remaining = ctx.max_retries,
        .start =
            std::chrono::steady_clock::now(),
        .deadline =
            std::chrono::steady_clock::now()
            + ctx.global_timeout
    };

    ClientState current = state::InitSocket {};

    while (true)
    {
        bool terminal = false;

        std::visit([&]<typename T0>(const T0& current_state) {
            using StateType = std::decay_t<T0>;

            if constexpr (std::is_same_v<StateType, state::Success>) {
                step(current_state, e_ctx);
                e_ctx.result.success = true;
                terminal = true;
            }
            else if constexpr (std::is_same_v<StateType, state::Failure>) {
                step(current_state, e_ctx);
                e_ctx.result.success = false;
                terminal = true;
            }
            else {
                current = step(current_state, e_ctx);
            }
        }, current);

        e_ctx.result.exe_time = duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - e_ctx.start).count();

        if (terminal)
            return e_ctx.result;
    }
}