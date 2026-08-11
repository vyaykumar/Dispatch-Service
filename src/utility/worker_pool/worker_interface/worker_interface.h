// Created by VyayKumar on 0933 10-08-2026.

#ifndef DISPATCH_SERVICE_SMOKETEST_WORKER_INTERFACE_H
#define DISPATCH_SERVICE_SMOKETEST_WORKER_INTERFACE_H

#include "../worker_pool.h"
#include "../../Wire/transport.h"

namespace worker {
    void HandleConnection (transport::socket_t, const worker_pool::Profile&);
}

#endif //DISPATCH_SERVICE_SMOKETEST_WORKER_INTERFACE_H
