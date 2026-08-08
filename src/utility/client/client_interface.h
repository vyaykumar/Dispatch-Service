#ifndef DISPATCH_SERVICE_CLIENT_INTERFACE_H
#define DISPATCH_SERVICE_CLIENT_INTERFACE_H

#include <bits/stdc++.h>

#include "client_types.h"

namespace client{
    struct cli_ctx {
        std::jthread worker;
        std::future<Result> result;
    };

    Result RunClient(const Context& ctx);
    cli_ctx SpawnClient (const Context& ctx);
}

#endif //DISPATCH_SERVICE_CLIENT_INTERFACE_H
