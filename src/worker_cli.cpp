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
#include "utility/worker_pool/worker_pool.h"
#include "utility/worker_pool/worker_interface/worker_interface.h"

using transport::socket_t;

namespace {
    constexpr uint16_t kPort = 50051;

    void RunWorker() {
        const socket_t socket_ = socket (AF_INET, SOCK_STREAM, 0);

        if (socket_ == transport::kInvalidSocket) {
            logging::Event("Dead Socket. Terminating.");
            return;
        }

        sockaddr_in socket_address{};
        socket_address.sin_family = AF_INET;
        socket_address.sin_port = htons(kPort);
        socket_address.sin_addr.s_addr = INADDR_ANY;

        constexpr int option {1};
        setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&option), sizeof(option));
        auto flag = bind(socket_, reinterpret_cast<sockaddr*>(&socket_address), sizeof(socket_address));

        if (flag) {
            logging::Event("Binding failed.");
            return;
        }

        flag = listen(socket_, SOMAXCONN);
        if (flag) {
            logging::Event("Listening failed.");
            return;
        }

        std::vector <size_t> profiles = {0,1,2,2};
        worker_pool::WorkerPool pool {profiles};

        // Uncomment this for the (now rectified) shutdown bug.
        // {
        //     std::vector <size_t> temp_prof = {1,1,1,1};
        //     worker_pool::WorkerPool temp_pool{temp_prof};
        //
        //     std::this_thread::sleep_for(std::chrono::seconds(1));
        // }

        while (true) {
            const socket_t client = accept(socket_, nullptr, nullptr);

            logging::Event("Connection accepted.");

            if (client == transport::kInvalidSocket)
                continue;

            pool.Enqueue( {.socket = client} );
        }
    }
}

int main() {
    transport::PlatformInit();

    RunWorker();

    transport::PlatformCleanup();
    return 0;
}
