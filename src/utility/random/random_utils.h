#pragma once

#include <bits/stdc++.h>
#include <random>

namespace rando {
    namespace {
        std::mt19937 rng {
            std::random_device{}()
        };

        // Random Delay
        std::uniform_int_distribution<int> dist_1 (0,5000);
        auto delay = std::chrono::milliseconds(dist_1(rng));

        // Random Success/Failure
        std::bernoulli_distribution dist_2(0.7);
        bool success = dist_2(rng);
    }

    std::chrono::milliseconds Delay(std::chrono::milliseconds min, std::chrono::milliseconds max);
    bool Success ();
}
