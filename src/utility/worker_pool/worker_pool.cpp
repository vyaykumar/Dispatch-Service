//
// Created by VijayKumar on 10-08-2026.
//

#include "worker_pool.h"

#include <functional>
#include <mutex>

#include "worker_interface/worker_interface.h"

namespace worker_pool {
    namespace {

    }

    void WorkerPool::WorkerLoop(const std::stop_token &stop_token) {
        while (true) {
            // Acquire mutex.
            std::unique_lock lock(mutex_);

            // Wait while queue is empty.
            condition_variable_.wait(lock ,[this] { return !queue_.empty(); });

            // Pop queue.
            auto [socket] = queue_.front();
            queue_.pop();

            // Release mutex.
            lock.unlock();

            worker::HandleConnection(stop_token, socket);
        }
    }

    WorkerPool::WorkerPool(const size_t worker_count) {
        workers_.reserve(worker_count);

        for (size_t idx {0}; idx < worker_count; idx++) {
            Worker worker {
                .profile = {
                    .speed = SpeedClass::Normal,
                    .speed_factor = 1.0
                },
                .thread = std::jthread(&WorkerPool::WorkerLoop, this)
            };
            workers_.push_back(std::move(worker));
        }
    }

    void WorkerPool::Enqueue(Item item) {
        // Acquire mutex.
        std::unique_lock lock(mutex_);

        // Push into queue.
        queue_.push(std::move(item));

        // Release mutex.
        lock.unlock();

        condition_variable_.notify_one();
    }
}
