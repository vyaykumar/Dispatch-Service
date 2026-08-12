//
// Created by VijayKumar on 04-08-2026.
//

#include <random>
#include "random_utils.h"


namespace random_utils {
    namespace {
        std::mt19937 rng {
            std::random_device{}()
        };
    }

    std::chrono::milliseconds Delay(const std::pair<size_t, size_t> &values) {
        const std::chrono::milliseconds t_min {values.first}, t_max {values.second};

        std::uniform_int_distribution dist (t_min.count(), t_max.count());
        return std::chrono::milliseconds{dist(rng)};
    }

    bool Success(const double probability) {
        std::bernoulli_distribution dist(probability);
        return dist(rng);
    }
}
