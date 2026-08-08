//
// Created by VijayKumar on 08-08-2026.
//

#ifndef DISPATCH_SERVICE_SMOKETEST_SCOPE_LOGGER_H
#define DISPATCH_SERVICE_SMOKETEST_SCOPE_LOGGER_H

#include <chrono>
#include <string>
#include <vector>

namespace logging {

    void Event (std::string_view message);
    static std::string Path();

    class EventContext
    {
    public:
        static void Push(std::string_view);
        static void Pop();
        static size_t Depth();
        static std::string Indent(bool is_last = false);

        friend std::string Path();
    private:
        static std::vector<std::string>& Stack();
    };

    class Scope
    {
    public:
        explicit Scope(
            std::string_view name
        );
        ~Scope();

        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
    private:
        std::chrono::steady_clock::time_point start_;
        std::string name_;
    };

}

#define LOG_SCOPE(name) \
    logging::Scope scope_##__LINE__(name)

#endif //DISPATCH_SERVICE_SMOKETEST_SCOPE_LOGGER_H
