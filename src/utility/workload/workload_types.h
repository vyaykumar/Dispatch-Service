#pragma once
#include <cstdint>

namespace workload {
    enum class Workload : uint8_t {
        kSlowSuccess,
        kFastSuccess,
        kImmediateFailure,
        kDelayedFailure,
        kRandomChance,
        kRandomDelay
    };
}
