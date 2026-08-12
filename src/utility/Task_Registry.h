#pragma once

#include <iostream>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

#include "Wire/protocol.h"

namespace task_registry {

    enum class ExecutionStatus {
        Processing,
        Completed
    };

    enum class Action {
        Reject,
        Execute,
        Cached
    };

    class TaskRegistry {
    public:
        struct TaskRecord {
            ExecutionStatus status {};
            std::optional<protocol::TaskResult> result {};
        };

        struct Result {
            Action action {};
            std::optional<protocol::TaskResult> result {};
        };

        Result tryClaim (const protocol::T_ID &id) {
            std::scoped_lock lock (mutex_);

            // Check if it exists. And it does.
            if (const auto entry_it = task_table_.find(id); entry_it != task_table_.end()) {
                if (entry_it->second.status == ExecutionStatus::Completed)
                    return {.action = Action::Cached, .result = entry_it->second.result};

                // It has to be Processing, then.
                return {.action = Action::Reject};
            }

            // It doesnt exist.
            task_table_[id] = {.status = ExecutionStatus::Processing};
            return {.action = Action::Execute};
        }

        void markComplete (const protocol::T_ID &id, protocol::TaskResult task) {
            std::scoped_lock lock (mutex_);

            const auto it = task_table_.find(id);

            if (it == task_table_.end() or it->second.status != ExecutionStatus::Processing) {
                std::cout << "Function mark_complete called on non-Processing task.\nTerminating.\n";
                std::terminate();
            }

            it->second.status = ExecutionStatus::Completed;
            it->second.result = std::move(task);
        }

    private:
        std::unordered_map<protocol::T_ID, TaskRecord> task_table_;
        std::mutex mutex_;
    };
}
