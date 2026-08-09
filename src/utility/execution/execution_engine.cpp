//
// Created by VijayKumar on 06-08-2026.
//

#include "execution_engine.h"

namespace execution {
    Context BuildContext (const scenario::Config& conf, size_t idx) {
        Context ctx = conf.client_template;
        ctx.client_id = "client"+std::to_string(idx);
        switch (conf.strategy) {
            case scenario::TaskStrategy::Unique:
                ctx.task_id = "task-" + std::to_string(idx);
                break;
            case scenario::TaskStrategy::Shared:
                ctx.task_id = "shared-task";
                break;
        }
        return ctx;
    }

    ExecutionResult ExecuteScenario (const scenario::Config& conf) {
        ExecutionResult res_vec {};

        for (size_t idx {0}; idx < conf.clients; idx++) {
            auto ctx = BuildContext(conf, idx);
            res_vec.results.push_back(client::RunClient(ctx));
            std::this_thread::sleep_for(conf.stagger);
        }

        return res_vec;
    }
}