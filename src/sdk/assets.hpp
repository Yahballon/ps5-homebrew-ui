// ps5-homebrew-ui - Memory-bounded decoded image cache with LRU eviction.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hui::sdk
{

// Decode image bytes on a worker; create and destroy GL textures on UI thread.
struct Image
{
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;

    bool valid() const
    {
        if (width <= 0 || height <= 0)
            return false;
        const auto w = static_cast<std::size_t>(width);
        const auto h = static_cast<std::size_t>(height);
        return w <= std::numeric_limits<std::size_t>::max() / h / 4 &&
               rgba.size() == w * h * 4;
    }
};

class ImageCache
{
  public:
    explicit ImageCache(std::size_t budget_bytes = 32u * 1024u * 1024u)
        : budget_(budget_bytes)
    {
    }

    bool insert(std::string key, Image image)
    {
        if (key.empty() || !image.valid() || image.rgba.size() > budget_)
            return false;
        auto value = std::make_shared<const Image>(std::move(image));
        const auto it = images_.find(key);
        if (it != images_.end())
        {
            used_ -= it->second.value->rgba.size();
            images_.erase(it);
        }
        used_ += value->rgba.size();
        images_.emplace(std::move(key), Entry{std::move(value), ++tick_});
        evict();
        return true;
    }

    std::shared_ptr<const Image> find(std::string_view key)
    {
        const auto it = images_.find(std::string(key));
        if (it == images_.end())
            return {};
        it->second.last_used = ++tick_;
        return it->second.value;
    }

    void erase(std::string_view key)
    {
        const auto it = images_.find(std::string(key));
        if (it == images_.end())
            return;
        used_ -= it->second.value->rgba.size();
        images_.erase(it);
    }

    void clear()
    {
        images_.clear();
        used_ = 0;
    }

    void set_budget(std::size_t bytes)
    {
        budget_ = bytes;
        evict();
    }

    std::size_t used_bytes() const { return used_; }
    std::size_t entries() const { return images_.size(); }

  private:
    struct Entry
    {
        std::shared_ptr<const Image> value;
        std::uint64_t last_used;
    };

    void evict()
    {
        while (used_ > budget_ && !images_.empty())
        {
            auto oldest = images_.begin();
            for (auto it = images_.begin(); it != images_.end(); ++it)
                if (it->second.last_used < oldest->second.last_used)
                    oldest = it;
            used_ -= oldest->second.value->rgba.size();
            images_.erase(oldest);
        }
    }

    std::unordered_map<std::string, Entry> images_;
    std::size_t budget_;
    std::size_t used_ = 0;
    std::uint64_t tick_ = 0;
};

} // namespace hui::sdk
