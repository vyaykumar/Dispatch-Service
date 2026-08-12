#include "utility/scenario/scenarios.h"
#include "utility/execution/execution_engine.h"

int main() {
    // For HappyPath execution.
    const auto config = scenario::getHappyConfig();

    // For a single client timing-out.
    // const auto config = scenario::getTimeoutConfig();

    // For 1 timed-out submission, and then cached response for the next retry.
    // Evidence in execution times and worker logs.
    // const auto config = scenario::getCachedConfig();

    // For 4 clients sending out tasks at almost the exact same time.
    // HiTrLo struggles here.
    // const auto config = scenario::getConcurrentConfig();

    // For 4 Tasks sent to a pool of 4 varied workers.
    // (Uncomment relevant worker pool initialisation)
    // HiTrLo struggles here.
    // const auto config = scenario::getSpeedWorkersConfig();

    auto [results] = execution::ExecuteScenario(config);

    std::cout << "\nExecution count: " << results.size() << "\n\n";

    for (const auto& result : results) {
        std::cout << "Success: " << std::boolalpha << result.success << '\n'
                  << "Retries: " << result.retry_count << '\n'
                  << "Error: "   << result.error << '\n'
                  << "Payload: " << result.payload << "\n\n";
    }
}