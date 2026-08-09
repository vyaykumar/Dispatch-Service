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
#include "utility/scope_logger/scope_logger.h"

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
        LOG_SCOPE("Receiving Message");
        auto message = protocol::ReceiveMessage(client);

        if (message == std::nullopt) {
            logging::Event ("Malformed message. Rejected.");
            return std::unexpected(ErrorStates::MalformedMessage);
        }

        if (message->type != protocol::MessageType::kTaskSubmit) {
            logging::Event ("Anomalous message received. Rejected.");
            return std::unexpected(ErrorStates::AnomalousMessage);
        }

        return message.value();
    }

    std::expected <void, ErrorStates> SendACK (const socket_t client, const protocol::TaskId& taskID) {
        LOG_SCOPE("Sending ACK");

        if (!protocol::SendTaskAck(client, {taskID})) {
            logging::Event ("ACK Failure.");
            return std::unexpected(ErrorStates::ACKFailure);
        }
        logging::Event("ACK sent.");
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
        LOG_SCOPE("Executing");

        work_l::Config conf {};

        switch (workload) {
            case work_l::Workload::SlowSuccess:
                conf.duration = std::chrono::milliseconds(2000);
                conf.type = workload;
                break;
            case work_l::Workload::FastSuccess:
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

    protocol::TaskResult Rejected (const protocol::TaskId& task_id) {
        std::string payload = "Task exists.";
        return {
            .taskId = task_id,
            .status = protocol::TaskStatus::kInProgress,
            .payload = { payload.begin(), payload.end() }
        };
    }

    std::expected<protocol::TaskResult, ErrorStates> Process (const protocol::TaskId& taskID, work_l::Workload workload) {
        LOG_SCOPE("Processing");

        using Action = task_registry::Action;

        logging::Event("Checking for Task(" + taskID + ").");
        switch (auto [action, result] = g_registry.try_claim(taskID); action) {
            // case Action::Reject  : logging::Event("Registry decision: Reject.");  return std::unexpected (ErrorStates::JobExists);
            case Action::Reject  : logging::Event("Registry decision: Reject.");  return std::move (Rejected(taskID));
            case Action::Execute : logging::Event("Registry decision: Execute."); return std::move (Execute(taskID, workload));
            case Action::Cached  : logging::Event("Registry decision: Cached.");  return std::move (result.value());
        }
        return {};
    }

    std::expected<void, ErrorStates> SendResult (const socket_t client, const protocol::TaskResult& result) {
        LOG_SCOPE("Sending Result");

        if (protocol::SendTaskResult(client, result))
            logging::Event("Result successfully sent.");
        else
            logging::Event("Result for taskID(" + result.taskId + ") couldn't be sent.");

        return {};
    }


    void HandleConnection(const std::stop_token& stopToken, const socket_t client, std::atomic_bool& done) {
        LOG_SCOPE("Connection Handler");

        defer(transport::CloseSocket(client));
        defer(done.store(true));

        const auto message = ReceiveMessage(client).and_then(std::bind_front(SubmitACK, client));
        if (!message)
            return;

        auto result = Process(message.value().submit.taskId, message->submit.workload).and_then(std::bind_front(SendResult, client));
    }

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

        struct Connection {
            std::jthread thread;
            std::atomic_bool done {false};
        };
        std::vector<std::unique_ptr<Connection>> cvec {};

        while (true) {
            std::erase_if(cvec, [](const std::unique_ptr<Connection>& conn) { if (conn->done.load()) std::cout << "├── Reaped a thread.\n"; return conn && conn->done.load(); });

            socket_t client = accept(sock, nullptr, nullptr);

            logging::Event("Connection accepted.");
            if (client == transport::kInvalidSocket)
                continue;

            auto conn = std::make_unique<Connection> ();
            conn->thread = std::jthread(HandleConnection, client, std::ref(conn->done));
            cvec.push_back(std::move(conn));
        }
    }
}

int main() {
    transport::PlatformInit();

    RunWorker();

    transport::PlatformCleanup();
    return 0;
}
