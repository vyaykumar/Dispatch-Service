//
// Created by VijayKumar on 08-08-2026.
//

#ifndef DISPATCH_SERVICE_SMOKETEST_STATE_MACHINE_H
#define DISPATCH_SERVICE_SMOKETEST_STATE_MACHINE_H
#include <chrono>
#include <variant>

#include "client_types.h"

struct exec_ctx
{
    const Context& ctx;
    Result result;
    transport::socket_t sock {};
    sockaddr_in addr {};
    int retries_remaining {};
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point deadline;
};

ClientState step (const state::InitSocket&,       exec_ctx&);
ClientState step (const state::ConfigureAddress&, exec_ctx&);
ClientState step (const state::ConfigureTimeout&, exec_ctx&);
ClientState step (const state::Connect&,          exec_ctx&);
ClientState step (const state::SubmitTask&,       exec_ctx&);
ClientState step (const state::WaitAck&,          exec_ctx&);
ClientState step (const state::WaitResult&,       exec_ctx&);
ClientState step (const state::RetryDecision&,    exec_ctx&);
ClientState step (const state::Success&,          exec_ctx&);
ClientState step (const state::Failure&,          exec_ctx&);

#endif //DISPATCH_SERVICE_SMOKETEST_STATE_MACHINE_H
