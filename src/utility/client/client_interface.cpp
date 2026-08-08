#include "client_interface.h"
#include "../defer.h"

//////////              TO BE ENTIRELY REFACTORED              //////////

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


        /***    Operation on sockets    ***/

        [[nodiscard]] std::expected<void, std::string> submitTask (const transport::socket_t sock, const Context& e_ctx) {

        }

        [[nodiscard]] std::expected<void, std::string> recAck (const transport::socket_t sock) {

        }

        [[nodiscard]] std::expected<Result, std::string> recMes (const transport::socket_t sock, const Context& ctx) {
            const auto start = std::chrono::steady_clock::now();

            auto elapsed = std::chrono::steady_clock::now() - start;

            if (elapsed >= ctx.timeout)
                return std::unexpected("Timeout.");



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
