**Project:** Distributed Task Execution System  
**Language:** Modern C++23  
**Status:** Active development (Phase 15 complete, worker profiles implemented)  
**Lines of Code:** ~1,847 across core modules  
**Architecture:** Client → Dispatcher → Worker

## 1. Project Purpose and Scope

This project studies distributed systems engineering through a task execution platform. The goal is to understand networking, concurrency, reliability, retries, error handling, and the behaviour of distributed systems.

The system submits tasks from clients to a dispatcher, which routes them to workers for execution. Tasks have built-in retry logic and timeout handling. Workers track task execution state to prevent duplicate execution even when the network delivers the same task multiple times.

This is not a clone of Temporal. Rather, it focuses on the prerequisite layers that any orchestration system requires: protocol design, message framing, concurrency safety, and idempotent execution.

## 2. System Architecture

### 2.1 Three-Tier Design

```
┌─────────┐         ┌────────────┐         ┌────────┐
│ Client       │───▶│ Dispatcher     │───▶│ Worker   │
└─────────┘         └────────────┘         └────────┘
```

**Client:** Initiates task submission. Waits for acknowledgement and then waits for results. Retries on timeout.

**Dispatcher:** Receives task submissions from clients. Sends acknowledgement immediately. Routes to worker. Manages timeouts and retries. Currently single-worker, but multi-worker routing is planned.

**Worker:** Accepts inbound TCP connections and enqueues them as work items. A fixed-size WorkerPool owns worker threads which dequeue accepted connections and execute the worker protocol pipeline. The worker tracks task execution state per task ID, suppresses duplicate execution, and returns cached results for completed tasks.

```
Accept Loop
    ↓
Work Queue
    ↓
WorkerPool
    ↓
HandleConnection
```

### 2.2 Message Types

Three core message types flow through the system:

**TASK_SUBMIT** (client → dispatcher/worker): Contains task ID, idempotency key, payload, and workload type. Carries the work to be done.

**TASK_ACK** (dispatcher/worker → client): Acknowledges receipt of TASK_SUBMIT. Sent as soon as the task is accepted.

**TASK_RESULT** (worker → dispatcher/worker → client): Contains task status (pending, running, succeeded, failed), payload (result data), and timing information.

A fourth message type (CANCEL) is defined in the protocol but not yet implemented.

### 2.3 Execution Flow

```
1. Client creates socket, connects to dispatcher
2. Client submits task (TASK_SUBMIT)
3. Dispatcher sends ACK (TASK_ACK)
4. Dispatcher routes task to worker
5. Worker marks task as "Processing" in its registry
6. Worker executes task
7. Worker marks task as "Completed" with result
8. Dispatcher relays TASK_RESULT to client
9. Client receives result and closes connection
```

If timeout occurs before TASK_RESULT arrives, the client retries from step 1. The dispatcher or worker will detect that the task is already being processed or is cached, and respond accordingly.

## 3. Protocol Layer

### 3.1 Wire Format

All messages use length-prefixed framing to handle TCP's lack of message boundaries.

```
Frame: [4-byte big-endian length][1-byte type][payload bytes]
```

The 4-byte length indicates the number of bytes that follow (including the type byte). This allows the receiver to know exactly how many bytes to read.

### 3.2 Transport Layer (transport.h)

The transport layer sits below the protocol and knows nothing about tasks or business logic. It solves one problem: sending and receiving discrete frames over TCP.

Key functions:

**SendFrame(socket, type, payload):** Wraps payload in length-prefixed frame. Loops over partial writes to ensure all bytes are sent. Returns false on socket error.

**RecvFrame(socket):** Reads 5-byte header (length + type). Allocates payload buffer. Loops over partial reads. Returns std::optional (empty on error). Handles TCP's partial delivery guarantee transparently.

**RecvExact(socket, buffer, n):** Helper that loops until exactly n bytes are read. Called by RecvFrame for the header and payload.

The transport layer is platform-agnostic. It abstracts socket type (SOCKET on Windows, int on Unix) and provides platform-specific socket close operations.

### 3.3 Protocol Layer (protocol.h)

