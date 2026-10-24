// ps5-homebrew-ui - Unit tests for the bounded image decoder.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdk/image_decode.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

namespace
{

TEST(SdkImageDecode, DecodesPngToRgba)
{
    constexpr std::array<std::uint8_t, 70> kPng = {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48,
        0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00,
        0x00, 0x1f, 0x15, 0xc4, 0x89, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x44, 0x41, 0x54, 0x78,
        0x9c, 0x63, 0x10, 0x50, 0x30, 0xf8, 0x0f, 0x00, 0x02, 0x04, 0x01, 0x60, 0x8d, 0xbc,
        0xbb, 0x71, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};
    const auto decoded = hui::sdk::decode_image(kPng);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->width, 1);
    EXPECT_EQ(decoded->height, 1);
    ASSERT_EQ(decoded->rgba.size(), 4u);
    EXPECT_EQ(decoded->rgba[0], 0x10);
    EXPECT_EQ(decoded->rgba[1], 0x20);
    EXPECT_EQ(decoded->rgba[2], 0x30);
    EXPECT_EQ(decoded->rgba[3], 255);
    EXPECT_FALSE(hui::sdk::decode_image(kPng, 0));      // pixel cap
    EXPECT_FALSE(hui::sdk::decode_image(kPng, 10, 40)); // input cap
}

TEST(SdkImageDecode, RejectsDamagedImages)
{
    const std::array<std::uint8_t, 6> bad{{'N', 'O', 'T', 'P', 'N', 'G'}};
    EXPECT_FALSE(hui::sdk::decode_image(bad));
}

} // namespace
