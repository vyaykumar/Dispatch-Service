#pragma once
#include <chrono>

#include "../Wire/protocol.h"

namespace work {
    enum class Workload {
        SlowSuccess,
        FastSuccess,
        ImmediateFailure,
        DelayedFailure,
        RandomChance,
        RandomDelay
    };

    struct Config {
        Workload type { Workload::SlowSuccess };
        std::chrono::milliseconds duration { std::chrono::seconds(2) };
    };

    protocol::TaskResult ExecuteWorkload (const Config& config, const protocol::TaskId& taskID);
}
