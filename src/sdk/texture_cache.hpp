// ps5-homebrew-ui - Explicit render-thread GPU image cache with a byte budget.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sdk/assets.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace hui::sdk
{

// This adapter never calls GL itself. Bind upload/free to GlBatch methods.
// All methods must be called on the rendering thread with an active context.
// Call clear() before destroying the context, or context_lost() afterwards.
class TextureCache
{
  public:
    using Upload = std::function<std::uint32_t(const Image &)>;
    using Free = std::function<void(std::uint32_t)>;

    TextureCache(std::size_t max_bytes, Upload upload, Free free)
        : budget_(max_bytes), upload_(std::move(upload)), free_(std::move(free))
    {
    }

    std::uint32_t get_or_upload(std::string key, const Image &image)
    {
        auto it = textures_.find(key);
        if (it != textures_.end())
        {
            it->second.last_used = ++clock_;
            return it->second.texture;
        }
        if (key.empty() || !image.valid() || image.rgba.size() > budget_ || !upload_ ||
            !free_)
            return 0;
        while (used_ > budget_ - image.rgba.size())
            evict_oldest();
        const auto texture = upload_(image);
        if (!texture)
            return 0;
        used_ += image.rgba.size();
        textures_.emplace(std::move(key), Entry{texture, image.rgba.size(), ++clock_});
        return texture;
    }

    std::uint32_t find(std::string_view key)
    {
        const auto it = textures_.find(std::string(key));
        if (it == textures_.end())
            return 0;
        it->second.last_used = ++clock_;
        return it->second.texture;
    }

    bool erase(std::string_view key)
    {
        const auto it = textures_.find(std::string(key));
        if (it == textures_.end())
            return false;
        free_(it->second.texture);
        used_ -= it->second.bytes;
        textures_.erase(it);
        return true;
    }

    void clear()
    {
        if (free_)
            for (const auto &entry : textures_)
                free_(entry.second.texture);
        context_lost();
    }

    // GL objects disappeared with EGL; DO NOT issue glDeleteTextures here.
    void context_lost()
    {
        textures_.clear();
        used_ = 0;
    }

    void set_budget(std::size_t limit)
    {
        budget_ = limit;
        while (used_ > budget_)
            evict_oldest();
    }

    std::size_t used_bytes() const { return used_; }
    std::size_t entries() const { return textures_.size(); }

  private:
    struct Entry
    {
        std::uint32_t texture;
        std::size_t bytes;
        std::uint64_t last_used;
    };

    void evict_oldest()
    {
        if (textures_.empty())
            return;
        auto oldest = textures_.begin();
        for (auto it = textures_.begin(); it != textures_.end(); ++it)
            if (it->second.last_used < oldest->second.last_used)
                oldest = it;
        free_(oldest->second.texture);
        used_ -= oldest->second.bytes;
        textures_.erase(oldest);
    }

    std::size_t budget_ = 0;
    std::size_t used_ = 0;
    std::uint64_t clock_ = 0;
    Upload upload_;
    Free free_;
    std::unordered_map<std::string, Entry> textures_;
};

} // namespace hui::sdk
