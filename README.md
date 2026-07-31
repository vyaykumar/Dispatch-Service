# Dispatch

A distributed systems project written in modern C++23, exploring fundamental distributed systems concepts through a small, understandable codebase.

The project models a simple request-processing pipeline:

```text
Client
   │
   ▼
Dispatcher
   │
   ▼
Worker
   │
   ├── ACK
   │
   └── RESULT
```

The goal is not to build production-grade infrastructure. The goal is to explore and demonstrate: TCP networking, message protocols, concurrency, timeouts, retries, at-least-once delivery, idempotency, resource ownership (RAII), and error handling with `std::expected`.

---

## Design Goals

### Reliable Request Delivery

A dispatcher submits work to a worker. The worker acknowledges receipt before executing the task.

```text
TASK_SUBMIT → TASK_ACK → TASK_RESULT
```

### Timeout Handling

Network operations can stall. The dispatcher uses receive timeouts (`SO_RCVTIMEO`) to detect when a response may have been lost. A timeout does not imply task failure — only that the dispatcher no longer knows the task state.

### Retry Logic

When a timeout occurs, the dispatcher retries the request. Retries intentionally introduce the possibility of duplicate delivery.

### At-Least-Once Delivery

Retries create duplicate submissions:
```text
Timeout → Retry → Duplicate Submission
```

A task may be executed more than once. This is a key property of at-least-once delivery systems.

### Idempotency

Worker-side task tracking prevents duplicate execution. The worker maintains task state and recognizes repeated submissions:
```text
Fresh → Processing → Completed
```

---

## Architecture

**Client:** Connect to dispatcher, submit task, receive result.

**Dispatcher:** Create connection, configure timeouts, send task requests, receive acknowledgements and results, retry when appropriate.

**Worker:** Accept incoming connections, process requests concurrently (one `std::jthread` per connection), execute tasks, return results, maintain task state.

Connection lifecycle per worker thread:
1. Receive `TASK_SUBMIT`
2. Check task registry (already executed?)
3. Send `TASK_ACK`
4. Execute task (or return cached result)
5. Send `TASK_RESULT`
6. Close connection

---

## Current Implementation Status

### Completed ✅

**Protocol layer (`transport.h`, `protocol.h`):**
- Length-prefixed frame format with partial-read/write safety
- TLV field encoding with forward compatibility
- Message types: `TASK_SUBMIT`, `TASK_ACK`, `TASK_RESULT`, `CANCEL`
- Verified via round-trip smoke tests

**Worker service (`worker.cpp`):**
- Listener socket setup (bind, listen, `SO_REUSEADDR`)
- Thread-per-connection via `std::jthread`
- Connection lifecycle management: `vector<unique_ptr<Connection{jthread, atomic_bool done}>>` with automatic sweep/reap cleanup
- Basic task execution (placeholder delays: 10ms for cache hit, 100ms for cache miss, 2s for actual execution)

**Dispatcher client (`client.cpp`):**
- Socket creation, address setup, timeout configuration
- Task submission with `TASK_ACK` wait and `TASK_RESULT` wait
- Socket read timeout configured upfront (1.5s, matching worker's 2s execution for testing timeout logic)
- Single retry-on-timeout implemented
- `std::expected`-based error handling with distinguishing error types

**Concurrency & Timeouts:**
- Multiple clients can submit to worker simultaneously — all execute concurrently
- Timeout handling verified: dispatcher correctly detects when responses are slow
- Retry logic verified: dispatcher submits same task twice on timeout (demonstrating at-least-once delivery)

### In Progress 🚧

**Worker refactor & idempotency:**
- `completed_results` map exists but insert is commented out (needs thread-safety fix first)
- `HandleConnection` currently monolithic — needs decomposition into smaller `std::expected`-returning steps (pattern mirrors `dispatch_once` in client)
- Dedup cache needs mutex protection before re-enabling: multiple `jthread`s access it concurrently with no synchronization
- In-flight race still open: two simultaneous submissions of the same task ID both see `contains() == false` and both execute

---

## Building & Testing

```bash
cmake -B build
cmake --build build
```

Binaries:
- `worker` — listens on `localhost:50051`
- `client` — (dispatcher) connects to worker, submits task, receives result

**Testing:**

Terminal 1 (worker):
```bash
./build/worker
```

Terminal 2 (dispatcher):
```bash
./build/client
```

Expected output (dispatcher):
```
[dispatch]: Socket initialized.
[dispatch]: Server Address initialized.
[dispatch]: Receive_timeout set.
[dispatch]: Connection established.
[dispatch]: Task submitted successfully.
[dispatch]: TaskID(task-0001) acknowledged.
[dispatch]: received TASK_RESULT: status(2). Payload: "done: Task executed."
[main]: Protocol works.
```

Expected output (worker):
```
Connection accepted.
Thread started
Message Type is submit. Accepted.
Event Logger: ACK passed.
Checking for Task(task-0001).
[worker]: Result doesn't exist. Executing it.
Event Logger: Result sending passed. Terminating connection.
Thread finished
```

---

## Immediate To-Do

### High Priority (blocking further work)

- [ ] **Implement thread-safe task registry** — Add `std::shared_mutex` to protect `completed_results`; distinguish task lifecycle states (in-flight vs completed) to solve the in-flight race
- [ ] **Refactor `HandleConnection`** — Decompose into smaller `std::expected`-returning steps (receive, ack, check-cache, execute, send-result); mirror the clean composition pattern from `dispatch_once` in the dispatcher
- [ ] **Add socket RAII wrapper** — Replace ad-hoc `transport::CloseSocket(client)` calls with a `ScopedSocket` guard (RAII pattern, consistent with `scoped_thread` from ThreadPool project)

### Medium Priority (Steps 4–5)

- [ ] **Worker selection interface** — Abstract `WorkerSelector` base class; currently hardcoded single address
- [ ] **Event log interface** — Abstract `EventLogger` base class; currently ad-hoc `std::cout` statements
- [ ] **Graceful shutdown support** — Use `std::stop_token` in worker threads to allow clean exit on signal

### Lower Priority (Steps 6–8)

- [ ] **Configurable retry/backoff policy** — Currently single hardcoded retry; generalize to (max_retries, backoff_curve)
- [ ] **Cancellation via `CANCEL` frames** — Worker-side design for handling mid-task cancellation
- [ ] **Dynamic task ID generation** — Replace hardcoded `"task-0001"`; UUID or sequential counter

### Long Term

- [ ] Persistence-backed task state (for worker recovery after restart)
- [ ] Multiple worker support with routing
- [ ] Connection pooling
- [ ] Metrics and observability
- [ ] Exactly-once semantics investigation

---

## Learning Objectives

This project serves as a practical study of:
- Modern C++23 (`std::expected`, `std::jthread`, `std::span`, `std::string_view`)
- TCP networking and socket programming
- Message protocol design (framing, serialization, forward compatibility)
- Concurrency patterns (thread-per-connection, task tracking, synchronization)
- Distributed systems concepts (at-least-once delivery, idempotency, timeouts, retries)
- Resource ownership and RAII
- Error handling and failure modes
- System design trade-offs

Code intentionally favours clarity and experimentation over production complexity.
