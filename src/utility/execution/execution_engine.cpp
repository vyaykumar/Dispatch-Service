//
// Created by VijayKumar on 06-08-2026.
//

#include "execution_engine.h"

namespace execution {
    ExecutionResult ExecuteScenario (const scenario::Config& config,
    const std::string& serverAddr,
        uint16_t port) {
        ExecutionResult result {};

        // Parse the config.
        auto [scene,work_conf,clients,stagger] = config;
        // Spawn N clients, with stagger time between them.
        while (clients--) {
            //Spawn 1 client.
            //Sleep for stagger seconds. Make the timekeeping absolute.
            // Collect Results.
        }
        // Return aggregated results.
    }
}