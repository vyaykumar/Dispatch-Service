#pragma once

#include <bits/stdc++.h>
#include "../scenario/scenarios.h"
#include "../client/client_interface.h"
#include "../client/client_types.h"

namespace execution {

    struct ExecutionResult {
        std::vector<Result> results;
    };

    Context BuildContext (const scenario::Config& conf, size_t idx);

    ExecutionResult ExecuteScenario(const scenario::Config& conf);
}