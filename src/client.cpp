#include<bits/stdc++.h>
#include "protocol.h"
#include "transport.h"

// Refactoring Begins.

namespace {
    constexpr DWORD timeoutMs = 1500; // for Windows
    constexpr uint16_t kPort = 50051;

    // Following 2 are to be a context struct that is passed around. definitely.
    transport::socket_t sock {};
    sockaddr_in serv_addr{};

    enum class DispatchError {
        SockCreation_Failure,
        ServAddress_Failure,
        TimeoutConf_Failure,
        Conn_Failure,
        TaskSub_Failure,
        ACK_Failure,
        TimeOut,
        Receive_Failure,
        Unexpected_Message
    };
}

// Socket initialization.
[[nodiscard]] std::expected<void, DispatchError>
initSock () {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    // THis can fail. But where to put the check?
    // Maybe return a bool, but that will need a check too?
    // Check here instead of cluttering the disp_func

    if (sock == transport::kInvalidSocket)
        // std::cout << "[dispatcher] socket creation failed\n";
        // Output will be handled in main();
        return std::unexpected(DispatchError::SockCreation_Failure);

    std::cout << "[dispatch]: Socket initialized.\n";
    return {};
}

// Populating server address.
[[nodiscard]] std::expected<void, DispatchError>
initServAddr () {
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(kPort);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // Forgot the error code for this.
    // if (serv_addr == transport::) {
    //     return true;
    // }

    // There are no failure states here.
    std::cout << "[dispatch]: Server Address initialized.\n";
    return {};
}

// Enforcing timeout.
[[nodiscard]] std::expected <void, DispatchError>
initTimeOut () {
    const auto flag = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
        reinterpret_cast<const char *>(&timeoutMs), sizeof(timeoutMs));

    if (flag != 0)
        return std::unexpected(DispatchError::TimeoutConf_Failure);

    std::cout << "[dispatch]: Receive_timeout set.\n";
    return {};
}

// Starting connection.
[[nodiscard]] std::expected <void, DispatchError>
startConn() {
    if (connect(sock, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) != 0)
        return std::unexpected(DispatchError::Conn_Failure);

    std::cout << "[dispatch]: Connection established.\n";
    return {};
}

// Submitting task.
[[nodiscard]] std::expected<void, DispatchError>
submitTask () {
    const protocol::TaskSubmit submit{
        .taskId = "task-0001",
        .idempotencyKey = "idem-key-abc",
        .payload = std::vector<uint8_t>{'h', 'e', 'l', 'l', 'o'}
    };

    if (!protocol::SendTaskSubmit(sock, submit))
        return std::unexpected(DispatchError::TaskSub_Failure);

    std::cout << "[dispatch]: Task submitted successfully.\n";
    return {};
}

// Awaiting ACK.
[[nodiscard]] std::expected<protocol::TaskAck, DispatchError>
recAck () {
    // Do we propagate std::expected to receiveMessage too?
    // Change type in the namespace, if yes.
    auto ackMsg = protocol::ReceiveMessage(sock);

    if (!ackMsg or ackMsg->type != protocol::MessageType::kTaskAck)
        // Should we return separate error enums for non-arrival, and unexpected arrival?
        return std::unexpected(DispatchError::ACK_Failure);

    std::cout << "[dispatch]: TaskID(" << ackMsg->ack.taskId << ") acknowledged.\n";

    return ackMsg->ack;
}

// Awaiting Message.
[[nodiscard]] std::expected<protocol::TaskResult, DispatchError>
recMes () {
    auto resultMsg = protocol::ReceiveMessage(sock);

    // Check if Message is empty.
    if (!resultMsg) {
        // Cause of timeout.
        if (WSAGetLastError() == WSAETIMEDOUT) {
            std::cout << "[dispatch]: Connection timed out.\n";
            return std::unexpected(DispatchError::TimeOut);
        }
        // Any other reason.
        std::cout << "[dispatch]: Failed to receive message.\n";
        return std::unexpected(DispatchError::Receive_Failure);
    }

    if (resultMsg->type != protocol::MessageType::kTaskResult) {
        std::cout << "[dispatch]: Expected TASK_RESULT. Received else.\n";
        return std::unexpected(DispatchError::Unexpected_Message);
    }

    const std::string resultText(resultMsg->result.payload.begin(), resultMsg->result.payload.end());
    std::cout << "[dispatch]: received TASK_RESULT: status("
              << static_cast<int>(resultMsg->result.status)
              << "). Payload: \"" << resultText << "\"\n";

    return resultMsg->result;
}


void close_sock() {
    std::cout << "[dispatch]: Socket is terminated.\n\n";
    transport::CloseSocket(sock);
}
// Dispatcher.
[[nodiscard]] std::expected<protocol::TaskResult, DispatchError>
dispatch_once () {

    if (auto result = initSock(); !result)
        return std::unexpected(result.error());

    if (auto result = initServAddr(); !result)
        return std::unexpected(result.error());

    if (auto result = initTimeOut(); !result)
        return std::unexpected(result.error());

    if (auto result = startConn(); !result)
        return std::unexpected(result.error());

    if (auto result = submitTask(); !result)
        return std::unexpected(result.error());

    if (auto result = recAck(); !result)
        return std::unexpected(result.error());

    auto result = recMes();
    if (!result)
        return std::unexpected(result.error());

    return result.value();
}

[[nodiscard]] std::expected<protocol::TaskResult, DispatchError>
dispatch () {
    auto result {dispatch_once()};

    if (!result && result.error() == DispatchError::TimeOut) {
        close_sock();
        std::cout << "[dispatch]: First Retry.\n";
        return dispatch_once();
    }
    return result;
}

int main() {
    std::cout << "[main]: Client starting\n";
    transport::PlatformInit();

    // Switch for the failure states. // We don't need this now. errors are printed out inside the functions themselves.
    if (auto failure = dispatch())
        std::cout << "[main]: Protocol works.\n";
    else
        std::cout << "[main]: Protocol failed.\n";

    close_sock();
    transport::PlatformCleanup();
}