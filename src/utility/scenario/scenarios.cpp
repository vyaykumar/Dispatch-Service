
#include "scenarios.h"

#include "../workload/workload_types.h"

namespace scenario {
    namespace {

    }

    Config getHappyConf () {
        Config conf{
            .scene = Scenario::HappyPath,
            .client_template = {
                // .client_id = ,
                // .task_id = ,
                .serverAddr = "127.0.0.1",
                .port = 50051,
                .w_conf = {
                    .type = work_l::Workload::FastSuccess,
                    .duration = std::chrono::milliseconds(500),
                    // .payload = ,
                    // .retry_backoff_ms = ,
                },
                .timeout = std::chrono::milliseconds(3000),
                .max_retries = 2,

            },
            .clients = 1,
            .stagger = std::chrono::milliseconds(0),
            .strategy = TaskStrategy::Unique,
        };
        return conf;
    }

    Config getTimeoutConf () {
        return {};
    }

    Config getCachedConf () {
        return {};
    }

    Config getConcurrentConf () {
        return {};
    }
}
