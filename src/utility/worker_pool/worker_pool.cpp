//
// Created by VijayKumar on 10-08-2026.
//

#include "worker_pool.h"

#include <functional>
#include <mutex>

#include "../scope_logger/scope_logger.h"
#include "worker_interface/worker_interface.h"

namespace worker_pool {
    namespace {
        Profile constructProfile (const size_t choice) {
            switch (choice) {
                case 0 :
                    return {
                    .speed = SpeedClass::kFast,
                    .duration_factor = 0.5,
                };
                case 1 :
                    return {
                    .speed = SpeedClass::kNormal,
                    .duration_factor = 1.0,
                };

                default:
                    return {
                    .speed = SpeedClass::kSlow,
                    .duration_factor = 2.0,
                };
            }
        }
    }

    void WorkerPool::WorkerLoop(const Profile profile) {
        while (!shutting_down_) {
            std::unique_lock lock(mutex_);

            condition_variable_.wait(lock,
                [this] { return !queue_.empty() or shutting_down_; });

            if (shutting_down_)
                return logging::Event("Worker terminated.");;

            if (queue_.empty())
                continue;

            auto [socket] = queue_.front();
            queue_.pop();
            lock.unlock();

            worker::HandleConnection(socket, profile);
            logging::Event("Worker exiting.");
        }
    }

    WorkerPool::WorkerPool(const std::span<size_t> profiles) {
        workers_.reserve(profiles.size());

        for (size_t idx {0}; idx < profiles.size(); idx++) {
            Worker worker { .profile = constructProfile(profiles[idx]) };
            worker.thread = std::jthread(&WorkerPool::WorkerLoop, this, worker.profile);

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