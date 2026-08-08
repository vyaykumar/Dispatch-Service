#pragma once
#include <chrono>

#include "../Wire/protocol.h"
#include "workload_types.h"

namespace work_l {

    struct Config {
        Workload type { Workload::SlowSuccess };
        std::chrono::milliseconds duration { std::chrono::seconds(2) };
        std::vector<uint8_t> payload {'H','e','l','l','o'};
    };

    protocol::TaskResult ExecuteWorkload (const Config& config, const protocol::TaskId& taskID);
}
