// ps5-homebrew-ui - Decode common encoded images into bounded RGBA8 pixel buffers.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sdk/assets.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace hui::sdk
{

// Decode PNG, JPEG, BMP, GIF (first frame), TGA, PNM, PSD, PIC or HDR/LDR via
// the vendored stb_image decoder. WebP is not supported here.
// Call only from an I/O/worker thread; never decode in the frame loop.
// Limits are checked before allocating decoded pixels.
std::optional<Image> decode_image(std::span<const std::uint8_t> bytes,
                                  std::size_t max_pixels = 16u * 1024u * 1024u,
                                  std::size_t max_encoded_bytes = 32u * 1024u * 1024u);

} // namespace hui::sdk
