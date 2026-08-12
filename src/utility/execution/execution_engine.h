#pragma once

#include <bits/stdc++.h>
#include "../scenario/scenarios.h"

namespace execution {

    struct ExecutionResult {
        std::vector<Result> results;
    };

    Context BuildContext (const scenario::Config&, size_t);
    ExecutionResult ExecuteScenario(const scenario::Config&);
}