The protocol layer sits above transport and interprets frames as task messages. It defines:

**MessageType enum:** kTaskSubmit (1), kTaskAck (2), kTaskResult (3), kCancel (4).

**TaskStatus enum:** kPending, kRunning, kSucceeded, kFailed, kInProgress.

**Message Structures:**

- `TaskSubmit`: taskId (string), idempotencyKey (string), payload (bytes), workload (enum)
- `TaskAck`: taskId (string)
- `TaskResult`: status (enum), payload (bytes)

**TLV Encoding:** Messages use Tag-Length-Value encoding within the payload. The FieldWriter class builds payloads by appending field tags, 4-byte lengths, and values. This is extensible: new fields can be added without breaking old clients.

**Serialisation and Deserialisation:**

The protocol module provides `SendTaskSubmit`, `SendTaskAck`, `SendTaskResult` functions. These create the message structure, encode it into a byte vector, and call `SendFrame`. Similarly, `ReceiveMessage` calls `RecvFrame` and decodes the payload back into a DecodedMessage variant.

Error handling uses std::optional. A nullopt result indicates a malformed message (truncated header, truncated value, or unrecognised message type).

## 4. Client Implementation

### 4.1 Client State Machine

The client does not perform all steps sequentially. Instead, it uses a state machine with explicit states for each step of the protocol.

**States:**

- InitSocket: Create socket
- ConfigureAddress: Set dispatcher address
- ConfigureTimeout: Set SO_RCVTIMEO (socket receive timeout)
- Connect: TCP connect to dispatcher
- SubmitTask: Send TASK_SUBMIT
- WaitAck: Receive TASK_ACK
- WaitResult: Receive TASK_RESULT
- RetryDecision: Check if retry is allowed
- CloseSocket: Close socket
- BackOff: Wait before retry
- Success: Task completed
- Failure: Task failed after retries exhausted

Each state is a C++ type. A single `step()` function is overloaded for each state type. The state machine maintains execution context (socket, server address, retry count, timing) and transitions based on operation outcomes.

**Monadic Error Handling:**

The state machine transitions use std::expected. Each step returns either a new state or an error. This allows states to chain operations using `.and_then()` and `.transform()`.

Example:

```cpp
return SendACK(client, taskID)
    .transform([msg = std::move(message)]() mutable {
        return std::move(msg);
    });
```

This reads: send the ACK. If it succeeds, return the message. If it fails, propagate the error. The transform captures and moves the message to avoid copying.

### 4.2 Client CLI (client_cli.cpp)

A simple synchronous client that dispatches a single task. It performs these steps:

1. Create socket
2. Set server address to 127.0.0.1:50051
3. Set receive timeout to 1500ms
4. Connect
5. Submit task (hard-coded as "task-0001")
6. Wait for ACK (with timeout)
7. Wait for result (with timeout)
8. Close socket

On timeout, it retries once. On success or failure, it terminates.

This client exists for interactive testing and protocol validation. For scenario testing, use the state machine and scenario harness.

### 4.3 Execution Engine

The Execution Engine is responsible for:

- Building client contexts
- Launching scenario clients
- Applying stagger policies
- Aggregating results
- Returning scenario outcomes

Execution flow:

```
Scenario Configuration
    ↓
Execution Engine
    ↓
Spawn Client
    ↓
future<Result>
    ↓
Aggregate Results
```

The Execution Engine supports:

- HappyPath
- TimeoutRetry
- CachedResult
- ConcurrentClients
- SpedUpWorkers

## 5. Worker Implementation

### 5.1 Worker Runtime

The worker runtime owns the listening socket and accepts inbound TCP connections.

Each accepted connection is converted into a work item and submitted to the WorkerPool.

### 5.1.1 WorkerPool

The WorkerPool owns:

- Fixed-size worker thread set
- Shared work queue
- Queue mutex
- Condition variable
- Graceful shutdown state

Execution flow:

```
Accept Connection
    ↓
Enqueue Item
    ↓
Worker Thread
    ↓
HandleConnection
```

Workers remain idle until work is available.

### 5.1.2 Graceful Shutdown

The WorkerPool performs coordinated shutdown using an explicit pool shutdown state.

