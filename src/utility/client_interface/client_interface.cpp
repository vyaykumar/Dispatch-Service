#include "client_interface.h"

#include "../defer.h"

namespace client {
    namespace {
        struct ExecConfig {
            const Context& ctx;
            Result& res;
            transport::socket_t sock;
            sockaddr_in addr;
            std::chrono::steady_clock::time_point start;
            std::chrono::steady_clock::time_point deadline;
        };

        // Result failure_struct (const Context& ctx, const std::string& error) {
        //     return {.success = false, .error = error,
        //     .task_id = ctx.task_id, .client_id = ctx.client_id,
        //     .exe_time = 0, .retry_count = 0, .metadata = {}};
        // }

        /***    Helper Functions    ***/

        void logEvent(Result& res, const ClientEvent event) {
            res.metadata.push_back(event);
        }

        void checkDeadline(const ExecConfig& exec, const ClientEvent timeoutEvent) {
            if (std::chrono::steady_clock::now() >= exec.deadline) {
                exec.res.success = false;
                exec.res.error = "Timeout";
                logEvent(exec.res, timeoutEvent);
            }
        }

        /***   Network Stuff     ***/
        void initSock(ExecConfig& conf) {
            auto& sock = conf.sock;

            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == transport::kInvalidSocket)

            return {};
        }

        [[nodiscard]] std::expected<void, std::string>
        initServAddr(ExecConfig& conf) {
            addr.sin_family = AF_INET;
            addr.sin_port = htons(ctx.port);
            inet_pton(AF_INET, ctx.serverAddr.c_str(), &addr.sin_addr);
            return {};
        }

        [[nodiscard]] std::expected<void, std::string>
        initTimeOut(const transport::socket_t sock, const Context& ctx) {
            const auto timeout_ms = ctx.timeout.count();
            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms)) != 0)
                return std::unexpected("Timeout configuration failed");
            return {};
        }

        [[nodiscard]] std::expected<void, std::string>
        startConn(const transport::socket_t sock, sockaddr_in& addr) {
            if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
                return std::unexpected("Connection failed");
            return {};
        }

        /***    Cumulating the network stuff    ***/
        std::expected<transport::socket_t, std::string> preflight(const Context& ctx) {
            transport::socket_t sock;
            sockaddr_in addr;

            if (auto result = initSock(sock, ctx); !result)
                return std::unexpected(result.error());

            if (auto result = initServAddr(addr, ctx); !result)
                return std::unexpected(result.error());

            if (auto result = initTimeOut(sock, ctx); !result)
                return std::unexpected(result.error());

            if (auto result = startConn(sock, addr); !result)
                return std::unexpected(result.error());

            return sock;
        }

        /***    Operation on sockets    ***/

        [[nodiscard]] std::expected<void, std::string> submitTask (const transport::socket_t sock, const Context& ctx) {
            const protocol::TaskSubmit submit{
                .taskId = ctx.task_id,
                .idempotencyKey = ctx.task_id+ctx.client_id,
                .payload = ctx.w_conf.payload,
            };

            if (!protocol::SendTaskSubmit(sock, submit))
                return std::unexpected("Task couldn't be submitted.");

            return {};
        }

        [[nodiscard]] std::expected<void, std::string> recAck (const transport::socket_t sock) {
            if (const auto ackMsg = protocol::ReceiveMessage(sock); !ackMsg or ackMsg->type != protocol::MessageType::kTaskAck) {
                if (WSAGetLastError() == WSAETIMEDOUT)
                    return std::unexpected("Timeout waiting for ACK.");
                return std::unexpected("ACK validation failed.");
            }
            return {};
        }

        [[nodiscard]] std::expected<Result, std::string> recMes (const transport::socket_t sock, const Context& ctx) {
            const auto start = std::chrono::steady_clock::now();
            const auto message = protocol::ReceiveMessage(sock);
            auto elapsed = std::chrono::steady_clock::now() - start;

            if (elapsed >= ctx.timeout)
                return std::unexpected("Timeout.");

            if (message->type != protocol::MessageType::kTaskResult)
                return std::unexpected("Malformed result.");

            const std::string resultText(message->result.payload.begin(), message->result.payload.end());

        }
    }


    Result RunClient(const Context& ctx) {
        transport::PlatformInit();

        const auto sock_e = preflight(ctx);

        // Either a string,
        if (!sock_e)
            return failure_struct(ctx, sock_e.error());
        // or a connected socket.

        const auto sock = sock_e.value();
        defer(transport::CloseSocket(sock));




    }

    std::jthread SpawnClient(const Context& ctx) {
        return std::jthread([ctx]() {
           Result res = RunClient(ctx);
            // Success/Failure populated here.
        });
    }
}
