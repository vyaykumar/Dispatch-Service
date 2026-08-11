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

    void WorkerPool::WorkerLoop(const std::stop_token &stop_token, Profile profile) {
        while (!stop_token.stop_requested()) {
            std::unique_lock lock(mutex_);

            condition_variable_.wait(lock ,[this] { return !queue_.empty(); });

            auto [socket] = queue_.front();
            queue_.pop();

            lock.unlock();

            worker::HandleConnection(stop_token, socket, profile);
        }
    }

    WorkerPool::WorkerPool(const size_t worker_count, const Profile profile) {
        workers_.reserve(worker_count);

        for (size_t idx {0}; idx < worker_count; idx++) {
            Worker worker {
                .profile = profile,
                .thread = std::jthread(&WorkerPool::WorkerLoop, this, profile)
            };
            workers_.push_back(std::move(worker));
        }
    }

    void WorkerPool::Enqueue(const Item item) {
        std::unique_lock lock(mutex_);
        queue_.push(item);
        lock.unlock();
        condition_variable_.notify_one();
    }
}
