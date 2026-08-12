#include "utility/scenario/scenarios.h"
#include "utility/execution/execution_engine.h"

int main() {
    const auto conf = scenario::getHappyConfig();
    // const auto conf = scenario::getTimeoutConf();
    // const auto conf = scenario::getCachedConf();
    // const auto conf = scenario::getConcurrentConf();
    // const auto conf = scenario::getSpeedWorkers();

    auto [results] = execution::ExecuteScenario(conf);

    std::cout << "\nExecution count: " << results.size() << "\n\n";

    for (const auto& r : results) {
        std::cout << "Success: " << std::boolalpha << r.success << '\n'
                  << "Retries: " << r.retry_count << '\n'
                  << "Error: "   << r.error << '\n'
                  << "Payload: " << r.payload << "\n\n";
    }
}