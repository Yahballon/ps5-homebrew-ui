// ps5-homebrew-ui - Portable SDK regression tests.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdk/sdk.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{

TEST(SdkNavigation, SupportsPushReplaceAndBack)
{
    hui::sdk::Navigator nav(3);
    EXPECT_FALSE(nav.push("orphan"));
    ASSERT_TRUE(nav.set_root("home"));
    ASSERT_TRUE(nav.push("library", "page=2"));
    ASSERT_TRUE(nav.push("details", "id=7"));
    EXPECT_FALSE(nav.push("too-deep"));
    EXPECT_EQ(nav.depth(), 3u);
    EXPECT_EQ(nav.current()->state, "id=7");
    EXPECT_TRUE(nav.replace("settings"));
    EXPECT_TRUE(nav.pop_to("home"));
    EXPECT_EQ(nav.current()->name, "home");
    EXPECT_FALSE(nav.back());
}

TEST(SdkJobs, RunsOnWorkerAndReturnsCompletionToUi)
{
    hui::sdk::JobQueue jobs;
    std::atomic<int> invoked{0};
    auto handle = jobs.submit([&invoked](hui::sdk::JobQueue::Context &ctx) {
        invoked.fetch_add(1);
        ctx.set_progress(0.6f);
    });
    std::thread worker([&jobs]() { EXPECT_TRUE(jobs.process_one()); });
    worker.join();
    EXPECT_EQ(invoked.load(), 1);
    EXPECT_TRUE(handle.finished());
    EXPECT_FLOAT_EQ(handle.progress(), 1.0f);
    EXPECT_EQ(jobs.take_completed().size(), 1u);
    EXPECT_TRUE(jobs.take_completed().empty());
}

TEST(SdkJobs, CancelledWorkIsNotRun)
{
    hui::sdk::JobQueue jobs;
    bool ran = false;
    auto handle = jobs.submit([&ran](hui::sdk::JobQueue::Context &) { ran = true; });
    handle.cancel();
    EXPECT_TRUE(jobs.process_one());
    EXPECT_FALSE(ran);
    EXPECT_TRUE(handle.finished());
    EXPECT_TRUE(handle.cancelled());
    EXPECT_EQ(jobs.take_completed().size(), 1u);
}

TEST(SdkAssets, ValidatesAndEvictsLeastRecentlyUsed)
{
    hui::sdk::ImageCache cache(32);
    auto make = []() {
        hui::sdk::Image image;
        image.width = 2;
        image.height = 2;
        image.rgba.assign(16, 255);
        return image;
    };
    EXPECT_TRUE(cache.insert("first", make()));
    EXPECT_TRUE(cache.insert("second", make()));
    ASSERT_NE(cache.find("first"), nullptr);
    EXPECT_TRUE(cache.insert("third", make()));
    EXPECT_EQ(cache.find("second"), nullptr);
    EXPECT_NE(cache.find("first"), nullptr);
    EXPECT_EQ(cache.used_bytes(), 32u);
    auto invalid = make();
    invalid.rgba.pop_back();
    EXPECT_FALSE(cache.insert("invalid", std::move(invalid)));
    cache.clear();
    EXPECT_EQ(cache.entries(), 0u);
}

TEST(SdkLocalization, UsesFallbackAndRetainsUtf8)
{
    hui::sdk::Translations text;
    text.set("en", "hello", "Hello, {name}!");
    text.set("de", "hello", "Hallo, {name}!");
    text.set("de", "done", "Fertig – ✓");
    EXPECT_EQ(text.lookup("de", "done"), "Fertig – ✓");
    EXPECT_EQ(text.lookup("fr", "hello"), "Hello, {name}!");
    EXPECT_EQ(text.lookup("de", "missing"), "missing");
    EXPECT_EQ(text.format("de", "hello", {{"name", "YAHBALLON"}}), "Hallo, YAHBALLON!");
}

TEST(SdkDiagnostics, ReportsPercentileAndBudget)
{
    hui::sdk::FrameMonitor monitor;
    for (int i = 0; i < 99; ++i)
        monitor.record(16.0f);
    monitor.record(30.0f);
    const auto report = monitor.summary();
    EXPECT_EQ(report.samples, 100u);
    EXPECT_EQ(report.over_budget, 1u);
    EXPECT_FLOAT_EQ(report.worst_ms, 30.0f);
    EXPECT_FLOAT_EQ(report.p99_ms, 16.0f);
    EXPECT_GT(report.average_ms, 16.0f);
}

TEST(SdkPaths, RefusesTraversalAndUsesConfiguredRoots)
{
    hui::sdk::StoragePaths paths;
    paths.set_root(hui::sdk::StorageRoot::install, "/data/homebrew/PPSA12345");
    EXPECT_EQ(paths.resolve(hui::sdk::StorageRoot::install, "assets/icon0.png"),
              "/data/homebrew/PPSA12345/assets/icon0.png");
    EXPECT_FALSE(paths.resolve(hui::sdk::StorageRoot::install, "../secret"));
    EXPECT_FALSE(paths.resolve(hui::sdk::StorageRoot::install, "a/../secret"));
    EXPECT_FALSE(paths.resolve(hui::sdk::StorageRoot::install, "/absolute"));
    EXPECT_FALSE(paths.resolve(hui::sdk::StorageRoot::install, "a//b"));
}

} // namespace
