#pragma once
#include <cstdint>

namespace work_l {
    enum class Workload : uint8_t {
        SlowSuccess,
        FastSuccess,
        ImmediateFailure,
        DelayedFailure,
        RandomChance,
        RandomDelay
    };
}
