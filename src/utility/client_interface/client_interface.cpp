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

        /***    Helper Functions    ***/

        void logEvent (const ExecConfig& conf, const ClientEvent event) {
            conf.res.metadata.push_back(event);
        }

        [[nodiscard]] bool timed_out(const ExecConfig& conf) {
            if (std::chrono::steady_clock::now() >= conf.deadline) {
                logEvent(conf, OutOfTime);
                return true;
            }
            return false;
        }

        /***   Network Stuff     ***/
        void initSock(ExecConfig& conf) {
            // if (timed_out(conf)) return; // Connection hasn't yet been established.
            auto& sock = conf.sock;

            sock = socket(AF_INET, SOCK_STREAM, 0);

            if (sock == transport::kInvalidSocket)
                return logEvent(conf, Socket_Failure);
            logEvent(conf, Socket_Success);
        }

        void initServAddr(ExecConfig& conf) {
            // if (timed_out(conf)) return;
            auto& addr = conf.addr;
            auto& ctx = conf.ctx;

            addr.sin_family = AF_INET;
            addr.sin_port = htons(ctx.port);
            inet_pton(AF_INET, ctx.serverAddr.c_str(), &addr.sin_addr);

            logEvent(conf, Address_Configured);
        }

        void initTimeOut(const ExecConfig& conf) {
            auto& sock = conf.sock;
            auto& ctx = conf.ctx;

            const auto timeout_ms = ctx.timeout.count();

            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms)) != 0)
                return logEvent(conf, Timeout_Failure);

            return logEvent(conf, Timeout_Success);
        }

        void startConn(ExecConfig& conf) {
            auto& sock = conf.sock;
            auto& addr = conf.addr;

            if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
                return logEvent(conf, Connect_Failure);
            return logEvent(conf, Connect_Success);
        }

        /***    Accumulating the network stuff    ***/
        void preflight(ExecConfig& conf) {
            initSock(conf);
            initServAddr(conf);
            initTimeOut(conf);
            startConn(conf);
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
