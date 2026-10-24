// ps5-homebrew-ui - Thread-safe background job queue with cancellation and progress.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace hui::sdk
{

// Worker threads are created by the app's PS5 platform adapter. This class
// never starts a thread or calls a UI callback from the worker.
class JobQueue
{
  public:
    struct Shared
    {
        std::atomic<bool> cancelled{false};
        std::atomic<bool> finished{false};
        std::atomic<float> progress{0.0f};
    };

    class Handle
    {
      public:
        bool valid() const { return static_cast<bool>(shared_); }
        void cancel() const
        {
            if (shared_)
                shared_->cancelled.store(true);
        }
        bool cancelled() const { return shared_ && shared_->cancelled.load(); }
        bool finished() const { return shared_ && shared_->finished.load(); }
        float progress() const { return shared_ ? shared_->progress.load() : 0.0f; }

      private:
        explicit Handle(std::shared_ptr<Shared> shared) : shared_(std::move(shared)) {}
        std::shared_ptr<Shared> shared_;
        friend class JobQueue;
    };

    class Context
    {
      public:
        explicit Context(Shared &shared) : shared_(shared) {}
        bool cancelled() const { return shared_.cancelled.load(); }
        void set_progress(float fraction) const
        {
            if (fraction < 0.0f)
                fraction = 0.0f;
            if (fraction > 1.0f)
                fraction = 1.0f;
            shared_.progress.store(fraction);
        }

      private:
        Shared &shared_;
    };

    using Work = std::function<void(Context &)>;

    Handle submit(Work work)
    {
        auto state = std::make_shared<Shared>();
        Handle handle(state);
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.push_back({std::move(work), std::move(state)});
        return handle;
    }

    // Call on a platform worker thread, not in the draw loop.
    bool process_one()
    {
        Job job;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (pending_.empty())
                return false;
            job = std::move(pending_.front());
            pending_.pop_front();
        }
        if (!job.state->cancelled.load() && job.work)
        {
            Context context(*job.state);
            job.work(context);
            if (!context.cancelled())
                context.set_progress(1.0f);
        }
        job.state->finished.store(true);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            completed_.emplace_back(job.state);
        }
        return true;
    }

    // Call on the UI thread; apply results and upload textures there.
    std::vector<Handle> take_completed()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<Handle> result;
        result.reserve(completed_.size());
        while (!completed_.empty())
        {
            result.push_back(std::move(completed_.front()));
            completed_.pop_front();
        }
        return result;
    }

    void cancel_pending()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto &job : pending_)
            job.state->cancelled.store(true);
    }

    std::size_t pending_count() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return pending_.size();
    }

  private:
    struct Job
    {
        Work work;
        std::shared_ptr<Shared> state;
    };

    mutable std::mutex mutex_;
    std::deque<Job> pending_;
    std::deque<Handle> completed_;
};

} // namespace hui::sdk
