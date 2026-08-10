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
        Fast,
        Normal,
        Slow
    };

    struct Profile {
        SpeedClass speed;
        double speed_factor;
    };

    struct Worker {
        Profile profile;
        std::jthread thread;
    };

    class WorkerPool {
    public:
        explicit WorkerPool (size_t worker_count);
        void Enqueue (Item item);

    private:
        void WorkerLoop(const std::stop_token &stop_token);

        std::vector<Worker> workers_;
        std::queue<Item> queue_;
        std::mutex mutex_;
        std::condition_variable condition_variable_;
    };
}


#endif //DISPATCH_SERVICE_SMOKETEST_WORKER_POOL_H
