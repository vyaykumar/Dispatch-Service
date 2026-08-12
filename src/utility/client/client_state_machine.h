//
// Created by VijayKumar on 08-08-2026.
//

#ifndef DISPATCH_SERVICE_SMOKETEST_STATE_MACHINE_H
#define DISPATCH_SERVICE_SMOKETEST_STATE_MACHINE_H

#include <chrono>
#include <variant>

#include "client_types.h"

struct StateContext
{
    const Context& context;
    Result result;
    transport::socket_t socket {};
    sockaddr_in address {};
    int retries_remaining {};
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point deadline;
};

ClientState step (const state::InitSocket&,         StateContext&);
ClientState step (const state::ConfigureAddress&,   StateContext&);
ClientState step (const state::ConfigureTimeout&,   StateContext&);
ClientState step (const state::Connect&,            StateContext&);
ClientState step (const state::SubmitTask&,         StateContext&);
ClientState step (const state::WaitAck&,            StateContext&);
ClientState step (const state::WaitResult&,         StateContext&);
ClientState step (const state::RetryDecision&,      StateContext&);
ClientState step (const state::CloseSocket&, const StateContext&);
ClientState step (const state::BackOff&, const StateContext&);
void        step (const state::Success&,            StateContext&);
void        step (const state::Failure&,            StateContext&);

Result RunStateMachine(const Context&);

#endif //DISPATCH_SERVICE_SMOKETEST_STATE_MACHINE_H
