
using transport::socket_t;

#include <bits/stdc++.h>

namespace {

    // Magic Number
    constexpr uint16_t kPort = 50051;




    // Functions yonder.

    // 1. void* recSub                - ReceiveSubmit: Receive a message and validate it is a TASK_SUBMIT.
    // 2. void* recSub                - Protocol Handshake: Send the ACK.
    // 3. ExecutionStatus checkState  - Idempotency: Check whether the task is new, executing or done.
    // 4. bool exec                   - Execution: Do the work.
    // 5. void* SendResult            - Response: Build and send the TASK_RESULT, close the socket, flip done.






    void HandleConnection(std::stop_token stopToken, socket_t client, std::atomic_bool& done) {
        std::cout << "[worker]: Thread started.\n";

    }

}
