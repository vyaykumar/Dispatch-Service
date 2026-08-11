#pragma once
#include <chrono>
#include <cstddef>

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
        size_t clients {1};
        std::chrono::milliseconds stagger {};
        TaskStrategy strategy;
    };

    // These confs are passed TO RunScenario in runner.
    // I pass them to Run();
    Config getHappyConf ();
    Config getTimeoutConf ();
    Config getCachedConf ();
    Config getConcurrentConf ();
    Config getSpeedWorkers ();
}
