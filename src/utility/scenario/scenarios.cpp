
#include "scenarios.h"

#include "../workload/workload_types.h"

namespace scenario {
    namespace {

    }

    Config getHappyConf () {
        return {
            .scene = Scenario::HappyPath,
            .client_template = {
                .serverAddr = "127.0.0.1",
                .port = 50051,

                .w_conf = {
                    .type = work_l::Workload::FastSuccess,
                    .duration = std::chrono::milliseconds(500),
                },

                .timeout = std::chrono::milliseconds(3000),
                .max_retries = 2,
            },

            .clients = 1,
            .stagger = std::chrono::milliseconds(0),
            .strategy = TaskStrategy::Unique,
        };
    }

    Config getTimeoutConf () {
        return {
            .scene = Scenario::HappyPath,
            .client_template = {
                .serverAddr = "127.0.0.1",
                .port = 50051,

                .w_conf = {
                    .type = work_l::Workload::SlowSuccess,
                    .duration = std::chrono::milliseconds(2500),
                    .retry_backoff_ms = {50,100}
                },

                .timeout = std::chrono::milliseconds(1000),
                .global_timeout = std::chrono::milliseconds(5000),
                .max_retries = 2,
            },
            .clients = 1,
            .stagger = std::chrono::milliseconds(0),
            .strategy = TaskStrategy::Unique,
        };
    }

    Config getCachedConf () {
        return {
            .scene = Scenario::CachedResult,

            .client_template = {
                .serverAddr = "127.0.0.1",
                .port = 50051,

                .w_conf = {
                    .type = work_l::Workload::SlowSuccess,
                    .duration = std::chrono::milliseconds(2500),
                    .retry_backoff_ms = {50,100}
                },

                .timeout = std::chrono::milliseconds(5000),
                .global_timeout = std::chrono::milliseconds(10'000),
                .max_retries = 0,
            },

            .clients = 2,
            .stagger = std::chrono::milliseconds(2500),
            .strategy = TaskStrategy::Shared,
        };
    }

    Config getConcurrentConf () {
        return {
            .scene = Scenario::ConcurrentClients,

            .client_template = {
                .serverAddr = "127.0.0.1",
                .port = 50051,

                .w_conf = {
                    .type = work_l::Workload::SlowSuccess,
                    .duration = std::chrono::milliseconds(2000),
                    .retry_backoff_ms = {50,100}
                },

                .timeout = std::chrono::milliseconds(5000),
                .global_timeout = std::chrono::milliseconds(10'000),
                .max_retries = 0,
            },

            .clients = 4,
            .stagger = std::chrono::milliseconds(0),
            .strategy = TaskStrategy::Unique,
        };
    }

    Config getSpeedWorkers () {
        return {
            .scene = Scenario::SpedUpWorkers,

            .client_template = {
                .serverAddr = "127.0.0.1",
                .port = 50051,

                .w_conf = {
                    .type = work_l::Workload::SlowSuccess,
                    .duration = std::chrono::milliseconds(2000),
                    .retry_backoff_ms = {50,100}
                },

                .timeout = std::chrono::milliseconds(5000),
                .global_timeout = std::chrono::milliseconds(10'000),
                .max_retries = 0,
            },

            .clients = 4,
            .stagger = std::chrono::milliseconds(0),
            .strategy = TaskStrategy::Unique,
        };
    }
}
