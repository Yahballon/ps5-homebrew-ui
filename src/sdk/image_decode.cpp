// ps5-homebrew-ui - Bounded image decode adapter around vendored stb_image.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdk/image_decode.hpp"

#define STBI_NO_STDIO
#define STBI_NO_SIMD
#include "third_party/stb/stb_image.h"

#include <cstring>
#include <limits>

namespace hui::sdk
{

std::optional<Image> decode_image(std::span<const std::uint8_t> bytes,
                                  std::size_t max_pixels, std::size_t max_encoded_bytes)
{
    if (bytes.empty() || bytes.size() > max_encoded_bytes ||
        bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return std::nullopt;

    const auto length = static_cast<int>(bytes.size());
    int width = 0;
    int height = 0;
    int channels = 0;
    if (!stbi_info_from_memory(bytes.data(), length, &width, &height, &channels) ||
        width <= 0 || height <= 0)
        return std::nullopt;

    const auto w = static_cast<std::size_t>(width);
    const auto h = static_cast<std::size_t>(height);
    if (h > max_pixels || w > max_pixels / h ||
        w > std::numeric_limits<std::size_t>::max() / h / 4u)
        return std::nullopt;

    stbi_uc *pixels = stbi_load_from_memory(bytes.data(), length, &width, &height, &channels, 4);
    if (!pixels)
        return std::nullopt;

    Image result;
    result.width = width;
    result.height = height;
    result.rgba.resize(w * h * 4u);
    std::memcpy(result.rgba.data(), pixels, result.rgba.size());
    stbi_image_free(pixels);
    return result;
}

} // namespace hui::sdk
