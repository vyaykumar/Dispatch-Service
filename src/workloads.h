#pragma once
#include <chrono>

#include "protocol.h"

namespace work {
    enum class Workload {
        SlowSuccess,
        FastSuccess,
        ImmediateFailure,
        DelayedFailure
    };

    struct Config {
        Workload type { Workload::SlowSuccess };
        std::chrono::milliseconds duration { std::chrono::seconds(2) };
    };

    protocol::TaskResult ExecuteWorkload (const Config& config, const protocol::TaskId& taskID);
}
