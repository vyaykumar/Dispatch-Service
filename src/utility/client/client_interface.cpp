#include "client_interface.h"
#include "client_state_machine.h"

namespace client {
    Result RunClient(const Context& ctx) {
        transport::PlatformInit();
        return RunStateMachine(ctx);
    }

    cli_ctx SpawnClient(const Context& ctx) {
        std::packaged_task<Result()> task (
            [ctx] { return RunClient(ctx); }
        );
        auto future = task.get_future();
        std::jthread worker (std::move(task));

        return {
            .worker = std::move(worker),
            .result = std::move(future),
        };
    }
}
