#include <bits/stdc++.h>

#include "runner.h"
#include "../scenario/scenarios.h"

namespace runner {
    namespace {
        // Almost the same decisions. Get confs from the scenario's helpers, give to worker (wrong, give it to client.).
        // The confusion arises from conf. Given a scenario::config, and create a relevant work::config.

        // I doubt we need special functions.
        // N clients, with X stagger between them, submitting the given workload.
        // I think this requires a full ExecutionEngine header+impl file.

        void Happy() { // add the conf as a parameter? Allows for "get N tasks, and complete all of them."
            // 1. Spawn a client.
            // 2. Give it the workload.
            // 3. Sleep stagger.
            // 4. Repeat 1-3 N times.
            // return;
        }

        void Timeout() {
            // 1. Spawn a client.
            // 2. Give it the workload.
            // return;
        }

        void Cached() {
            // 1. Spawn a client. Have it timeout/complete.
            // 2. Same/Different client submits same task.
            // 3. It will get the cached result.
            // return;
        }

        void Concurrent() {
            // 1. Spawn a client.
            // 2. Give it the workload.
            // 3. Sleep stagger.
            // 4. Repeat 1-3 N times.
            // return;
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
