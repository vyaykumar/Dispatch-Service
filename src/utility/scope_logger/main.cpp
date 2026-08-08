#include "scope_logger.h"

#include <thread>
#include <chrono>

using namespace std::chrono_literals;

void Connect()
{
    std::string bingus_id {"615"};
    LOG_SCOPE("Connect"+bingus_id);
    logging::Event("Opening socket");
    std::this_thread::sleep_for(30ms);
    logging::Event("Connection established");
}

void SubmitTask()
{
    LOG_SCOPE("SubmitTask");
    logging::Event("Submitting task");
    std::this_thread::sleep_for(20ms);
    logging::Event("Task submitted");
}

void RunClient()
{
    LOG_SCOPE("RunClient");
    Connect();
    SubmitTask();
}

int main()
{
    LOG_SCOPE("Main");
    RunClient();
}