#include "utility/scenario/scenarios.h"
#include "utility/execution/execution_engine.h"

int main() {
    auto conf =
        scenario::getHappyConf();

    auto res =
        execution::ExecuteScenario(conf);

    std::cout
        << "Execution count: "
        << res.results.size()
        << '\n';

    for (const auto& r : res.results)
    {
        std::cout
            << "Success: "
            << std::boolalpha
            << r.success
            << '\n';

        std::cout
            << "Retries: "
            << r.retry_count
            << '\n';

        std::cout
            << "Error: "
            << r.error
            << '\n';

        std::cout
            << "Payload: "
            << r.payload
            << '\n';
    }
}