#include <bits/stdc++.h>

#include "protocol.h"
#include "transport.h"
#include "Task_Registry.h"
#include "defer.h"
using transport::socket_t;

namespace {

    // Magic Number
    constexpr uint16_t kPort = 50051;

    enum class ErrorStates {
        MalformedMessage,
        AnomalousMessage,
        ACKFailure,
        JobExists
    };


    // Functions yonder. void* is a placeholder.

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

        return std::move(message.value());
    }

    // 2. void* recSub                - Protocol Handshake: Send the ACK.
    std::expected <void, ErrorStates> SendACK (const socket_t client, const protocol::TaskId& taskID) {
        // Quick return, in case of failure.
        if (!protocol::SendTaskAck(client, {taskID})) {
            std::println ("[worker]: ACK Failure. Rejected.");
            return std::unexpected(ErrorStates::ACKFailure);
        }
        std::println("[worker]: ACK sent successfully.");
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

    protocol::TaskResult work(const protocol::TaskId &taskID) {
        // We know that there exists only one copy of the job.
        // execute_function(function);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::string resPayload {"[worker]: Task(" + taskID + ") executed."};
        task_registry::TaskRegistry task_reg;

        const protocol::TaskResult res {
            .taskId = taskID,
            .status = protocol::TaskStatus::kSucceeded,
            .payload = std::vector<uint8_t>(resPayload.begin(), resPayload.end())
        };

        task_reg.mark_complete(taskID, res);
        return std::move(res);
    }

    std::expected<protocol::TaskResult, ErrorStates> Process (const protocol::TaskId& taskID) {
        task_registry::TaskRegistry task_reg;
        using Action = task_registry::Action;

        // Check for task in Task_Registry.
        std::cout << "Checking for Task(" << taskID << ").\n";

        // `action` tells us what to do.
        switch (auto [action, result] = task_reg.try_claim(taskID); action) {
            case Action::Reject  : return std::unexpected (ErrorStates::JobExists);
            case Action::Execute : return std::move (work(taskID));
            case Action::Cached  : return std::move (result.value());
        }
    }

    // 5. void* SendResult            - Response: Build and send the TASK_RESULT, close the socket, flip done.

    std::expected<void, ErrorStates> SendResult (const socket_t client, const protocol::TaskResult& result) {
        if (protocol::SendTaskResult(client, result))
            std::println ("[worker]: Result successfully sent.");
        else
            std::println ("[worker]: Result for taskID(",result.taskId,") couldn't be sent.");
        std::println("[worker]: Terminating.");
    }


    void HandleConnection(std::stop_token stopToken, const socket_t client, std::atomic_bool& done) {
        // Preprocessor RAII magic.
        defer(transport::CloseSocket(client));
        defer(done.store(true));

        // Function starts here.
        std::println("[worker]: Thread started.");

        const auto message = ReceiveMessage(client).and_then(std::bind_front(SubmitACK, std::ref(client)));
        if (!message)
            return;

        if (const auto result = Process(message.value().submit.taskId).and_then(std::bind_front(SendResult, client)); !result)
            return;

        std::println("Thread finished.");
    }
}
