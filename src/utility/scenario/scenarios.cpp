#include "scenarios.h"

#include "../workload/workload_types.h"

namespace scenario {

    Config getHappyConfig () {
        return {
            .scene = Scenario::HappyPath,
            .client_template = {
                .server_address = "127.0.0.1",
                .port = 50051,

                .workload_config = {
                    .type = work_l::Workload::FastSuccess,
                    .duration = std::chrono::milliseconds(500),
                },

                .timeout = std::chrono::milliseconds(3000),
                .max_retries = 2,
            },

            .n_clients = 1,
            .stagger = std::chrono::milliseconds(0),
            .strategy = TaskStrategy::Unique,
        };
    }

    Config getTimeoutConfig () {
        return {
            .scene = Scenario::HappyPath,
            .client_template = {
                .server_address = "127.0.0.1",
                .port = 50051,

                .workload_config = {
                    .type = work_l::Workload::SlowSuccess,
                    .duration = std::chrono::milliseconds(2500),
                    .retry_backoff_ms = {50,100}
                },

                .timeout = std::chrono::milliseconds(1000),
                .global_timeout = std::chrono::milliseconds(5000),
                .max_retries = 2,
            },
            .n_clients = 1,
            .stagger = std::chrono::milliseconds(0),
            .strategy = TaskStrategy::Unique,
        };
    }

    Config getCachedConfig () {
        return {
            .scene = Scenario::CachedResult,

            .client_template = {
                .server_address = "127.0.0.1",
                .port = 50051,

                .workload_config = {
                    .type = work_l::Workload::SlowSuccess,
                    .duration = std::chrono::milliseconds(2500),
                    .retry_backoff_ms = {50,100}
                },

                .timeout = std::chrono::milliseconds(5000),
                .global_timeout = std::chrono::milliseconds(10'000),
                .max_retries = 0,
            },

            .n_clients = 2,
            .stagger = std::chrono::milliseconds(2500),
            .strategy = TaskStrategy::Shared,
        };
    }

    Config getConcurrentConfig () {
        return {
            .scene = Scenario::ConcurrentClients,

            .client_template = {
                .server_address = "127.0.0.1",
                .port = 50051,

                .workload_config = {
                    .type = work_l::Workload::SlowSuccess,
                    .duration = std::chrono::milliseconds(2000),
                    .retry_backoff_ms = {50,100}
                },

                .timeout = std::chrono::milliseconds(5000),
                .global_timeout = std::chrono::milliseconds(10'000),
                .max_retries = 0,
            },

            .n_clients = 4,
            .stagger = std::chrono::milliseconds(0),
            .strategy = TaskStrategy::Unique,
        };
    }

    Config getSpeedWorkersConfig () {
        return {
            .scene = Scenario::SpedUpWorkers,

            .client_template = {
                .server_address = "127.0.0.1",
                .port = 50051,

                .workload_config = {
                    .type = work_l::Workload::SlowSuccess,
                    .duration = std::chrono::milliseconds(2000),
                    .retry_backoff_ms = {50,100}
                },

                .timeout = std::chrono::milliseconds(5000),
                .global_timeout = std::chrono::milliseconds(10'000),
                .max_retries = 0,
            },

            .n_clients = 4,
            .stagger = std::chrono::milliseconds(0),
            .strategy = TaskStrategy::Unique,
        };
    }
}
