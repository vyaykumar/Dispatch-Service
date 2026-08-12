#pragma once

#include <chrono>

#include "../client/client_types.h"
#include "../workload/workload.h"

namespace scenario {
    enum class Scenario {
        HappyPath,
        TimeoutRetry,
        CachedResult,
        ConcurrentClients,
        SpedUpWorkers
    };

    enum class TaskStrategy {
        Unique,
        Shared
    };

    struct Config {
        Scenario scene;
        Context client_template;
        size_t n_clients {1};
        std::chrono::milliseconds stagger {};
        TaskStrategy strategy;
    };

    Config getHappyConfig ();
    Config getTimeoutConfig ();
    Config getCachedConfig ();
    Config getConcurrentConfig ();
    Config getSpeedWorkersConfig ();
}
