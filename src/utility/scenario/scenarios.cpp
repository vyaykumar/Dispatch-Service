#include "scenarios.h"

#include "../workload/workload_types.h"

namespace scenario {

    Config getHappyConfig () {
        return {
            .scene = Scenario::kHappyPath,
            .client_template = {
                .server_address = "127.0.0.1",
                .port = 50051,

                .workload_config = {
                    .type = workload::Workload::kFastSuccess,
                    .duration = std::chrono::milliseconds(500),
                },

                .timeout = std::chrono::milliseconds(3000),
                .max_retries = 2,
            },

            .n_clients = 1,
            .stagger = std::chrono::milliseconds(0),
            .strategy = TaskStrategy::kUnique,
        };
    }

    Config getTimeoutConfig () {
        return {
            .scene = Scenario::kTimeoutRetry,
            .client_template = {
                .server_address = "127.0.0.1",
                .port = 50051,

                .workload_config = {
                    .type = workload::Workload::kSlowSuccess,
                    .duration = std::chrono::milliseconds(2500),
                    .retry_backoff_ms = {50,100}
                },

                .timeout = std::chrono::milliseconds(1000),
                .global_timeout = std::chrono::milliseconds(5000),
                .max_retries = 0,
            },
            .n_clients = 1,
            .stagger = std::chrono::milliseconds(0),
            .strategy = TaskStrategy::kUnique,
        };
    }

    Config getCachedConfig () {
        return {
            .scene = Scenario::kCachedResult,

            .client_template = {
                .server_address = "127.0.0.1",
                .port = 50051,

                .workload_config = {
                    .type = workload::Workload::kSlowSuccess,
                    .duration = std::chrono::milliseconds(2500),
                    .retry_backoff_ms = {50,100}
                },

                .timeout = std::chrono::milliseconds(5000),
                .global_timeout = std::chrono::milliseconds(10'000),
                .max_retries = 0,
            },

            .n_clients = 2,
            .stagger = std::chrono::milliseconds(2500),
            .strategy = TaskStrategy::kShared,
        };
    }

    Config getConcurrentConfig () {
        return {
            .scene = Scenario::kConcurrentClients,

            .client_template = {
                .server_address = "127.0.0.1",
                .port = 50051,

                .workload_config = {
                    .type = workload::Workload::kSlowSuccess,
                    .duration = std::chrono::milliseconds(2000),
                    .retry_backoff_ms = {50,100}
                },

                .timeout = std::chrono::milliseconds(5000),
                .global_timeout = std::chrono::milliseconds(10'000),
                .max_retries = 0,
            },

            .n_clients = 4,
            .stagger = std::chrono::milliseconds(0),
            .strategy = TaskStrategy::kUnique,
        };
    }

    Config getSpeedWorkersConfig () {
        return {
            .scene = Scenario::kSpedUpWorkers,

            .client_template = {
                .server_address = "127.0.0.1",
                .port = 50051,

                .workload_config = {
                    .type = workload::Workload::kSlowSuccess,
                    .duration = std::chrono::milliseconds(2000),
                    .retry_backoff_ms = {50,100}
                },

                .timeout = std::chrono::milliseconds(5000),
                .global_timeout = std::chrono::milliseconds(10'000),
                .max_retries = 0,
            },

            .n_clients = 4,
            .stagger = std::chrono::milliseconds(0),
            .strategy = TaskStrategy::kUnique,
        };
    }
}
