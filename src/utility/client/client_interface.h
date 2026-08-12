#ifndef DISPATCH_SERVICE_CLIENT_INTERFACE_H
#define DISPATCH_SERVICE_CLIENT_INTERFACE_H

#include "client_types.h"

namespace client{

    struct client_context {
        std::jthread worker;
        std::future<Result> result;
    };

    Result RunClient(const Context& context);
    client_context SpawnClient (const Context& context);
}

#endif //DISPATCH_SERVICE_CLIENT_INTERFACE_H
