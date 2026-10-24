// ps5-homebrew-ui - Portable pthread-backed worker ownership for SDK jobs.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sdk/jobs.hpp"

#include <pthread.h>

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace hui::sdk
{

// Owns exactly one pthread, including its stack size and join. The idle
// callback is injected so PS5 apps can use sys::sleep_us and host tests can
// use their platform's sleep without pulling platform symbols into this header.
// Do not destroy JobQueue or objects captured by jobs before stop() returns.
class WorkerThread
{
  public:
    using Idle = void (*)(std::uint32_t);

    WorkerThread(JobQueue &queue, Idle idle, std::size_t stack_bytes = 512u * 1024u,
                 std::uint32_t idle_us = 3000)
        : queue_(queue), idle_(idle), stack_bytes_(stack_bytes), idle_us_(idle_us)
    {
    }

    WorkerThread(const WorkerThread &) = delete;
    WorkerThread &operator=(const WorkerThread &) = delete;

    ~WorkerThread()
    {
        stop();
    }

    bool start()
    {
        if (started_ || idle_ == nullptr)
            return false;
        pthread_attr_t attributes{};
        if (pthread_attr_init(&attributes) != 0)
            return false;
        const bool stack_ok = pthread_attr_setstacksize(&attributes, stack_bytes_) == 0;
        stop_.store(false);
        const int status = stack_ok ? pthread_create(&thread_, &attributes, &WorkerThread::entry,
                                                     this)
                                    : -1;
        pthread_attr_destroy(&attributes);
        started_ = status == 0;
        return started_;
    }

    // The caller first cancels cooperative, actively running job handles.
    // Unstarted queued work is cancelled/acknowledged here; joining ensures no
    // worker callback accesses a destroyed image cache after this returns.
    void stop(bool cancel_queued = true)
    {
        if (cancel_queued)
            queue_.cancel_queued();
        if (!started_)
            return;
        stop_.store(true);
        pthread_join(thread_, nullptr);
        started_ = false;
    }

    bool running() const
    {
        return started_;
    }

    std::uint64_t processed_jobs() const
    {
        return processed_.load();
    }

  private:
    static void *entry(void *raw)
    {
        auto *self = static_cast<WorkerThread *>(raw);
        while (!self->stop_.load())
        {
            if (self->queue_.process_one())
                self->processed_.fetch_add(1);
            else
                self->idle_(self->idle_us_);
        }
        return nullptr;
    }

    JobQueue &queue_;
    Idle idle_;
    std::size_t stack_bytes_;
    std::uint32_t idle_us_;
    pthread_t thread_{};
    std::atomic<bool> stop_{false};
    std::atomic<std::uint64_t> processed_{0};
    bool started_ = false;
};

} // namespace hui::sdk
