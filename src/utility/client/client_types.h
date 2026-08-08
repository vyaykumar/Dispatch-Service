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
    state::Success,
    state::Failure
>;

struct Context {
    std::string serverAddr;
    uint16_t port;
    protocol::TaskId task_id;
    work_l::Config w_conf;
    std::chrono::milliseconds timeout;
    int max_retries {0};
    std::string client_id;
    // std::map<std::string, std::string> metadata; // Logging and tracing.
};

struct LogEntry {
    std::chrono::steady_clock::duration elapsed;
    Layer layer;
    std::string stage;
    std::string message;
};

struct Result {
    bool success;
    std::string error;
    protocol::TaskId task_id;
    std::string client_id;
    uint64_t exe_time;
    size_t retry_count;
    std::vector<LogEntry> metadata;    // Response details.
};

#endif //DISPATCH_SERVICE_SMOKETEST_CLIENT_TYPES_H
