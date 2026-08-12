#pragma once
#include <chrono>

#include "../Wire/protocol.h"
#include "workload_types.h"

namespace workload {

    struct Config {
        Workload type { Workload::SlowSuccess };
        std::chrono::milliseconds duration { std::chrono::seconds(2) };
        std::vector<uint8_t> payload {'H','e','l','l','o'};
        std::pair<size_t,size_t> retry_backoff_ms {100,500};
    };

    protocol::TaskResult ExecuteWorkload (const Config& config, const protocol::T_ID& task_id);
}
