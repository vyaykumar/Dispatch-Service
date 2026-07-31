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

            // Check if it exists.
            const auto entry_it = task_table_.find(id);

            // It exists.
            if (entry_it != task_table_.end()) {
                if (entry_it->second.status == executionStatus::Completed)
                    return {.action = Action::Cached, .result = entry_it->second.result};

                // It has to be Processing, then.
                return {.action = Action::Reject};
            }

            // It doesnt exist.
            task_table_[id] = {.status = executionStatus::Processing};
            return {.action = Action::Execute};
        }

        void mark_complete (const protocol::TaskId &id, protocol::TaskResult task) {
            // Acquire lock.
            std::scoped_lock lock (mutex_);

            // Check if id exists.
            const auto it = task_table_.find(id);

            // Incorrect input, or has been tampered with.
            if (it == task_table_.end() or it->second.status != executionStatus::Processing) {
                std::println("mark_complete called on non-Processing task.\nTerminating.\n");
                std::terminate();
            }

            // Set task associated with TaskID, attached to TaskID parameter, to Completed.
            it->second.status = executionStatus::Completed;
            it->second.result = std::move(task);
        }

    private:
        std::unordered_map<protocol::TaskId, Task> task_table_;
        std::mutex mutex_;
    };
}
