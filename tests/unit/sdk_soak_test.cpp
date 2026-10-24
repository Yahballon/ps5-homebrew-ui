// ps5-homebrew-ui - Tests for bounded cache-soak measurements.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdk/soak_monitor.hpp"

#include <gtest/gtest.h>

namespace
{
TEST(SdkSoakMonitor, TracksPeakCachesAndSlowFrames)
{
    hui::sdk::SoakMonitor monitor;
    monitor.begin(100000);
    monitor.observe(120000, 4096, 2048, 8, 16.6f);
    monitor.observe(150000, 8192, 6144, 2, 45.0f);
    monitor.observe(200000, 1024, 512, 0, 16.7f);
    monitor.finish(240000);
    EXPECT_FALSE(monitor.active());
    EXPECT_TRUE(monitor.finished());
    EXPECT_EQ(monitor.elapsed_ms(), 140u);
    EXPECT_EQ(monitor.frames(), 3u);
    EXPECT_EQ(monitor.slow_frames(), 1u);
    EXPECT_EQ(monitor.peak_cpu_bytes(), 8192u);
    EXPECT_EQ(monitor.peak_gpu_bytes(), 6144u);
    EXPECT_EQ(monitor.peak_pending(), 8u);
    EXPECT_FLOAT_EQ(monitor.worst_frame_ms(), 45.0f);
    monitor.observe(250000, 999999, 999999, 20, 60.0f);
    EXPECT_EQ(monitor.peak_cpu_bytes(), 8192u);
}

TEST(SdkSoakMonitor, CanRestartAndIgnoresInvalidFrames)
{
    hui::sdk::SoakMonitor monitor;
    monitor.begin(100);
    monitor.observe(50, 100, 200, 3, -4.0f);
    EXPECT_EQ(monitor.elapsed_ms(), 0u);
    EXPECT_EQ(monitor.worst_frame_ms(), 0.0f);
    monitor.finish(110);
    monitor.begin(5000);
    EXPECT_TRUE(monitor.active());
    EXPECT_FALSE(monitor.finished());
    EXPECT_EQ(monitor.frames(), 0u);
    EXPECT_EQ(monitor.peak_gpu_bytes(), 0u);
}
} // namespace
