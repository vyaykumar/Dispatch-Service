#pragma once
#include <chrono>
#include <cstddef>

#include "../work/workloads.h"

namespace scenario {
    enum class Scenario {
        HappyPath,
        TimeoutRetry,
        CachedResult,
        ConcurrentClients
    };

    struct Config {
        Scenario scene;
        work_l::Config workload;
        size_t clients {1};
        std::chrono::milliseconds stagger {};
    };

    // These confs are passed TO RunScenario in runner.
    // I pass them to Run();
    Config getHappyConf ();
    Config getTimeoutConf ();
    Config getCachedConf ();
    Config getConcurrentConf ();
}
