// ps5-homebrew-ui - Unit tests for the async image pipeline.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdk/async_images.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace
{

// Handcrafted 1x1 RGBA PNG to test decode -> upload without a real GPU.
constexpr std::array<std::uint8_t, 70> kPng = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x08, 0x06, 0x00, 0x00, 0x00, 0x1f, 0x15, 0xc4, 0x89, 0x00, 0x00, 0x00,
    0x0d, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x10, 0x50, 0x30, 0xf8,
    0x0f, 0x00, 0x02, 0x04, 0x01, 0x60, 0x8d, 0xbc, 0xbb, 0x71, 0x00, 0x00,
    0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

TEST(SdkAsyncImages, LoadsInWorkerAndUploadsOnlyOnPump)
{
    hui::sdk::JobQueue jobs;
    hui::sdk::ImageCache cpu(4096);
    unsigned uploads = 0;
    hui::sdk::TextureCache gpu(
        4096, [&uploads](const hui::sdk::Image &image) {
            EXPECT_EQ(image.width, 1);
            ++uploads;
            return 77u;
        },
        [](std::uint32_t) {});
    hui::sdk::AsyncImages loader(
        jobs, cpu, gpu,
        [](const std::string &, std::size_t) {
            return std::vector<std::uint8_t>(kPng.begin(), kPng.end());
        });

    EXPECT_TRUE(loader.request("cover", "/data/cover.png"));
    EXPECT_EQ(loader.status("cover"), hui::sdk::ImageLoadState::loading);
    EXPECT_FALSE(loader.request("cover", "/data/cover.png"));
    EXPECT_TRUE(jobs.process_one());
    EXPECT_EQ(uploads, 0u);
    EXPECT_EQ(loader.pump(), 1u);
    EXPECT_EQ(uploads, 1u);
    EXPECT_EQ(loader.texture("cover"), 77u);
    EXPECT_EQ(loader.status("cover"), hui::sdk::ImageLoadState::ready);
    EXPECT_EQ(loader.pending_count(), 0u);
}

TEST(SdkAsyncImages, CancelsWithoutDecoding)
{
    hui::sdk::JobQueue jobs;
    hui::sdk::ImageCache cpu(4096);
    hui::sdk::TextureCache gpu(
        4096, [](const hui::sdk::Image &) { return 1u; },
        [](std::uint32_t) {});
    bool read_called = false;
    hui::sdk::AsyncImages loader(
        jobs, cpu, gpu,
        [&read_called](const std::string &, std::size_t) {
            read_called = true;
            return std::vector<std::uint8_t>{};
        });
    ASSERT_TRUE(loader.request("a", "path"));
    loader.cancel_all();
    EXPECT_TRUE(jobs.process_one());
    EXPECT_EQ(loader.pump(), 0u);
    EXPECT_FALSE(read_called);
    EXPECT_EQ(loader.status("a"), hui::sdk::ImageLoadState::cancelled);
}

} // namespace