Shutdown sequence:

```
Set shutting_down_
    ↓
notify_all()
    ↓
Workers wake
    ↓
Workers exit
    ↓
Join Threads
    ↓
Destroy Pool
```

This guarantees that all worker threads terminate before the pool object is destroyed.

### 5.1.3 Worker Interface

Connection processing is isolated inside worker_interface.

Pipeline:

```
ReceiveMessage
    ↓
SubmitACK
    ↓
Process
    ↓
Execute
    ↓
SendResult
```

HandleConnection coordinates the complete lifecycle of a single connection.

### 5.2 Task Registry (Task_Registry.h)

The registry is a global, mutex-protected data structure on the worker. It tracks the execution state of every task the worker has ever seen.

**Task States:**

- Absent: Not in registry
- Processing: Task is currently executing
- Completed: Task has executed and result is cached

**Registry Operations:**

`try_claim(taskId)`: Acquires the mutex. Checks if taskId exists.

- If absent: Adds to registry with status Processing. Returns Action::Execute.
- If Processing: Returns Action::Reject (duplicate in flight).
- If Completed: Returns Action::Cached with cached result.

`mark_complete(taskId, result)`: Acquires mutex. Changes status from Processing to Completed and stores the result.

**Correctness:** The mutex is held for the entire duration of each operation. This ensures that the decision "execute or reject or cache" is atomic with respect to the status change.

This design guarantees idempotent execution: if the same task arrives twice, only one execution happens. The second arrival gets the cached result.

### 5.2.1 Registry Actions

try_claim(taskId) may produce one of three actions:

**Execute**

The task has never been observed before and should execute.

**Reject**

A task with the same TaskId is currently executing.

**Cached**

The task completed previously and a cached result is available.

### 5.2.2 In-Flight Duplicate Handling

When a duplicate submission arrives while the original task is still executing, the registry returns:

```
Action::Reject
```

The worker converts this state into:

```
TaskStatus::kInProgress
```

This communicates that the task already exists and is currently executing.

Current client behaviour treats this as a failure-like outcome.

Future work:

```
kInProgress
    ↓
RetryDecision
```

instead of immediate failure.

### 5.3 Workload Types

The system supports six workload types for testing:

- **SlowSuccess:** Sleeps for 2 seconds then succeeds
- **FastSuccess:** Succeeds immediately
- **ImmediateFailure:** Fails immediately
- **DelayedFailure:** Sleeps for 2 seconds then fails
- **RandomChance:** Succeeds or fails randomly
- **RandomDelay:** Random sleep duration

Each workload type is used to test different failure scenarios. SlowSuccess is useful for testing timeouts and retries.

### 5.4 Worker Profiles

Each worker owns a Profile:

```
Profile
{
    SpeedClass speed;
    double duration_factor;
}
```

Supported speed classes:

```
Fast
Normal
Slow
```

Execution duration is adjusted according to:

```
effective_duration =
    base_duration * duration_factor
```

Example:

```
Fast   = 0.5
Normal = 1.0
Slow   = 2.0
```

The profile is propagated through:

```
WorkerPool
    ↓
WorkerLoop
    ↓
HandleConnection
    ↓
Process
    ↓
Execute
```

Current default workers use:

```
SpeedClass::Normal
duration_factor = 1.0
```

preserving historical scenario timing.

## 6. Reliability Mechanisms

### 6.1 Timeouts

The client sets SO_RCVTIMEO on its socket. This is a socket option that causes `recv()` to return after a timeout if no data arrives.

**Default:** 1500ms (milliseconds)

**Trigger:** If the worker does not send TASK_RESULT within 1500ms, the recv() call returns 0 (EOF) and WSAGetLastError() returns WSAETIMEDOUT.

**Response:** The client interprets this as a timeout error and may retry.

**Testing:** Set the workload to SlowSuccess (2000ms) and the client timeout to 1500ms. The client will timeout while the worker is still executing. The retry will arrive while the first execution is still Processing. The registry will reject the duplicate. The worker's second execution will complete faster or the client will timeout again, leading to a decision to terminate or retry further.

### 6.2 Retries

