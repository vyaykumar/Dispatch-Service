#pragma once

#include <bits/stdc++.h>
#include "../scenario/scenarios.h"

namespace execution {
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