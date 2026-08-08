//
// Created by VijayKumar on 06-08-2026.
//

#include "execution_engine.h"

namespace execution {
    ExecutionResult ExecuteScenario (const scenario::Config& config) {

        ExecutionResult res_vec;

        // Parse the config.
        auto [scene,work_conf,clients,stagger] = config;
        // Spawn N clients, with stagger time between them.
        while (clients--) {
            Result res = client::RunClient(ctx);
            res_vec.results.push_back(std::move(res));
            std::this_thread::sleep_for(stagger);
        }
        // Return aggregated results.
    }
}