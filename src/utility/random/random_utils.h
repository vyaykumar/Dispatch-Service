#pragma once

#include <chrono>

namespace random_utils {
    std::chrono::milliseconds Delay(const std::pair<size_t,size_t>&);
    bool Success (double);
}
