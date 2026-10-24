// ps5-homebrew-ui - Regression tests for BMP decoding and the GPU cache.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdk/bmp.hpp"
#include "sdk/texture_cache.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

namespace
{

TEST(SdkBitmap, DecodesOnePixel24BitBgr)
{
    std::array<std::uint8_t, 58> bytes{};
    bytes[0] = 'B';
    bytes[1] = 'M';
    bytes[2] = 58;
    bytes[10] = 54; // pixel data offset
    bytes[14] = 40; // BITMAPINFOHEADER
    bytes[18] = 1;  // width 1
    bytes[22] = 1;  // height 1 (bottom up)
    bytes[26] = 1;  // planes
    bytes[28] = 24; // 24 bits/pixel
    bytes[54] = 0x12; // blue
    bytes[55] = 0x34; // green
    bytes[56] = 0x56; // red
    const auto image = hui::sdk::decode_bmp(bytes);
    ASSERT_TRUE(image.has_value());
    EXPECT_EQ(image->width, 1);
    EXPECT_EQ(image->height, 1);
    EXPECT_EQ(image->rgba, (std::vector<std::uint8_t>{0x56, 0x34, 0x12, 255}));
    bytes[28] = 8; // indexed formats are not silently misdecoded
    EXPECT_FALSE(hui::sdk::decode_bmp(bytes));
}

TEST(SdkBitmap, RejectsTruncatedAndHugeImages)
{
    std::array<std::uint8_t, 54> bytes{};
    bytes[0] = 'B';
    bytes[1] = 'M';
    bytes[10] = 54;
    bytes[14] = 40;
    bytes[18] = 1;
    bytes[22] = 1;
    bytes[26] = 1;
    bytes[28] = 24;
    EXPECT_FALSE(hui::sdk::decode_bmp(bytes));
    bytes[18] = 255;
    bytes[19] = 255;
    EXPECT_FALSE(hui::sdk::decode_bmp(bytes, 12));
}

TEST(SdkTextureCache, EvictsAndExplicitlyFreesTextures)
{
    std::vector<std::uint32_t> destroyed;
    std::uint32_t id = 10;
    hui::sdk::TextureCache cache(
        8, [&id](const hui::sdk::Image &) { return ++id; },
        [&destroyed](std::uint32_t value) { destroyed.push_back(value); });
    hui::sdk::Image image;
    image.width = 1;
    image.height = 1;
    image.rgba.assign(4, 255);
    EXPECT_EQ(cache.get_or_upload("a", image), 11u);
    EXPECT_EQ(cache.get_or_upload("b", image), 12u);
    EXPECT_EQ(cache.get_or_upload("a", image), 11u); // touch a
    EXPECT_EQ(cache.get_or_upload("c", image), 13u); // evicts b
    ASSERT_EQ(destroyed.size(), 1u);
    EXPECT_EQ(destroyed[0], 12u);
    EXPECT_EQ(cache.entries(), 2u);
    cache.clear();
    EXPECT_EQ(destroyed.size(), 3u);
    EXPECT_EQ(cache.used_bytes(), 0u);
}

TEST(SdkTextureCache, ContextLossDoesNotCallGl)
{
    int frees = 0;
    hui::sdk::TextureCache cache(
        4, [](const hui::sdk::Image &) { return 55u; },
        [&frees](std::uint32_t) { ++frees; });
    hui::sdk::Image image;
    image.width = 1;
    image.height = 1;
    image.rgba.assign(4, 1);
    EXPECT_EQ(cache.get_or_upload("icon", image), 55u);
    cache.context_lost();
    EXPECT_EQ(frees, 0);
    EXPECT_EQ(cache.entries(), 0u);
}

} // namespace
