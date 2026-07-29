#include<bits/stdc++.h>
#include "protocol.h"
#include "transport.h"

namespace {
    constexpr uint16_t kPort = 50051;

}

int main() {
    std::cout << "Client starting\n";
    transport::PlatformInit();

    transport::socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == transport::kInvalidSocket) {
        std::cout << "[dispatcher] socket creation failed\n";
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(kPort);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // POSIX stuff.
    // struct timeval timeout;
    // timeout.tv_sec = 5; //seconds
    // timeout.tv_usec = 0;//microseconds

    DWORD timeoutMs = 5000; //Windows
    auto flag = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeoutMs), sizeof(timeoutMs));
    if (flag != 0) {
        std::cout << "[dispatcher] failed to set receive timeout\n";
    }

    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) != 0) {
        std::cout << "[dispatcher] connect failed\n";
        return 1;
    }

    protocol::TaskSubmit submit{
        "task-0001",
        "idem-key-abc",
        std::vector<uint8_t>{'h', 'e', 'l', 'l', 'o'}
    };
    protocol::SendTaskSubmit(sock, submit);
    std::cout << "[dispatcher] sent TASK_SUBMIT\n";

    auto ackMsg = protocol::ReceiveMessage(sock);
    if (!ackMsg or ackMsg->type != protocol::MessageType::kTaskAck) {
        std::cout << "[dispatcher] Did not receive expected TASK_ACK\n";
        return 1;
    }
    std::cout << "[dispatcher] received TASK_ACK for task_id=" << ackMsg->ack.taskId << "\n";

    auto resultMsg = protocol::ReceiveMessage(sock);

    if (!resultMsg) {
        std::cout << "WSA Error: "
                  << WSAGetLastError()
                  << '\n';
        return 1;
    }

    if (resultMsg->type != protocol::MessageType::kTaskResult) {
        std::cout << "[dispatcher] did not receive expected TASK_RESULT\n";
        return 1;
    }
    std::string resultText(resultMsg->result.payload.begin(), resultMsg->result.payload.end());
    std::cout << "[dispatcher] received TASK_RESULT: status="
              << static_cast<int>(resultMsg->result.status)
              << " payload=\"" << resultText << "\"\n";

    std::cout << "Protocol OK.\n";

    transport::CloseSocket(sock);

    transport::PlatformCleanup();
}