#pragma once

#include <chrono>

namespace rando {
    std::chrono::milliseconds Delay(size_t min, size_t max);
    bool Success (double probability);
}
