#include <atomic>
#include <chrono>
#include <expected>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "utility/Wire/protocol.h"
#include "utility/Task_Registry.h"
#include "utility/defer.h"
#include "utility/workload/workload.h"

using transport::socket_t;

namespace {
    // Usual Stuff.
    constexpr uint16_t kPort = 50051;
    task_registry::TaskRegistry g_registry;

    enum class ErrorStates {
        MalformedMessage,
        AnomalousMessage,
        ACKFailure,
        JobExists
    };

    std::expected<protocol::DecodedMessage, ErrorStates> ReceiveMessage (const socket_t client) {
        auto message = protocol::ReceiveMessage(client);

        // Malformed Message
        if (message == std::nullopt) {
            std::cout << "[worker]: Malformed message. Rejected.\n";
            return std::unexpected(ErrorStates::MalformedMessage);
        }

        // Out-Of-Sequence
        if (message->type != protocol::MessageType::kTaskSubmit) {
            std::cout << "[worker]: Incorrect type of message received. Rejected.\n";
            return std::unexpected(ErrorStates::AnomalousMessage);
        }

        return message.value();
    }

    // 2. void* recSub                - Protocol Handshake: Send the ACK.
    std::expected <void, ErrorStates> SendACK (const socket_t client, const protocol::TaskId& taskID) {
        // Quick return, in case of failure.
        if (!protocol::SendTaskAck(client, {taskID})) {
            std::cout << "[worker]: ACK Failure. Rejected.\n";
            return std::unexpected(ErrorStates::ACKFailure);
        }
        std::cout << "[worker]: ACK sent successfully.\n";
        return {};
    }

    // Monadic chain function
    std::expected<protocol::DecodedMessage, ErrorStates> SubmitACK(const socket_t client, protocol::DecodedMessage message)
    {
        return SendACK(
            client,
            message.submit.taskId
        )
        .transform([msg = std::move(message)]() mutable
        {
            return std::move(msg);
        });
    }

    protocol::TaskResult Execute(const protocol::TaskId &taskID, const work_l::Workload& workload) {
        work_l::Config conf {};
        switch (workload) {
            case work_l::Workload::SlowSuccess:
                conf.duration = std::chrono::milliseconds(2000);
                conf.type = workload;
                break;
            case work_l::Workload::FastSuccess:
                conf.duration = std::chrono::milliseconds(0);
                conf.type = workload;
                break;
            case work_l::Workload::ImmediateFailure:
                conf.duration = std::chrono::milliseconds(0);
                conf.type = workload;
                break;
            case work_l::Workload::DelayedFailure:
                conf.duration = std::chrono::milliseconds(2000);
                conf.type = workload;
                break;
            case work_l::Workload::RandomChance:
            case work_l::Workload::RandomDelay:
                // Add random.
                conf.type = workload;
                break;
        }

        auto res = work_l::ExecuteWorkload(conf, taskID);
        g_registry.mark_complete(taskID, res);
        return res;
    }

    std::expected<protocol::TaskResult, ErrorStates> Process (const protocol::TaskId& taskID, work_l::Workload workload) {
        using Action = task_registry::Action;

        // Check for task in Task_Registry.
        std::cout << "Checking for Task(" << taskID << ").\n";

        // `action` tells us what to do.
        switch (auto [action, result] = g_registry.try_claim(taskID); action) {
            case Action::Reject  : std::cout << "REJECT\n"; return std::unexpected (ErrorStates::JobExists);
            case Action::Execute : std::cout << "EXECUTE\n"; return std::move (Execute(taskID, workload));
            case Action::Cached  : std::cout << "CACHED\n"; return std::move (result.value());
        }
        return {};
    }

    // 5. void* SendResult            - Response: Build and send the TASK_RESULT, close the socket, flip done.

    std::expected<void, ErrorStates> SendResult (const socket_t client, const protocol::TaskResult& result) {
        if (protocol::SendTaskResult(client, result))
            std::cout << "[worker]: Result successfully sent.\n";
        else
            std::cout << "[worker]: Result for taskID(" << result.taskId << ") couldn't be sent.\n";

        std::cout << "[worker]: Terminating.\n";
        return {};
    }


    void HandleConnection(const std::stop_token& stopToken, const socket_t client, std::atomic_bool& done) {
        // Preprocessor RAII magic.
        defer(transport::CloseSocket(client));
        defer(done.store(true));

        // Function starts here.
        std::cout << "[worker]: Thread started.\n";

        const auto message = ReceiveMessage(client).and_then(std::bind_front(SubmitACK, client));
        if (!message)
            return;

        if (const auto result = Process(message.value().submit.taskId, message->submit.workload).and_then(std::bind_front(SendResult, client)); !result)
            return;

        std::cout << "Thread finished.\n\n";
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
            std::erase_if(cvec, [](const std::unique_ptr<Connection>& conn) { if (conn->done.load()) std::cout << "Reaped a thread.\n"; return conn && conn->done.load(); });

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
    std::cout << "Worker is a go.";
    // TODO: call RunWorker() (or inline its contents here if you'd rather not split it out — your call).
    RunWorker();

    transport::PlatformCleanup();
    return 0;
}
