#include "client_interface.h"
#include "client_state_machine.h"

namespace client {
    Result RunClient(const Context& context) {
        transport::PlatformInit();
        return RunStateMachine(context);
    }

    client_context SpawnClient(const Context& context) {
        std::packaged_task<Result()> task (
            [context] { return RunClient(context); }
        );
        auto future = task.get_future();
        std::jthread worker (std::move(task));

        return {
            .worker = std::move(worker),
            .result = std::move(future),
        };
    }
}
