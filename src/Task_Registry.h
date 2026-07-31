#pragma once

#include <bits/stdc++.h>
#include "protocol.h"
#include "transport.h"

namespace task_registry {

    class TaskRegistry {
    public:
        enum class executionStatus {
            Processing,
            Completed
        };

        enum class Action {
            Reject,
            Execute,
            Cached
        };
        struct Task {
            executionStatus status;
            std::optional<protocol::TaskResult> result;
        };

        struct Result {
            Action action;
            std::optional<protocol::TaskResult> result;
        };

        Result try_claim (const protocol::TaskId &id) {
            // Acquire lock.
            std::scoped_lock lock (mutex_);

            // Check for status.
            switch (task_table_[id].status) {
                case executionStatus::Processing : return {.action = Action::Reject};
                    break;
                case executionStatus::Completed : return {.action = Action::Cached, .result = task_table_[id].result};
                    break;
                default: return {.action = Action::Execute};
            }
        }

        void mark_complete (protocol::TaskId id, protocol::TaskResult task) {
            // Acquire lock.
            std::scoped_lock lock (mutex_);

            // Set task associated with TaskID, attached to TaskID parameter, to Completed.
            task_table_[id].status = executionStatus::Completed;
            task_table_[id].result = std::move(task);
        }

    private:
        std::unordered_map<protocol::TaskId, Task> task_table_;
        std::mutex mutex_;
    };
}
