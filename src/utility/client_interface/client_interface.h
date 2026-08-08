#ifndef DISPATCH_SERVICE_CLIENT_INTERFACE_H
#define DISPATCH_SERVICE_CLIENT_INTERFACE_H

#include <bits/stdc++.h>

#include "../Wire/protocol.h"
#include "../workload/workload.h"

namespace client{
    enum ClientEvent {
        SocketCreated,
        AddressConfigured,
        TimeoutSet,
        Connected,
        TaskSubmitted,
        ACKReceived,
        ResultReceived,
        ConnectionTimeout,
        SubmissionFailed,
        ACKFailed,
        ResultFailed,
        MalformedResponse
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
