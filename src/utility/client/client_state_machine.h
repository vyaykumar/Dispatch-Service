//
// Created by VijayKumar on 08-08-2026.
//

#ifndef DISPATCH_SERVICE_SMOKETEST_STATE_MACHINE_H
#define DISPATCH_SERVICE_SMOKETEST_STATE_MACHINE_H
#include <chrono>
#include <variant>

#include "client_types.h"
#include "../scope_logger/scope_logger.h"

struct exec_ctx
{
    const Context& ctx;
    Result result;
    transport::socket_t sock {};
    sockaddr_in addr {};
    int retries_remaining {};
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point deadline;
    RetryCause reason {};
};

ClientState step (const state::InitSocket&,                 exec_ctx&);
ClientState step (const state::ConfigureAddress&,           exec_ctx&);
ClientState step (const state::ConfigureTimeout&,   const   exec_ctx&);
ClientState step (const state::Connect&,                    exec_ctx&);
ClientState step (const state::SubmitTask&,         const   exec_ctx&);
ClientState step (const state::WaitAck&,            const   exec_ctx&);
ClientState step (const state::WaitResult&,         const   exec_ctx&);
ClientState step (const state::RetryDecision&,              exec_ctx&);
ClientState step (const state::CloseSocket&,        const   exec_ctx&);
void        step (const state::Success&,            const   exec_ctx&);
void        step (const state::Failure&,            const   exec_ctx&);

Result RunStateMachine(const Context&);

#endif //DISPATCH_SERVICE_SMOKETEST_STATE_MACHINE_H
