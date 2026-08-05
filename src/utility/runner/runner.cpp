#include <bits/stdc++.h>

#include "runner.h"
#include "../scenario/scenarios.h"

namespace runner {
    namespace {
        // Almost the same decisions. Get confs from the scenario's helpers, give to worker.
        // The confusion arises from conf. Given a scenario::config, and create a relevant work::config.

        void Happy() {

        }

        void Timeout() {

        }

        void Cached() {

        }

        void Concurrent() {

        }
    }

    // This config comes from ME.
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