The client maintains a retry counter. On timeout, it increments the counter and retries if the counter is below a threshold (currently set in the state machine).

**Current Design:** The client retries once (two total attempts). After the second timeout, it gives up.

**Future Enhancement:** The retry logic should be configurable per task. Some tasks might warrant more retries; others might need exponential backoff.

### 6.3 At-Least-Once Delivery

Because the client retries on timeout, the same task can be submitted multiple times. The dispatcher (or worker, in current design) does not deduplicate based on network-level information. Instead, it relies on the application layer.

The client does include an idempotencyKey field in TASK_SUBMIT. This is currently not used, but the design anticipates per-task deduplication at the dispatcher or a distributed store.

### 6.4 Idempotent Execution

The worker implements idempotency locally via the task registry. Because the registry tracks task state, the second submission of the same task (same taskId) will hit the Completed branch and return the cached result without re-executing.

**Guarantee:** Exactly one execution of the task on this worker, regardless of how many times TASK_SUBMIT arrives.

**Limitation:** This guarantee holds only within a single worker. If the task is submitted to multiple workers, each will execute it once. True deduplication across a cluster requires coordination, which is not yet implemented.

## 7. Development Phases

### Phase 1: Protocol Layer

Implemented transport.h and protocol.h. Verified that TASK_SUBMIT, TASK_ACK, and TASK_RESULT frames are correctly sent and received.

### Phase 2: Standalone Worker

Implemented socket creation, bind, listen, and accept on the worker side. Verified a single inbound connection could be handled.

### Phase 3: Concurrent Worker

Wrapped the connection handler in a std::jthread. Verified multiple concurrent connections are handled independently.

### Phase 4: Dispatcher Refactor

Refactored the client's networking code into discrete functions (initSock, initServAddr, initTimeOut, startConn, submitTask, recAck, recMes). Introduced std::expected for structured error handling instead of return codes.

### Phase 5: Timeouts

Implemented SO_RCVTIMEO socket option. Tested with SlowSuccess workload (2s) and 1500ms timeout. Confirmed timeout correctly triggers retry.

### Phase 6: Retries

Added retry logic to the client state machine. On timeout, the client retries once. Verified retry attempts arrive at the worker.

### Phase 7: At-Least-Once Delivery

Observed that the same task could be submitted multiple times, proving at-least-once behaviour. Logged duplicate deliveries.

### Phase 8: Worker Idempotency

Implemented the TaskRegistry to track per-task execution state. Added the three-state lifecycle: Absent → Processing → Completed. Worker now deduplicates in-flight and completed tasks.

### Phase 9: HandleConnection Refactor

Refactored the worker connection handler into discrete steps: ReceiveMessage, SendACK, SubmitACK, Process, work, SendResult. Used monadic chains with std::expected to reduce nesting and improve readability.

### Phase 10: Duplicate Suppression Validation

End-to-end test with retry-inducing timeout. Confirmed that duplicate submissions are rejected (not re-executed). Demonstrated at-least-once delivery combined with idempotent execution equals exactly-once semantics at the application level.

### Phase 11: Scenario Execution Engine

Introduced a dedicated ExecutionEngine subsystem.

Implemented:

- Scenario-driven execution
- Configurable workloads
- Client spawning
- Result aggregation

### Phase 12: Concurrent Client Execution

Replaced sequential execution with SpawnClient and std::future based collection.

Validated concurrent execution through the ConcurrentClients scenario.

### Phase 13: Worker Interface Refactor

Moved worker request-processing logic into worker_interface.

Separated protocol execution from runtime management.

### Phase 14: WorkerPool Refactor

Removed per-connection thread ownership.

Introduced:

- WorkerPool
- Work queue
- Condition variable synchronization
- Fixed-size worker set

Validated existing scenarios against the WorkerPool implementation.

### Phase 15: Worker Profiles

Introduced heterogeneous worker model.

Workers now own execution profiles containing:

- SpeedClass
- duration_factor

Execution duration is adjusted according to worker characteristics.

## 8. Code Organisation

### Directory Structure

