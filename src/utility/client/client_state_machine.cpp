#include "client_state_machine.h"
#include "../scope_logger/scope_logger.h"
#include "../random/random_utils.h"

ClientState step (const state::InitSocket&, StateContext& state_context) {
    LOG_SCOPE("Initialising Socket");

    auto& socket_ = state_context.socket;
    socket_ = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_ == transport::kInvalidSocket) {
        const std::string error = "Socket creation failed.";
        logging::Event(error);
        state_context.result.error = error;
        return state::RetryDecision {};
    }

    logging::Event("Socket created.");
    return state::ConfigureAddress {};
}

ClientState step (const state::ConfigureAddress&, StateContext& state_context) {
    LOG_SCOPE("Configuring Address");

    auto& address = state_context.address;
    auto& context = state_context.context;

    address.sin_family = AF_INET;
    address.sin_port = htons(context.port);
    inet_pton(AF_INET, context.server_address.c_str(), &address.sin_addr);

    logging::Event("Address configured.");
    return state::ConfigureTimeout {};
}

ClientState step (const state::ConfigureTimeout&, StateContext& state_context) {
    LOG_SCOPE("Configuring Timeout");

    const auto& socket_ = state_context.socket;
    auto& context = state_context.context;
    const auto timeout_ms = context.timeout.count();

    if (setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO,
        reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms)) != 0) {
        const std::string error = "Timeout configured unsuccessfully.";
        logging::Event(error);
        state_context.result.error = error;
        return state::RetryDecision {};
    }

    logging::Event("Timeout configured successfully.");
    return state::Connect {};
}

ClientState step (const state::Connect&, StateContext& state_context) {
    LOG_SCOPE("Connecting");

    const auto& socket_ = state_context.socket;
    auto& address = state_context.address;

    if (connect(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        logging::Event(state_context.result.error = "Connection failed.");
        return state::RetryDecision {};
    }

    logging::Event("Connected.");
    return state::SubmitTask {};
}

ClientState step (const state::SubmitTask&, StateContext& state_context) {
    LOG_SCOPE("Submitting Task");

    const auto& socket_ = state_context.socket;

    const protocol::TaskSubmit t_submit{
        .t_id = state_context.context.task_id,
        .idempotency_key = state_context.context.task_id+state_context.context.client_id,
        .payload = state_context.context.workload_config.payload,
    };

    if (!protocol::SendTaskSubmit(socket_, t_submit)) {
        logging::Event(state_context.result.error = "Task submitted unsuccessfully.");
        return state::RetryDecision {};
    }

    logging::Event("Task submitted.");
    return state::WaitAck {};
}

ClientState step (const state::WaitAck&, StateContext& state_context) {
    LOG_SCOPE("Awaiting ACK");

    const auto& socket_ = state_context.socket;

    if (const auto m_ack = protocol::ReceiveMessage(socket_);
        !m_ack or m_ack->type != protocol::MessageType::kTaskAck) {
        logging::Event(state_context.result.error = "ACK validation failed.");
        return state::RetryDecision {};
    }

    logging::Event("ACK received.");
    return state::WaitResult {};
}

ClientState step (const state::WaitResult&, StateContext& state_context) {
    LOG_SCOPE("Awaiting result");

    const auto& socket_ = state_context.socket;
    const auto message = protocol::ReceiveMessage(socket_);

    if (!message) {
        logging::Event(state_context.result.error = "Connection timed-out.");
        return state::RetryDecision {};
    }

    if (message->type != protocol::MessageType::kTaskResult) {
        logging::Event(state_context.result.error = "Malformed result.");
        return state::RetryDecision {};
    }

    logging::Event("Received result.");

    state_context.result.payload.assign(
        message->t_result.payload.begin(), message->t_result.payload.end());
    state_context.result.status = message->t_result.t_status;

    if (message->t_result.t_status == protocol::TaskStatus::kSucceeded)
        return state::Success {};

    if (message->t_result.t_status == protocol::TaskStatus::kInProgress) {
        logging::Event("Task already in progress.");
        return state::RetryDecision {};
    }

    state_context.result.error = state_context.result.payload;

    logging::Event("Task failed: " + state_context.result.error);
    return state::Failure {};
}

ClientState step (const state::RetryDecision&, StateContext& state_context) {
    LOG_SCOPE("Retry Handler");

    if (std::chrono::steady_clock::now() >= state_context.deadline) {
        logging::Event(state_context.result.error += " And out of execution time.");
        return state::Failure {};
    }

    if (state_context.retries_remaining == 0) {
        logging::Event(state_context.result.error += " And no retries left.");
        return state::Failure {};
    }

    --state_context.retries_remaining;
    ++state_context.result.retry_count;

    logging::Event("Retries left: " + std::to_string(state_context.retries_remaining));

    return state::CloseSocket {};
}

ClientState step (const state::CloseSocket&, const StateContext& state_context) {
    LOG_SCOPE("Closing socket.");
    transport::CloseSocket(state_context.socket);
    logging::Event("Socket closed.");
    return state::BackOff {};
}

ClientState step (const state::BackOff&, const StateContext& state_context) {
    const auto delay = random_utils::Delay(state_context.context.workload_config.retry_backoff_ms);
    LOG_SCOPE("Backing Off for " + std::to_string(delay.count()) + "ms.");

    std::this_thread::sleep_for(delay);
    logging::Event("Back again.");

    return state::InitSocket {};
}

void step (const state::Success&, StateContext& state_context) {
    LOG_SCOPE("Succeeded.");

    transport::CloseSocket(state_context.socket);
    logging::Event("Socket closed.");

    state_context.result.success = true;
    state_context.result.error.clear();
}

void step (const state::Failure&, StateContext& state_context) {
    LOG_SCOPE("Failed.");

    transport::CloseSocket(state_context.socket);
    logging::Event("Socket closed.");

    state_context.result.success = false;
}

Result RunStateMachine(const Context& context) {
    LOG_SCOPE(context.client_id);

    StateContext state_context {
        .context = context,
        .result {
            .task_id = context.task_id,
            .client_id = context.client_id
        },
        .retries_remaining = context.max_retries,
        .start =
            std::chrono::steady_clock::now(),
        .deadline =
            std::chrono::steady_clock::now()
            + context.global_timeout
    };

    ClientState current = state::InitSocket {};

    while (true)
    {
        bool terminal = false;

        std::visit([&]<typename T0>(const T0& current_state) {
            using StateType = std::decay_t<T0>;

            if constexpr (std::is_same_v<StateType, state::Success>) {
                step(current_state, state_context);
                state_context.result.success = true;
                terminal = true;
            }

            else if constexpr (std::is_same_v<StateType, state::Failure>) {
                step(current_state, state_context);
                state_context.result.success = false;
                terminal = true;
            }

            else
                current = step(current_state, state_context);

        }, current);

        state_context.result.execution_time = duration_cast<std::chrono::milliseconds>
            (std::chrono::steady_clock::now() - state_context.start).count();

        if (terminal)
            return state_context.result;
    }
}