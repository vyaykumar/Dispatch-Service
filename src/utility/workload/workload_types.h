#pragma once
#include <cstdint>

namespace workload {
    enum class Workload : uint8_t {
        SlowSuccess,
        FastSuccess,
        ImmediateFailure,
        DelayedFailure,
        RandomChance,
        RandomDelay
    };
}
