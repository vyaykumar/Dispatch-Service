#include "scope_logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#endif

namespace logging {

    namespace {
        struct WindowsUtf8Setup {
            WindowsUtf8Setup() {
#ifdef _WIN32
                SetConsoleOutputCP(CP_UTF8);
#endif
            }
        };
    }
    static WindowsUtf8Setup utf8_initializer;


    // --- EventContext Implementation ---
    void EventContext::Push(std::string_view name) {
        Stack().emplace_back(name);
    }
    void EventContext::Pop() {
        if (!Stack().empty()) {
            Stack().pop_back();
        }
    }
    size_t EventContext::Depth() {
        return Stack().size();
    }

    // Generates continuous vertical tree lines for deep scopes
    std::string EventContext::Indent(bool is_last) {
        if (Stack().empty()) {
            return "";
        }

        std::string result;
        size_t depth = Stack().size() - 1;

        // Draw vertical bars for all active parent layers above this one
        for (size_t i = 0; i < depth; ++i) {
            result += "│   ";
        }

        return result;
    }

    std::vector<std::string>& EventContext::Stack() {
        // Thread-local ensures isolated stacks per thread
        thread_local std::vector<std::string> stack;
        return stack;
    }

    // --- Scope Implementation ---
    Scope::Scope(std::string_view name)
        : start_(std::chrono::steady_clock::now()), name_(name)
    {
        // 1. PUSH FIRST so stack depth reflects this scope
        EventContext::Push(name_);

        // 2. Print header at current parent indent depth
        std::cout << EventContext::Indent() << "[" << name_ << "]\n";
    }

    Scope::~Scope() {
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> duration_ms = end - start_;

        // 1. Print completion branch at current depth FIRST
        std::cout << EventContext::Indent()
                  << "└── completed in "
                  << std::fixed << std::setprecision(2) << duration_ms.count() << " ms\n";

        // 2. POP LAST so child scope completes before leaving
        EventContext::Pop();
    }

    // --- Namespace Helper Functions ---
    // Standalone Event Log
    void Event(std::string_view message) {
        std::cout << EventContext::Indent() << "├── " << message << "\n";
    }

    // Returns full path like "[Root][Parent][Child]"
    std::string Path() {
        std::ostringstream oss;
        for (const auto& item : EventContext::Stack()) {
            oss << "[" << item << "]";
        }
        return oss.str();
    }
} // namespace logging