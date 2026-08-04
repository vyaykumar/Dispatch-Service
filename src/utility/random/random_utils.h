#pragma once

#include <chrono>

namespace rando {
    std::chrono::milliseconds Delay(int min, int max);
    bool Success (double probability);
}
