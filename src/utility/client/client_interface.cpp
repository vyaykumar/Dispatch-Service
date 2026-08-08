#include "client_interface.h"
#include "client_state_machine.h"

namespace client {
    Result RunClient(const Context& ctx) {
        transport::PlatformInit();
        return RunStateMachine(ctx);
    }

    cli_ctx SpawnClient(const Context& ctx) {
        std::promise<Result> promise;
        auto future = promise.get_future();

        std::jthread worker([ctx, promise = std::move(promise)] mutable {
            {   // try here, and catch below.
                promise.set_value(RunClient(ctx));
            }
        });
        return {
            .worker = std::move(worker),
            .result = std::move(future),
        };
    }
}
