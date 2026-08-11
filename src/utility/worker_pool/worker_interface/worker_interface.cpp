// Created by VyayKumar on 10-08-2026.

#include "worker_interface.h"

#include <functional>
#include <string>
#include <string_view>

#include "../worker_pool.h"
#include "../../Task_Registry.h"
#include "../../defer.h"
#include "../../scope_logger/scope_logger.h"
#include "../../workload/workload.h"

namespace worker {
    using transport::socket_t;

    namespace {
        task_registry::TaskRegistry g_registry;

        enum class ErrorStates {
            MalformedMessage,
            AnomalousMessage,
            ACKFailure,
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

        protocol::TaskResult Rejected (const protocol::TaskId& task_id) {
            std::string payload = "Task exists.";
            return {
                .taskId = task_id,
                .status = protocol::TaskStatus::kInProgress,
                .payload = { payload.begin(), payload.end() }
            };
        }

        std::expected<void, ErrorStates> SendResult (const socket_t client, const protocol::TaskResult& result) {
            LOG_SCOPE("Sending Result");

            if (protocol::SendTaskResult(client, result))
                logging::Event("Result successfully sent.");
            else
                logging::Event("Result for taskID(" + result.taskId + ") couldn't be sent.");

            return {};
        }

        protocol::TaskResult Execute(const protocol::TaskId &taskID, const work_l::Workload& workload, const worker_pool::Profile& profile) {
            LOG_SCOPE("Executing");

            work_l::Config conf {};

            auto temp1 = std::string("Worker speed class: ");
            temp1.append(worker_pool::getSpeed(profile.speed));
            logging::Event(temp1);

            logging::Event("Worker speed factor: "+ std::to_string(profile.duration_factor));

            switch (workload) {
                case work_l::Workload::SlowSuccess:
                    conf.duration = std::chrono::round<std::chrono::milliseconds>
                        (std::chrono::milliseconds(2000)*profile.duration_factor);
                    conf.type = workload;
                    break;
                case work_l::Workload::FastSuccess:
                case work_l::Workload::ImmediateFailure:
                    conf.duration = std::chrono::round<std::chrono::milliseconds>
                        (std::chrono::milliseconds(0)*profile.duration_factor);
                    conf.type = workload;
                    break;
                case work_l::Workload::DelayedFailure:
                    conf.duration = std::chrono::round<std::chrono::milliseconds>
                        (std::chrono::milliseconds(2000)*profile.duration_factor);
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

        std::expected<protocol::TaskResult, ErrorStates> Process (const protocol::TaskId& taskID, const work_l::Workload workload, const worker_pool::Profile& profile) {
            LOG_SCOPE("Processing");

            using Action = task_registry::Action;

            logging::Event("Checking for Task(" + taskID + ").");
            switch (auto [action, result] = g_registry.try_claim(taskID); action) {
                // case Action::Reject  : logging::Event("Registry decision: Reject.");  return std::unexpected (ErrorStates::JobExists);
                case Action::Reject  : logging::Event("Registry decision: Reject.");  return std::move (Rejected(taskID));
                case Action::Execute : logging::Event("Registry decision: Execute."); return std::move (Execute(taskID, workload, profile));
                case Action::Cached  : logging::Event("Registry decision: Cached.");  return std::move (result.value());
            }
            return {};
        }
    }

    void HandleConnection(const socket_t client, worker_pool::Profile profile) {
        LOG_SCOPE("Connection Handler");

        defer(transport::CloseSocket(client));

        const auto message = ReceiveMessage(client).and_then(std::bind_front(SubmitACK, client));
        if (!message)
            return;

        auto result = Process(message.value().submit.taskId, message->submit.workload, profile).and_then(std::bind_front(SendResult, client));
    }
}