//
// Created by VijayKumar on 08-08-2026.
//

#ifndef DISPATCH_SERVICE_SMOKETEST_CLIENT_TYPES_H
#define DISPATCH_SERVICE_SMOKETEST_CLIENT_TYPES_H
#include <bits/stdc++.h>
#include "../Wire/protocol.h"
#include "../workload/workload.h"

namespace state {
    struct InitSocket {};
    struct ConfigureAddress {};
    struct ConfigureTimeout {};
    struct Connect {};

    struct SubmitTask {};

    struct WaitAck {};
    struct WaitResult {};

    struct RetryDecision {};
    struct CloseSocket {};

    struct BackOff {};

    struct Success {};
    struct Failure {};
};

using ClientState = std::variant<
    state::InitSocket,
    state::ConfigureAddress,
    state::ConfigureTimeout,
    state::Connect,
    state::SubmitTask,
    state::WaitAck,
    state::WaitResult,
    state::RetryDecision,
    state::CloseSocket,
    state::BackOff,
    state::Success,
    state::Failure
>;

struct Context {
    std::string server_address;
    uint16_t port;
    protocol::T_ID task_id;
    workload::Config workload_config;
    std::chrono::milliseconds timeout;
    std::chrono::milliseconds global_timeout;
    int max_retries {0};
    std::string client_id;
    // std::map<std::string, std::string> metadata; // Logging and tracing.
};

struct Result {
    bool success;
    std::string error;
    protocol::T_ID task_id;
    std::string client_id;
    uint64_t execution_time;
    size_t retry_count;
    std::string payload;
    protocol::TaskStatus status;
    // std::vector<LogEntry> metadata;    // Response details.
};

#endif //DISPATCH_SERVICE_SMOKETEST_CLIENT_TYPES_H
