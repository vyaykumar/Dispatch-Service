#include "client_state_machine.h"

ClientState step (const state::InitSocket&,       exec_ctx& e_ctx) {
    auto& sock = e_ctx.sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == transport::kInvalidSocket)
        return logEvent(e_ctx, Socket_Failure);
    logEvent(e_ctx, Socket_Success);
}

ClientState step (const state::ConfigureAddress&, exec_ctx&) {

}

ClientState step (const state::ConfigureTimeout&, exec_ctx&) {

}

ClientState step (const state::Connect&,          exec_ctx&) {

}

ClientState step (const state::SubmitTask&,       exec_ctx&) {

}

ClientState step (const state::WaitAck&,          exec_ctx&) {

}

ClientState step (const state::WaitResult&,       exec_ctx&) {

}

ClientState step (const state::RetryDecision&,    exec_ctx&) {

}

ClientState step (const state::Success&,          exec_ctx&) {

}

ClientState step (const state::Failure&,          exec_ctx&){

}