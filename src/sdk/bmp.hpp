// ps5-homebrew-ui - Bounds-checked uncompressed BMP decode to RGBA8.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sdk/assets.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace hui::sdk
{
namespace detail
{
inline std::uint16_t bmp_u16(std::span<const std::uint8_t> data, std::size_t offset)
{
    return static_cast<std::uint16_t>(data[offset]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[offset + 1]) << 8);
}

inline std::uint32_t bmp_u32(std::span<const std::uint8_t> data, std::size_t offset)
{
    return static_cast<std::uint32_t>(data[offset]) |
           (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(data[offset + 3]) << 24);
}
} // namespace detail

// Supports BMP BITMAPINFOHEADER (or larger) with BI_RGB 24/32-bit pixels.
// Alpha in 32-bit BI_RGB is not defined by the format, so it is opaque.
// Indexed images, bitfields, JPEG/PNG-in-BMP and RLE are deliberately rejected.
// Decode on a worker thread, upload via GlBatch on the rendering thread.
inline std::optional<Image> decode_bmp(std::span<const std::uint8_t> bytes,
                                       std::size_t max_pixels = 16u * 1024u * 1024u)
{
    if (bytes.size() < 54 || bytes[0] != 'B' || bytes[1] != 'M')
        return std::nullopt;
    const auto dib_size = detail::bmp_u32(bytes, 14);
    const auto offset = static_cast<std::size_t>(detail::bmp_u32(bytes, 10));
    if (dib_size < 40 || static_cast<std::size_t>(dib_size) > bytes.size() - 14 ||
        offset < static_cast<std::size_t>(14) + dib_size || offset > bytes.size())
        return std::nullopt;

    const std::uint32_t raw_width = detail::bmp_u32(bytes, 18);
    const std::uint32_t raw_height = detail::bmp_u32(bytes, 22);
    if (raw_width == 0 || raw_width > 0x7fffffffu || raw_height == 0 || raw_height == 0x80000000u)
        return std::nullopt;

    const bool top_down = (raw_height & 0x80000000u) != 0;
    const std::size_t width = raw_width;
    const std::size_t height =
        top_down ? 0u - static_cast<std::size_t>(static_cast<std::int32_t>(raw_height))
                 : raw_height;
    if (height == 0 || height > 0x7fffffffu || width > max_pixels / height ||
        detail::bmp_u16(bytes, 26) != 1 || detail::bmp_u32(bytes, 30) != 0)
        return std::nullopt;
    const auto bpp = detail::bmp_u16(bytes, 28);
    if (bpp != 24 && bpp != 32)
        return std::nullopt;

    const auto channels = static_cast<std::size_t>(bpp / 8);
    if (width > (std::numeric_limits<std::size_t>::max() - 3u) / channels)
        return std::nullopt;
    const auto stride = ((width * channels + 3u) / 4u) * 4u;
    if (height > (bytes.size() - offset) / stride)
        return std::nullopt;
    if (width > std::numeric_limits<std::size_t>::max() / height / 4u)
        return std::nullopt;

    Image output;
    output.width = static_cast<int>(width);
    output.height = static_cast<int>(height);
    output.rgba.resize(width * height * 4u);
    for (std::size_t y = 0; y < height; ++y)
    {
        const std::size_t source_y = top_down ? y : height - 1u - y;
        const auto *source = bytes.data() + offset + source_y * stride;
        auto *destination = output.rgba.data() + y * width * 4u;
        for (std::size_t x = 0; x < width; ++x)
        {
            destination[x * 4u + 0u] = source[x * channels + 2u];
            destination[x * 4u + 1u] = source[x * channels + 1u];
            destination[x * 4u + 2u] = source[x * channels + 0u];
            destination[x * 4u + 3u] = 255;
        }
    }
    return output;
}

} // namespace hui::sdk
