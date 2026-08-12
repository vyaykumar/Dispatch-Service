#include <expected>
#include <iostream>
#include <string>
#include <vector>

#include "utility/Wire/protocol.h"
#include "utility/Wire/transport.h"

namespace {
    constexpr DWORD timeoutMs = 1500; // for Windows
    constexpr uint16_t kPort = 50051;

    struct DispatchContext {
        transport::socket_t socket_ {};
        sockaddr_in server_address{};
    };

    enum class DispatchError {
        kSockCreation_Failure,
        kServAddress_Failure,
        kTimeoutConf_Failure,
        kConn_Failure,
        kTaskSub_Failure,
        kACK_Failure,
        kTimeOut,
        kReceive_Failure,
        kUnexpected_Message
    };
}

// Socket initialization.
[[nodiscard]] std::expected<void, DispatchError>
initSock (DispatchContext& context) {
    auto& [socket_,_] = context;

    socket_ = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_ == transport::kInvalidSocket)
        return std::unexpected(DispatchError::kSockCreation_Failure);

    std::cout << "[dispatch]: Socket initialized.\n";
    return {};
}

// Populating server address.
[[nodiscard]] std::expected<void, DispatchError>
initServAddr (DispatchContext& context) {
    auto& [_,server_address] = context;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(kPort);
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

    std::cout << "[dispatch]: Server Address initialized.\n";
    return {};
}

// Enforcing timeout.
[[nodiscard]] std::expected <void, DispatchError>
initTimeOut (const DispatchContext &context) {
    auto [socket_, _] = context;

    const auto flag = setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO,
        reinterpret_cast<const char *>(&timeoutMs), sizeof(timeoutMs));

    if (flag != 0)
        return std::unexpected(DispatchError::kTimeoutConf_Failure);

    std::cout << "[dispatch]: Receive_timeout set.\n";
    return {};
}

// Starting connection.
[[nodiscard]] std::expected <void, DispatchError>
startConn(const DispatchContext &context) {
    auto [socket_, server_address] = context;

    if (connect(socket_, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) != 0)
        return std::unexpected(DispatchError::kConn_Failure);

    std::cout << "[dispatch]: Connection established.\n";
    return {};
}

// Submitting task.
[[nodiscard]] std::expected<void, DispatchError>
submitTask (const DispatchContext &context) {
    auto [socket_, _] = context;
    const protocol::TaskSubmit t_submit{
        .t_id = "task-0001",
        .idempotency_key = "idem-key-abc",
        .payload = std::vector<uint8_t>{'h', 'e', 'l', 'l', 'o'}
    };

    if (!protocol::SendTaskSubmit(socket_, t_submit))
        return std::unexpected(DispatchError::kTaskSub_Failure);

    std::cout << "[dispatch]: Task submitted successfully.\n";
    return {};
}

// Awaiting ACK.
[[nodiscard]] std::expected<protocol::TaskAck, DispatchError>
recAck (const DispatchContext &context) {
    auto [socket_, _] = context;

    // Do we propagate std::expected to receiveMessage too?
    // Change type in the namespace, if yes.
    auto m_ack = protocol::ReceiveMessage(socket_);

    if (!m_ack or m_ack->type != protocol::MessageType::kTaskAck)
        return std::unexpected(DispatchError::kACK_Failure);

    std::cout << "[dispatch]: TaskID(" << m_ack->t_ack.t_id << ") acknowledged.\n";

    return m_ack->t_ack;
}

// Awaiting Message.
[[nodiscard]] std::expected<protocol::TaskResult, DispatchError>
recMes (const DispatchContext &context) {
    auto [socket_, _] = context;

    auto m_result = protocol::ReceiveMessage(socket_);

    // Check if Message is empty.
    if (!m_result) {
        // Cause of timeout.
        if (WSAGetLastError() == WSAETIMEDOUT) {
            std::cout << "[dispatch]: Connection timed out.\n";
            return std::unexpected(DispatchError::kTimeOut);
        }
        // Any other reason.
        std::cout << "[dispatch]: Failed to receive message.\n";
        return std::unexpected(DispatchError::kReceive_Failure);
    }

    if (m_result->type != protocol::MessageType::kTaskResult) {
        std::cout << "[dispatch]: Expected TASK_RESULT. Received else.\n";
        return std::unexpected(DispatchError::kUnexpected_Message);
    }

    const std::string resultText(m_result->t_result.payload.begin(), m_result->t_result.payload.end());
    std::cout << "[dispatch]: received TASK_RESULT: status("
              << static_cast<int>(m_result->t_result.t_status)
              << "). Payload: \"" << resultText << "\"\n";

    return m_result->t_result;
}

void close_sock(const DispatchContext &context) {
    auto [socket_, _] = context;

    std::cout << "[dispatch]: Socket is terminated.\n\n";
    transport::CloseSocket(socket_);
}

// Dispatcher.
[[nodiscard]] std::expected<protocol::TaskResult, DispatchError>
dispatch_once (DispatchContext& context) {

    if (auto result = initSock(context); !result)
        return std::unexpected(result.error());

    if (auto result = initServAddr(context); !result)
        return std::unexpected(result.error());

    if (auto result = initTimeOut(context); !result)
        return std::unexpected(result.error());

    if (auto result = startConn(context); !result)
        return std::unexpected(result.error());

    if (auto result = submitTask(context); !result)
        return std::unexpected(result.error());

    if (auto result = recAck(context); !result)
        return std::unexpected(result.error());

    auto result = recMes(context);
    if (!result)
        return std::unexpected(result.error());

    return result.value();
}

[[nodiscard]] std::expected<protocol::TaskResult, DispatchError>
dispatch (DispatchContext& context) {
    auto t_result {dispatch_once(context)};

    if (!t_result && t_result.error() == DispatchError::kTimeOut) {
        close_sock(context);
        std::cout << "[dispatch]: First Retry.\n";
        return dispatch_once(context);
    }
    return t_result;
}

int main() {
    std::cout << "[main]: Client starting\n";

    DispatchContext dispatch_context {};

    transport::PlatformInit();
    // Switch for the failure states. // We don't need this now. errors are printed out inside the functions themselves.
    if (auto failure = dispatch(dispatch_context))
        std::cout << "[main]: Protocol works.\n";
    else
        std::cout << "[main]: Protocol failed.\n";

    close_sock(dispatch_context);
    transport::PlatformCleanup();
}