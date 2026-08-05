

#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "workloads.h"
#include "../Wire/protocol.h"
#include "../random/random_utils.h"
#include <utility>


namespace work_l {
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
        return CreateMessage(taskID, status);
        }

        protocol::TaskStatus Chance (const double probability) {
            if (rando::Success(probability))
                return protocol::TaskStatus::kSucceeded;
            return protocol::TaskStatus::kFailed;
        }
    }

    protocol::TaskResult ExecuteWorkload (const work_l::Config &config, const protocol::TaskId &taskID) {
        switch (config.type) {
            case Workload::SlowSuccess:
                return doExecute(taskID, protocol::TaskStatus::kSucceeded, config.duration);
            case Workload::FastSuccess:
                return doExecute(taskID, protocol::TaskStatus::kSucceeded, std::nullopt);
            case Workload::ImmediateFailure:
                return doExecute(taskID, protocol::TaskStatus::kFailed, std::nullopt);
            case Workload::DelayedFailure:
                return doExecute(taskID, protocol::TaskStatus::kFailed, config.duration);
            case Workload::RandomChance:
                return doExecute(taskID, Chance(0.7), rando::Delay(1000, 5000));
            case Workload::RandomDelay:
                return doExecute(taskID, protocol::TaskStatus::kSucceeded, rando::Delay(1000, 5000));
        }
        std::unreachable();
    }

}