```
src/
├── main.cpp                          # Scenario harness entry point
├── client_cli.cpp                    # Simple synchronous client
├── worker_cli.cpp                    # Worker runtime entry point
│
└── utility/
    ├── Wire/
    │   ├── transport.h               # TCP framing primitives
    │   └── protocol.h                # Message types and TLV encoding
    │
    ├── Task_Registry.h               # Idempotency tracking
    │
    ├── worker_pool/
    │   ├── worker_pool.h
    │   ├── worker_pool.cpp
    │   │
    │   └── worker_interface/
    │       ├── worker_interface.h
    │       └── worker_interface.cpp
    │
    ├── workload/
    │   ├── workload_types.h          # Workload enums
    │   ├── workload.h / .cpp         # Workload execution
    │   └── workload.cpp
    │
    ├── client/
    │   ├── client_types.h            # Client state and context
    │   ├── client_state_machine.h    # State handlers
    │   ├── client_state_machine.cpp
    │   ├── client_interface.h        # Unified client API
    │   └── client_interface.cpp
    │
    ├── scenario/
    │   ├── scenarios.h               # Scenario configs
    │   └── scenarios.cpp             # Scenario builders
    │
    ├── execution/
    │   ├── execution_engine.h        # Scenario runner
    │   └── execution_engine.cpp
    │
    ├── runner/
    │   ├── runner.h
    │   └── runner.cpp
    │
    ├── random/
    │   ├── random_utils.h            # RNG for random workloads
    │   └── random_utils.cpp
    │
    ├── scope_logger/
    │   ├── scope_logger.h            # Scoped logging (RAII)
    │   ├── scope_logger.cpp
    │   └── main.cpp
    │
    └── defer.h                       # Defer/finally pattern
```

### Key Header-Only Files

**defer.h:** Implements a simple defer mechanism (scope guard). When a scope exits, the defer destructor runs a cleanup function. Used for socket closing and resource cleanup.

**scope_logger.h:** RAII logging. Entering a scope logs "Entering X". Exiting logs "Leaving X". Helps trace execution flow during debugging.

### Executable Targets

**worker:** Runs the worker server. Listens on port 50051. Handles concurrent connections.

**client:** Runs the simple synchronous client. Sends one hardcoded task and waits for result.

**smoke_test:** Runs scenario harness. Executes one of four built-in scenarios: HappyPath, TimeoutRetry, CachedResult, ConcurrentClients, or SpedUpWorkers.

**logger:** Debugging utility. Tests scope_logger separately.

## 9. Build System

### CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.20)
project(dispatch_service_smoketest CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

Requires C++23 compiler (GCC 14+, Clang 17+, or MSVC 2022+).

### Building

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

Outputs:

- `build/worker`
- `build/client`
- `build/smoke_test`
- `build/logger`

### Linking

Windows: Links against ws2_32 (Winsock2 library).  
Unix: Links against system socket library (automatic).

## 10. Running the System

### Start Worker

```bash
./build/worker
```

Worker listens on 127.0.0.1:50051. Output:

```
Worker listening on port 50051...
```

### Run Client

In a separate terminal:

```bash
./build/client
```

Client connects, submits task-0001, waits for result. Output:

```
[dispatch]: Socket initialized.
[dispatch]: Server Address initialized.
[dispatch]: Receive_timeout set.
[dispatch]: Connection established.
[dispatch]: Task submitted successfully.
[dispatch]: TaskID(task-0001) acknowledged.
[dispatch]: received TASK_RESULT: status(2). Payload: "..."
[main]: Protocol works.
```

### Run Scenario Tests

```bash
./build/smoke_test
```

Scenarios are selected in main.cpp by uncommenting one of:

```cpp
// const auto conf = scenario::getHappyConf();
// const auto conf = scenario::getTimeoutConf();
// const auto conf = scenario::getCachedConf();
// const auto conf = scenario::getConcurrentConf();
const auto conf = scenario::getSpedUpConf();
```

Each scenario automatically starts the worker and client(s) in separate threads, runs the test, and reports results.

## 11. Scenario Testing

### HappyPath

Submits a single task with FastSuccess workload. Verifies the task completes immediately with no retries.

### TimeoutRetry

Submits a task with SlowSuccess workload (2000ms) and client timeout set to 1500ms. Verifies the client times out, retries, and the duplicate is rejected by the worker registry. Confirms retry succeeds.

### CachedResult

Submits the same task ID twice. Verifies the second submission returns the cached result without re-executing the workload.

### ConcurrentClients

Spawns multiple clients submitting tasks concurrently. Verifies all tasks execute without deadlock and results are correct.

### SpedUpWorkers

Creates a heterogeneous worker pool:

```
Worker 0 → Fast   → 0.5
Worker 1 → Normal → 1.0
Worker 2 → Slow   → 2.0
Worker 3 → Slow   → 2.0
```

Identical workloads are submitted concurrently.

Expected behaviour:

```
Fast Worker ≈ 1 second
Normal Worker ≈ 2 seconds
Slow Worker ≈ 4 seconds
Slow Worker ≈ 4 seconds
```

This validates worker profile propagation and duration scaling.

## 12. Current Capabilities

### Working

- [x] TCP networking with platform abstraction (Windows and Unix)
- [x] Protocol framing (length-prefixed messages)
- [x] Message serialisation (TLV encoding)
- [x] Message deserialisation (TLV decoding)
- [x] Fixed-size WorkerPool
- [x] Shared work queue with condition variable synchronization
- [x] Graceful WorkerPool shutdown
- [x] Concurrent client execution using std::future
- [x] Execution Engine for scenario orchestration
- [x] Worker profile system (SpeedClass, duration_factor)
- [x] Heterogeneous worker execution speeds
- [x] Socket timeouts (SO_RCVTIMEO)
- [x] Retry logic (client-side)
- [x] At-least-once delivery (network layer)
- [x] Idempotent execution (worker-side via registry)
- [x] Task state tracking (Absent → Processing → Completed)
- [x] Error handling via std::expected
- [x] Monadic chaining (and_then, transform)
- [x] Structured logging with scope guards
- [x] Scenario-based testing framework

### Not Yet Implemented

- [ ] Dispatcher component (currently worker is both dispatcher and worker)
- [ ] Multi-worker routing (round-robin, least-loaded, etc.)
- [ ] Socket RAII (currently using defer pattern)
- [ ] Graceful shutdown signal handling
- [ ] Persistent task state (all state is in-memory)
- [ ] Automatic backoff (retries are immediate)
- [ ] Load balancing
- [ ] Health checks for workers
- [ ] Distributed idempotency (idempotency is per-worker)
- [ ] Exactly-once semantics across multiple workers
- [ ] Task cancellation (protocol has kCancel but not implemented)

## 13. Known Limitations

### In-Memory State

The task registry is a global HashMap protected by a mutex. It is never persisted. Restarting the worker loses all task history. This breaks idempotency: if a worker restarts and then receives a duplicate task, it will re-execute because the registry is empty.

### Tight Coupling of Retry Logic

Retry logic is embedded in the client state machine. This makes it difficult to test retry behaviour in isolation or adjust retry policy at runtime.

### Socket Resource Leak Potential

Sockets are not wrapped in RAII. The defer pattern is used instead. If an exception is thrown (std::expected still returns errors, so exceptions are rare, but possible), a socket might not be closed properly. This is noted as a planned improvement.

### Timeout Precision

SO_RCVTIMEO uses OS-level socket timeouts. Precision depends on the OS. On Windows, the minimum granularity is typically 1ms, but actual precision may be lower due to scheduler overhead.

### Worker Scheduling

Workers execute work from a shared queue.

Current limitations:

- No work stealing
- No task prioritisation
- No capability-aware scheduling
- No multiple worker pools

These concerns are intentionally deferred to future orchestrator work.

## 14. Future Roadmap

### Short Term (Next Days)

**Advanced Worker Scheduling:**

- Heterogeneous worker configurations
- Fast/Normal/Slow worker distributions
- Queue-aware scheduling
- Worker capability-aware routing

Potential future enhancements:

- Multiple worker pools
- Task prioritisation
- Work stealing
- Dynamic worker provisioning

**Workload Orchestration:** Build a framework to compose multiple tasks into a workload. Allows testing complex execution patterns (sequential, parallel, conditional).

### Medium Term (Next Week)

**Graceful Shutdown:** Add signal handling (SIGTERM on Unix, console Ctrl+C on Windows). Workers and dispatcher should finish in-flight tasks before terminating.

**Socket RAII:** Wrap sockets in a RAII class to eliminate resource leak risk.

**Exponential Backoff:** Add configurable backoff strategy for retries.

### Long Term (Next Month)

**Persistence:** Integrate a simple KV store (SQLite or RocksDB) to persist task state. Survive worker restarts.

**Distributed Idempotency:** Implement a distributed task registry (Redis or similar). Enable deduplication across multiple workers.

**Exactly-Once Semantics:** Combine multi-worker routing with distributed idempotency to achieve true exactly-once execution.

**Health Checks:** Workers report heartbeat to dispatcher. Dispatcher marks unavailable workers and reroutes tasks.

**Task Cancellation:** Implement kCancel message type. Allow client to cancel in-flight tasks.

**Performance Instrumentation:** Add timing instrumentation. Measure latency, throughput, and resource usage under load.

## 15. Learning Objectives Achieved

### Systems Thinking

This project enforces thinking about systems as layers. Transport knows nothing about tasks. Protocol knows nothing about workers. Workers know nothing about routing. This separation enables testing each layer independently and building correctness incrementally.

### Distributed Systems Concepts

- At-least-once delivery and idempotent execution
- State machines for managing complex interactions
- Timeouts and retry logic
- Duplicate suppression
- Task ownership and concurrency safety

### Modern C++

- std::expected for error handling (no exceptions for errors)
- Monadic chaining (and_then, transform)
- std::jthread for concurrent connection handling
- RAII and scope guards
- C++23 features (std::expected, std::variant, std::optional)
- Platform abstraction patterns
- Move semantics and std::move

### Networking

- TCP socket programming (cross-platform)
- Protocol design (framing, message types, field encoding)
- Handling partial reads and writes
- Socket options (timeouts, non-blocking)
- Buffer management and serialisation

### Testing and Debugging

- Scenario-based testing without a test framework
- Logging for tracing execution
- End-to-end testing across multiple processes
- Debugging concurrency issues

### Concurrency Architecture

- Producer-consumer queue design
- Condition variable synchronization
- Worker pool implementation
- Coordinated worker shutdown
- Queue-based work distribution
- Fixed-size worker threads
- Separation of execution and scheduling concerns

### Scenario-Based Validation

- Regression scenarios for distributed systems behaviour
- Concurrent execution validation
- Timeout and retry testing
- Duplicate suppression testing
- Cached-result validation
- Heterogeneous worker validation

## 16. References for Future Development

### When Adding Multi-Worker Routing

Consider:

- Dispatcher needs to track which tasks are assigned to which workers
- Failure of a worker means reassigning its in-flight tasks
- Different routing strategies have different trade-offs
- Load balancing requires metrics (queue depth, latency, error rate)

### When Implementing Graceful Shutdown

Consider:

- Signal handlers must be async-safe
- Worker threads must be joinable
- In-flight tasks should complete before shutdown
- Clients should receive a clear shutdown signal

### When Adding Persistence

Consider:

- What data needs to persist (task state, results, logs)
- Trade-off between consistency and performance
- How to recover from partial writes
- How to clean up old completed tasks (garbage collection)

## Conclusion

The Dispatch Service is a hands-on study of distributed task execution. It does not aim to be a full replacement for Temporal. Instead, it focuses on understanding the foundational layers: how to send messages reliably, how to handle failures gracefully, and how to ensure tasks execute idempotently even when submitted multiple times.

The phased development demonstrates that complex systems are built from simple, well-tested layers. Each phase adds one concern: networking, concurrency, timeouts, retries, and idempotency. By the end, the combination of these concerns yields a system that works reliably under realistic failure conditions.

The next milestone—multi-worker routing—will introduce the questions that real orchestration systems must answer: how to distribute work, how to detect failures, and how to recover. This is where the learning becomes practical and the system becomes closer to production-grade.