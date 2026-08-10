// Created by VyayKumar on 0933 10-08-2026.

#ifndef DISPATCH_SERVICE_SMOKETEST_WORKER_INTERFACE_H
#define DISPATCH_SERVICE_SMOKETEST_WORKER_INTERFACE_H
#include <stop_token>

#include "../../Wire/transport.h"

namespace worker {
    void HandleConnection (std::stop_token, transport::socket_t);
}

#endif //DISPATCH_SERVICE_SMOKETEST_WORKER_INTERFACE_H
