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
    void HandleConnection(socket_t client, std::stop_token stopToken) {
        auto dec_mes = protocol::ReceiveMessage(client);

        if (dec_mes == std::nullopt) {
            std::cout << "Whats's received isn't a proper message. Rejected.\n";
            transport::CloseSocket(client);
            return;
        }

        if (dec_mes->type != protocol::MessageType::kTaskSubmit) {
            std::cout << "Message Type isn't submit. Rejected.\n";
            transport::CloseSocket(client);
            return;
        }

        protocol::TaskAck ack {dec_mes->submit.taskId};
        bool flag = protocol::SendTaskAck(client, ack);

        if (flag) {
            std::cout << "Event Logger: ACK passed.\n";

            // PLACeHOLDER: Work Section.
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

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
    }

    // Sets up the listening socket and runs the accept loop.
    //
    // TODO: create a TCP socket (see main.c from Step 1's smoke test for the
    //       exact socket()/bind()/listen() calls — same pattern applies here).
    // TODO: loop calling accept(). For each accepted connection, spawn a
    //       std::jthread running HandleConnection with that client socket.
    // TODO: decide where those jthreads are kept. A std::jthread that goes out
    //       of scope immediately will block the accept loop waiting to join it
    //       (that defeats the point of one-thread-per-connection) — you need
    //       somewhere to stash them so the accept loop can keep going.
    //       (Think about what "somewhere" needs to guarantee about lifetime
    //       and thread-safety if the accept loop and a cleanup pass might
    //       touch it at different times — you don't need to solve full
    //       graceful shutdown yet, just don't paint yourself into a corner.)
    // TODO: think about what a clean exit path even looks like here for later
    //       — you don't have to build it now, but note it as you go.
    void RunWorker() {
        // your code here
    }

}  // namespace
int main() {
    transport::PlatformInit();

    // TODO: call RunWorker() (or inline its contents here if you'd rather
    // not split it out — your call).

    transport::PlatformCleanup();
    return 0;
}
