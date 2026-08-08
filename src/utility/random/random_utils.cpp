//
// Created by VijayKumar on 04-08-2026.
//

#include <random>
#include "random_utils.h"


namespace rando {
    namespace {
        std::mt19937 rng {
            std::random_device{}()
        };
    }

    std::chrono::milliseconds Delay(const size_t min, const size_t max) {
        const std::chrono::milliseconds t_min {min}, t_max {max};

        std::uniform_int_distribution dist (t_min.count(), t_max.count());
        return std::chrono::milliseconds{dist(rng)};
    }

    bool Success(const double probability) {
        std::bernoulli_distribution dist(probability);
        return dist(rng);
    }
}
