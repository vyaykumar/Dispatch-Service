
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
                // .duration; this defaults to 0, in doExecute.
            },
            .clients = 1,
            // .stagger = 0; This is empty-curly-initialized.
        };
    }

    Config getTimeoutConf () {
        return {
            .scene = Scenario::TimeoutRetry,
            .workload {
                .type = work_l::Workload::SlowSuccess,
                .duration = std::chrono::milliseconds(1200) // Hardcode this?
            },
            .clients = 1,
            // .stagger = 0;
        };
    }

    Config getCachedConf () {
        return {
            .scene = Scenario::CachedResult,
            .workload {
                .type = work_l::Workload::DelayedFailure,   // What does delayed failure even mean?
                .duration = std::chrono::milliseconds(1200) // First has to timeout
            },
            .clients = 2,
            .stagger = std::chrono::milliseconds(2100) //More than timeout, but not by much.
        };
    }

    Config getConcurrentConf () {
        return {
            .scene = Scenario::ConcurrentClients,
            .workload {
                .type = work_l::Workload::SlowSuccess,
                // .duration = std::chrono::milliseconds(1200) // Hardcode this?
            },
            .clients = 4,
            .stagger = std::chrono::milliseconds(0) // All at once?
        };
    }
}
