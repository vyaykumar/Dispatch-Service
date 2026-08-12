#include "execution_engine.h"

#include "../client/client_interface.h"
#include "../client/client_types.h"

namespace execution {
    Context BuildContext (const scenario::Config& config, const size_t idx) {
        Context context = config.client_template;
        context.client_id = "client_"+std::to_string(idx);

        switch (config.strategy) {
            case scenario::TaskStrategy::kUnique:
                context.task_id = "task_" + std::to_string(idx);
                break;
            case scenario::TaskStrategy::kShared:
                context.task_id = "shared_task";
                break;
        }

        return context;
    }

    ExecutionResult ExecuteScenario (const scenario::Config& config) {
        ExecutionResult execution_results {};
        std::vector<client::client_context> client_contexts {};

        for (size_t idx {0}; idx < config.n_clients; idx++) {
            auto context = BuildContext(config, idx);
            std::cout << "\n";
            client_contexts.push_back(client::SpawnClient(context));
            std::this_thread::sleep_for(config.stagger);
        }

        for (auto&[worker, result] : client_contexts)
            execution_results.results.push_back(result.get());

        return execution_results;
    }
}