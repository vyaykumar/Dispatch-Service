#pragma once
#include <bits/stdc++.h>

#include "utility/work/workloads.h"

namespace scenario {
    enum class Scenes {
        HappyPath,
        TimeoutRetry,
        CachedResult,
        ConcurrentClients,
        RandomChance
    };

    struct SceneConfig {
        Scenes scene;
        work::Config workload;
    };
}
