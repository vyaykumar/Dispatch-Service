#include <bits/stdc++.h>

#include "runner.h"
#include "../scenario/scenarios.h"

namespace runner {
    namespace {
        // Execution_Engine construction underway.
        void Happy() {

        }
        void Timeout() {

        }
        void Cached() {

        }
        void Concurrent() {

        }
    }

    // This config comes from ME, with the help of get*Conf() functions..
    void RunScenario (const scenario::Config& conf) {
        switch (conf.scene) {
            case scenario::Scenario::HappyPath:
                return Happy();
            case scenario::Scenario::TimeoutRetry:
                return Timeout();
            case scenario::Scenario::CachedResult:
                return Cached();
            case scenario::Scenario::ConcurrentClients:
                return Concurrent();
        }

    }
}
