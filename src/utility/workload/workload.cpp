#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "workload.h"
#include "../Wire/protocol.h"
#include "../random/random_utils.h"
#include <utility>


namespace workload {
    namespace {
        protocol::TaskResult CreateMessage (const protocol::T_ID& task_id, const protocol::TaskStatus t_status) {
            std::string resPayload {task_id + " executed."};

            protocol::TaskResult result {
                .task_id = task_id,
                .status = t_status,
                .payload = std::vector<uint8_t>(resPayload.begin(), resPayload.end())
            };
            return result;
        }

        protocol::TaskResult doExecute (const protocol::T_ID& task_id, const protocol::TaskStatus task_status, const std::optional<std::chrono::milliseconds> time) {
        if (time.has_value()) std::this_thread::sleep_for(time.value());
        return CreateMessage(task_id, task_status);
        }

        protocol::TaskStatus Chance (const double probability) {
            if (random_utils::Success(probability))
                return protocol::TaskStatus::kSucceeded;
            return protocol::TaskStatus::kFailed;
        }
    }

    protocol::TaskResult ExecuteWorkload (const Config &config, const protocol::T_ID &task_id) {
        switch (config.type) {
            case Workload::SlowSuccess:
                return doExecute(task_id, protocol::TaskStatus::kSucceeded, config.duration);
            case Workload::FastSuccess:
                return doExecute(task_id, protocol::TaskStatus::kSucceeded, std::nullopt);
            case Workload::ImmediateFailure:
                return doExecute(task_id, protocol::TaskStatus::kFailed, std::nullopt);
            case Workload::DelayedFailure:
                return doExecute(task_id, protocol::TaskStatus::kFailed, config.duration);
            case Workload::RandomChance:
                return doExecute(task_id, Chance(0.7), random_utils::Delay({1000, 5000}));
            case Workload::RandomDelay:
                return doExecute(task_id, protocol::TaskStatus::kSucceeded, random_utils::Delay({1000, 5000}));
        }
        std::unreachable();
    }

}