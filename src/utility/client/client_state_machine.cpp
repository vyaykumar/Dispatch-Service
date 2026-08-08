#include "client_state_machine.h"

ClientState step (const state::InitSocket&, exec_ctx& e_ctx) {
    LOG_SCOPE("Initialising Socket");
    auto& sock = e_ctx.sock;
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == transport::kInvalidSocket) {
        logging::Event("Socket creation failed.");
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

ClientState step (const state::ConfigureTimeout&, const exec_ctx& e_ctx) {
    LOG_SCOPE("Configuring Timeout");
    const auto& sock = e_ctx.sock;
    auto& ctx = e_ctx.ctx;

    const auto timeout_ms = ctx.timeout.count();

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
        reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms)) != 0) {
        logging::Event("Timeout configured unsuccessfully.");
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
        logging::Event("Connection failed.");
        return state::RetryDecision {};
    }
    logging::Event("Connected.");
    return state::SubmitTask {};
}

ClientState step (const state::SubmitTask&, const exec_ctx& e_ctx) {
    LOG_SCOPE("Submitting Task");
    const auto& sock = e_ctx.sock;

    const protocol::TaskSubmit submit{
        .taskId = e_ctx.ctx.task_id,
        .idempotencyKey = e_ctx.ctx.task_id+e_ctx.ctx.client_id,
        .payload = e_ctx.ctx.w_conf.payload,
    };

    if (!protocol::SendTaskSubmit(sock, submit)) {
        logging::Event("Task submitted unsuccessfully.");
        return state::RetryDecision {};
    }

    logging::Event("Task submitted.");
    return state::WaitAck {};
}

ClientState step (const state::WaitAck&, const exec_ctx& e_ctx) {
    LOG_SCOPE("Awaiting ACK");
    const auto& sock = e_ctx.sock;

    if (const auto ackMsg = protocol::ReceiveMessage(sock); !ackMsg or ackMsg->type != protocol::MessageType::kTaskAck) {
        if (WSAGetLastError() == WSAETIMEDOUT) {
            logging::Event("ACK awaited unsuccessfully.");
            return state::RetryDecision {};
        }
        logging::Event("ACK validation failed.");
        return state::RetryDecision {};
    }

    logging::Event("ACK received.");
    return state::WaitResult {};
}

ClientState step (const state::WaitResult&, const exec_ctx& e_ctx) {
    LOG_SCOPE("Awaiting result");
    const auto& sock = e_ctx.sock;

    const auto message = protocol::ReceiveMessage(sock);
    if (!message) {
        // Cause of timeout.
        if (WSAGetLastError() == WSAETIMEDOUT) {
            logging::Event("Connection timed out.");
            return state::RetryDecision {};
        }
        // Any other reason.
        logging::Event("No message received.");
        return state::RetryDecision {};
    }

    if (message->type != protocol::MessageType::kTaskResult) {
        logging::Event("Malformed result.");
        return state::RetryDecision {};
    }

    const std::string resultText(message->result.payload.begin(), message->result.payload.end());
    // std::string res = "Received TASK_RESULT: status(" + static_cast<int>(message->result.status) + "). Payload: \"" + resultText + "\"";
    logging::Event("Received result.");
    return state::Success {};
}

ClientState step (const state::RetryDecision&, const exec_ctx& e_ctx) {
    LOG_SCOPE("Retry Handler");

    if (std::chrono::steady_clock::now() >= e_ctx.deadline) {
        logging::Event("We are out of time.");
        return state::Failure {};
    }


    if (e_ctx.retries_remaining == 0) {
        logging::Event("No retries left.");
        return state::Failure {};
    }

    --e_ctx.retries_remaining;
    logging::Event("Retries left: " + std::to_string(e_ctx.retries_remaining));

    return state::CloseSocket {};
}

ClientState step (const state::CloseSocket&, const exec_ctx& e_ctx) {
    LOG_SCOPE("Closing socket.");
    transport::CloseSocket(e_ctx.sock);
    logging::Event("Socket closed.");
    return state::InitSocket {};
}

void step (const state::Success&, exec_ctx& e_ctx) {
    LOG_SCOPE("Succeeded");
}

void step (const state::Failure&, exec_ctx& e_ctx){
    LOG_SCOPE("Failed");
}