#pragma once

#include <bits/stdc++.h>
#include "../scenario/scenarios.h"
#include "../client/client_interface.h"

namespace execution {
    client::cli_ctx;

    struct ExecutionResult {
        std::vector<Result> results;
    };

    ExecutionResult ExecuteScenario(const scenario::Config& config);
}