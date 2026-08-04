#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "../workloads.h"

#include <utility>

#include "../Wire/protocol.h"


namespace work {
    namespace {
        protocol::TaskResult CreateMessage (const protocol::TaskId& taskID, const protocol::TaskStatus status) {
            std::string resPayload {"[worker]: Task(" + taskID + ") executed."};

            protocol::TaskResult result {
                .taskId = taskID,
                .status = status,
                .payload = std::vector<uint8_t>(resPayload.begin(), resPayload.end())
            };
            return result;
        }

        protocol::TaskResult doExecute (const protocol::TaskId& taskID, const protocol::TaskStatus status, const std::optional<std::chrono::milliseconds> time) {
        if (time.has_value()) std::this_thread::sleep_for(time.value());
        return std::move(CreateMessage(taskID, status));
        }
    }

    protocol::TaskResult ExecuteWorkload (const Config &config, const protocol::TaskId &taskID) {
        switch (config.type) {
            case Workload::SlowSuccess:
                return doExecute(taskID, protocol::TaskStatus::kSucceeded, config.duration);
            case Workload::FastSuccess:
                return doExecute(taskID, protocol::TaskStatus::kSucceeded, std::nullopt);
            case Workload::ImmediateFailure:
                return doExecute(taskID, protocol::TaskStatus::kFailed, std::nullopt);
            case Workload::DelayedFailure:
                return doExecute(taskID, protocol::TaskStatus::kFailed, config.duration);
        }
        std::unreachable();
    }

}