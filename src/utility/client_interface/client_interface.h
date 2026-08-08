#ifndef DISPATCH_SERVICE_CLIENT_INTERFACE_H
#define DISPATCH_SERVICE_CLIENT_INTERFACE_H

#include <bits/stdc++.h>

#include "../Wire/protocol.h"
#include "../workload/workload.h"

namespace client{
    enum ClientEvent {
        // Socket & Config
        Socket_Success,
        Socket_Failure,
        Address_Configured,
        Timeout_Success,
        Timeout_Failure,

        // Connection Phase
        Connect_Success,
        Connect_Failure,

        // Execution Phase
        Task_Submitted,
        Task_SubmitFailed,
        Ack_Received,
        Ack_Failed,

        // Result Phase
        Result_Received,
        Result_Failed,
        Result_Malformed,

        // Global Errors
        OutOfTime
    };

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

    struct Result {
        bool success;
        std::string error;
        protocol::TaskId task_id;
        std::string client_id;
        uint64_t exe_time;
        size_t retry_count;
        std::vector<ClientEvent> metadata;    // Response details.
    };

    Result RunClient(const Context& ctx);

    // Will be made obsolete by threadpool.
    std::jthread SpawnClient ();
}


#endif //DISPATCH_SERVICE_CLIENT_INTERFACE_H
