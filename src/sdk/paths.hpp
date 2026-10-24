// ps5-homebrew-ui - PS5 storage roots and traversal-safe relative paths.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace hui::sdk
{

enum class StorageRoot : std::size_t
{
    install,
    save,
    data,
    usb0,
};

class StoragePaths
{
  public:
    StoragePaths() : roots_{"/app0", "/download0", "/data", "/mnt/usb0"} {}

    void set_root(StorageRoot kind, std::string path)
    {
        roots_[static_cast<std::size_t>(kind)] = std::move(path);
    }

    std::optional<std::string> resolve(StorageRoot kind, std::string_view relative) const
    {
        if (relative.empty() || relative.front() == '/' || relative.front() == '\\')
            return std::nullopt;
        std::size_t start = 0;
        while (start < relative.size())
        {
            const auto end = relative.find('/', start);
            const auto part = relative.substr(start, end == std::string_view::npos
                                                         ? std::string_view::npos
                                                         : end - start);
            if (part.empty() || part == "." || part == ".." ||
                part.find('\\') != std::string_view::npos ||
                part.find(':') != std::string_view::npos)
                return std::nullopt;
            if (end == std::string_view::npos)
                break;
            start = end + 1;
        }
        const auto &root = roots_[static_cast<std::size_t>(kind)];
        if (root.empty() || root.front() != '/' || root.back() == '/')
            return std::nullopt;
        return root + "/" + std::string(relative);
    }

  private:
    std::array<std::string, 4> roots_;
};

} // namespace hui::sdk
