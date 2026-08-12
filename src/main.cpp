#include "utility/scenario/scenarios.h"
#include "utility/execution/execution_engine.h"

int main() {
    const auto config = scenario::getHappyConfig();
    // const auto conf = scenario::getTimeoutConf();
    // const auto conf = scenario::getCachedConf();
    // const auto conf = scenario::getConcurrentConf();
    // const auto conf = scenario::getSpeedWorkers();

    auto [results] = execution::ExecuteScenario(config);

    std::cout << "\nExecution count: " << results.size() << "\n\n";

    for (const auto& result : results) {
        std::cout << "Success: " << std::boolalpha << result.success << '\n'
                  << "Retries: " << result.retry_count << '\n'
                  << "Error: "   << result.error << '\n'
                  << "Payload: " << result.payload << "\n\n";
    }
}