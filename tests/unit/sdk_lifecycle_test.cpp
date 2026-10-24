// ps5-homebrew-ui - Regression tests for job lifecycle and diagnostics.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdk/breadcrumbs.hpp"
#include "sdk/jobs.hpp"
#include "sdk/lifecycle.hpp"
#include "sdk/worker_thread.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <thread>

namespace
{
void idle_host(std::uint32_t)
{
    std::this_thread::yield();
}

TEST(SdkWorker, OwnsThreadAndCanRestart)
{
    hui::sdk::JobQueue queue;
    hui::sdk::WorkerThread worker(queue, &idle_host);
    ASSERT_TRUE(worker.start());
    EXPECT_FALSE(worker.start());
    auto handle = queue.submit([](hui::sdk::JobQueue::Context &ctx) {
        ctx.set_progress(0.4f);
    });
    for (int attempt = 0; attempt < 200000 && !handle.finished(); ++attempt)
        std::this_thread::yield();
    ASSERT_TRUE(handle.finished());
    worker.stop();
    EXPECT_FALSE(worker.running());
    EXPECT_EQ(worker.processed_jobs(), 1u);
    EXPECT_TRUE(worker.start());
    worker.stop();
}

TEST(SdkWorker, CancelsQueuedJobsWithoutCallingThem)
{
    hui::sdk::JobQueue queue;
    std::atomic<int> calls{0};
    auto first = queue.submit([&calls](hui::sdk::JobQueue::Context &) {
        calls.fetch_add(1);
    });
    auto second = queue.submit([&calls](hui::sdk::JobQueue::Context &) {
        calls.fetch_add(1);
    }, false);
    // Stopping before start still rejects queued work.
    hui::sdk::WorkerThread worker(queue, &idle_host);
    worker.stop();
    EXPECT_EQ(calls.load(), 0);
    EXPECT_TRUE(first.finished());
    EXPECT_TRUE(first.cancelled());
    EXPECT_TRUE(second.finished());
    EXPECT_TRUE(second.cancelled());
    EXPECT_EQ(queue.pending_count(), 0u);
    EXPECT_EQ(queue.take_completed().size(), 1u);
}

TEST(SdkLifecycle, InterceptionIsNotSuspendResume)
{
    hui::sdk::LifecycleMonitor monitor;
    EXPECT_EQ(monitor.state(), hui::sdk::LifecycleState::active);
    EXPECT_FALSE(monitor.observe_focus(false));
    EXPECT_TRUE(monitor.observe_focus(true));
    EXPECT_FALSE(monitor.observe_focus(true));
    EXPECT_EQ(monitor.focus_losses(), 1u);
    EXPECT_TRUE(monitor.observe_focus(false));
    EXPECT_EQ(monitor.focus_returns(), 1u);
    EXPECT_TRUE(monitor.display_failed());
    EXPECT_EQ(monitor.state(), hui::sdk::LifecycleState::display_lost);
    EXPECT_FALSE(monitor.observe_focus(false));
    EXPECT_TRUE(monitor.display_recreated(true));
    EXPECT_EQ(monitor.state(), hui::sdk::LifecycleState::input_intercepted);
    EXPECT_EQ(monitor.display_resets(), 1u);
    monitor.begin_shutdown();
    EXPECT_FALSE(monitor.observe_focus(false));
    EXPECT_EQ(monitor.state(), hui::sdk::LifecycleState::stopping);
}

TEST(SdkBreadcrumbs, WrapsAndTruncatesSafely)
{
    hui::sdk::Breadcrumbs events;
    for (std::uint64_t index = 1; index <= 35; ++index)
        events.record(index, "worker", "completed");
    ASSERT_EQ(events.size(), hui::sdk::Breadcrumbs::kCapacity);
    ASSERT_NE(events.at(0), nullptr);
    EXPECT_EQ(events.at(0)->timestamp_us, 4u);
    EXPECT_EQ(events.at(31)->timestamp_us, 35u);
    EXPECT_EQ(events.at(32), nullptr);
    events.record(36, "0123456789012345678901234567",
                  "very long breadcrumb text does not need heap allocations or filesystem writes");
    const auto *last = events.at(31);
    ASSERT_NE(last, nullptr);
    EXPECT_EQ(last->category.back(), '\0');
    EXPECT_EQ(last->message.back(), '\0');
}
} // namespace
