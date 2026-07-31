#pragma once

#include <bits/stdc++.h>
#include "protocol.h"
#include "transport.h"

namespace task_registry {
    enum class executionStatus {
        Fresh,          // If Fresh, execute and cache.
        Processing,     // If Processing, reject task.
        Completed       // If Completed, return cached.
    };

    // Class TaskRegistry : Owns the task-table, and the mutex.
    // Methods for operation, but doesn't expose raw access.

    class TaskRegistry {
    public:
        struct Task {
            executionStatus status = executionStatus::Fresh;
            std::optional<protocol::TaskResult> result;
        };

        Task try_claim () {
            // Acquire lock.
            // Check for status.
            // If Fresh, return Execute.
            // If Processing, return Reject.
            // If Completed, return Cached.
        }

        void mark_complete () {
            // Acquire lock.
            // Set task associated with TaskID, attached to TaskID parameter, to Completed.
        }

        void fetch () const {
            // Acquire lock. This is a read operation. Do we need to acquire lock?
            // Return .result of the Task attached to TaskID parameter.
        }

    private:
        std::unordered_map<std::string, Task> status;
        std::mutex hold;
    };
}
