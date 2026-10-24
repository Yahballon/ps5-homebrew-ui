// ps5-homebrew-ui - Cache stress and stale-job regression tests.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdk/async_images.hpp"
#include "sdk/image_stress.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace
{
constexpr std::array<std::uint8_t, 70> kPng = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48,
    0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00,
    0x00, 0x1f, 0x15, 0xc4, 0x89, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x44, 0x41, 0x54, 0x78,
    0x9c, 0x63, 0x10, 0x50, 0x30, 0xf8, 0x0f, 0x00, 0x02, 0x04, 0x01, 0x60, 0x8d, 0xbc,
    0xbb, 0x71, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

TEST(SdkImageStress, CompletesExactNumberOfWaves)
{
    hui::sdk::ImageStress stress;
    ASSERT_TRUE(stress.start(3, 100));
    EXPECT_EQ(stress.tick(false, false, 100), hui::sdk::StressEvent::none);
    EXPECT_EQ(stress.tick(true, false, 200), hui::sdk::StressEvent::reload);
    EXPECT_EQ(stress.completed(), 1u);
    EXPECT_EQ(stress.tick(true, false, 300), hui::sdk::StressEvent::reload);
    EXPECT_EQ(stress.tick(true, false, 400), hui::sdk::StressEvent::completed);
    EXPECT_EQ(stress.completed(), 3u);
    EXPECT_FALSE(stress.running());
    EXPECT_FALSE(stress.failed());
    EXPECT_EQ(stress.tick(true, false, 450), hui::sdk::StressEvent::none);
}

TEST(SdkImageStress, DetectsFailureAndTimeoutWithoutOverflow)
{
    hui::sdk::ImageStress stress;
    EXPECT_FALSE(stress.start(0, 0));
    ASSERT_TRUE(stress.start(4, 5, 100));
    EXPECT_EQ(stress.tick(false, true, 10), hui::sdk::StressEvent::failed);
    EXPECT_TRUE(stress.failed());
    ASSERT_TRUE(stress.start(2, 200, 100));
    EXPECT_EQ(stress.tick(false, false, 300), hui::sdk::StressEvent::none);
    EXPECT_EQ(stress.tick(false, false, 301), hui::sdk::StressEvent::failed);
    ASSERT_TRUE(stress.start(1, 500, 100));
    stress.cancel();
    EXPECT_EQ(stress.tick(true, false, 600), hui::sdk::StressEvent::none);
}

TEST(SdkAsyncImages, ResetDiscardsOldResultsAndReloadsSameKey)
{
    hui::sdk::JobQueue jobs;
    hui::sdk::ImageCache cpu(4096);
    std::uint32_t next_id = 3;
    hui::sdk::TextureCache gpu(
        4096, [&next_id](const hui::sdk::Image &) { return ++next_id; }, [](std::uint32_t) {});
    std::size_t reads = 0;
    hui::sdk::AsyncImages images(jobs, cpu, gpu,
                                 [&reads](const std::string &, std::size_t)
                                 {
                                     ++reads;
                                     return std::vector<std::uint8_t>(kPng.begin(), kPng.end());
                                 });
    ASSERT_TRUE(images.request("a", "old.png"));
    images.reset();
    ASSERT_TRUE(images.request("a", "new.png"));
    ASSERT_TRUE(jobs.process_one()); // canceled old job
    ASSERT_TRUE(jobs.process_one()); // actual new job
    EXPECT_EQ(images.pump(1), 1u);
    EXPECT_EQ(images.texture("a"), 4u);
    EXPECT_EQ(images.status("a"), hui::sdk::ImageLoadState::ready);
    EXPECT_EQ(reads, 1u);
    EXPECT_EQ(images.statistics().requests, 2u);
    EXPECT_EQ(images.statistics().cancelled, 1u);
    EXPECT_EQ(images.statistics().uploaded, 1u);
}

TEST(SdkAsyncImages, ReportsFailedDecodeAndCanRetry)
{
    hui::sdk::JobQueue jobs;
    hui::sdk::ImageCache cpu(4096);
    hui::sdk::TextureCache gpu(
        4096, [](const hui::sdk::Image &) { return 1u; }, [](std::uint32_t) {});
    hui::sdk::AsyncImages images(jobs, cpu, gpu, [](const std::string &, std::size_t)
                                 { return std::vector<std::uint8_t>{}; });
    ASSERT_TRUE(images.request("broken", "missing.png"));
    ASSERT_TRUE(jobs.process_one());
    EXPECT_EQ(images.pump(), 0u);
    EXPECT_EQ(images.status("broken"), hui::sdk::ImageLoadState::failed);
    EXPECT_EQ(images.statistics().decode_failures, 1u);
    EXPECT_TRUE(images.request("broken", "retry.png"));
}
} // namespace
