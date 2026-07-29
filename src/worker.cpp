// worker.cc — standalone worker service (Step 2).
//
// Long-running process. Listens for dispatcher connections and handles
// each one on its own std::jthread: receive TASK_SUBMIT -> send TASK_ACK
// -> "execute" -> send TASK_RESULT.
//
// Reuses transport.h/protocol.h from Step 1 as-is — no changes needed
// there. This file is the only thing you're writing.

#include <iostream>
#include <thread>
#include <vector>

#include "protocol.h"
#include "transport.h"

using transport::socket_t;


namespace {

    constexpr uint16_t kPort = 50051;

    // Handles exactly one connection, end to end, on its own thread.
    void HandleConnection(std::stop_token stopToken, socket_t client, std::atomic_bool& done) {
        std::cout << "Thread started\n";
        auto dec_mes = protocol::ReceiveMessage(client);

        if (dec_mes == std::nullopt) {
            std::cout << "Whats's received isn't a proper message. Rejected.\n";
            transport::CloseSocket(client);
            done.store(true);
            return;
        }

        if (dec_mes->type != protocol::MessageType::kTaskSubmit) {
            std::cout << "Message Type isn't submit. Rejected.\n";
            transport::CloseSocket(client);
            done.store(true);
            return;
        }

        protocol::TaskAck ack {dec_mes->submit.taskId};
        bool flag = protocol::SendTaskAck(client, ack);

        if (flag) {
            std::cout << "Event Logger: ACK passed.\n";

            // PLACeHOLDER: Work Section.
            std::this_thread::sleep_for(std::chrono::seconds(10));

            std::string testInput {"done: Hello."};
            protocol::TaskResult res {  dec_mes->submit.taskId,
                                        protocol::TaskStatus::kSucceeded,
                                        std::vector<uint8_t>(testInput.begin(), testInput.end())};
            flag = protocol::SendTaskResult(client, res);
            if (flag)
                std::cout << "Event Logger: Result sending passed. Terminating connection.\n";
            else
                std::cout << "Event Logger: Result sending failed. Terminating connection.\n";
        }
        else {
            std::cout << "Event Logger: Ack failed. Skipping Result. Terminating connection.\n";
        }

        transport::CloseSocket(client);

        std::cout << "Thread finished\n";
        done.store(true);
    }

    void RunWorker() {
        socket_t sock = socket (AF_INET, SOCK_STREAM, 0);

        if (sock == transport::kInvalidSocket) {
            std::cout << "Dead Socket. Terminating.\n";
            return;
        }

        sockaddr_in cAddr{};
        cAddr.sin_family = AF_INET;
        cAddr.sin_port = htons(kPort);
        cAddr.sin_addr.s_addr = INADDR_ANY;

        int opt {1};
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&opt), sizeof(opt));
        auto flag = bind(sock, reinterpret_cast<sockaddr*>(&cAddr), sizeof(cAddr));

        if (flag) {
            std::cout << "Event Logger: Binding failed.\n";
            return;
        }

        flag = listen(sock, SOMAXCONN);
        if (flag) {
            std::cout << "Event Logger: Listening failed.\n";
            return;
        }

        struct Connection {
            std::jthread thread;
            std::atomic_bool done {false};
        };
        std::vector<std::unique_ptr<Connection>> cvec {};

        while (true) {
            // Sweep
            std::erase_if(cvec, [](const std::unique_ptr<Connection>& conn) { std::cout << "Reaped a thread.\n"; return conn && conn->done.load(); });

            // Accept
            socket_t client = accept(sock, nullptr, nullptr);

            std::cout << "Connection accepted.\n";
            if (client == transport::kInvalidSocket)
                continue;

            auto conn = std::make_unique<Connection> ();
            conn->thread = std::jthread(HandleConnection, client, std::ref(conn->done));
            cvec.push_back(std::move(conn));
        }
    }

}  // namespace
int main() {
    transport::PlatformInit();

    // TODO: call RunWorker() (or inline its contents here if you'd rather not split it out — your call).
    RunWorker();

    transport::PlatformCleanup();
    return 0;
}
