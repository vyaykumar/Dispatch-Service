
#include "scenarios.h"

#include "../workload/workload_types.h"

namespace scenario {
    namespace {

    }

    Config getHappyConf () {
        return {
            .scene = Scenario::HappyPath,
            .workload {
                .type = work_l::Workload::FastSuccess,
                .duration = std::chrono::milliseconds(0),
            },
            .clients = 1,
            .stagger = std::chrono::milliseconds(0),
        };
    }

    Config getTimeoutConf () {
        return {
            .scene = Scenario::TimeoutRetry,
            .workload {
                .type = work_l::Workload::SlowSuccess,
                .duration = std::chrono::milliseconds(2500),
            },
            .clients = 1,
            .stagger = std::chrono::milliseconds(0),
        };
    }

    Config getCachedConf () {
        return {
            .scene = Scenario::CachedResult,
            .workload {
                .type = work_l::Workload::SlowSuccess,
                .duration = std::chrono::milliseconds(2000),
            },
            .clients = 2,
            .stagger = std::chrono::milliseconds(2500),
        };
    }

    Config getConcurrentConf () {
        return {
            .scene = Scenario::ConcurrentClients,
            .workload {
                .type = work_l::Workload::SlowSuccess,
                .duration = std::chrono::milliseconds(1500),
            },
            .clients = 4,
            .stagger = std::chrono::milliseconds(0),
        };
    }
}
