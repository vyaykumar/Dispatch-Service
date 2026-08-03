# Dispatch

A distributed systems project written in modern C++23.

The goal of this project is to study networking, concurrency, reliability, retries, error handling, and distributed systems behaviour through a small task execution system.

---

# Architecture

```text
Client
   │
   ▼
Dispatcher
   │
   ▼
Worker
```

Messages:

```text
TASK_SUBMIT
    ↓
TASK_ACK
    ↓
TASK_RESULT
```

---

# Features

Implemented:

- TCP networking
- Protocol framing
- Message serialisation
- Message deserialisation
- Concurrent worker connections
- Timeout handling
- Retry handling
- At-Least-Once delivery
- Worker-side idempotency
- Task ownership tracking
- Automatic thread cleanup
- Error handling via std::expected
- Monadic chaining using std::expected::and_then()

---

# Event History

## Phase 1: Protocol Layer

Implemented:

- transport.h
- protocol.h

Verified:

```text
TASK_SUBMIT
TASK_ACK
TASK_RESULT
```

---

## Phase 2: Standalone Worker

Implemented:

- Socket creation
- Bind
- Listen
- Accept

---

## Phase 3: Concurrent Worker

Implemented:

```text
1 connection
    ↓
1 std::jthread
```

Verified concurrent execution.

---

## Phase 4: Dispatcher Refactor

Refactored networking code into:

```text
initSock()
initServAddr()
initTimeOut()
startConn()
submitTask()
recAck()
recMes()
```

Introduced:

```cpp
std::expected<T, DispatchError>
```

for structured error handling.

---

## Phase 5: Timeouts

Implemented:

```text
SO_RCVTIMEO
```

Test:

```text
Worker Runtime : 2s
Dispatcher Timeout : 1.5s
```

Result:

```text
TASK_ACK
    ↓
TIMEOUT
```

---

## Phase 6: Retries

Implemented retry support.

Behaviour:

```text
Timeout
    ↓
Retry
```

---

## Phase 7: At-Least-Once Delivery

Observed:

```text
Executing Task(task-0001)
Executing Task(task-0001)
```

A single logical task could execute multiple times.

---

## Phase 8: Worker Idempotency

Implemented:

```text
TaskRegistry
```

Task lifecycle:

```text
Absent
    ↓
Processing
    ↓
Completed
```

Worker actions:

```text
Absent
    ↓
Execute

Processing
    ↓
Reject

Completed
    ↓
Cached
```

Introduced:

```cpp
std::mutex
std::unordered_map
```

to protect task ownership.

---

## Phase 9: HandleConnection Refactor

Split responsibilities into:

```text
ReceiveMessage()
SendACK()
SubmitACK()
Process()
work()
SendResult()
```

Introduced monadic chains using:

```cpp
std::expected
.and_then()
.transform()
```

---

## Phase 10: Duplicate Suppression Validation

Test:

```text
Worker Runtime : 2s
Dispatcher Timeout : 1.5s
Retry Enabled
```

Observed:

```text
EXECUTE
REJECT
```

for the same TaskId.

Result:

```text
Duplicate delivery detected.
Duplicate execution prevented.
```

The system now demonstrates:

```text
At-Least-Once Delivery
+
Idempotent Execution
```

---

# Current Status

Completed:

- Protocol Layer
- Dispatcher
- Worker
- Concurrency
- Timeouts
- Retries
- At-Least-Once Delivery
- Task Ownership Tracking
- Idempotent Execution
- std::expected Error Handling

In Progress:

- Completed → Cached validation
- Wait-On-Processing duplicates

Planned:

- Experiment Runner
- Preset Workloads
- Server Assigned UUID Task IDs
- Socket RAII
- Graceful Shutdown
- Multi-Worker Routing

---

# Next Milestone

Validate:

```text
Completed
    ↓
Cached
```

Expected behaviour:

```text
EXECUTE
CACHED
```

with exactly one task execution.

---

# Long-Term Goals

- Preset workload framework
- Scenario execution harness
- UUID task assignment
- Graceful shutdown
- Multiple workers
- Routing
- Reliability experiments
- Failure injection
- Exactly-once investigations

---

# Learning Objectives

- Modern C++23
- Networking
- Concurrency
- RAII
- std::expected
- Distributed Systems
- Idempotency
- Reliability Guarantees
- System Design