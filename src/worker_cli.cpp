#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "utility/Wire/protocol.h"
#include "utility/Task_Registry.h"

#include "utility/scope_logger/scope_logger.h"
#include "utility/worker_pool/worker_interface/worker_interface.h"

using transport::socket_t;

namespace {
    constexpr uint16_t kPort = 50051;

    void RunWorker() {
        const socket_t sock = socket (AF_INET, SOCK_STREAM, 0);

        if (sock == transport::kInvalidSocket) {
            logging::Event("Dead Socket. Terminating.");
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
            logging::Event("Binding failed.");
            return;
        }

        flag = listen(sock, SOMAXCONN);
        if (flag) {
            logging::Event("Listening failed.");
            return;
        }

        while (true) {
            socket_t client = accept(sock, nullptr, nullptr);

            logging::Event("Connection accepted.");

            if (client == transport::kInvalidSocket)
                continue;

            std::jthread(worker::HandleConnection, client).detach();
        }
    }
}

int main() {
    transport::PlatformInit();

    RunWorker();

    transport::PlatformCleanup();
    return 0;
}
