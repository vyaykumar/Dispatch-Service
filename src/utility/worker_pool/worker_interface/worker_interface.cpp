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
        task_registry::TaskRegistry task_registry;

        enum class ErrorStates {
            MalformedMessage,
            AnomalousMessage,
            ACKFailure,
        };

        std::expected<protocol::DecodedMessage, ErrorStates> ReceiveMessage (const socket_t socket) {
            LOG_SCOPE("Receiving Message");
            auto message = protocol::ReceiveMessage(socket);

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

        std::expected <void, ErrorStates> SendACK (const socket_t socket, const protocol::TaskId& t_id) {
            LOG_SCOPE("Sending ACK");

            if (!protocol::SendTaskAck(socket, {t_id})) {
                logging::Event ("ACK Failure.");
                return std::unexpected(ErrorStates::ACKFailure);
            }
            logging::Event("ACK sent.");
            return {};
        }

        // Monadic chain function
        std::expected<protocol::DecodedMessage, ErrorStates> SubmitACK(const socket_t socket, protocol::DecodedMessage message)
        {
            return SendACK(socket,message.submit.taskId)
            .transform([msg = std::move(message)]() mutable
                { return std::move(msg); }
            );
        }

        protocol::TaskResult Rejected (const protocol::TaskId& t_id) {
            std::string payload = "Task exists.";
            return {
                .t_id = t_id,
                .status = protocol::TaskStatus::kInProgress,
                .payload = { payload.begin(), payload.end() }
            };
        }

        std::expected<void, ErrorStates> SendResult (const socket_t socket, const protocol::TaskResult& result) {
            LOG_SCOPE("Sending Result");

            if (protocol::SendTaskResult(socket, result))
                logging::Event("Result successfully sent.");
            else
                logging::Event("Result for taskID(" + result.t_id + ") couldn't be sent.");

            return {};
        }

        protocol::TaskResult Execute(const protocol::TaskId &taskID, const work_l::Workload& workload, const worker_pool::Profile& profile) {
            LOG_SCOPE("Executing");

            work_l::Config config {};

            logging::Event("Worker speed class: " + std::string(worker_pool::getSpeed(profile.speed)));
            logging::Event("Worker speed factor: "+ std::to_string(profile.duration_factor));

            switch (workload) {
                case work_l::Workload::SlowSuccess:
                    config.duration = std::chrono::round<std::chrono::milliseconds>
                        (std::chrono::milliseconds(2000)*profile.duration_factor);
                    config.type = workload;
                    break;
                case work_l::Workload::FastSuccess:
                case work_l::Workload::ImmediateFailure:
                    config.duration = std::chrono::round<std::chrono::milliseconds>
                        (std::chrono::milliseconds(0)*profile.duration_factor);
                    config.type = workload;
                    break;
                case work_l::Workload::DelayedFailure:
                    config.duration = std::chrono::round<std::chrono::milliseconds>
                        (std::chrono::milliseconds(2000)*profile.duration_factor);
                    config.type = workload;
                    break;
                case work_l::Workload::RandomChance:
                case work_l::Workload::RandomDelay:
                    config.type = workload;
                    break;
            }

            auto res = work_l::ExecuteWorkload(config, taskID);
            task_registry.markComplete(taskID, res);

            return res;
        }

        std::expected<protocol::TaskResult, ErrorStates> Process (const protocol::TaskId& t_id, const work_l::Workload workload, const worker_pool::Profile& profile) {
            LOG_SCOPE("Processing");

            using Action = task_registry::Action;

            logging::Event("Checking for Task(" + t_id + ").");
            switch (auto [action, result] = task_registry.tryClaim(t_id); action) {
                // case Action::Reject  : logging::Event("Registry decision: Reject.");  return std::unexpected (ErrorStates::JobExists);
                case Action::Reject  : logging::Event("Registry decision: Reject.");  return Rejected(t_id);
                case Action::Execute : logging::Event("Registry decision: Execute."); return Execute(t_id, workload, profile);
                case Action::Cached  : logging::Event("Registry decision: Cached.");  return result.value();
            }
            return {};
        }
    }

    void HandleConnection(const socket_t socket, const worker_pool::Profile& profile) {
        LOG_SCOPE("Connection Handler");

        defer(transport::CloseSocket(socket));

        const auto message = ReceiveMessage(socket).and_then(std::bind_front(SubmitACK, socket));
        if (!message)
            return;

        if (const auto result =
            Process(message.value().submit.taskId, message->submit.workload, profile)
            .and_then(std::bind_front(SendResult, socket));
            !result)
            logging::Event("Failed to send result.");
    }
}