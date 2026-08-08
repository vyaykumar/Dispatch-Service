#include "client_interface.h"
#include "client_state_machine.h"

namespace client {
    Result RunClient(const Context& ctx) {
        transport::PlatformInit();
        return RunStateMachine(ctx);
    }

    std::jthread SpawnClient(const Context& ctx) {
        return std::jthread([ctx]() {
           Result res = RunClient(ctx);
            // Success/Failure populated here.
        });
    }
}
