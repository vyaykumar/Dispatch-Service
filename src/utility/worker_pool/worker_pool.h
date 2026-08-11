#ifndef DISPATCH_SERVICE_SMOKETEST_WORKER_POOL_H
#define DISPATCH_SERVICE_SMOKETEST_WORKER_POOL_H

#include <condition_variable>
#include <queue>
#include <thread>

#include "../Wire/transport.h"

namespace worker_pool {
    struct Item {
        transport::socket_t socket;
    };

    enum class SpeedClass {
        Fast = 0,
        Normal = 1,
        Slow = 2
    };

    inline std::string_view getSpeed (SpeedClass speed) {
        constexpr std::string_view names[] = {"Fast", "Normal", "Slow"};
        return names[static_cast<size_t>(speed)];
    }

    struct Profile {
        SpeedClass speed = SpeedClass::Normal;
        double duration_factor  = 1.0;
    };

    struct Worker {
        Profile profile {};
        std::jthread thread;
    };

    class WorkerPool {
    public:
        explicit WorkerPool (std::span<size_t> profiles);
        void Enqueue (Item item);

    private:
        void WorkerLoop(const std::stop_token &stop_token, Profile profile);

        std::vector<Worker> workers_;
        std::queue<Item> queue_;
        std::mutex mutex_;
        std::condition_variable condition_variable_;
    };
}


#endif //DISPATCH_SERVICE_SMOKETEST_WORKER_POOL_H
