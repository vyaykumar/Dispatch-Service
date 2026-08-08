#pragma once

#include <bits/stdc++.h>
#include "../scenario/scenarios.h"

namespace execution {
    struct ClientContext {
        sockaddr_in serv_addr{};
    };

    struct ExecutionResult {
        int successCount;
        int failureCount;
        std::vector<std::string> errors;
    };

    ExecutionResult ExecuteScenario(
        const scenario::Config& config,
        const std::string& serverAddr,
        uint16_t port
    );
}