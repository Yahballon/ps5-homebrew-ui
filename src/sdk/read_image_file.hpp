// ps5-homebrew-ui - Bounded image-file read for the asynchronous cover pipeline.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/save_file.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hui::sdk
{

// Only call from a worker; the existing checked file reader supports both
// sandbox and elevated filesystem paths subject to title permissions.
// An empty result means the file is empty, missing, inaccessible or too large.
inline std::vector<std::uint8_t> read_image_file(const std::string &path, std::size_t max_bytes)
{
    std::string bytes;
    if (!save::read_file(path, &bytes, max_bytes))
        return {};
    return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
}

} // namespace hui::sdk
