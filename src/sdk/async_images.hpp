// ps5-homebrew-ui - Nonblocking decoded-cover pipeline with explicit UI-thread uploads.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sdk/assets.hpp"
#include "sdk/image_decode.hpp"
#include "sdk/jobs.hpp"
#include "sdk/texture_cache.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hui::sdk
{

enum class ImageLoadState : std::uint8_t
{
    missing,
    loading,
    ready,
    failed,
    cancelled,
    evicted,
};

// The app owns the OS worker thread which calls jobs.process_one().
// request() and pump() must run on the UI thread. Reader executes on a worker
// and must enforce the byte limit; its results are decoded by stb_image.
// pump() is where textures are uploaded, on the render thread.
class AsyncImages
{
  public:
    using Reader =
        std::function<std::vector<std::uint8_t>(const std::string &, std::size_t max_bytes)>;

    // Counters are owned by the UI thread, so reading them in the frame loop
    // requires no atomics, locks, filesystem access or worker synchronization.
    struct Statistics
    {
        std::uint64_t requests = 0;
        std::uint64_t uploaded = 0;
        std::uint64_t decode_failures = 0;
        std::uint64_t upload_failures = 0;
        std::uint64_t cancelled = 0;
    };

    AsyncImages(JobQueue &jobs, ImageCache &cpu, TextureCache &gpu, Reader read,
                std::size_t max_encoded = 32u * 1024u * 1024u,
                std::size_t max_pixels = 16u * 1024u * 1024u)
        : jobs_(jobs), cpu_(cpu), gpu_(gpu), read_(std::move(read)), max_encoded_(max_encoded),
          max_pixels_(max_pixels)
    {
    }

    bool request(std::string key, std::string path)
    {
        if (key.empty() || path.empty() || !read_)
            return false;
        if (gpu_.find(key))
        {
            states_[key] = ImageLoadState::ready;
            return true;
        }
        if (states_[key] == ImageLoadState::loading)
            return false;
        if (auto cached = cpu_.find(key))
        {
            ready_.push_back({key, std::move(cached)});
            states_[key] = ImageLoadState::loading;
            ++statistics_.requests;
            return true;
        }
        auto result = std::make_shared<PendingResult>();
        auto handle = jobs_.submit(
            [read = read_, path = std::move(path), result, max_bytes = max_encoded_,
             max_pixels = max_pixels_](JobQueue::Context &ctx)
            {
                if (ctx.cancelled())
                    return;
                const auto encoded = read(path, max_bytes);
                if (ctx.cancelled())
                    return;
                ctx.set_progress(0.5f);
                result->image = decode_image(encoded, max_pixels, max_bytes);
            },
            false); // The handle is polled by pump(), not the shared completion deque.
        states_[key] = ImageLoadState::loading;
        pending_.push_back({std::move(key), std::move(handle), std::move(result)});
        ++statistics_.requests;
        return true;
    }

    // Process completed work only; no decoding or disk I/O occurs here.
    // Limit the number of GPU uploads per frame to avoid stalls in cover grids.
    std::size_t pump(std::size_t max_uploads = 2)
    {
        std::size_t uploaded = 0;
        std::size_t attempts = 0;
        for (auto it = ready_.begin(); it != ready_.end() && attempts < max_uploads;)
        {
            const auto texture = gpu_.get_or_upload(it->key, *it->image);
            states_[it->key] = texture ? ImageLoadState::ready : ImageLoadState::failed;
            ++attempts;
            if (texture)
            {
                ++uploaded;
                ++statistics_.uploaded;
            }
            else
                ++statistics_.upload_failures;
            it = ready_.erase(it);
        }
        for (auto it = pending_.begin(); it != pending_.end();)
        {
            if (!it->handle.finished())
            {
                ++it;
                continue;
            }
            if (it->handle.cancelled())
            {
                states_[it->key] = ImageLoadState::cancelled;
                ++statistics_.cancelled;
            }
            else if (!it->result->image)
            {
                states_[it->key] = ImageLoadState::failed;
                ++statistics_.decode_failures;
            }
            else if (attempts >= max_uploads)
            {
                ++it;
                continue;
            }
            else
            {
                auto &decoded = *it->result->image;
                const auto texture = gpu_.get_or_upload(it->key, decoded);
                ++attempts;
                cpu_.insert(it->key, std::move(decoded));
                states_[it->key] = texture ? ImageLoadState::ready : ImageLoadState::failed;
                if (texture)
                {
                    ++uploaded;
                    ++statistics_.uploaded;
                }
                else
                    ++statistics_.upload_failures;
            }
            it = pending_.erase(it);
        }
        return uploaded;
    }

    std::uint32_t texture(std::string_view key)
    {
        return gpu_.find(key);
    }

    ImageLoadState status(std::string_view key)
    {
        const auto it = states_.find(std::string(key));
        if (it == states_.end())
            return ImageLoadState::missing;
        if (it->second == ImageLoadState::ready && !gpu_.find(key))
            return ImageLoadState::evicted;
        return it->second;
    }

    void cancel_all()
    {
        for (auto &pending : pending_)
            pending.handle.cancel();
        for (const auto &pending : ready_)
            states_[pending.key] = ImageLoadState::cancelled;
        statistics_.cancelled += ready_.size();
        ready_.clear();
    }

    // Safe to use on the UI thread for a complete cover-gallery reload.
    // Work callbacks capture their own result buffers (not this object), so
    // abandoned decodes can finish without overwriting freshly queued images.
    // Other JobQueue users are unaffected; no global queue cancellation.
    // CPU/GPU resources remain owned by their caches: clear them separately.
    void reset()
    {
        for (auto &pending : pending_)
            pending.handle.cancel();
        statistics_.cancelled += pending_.size() + ready_.size();
        pending_.clear();
        ready_.clear();
        states_.clear();
    }

    const Statistics &statistics() const
    {
        return statistics_;
    }

    void reset_statistics()
    {
        statistics_ = {};
    }

    std::size_t pending_count() const
    {
        return pending_.size() + ready_.size();
    }

  private:
    struct PendingResult
    {
        std::optional<Image> image;
    };

    struct Pending
    {
        std::string key;
        JobQueue::Handle handle;
        std::shared_ptr<PendingResult> result;
    };

    struct Ready
    {
        std::string key;
        std::shared_ptr<const Image> image;
    };

    JobQueue &jobs_;
    ImageCache &cpu_;
    TextureCache &gpu_;
    Reader read_;
    std::size_t max_encoded_;
    std::size_t max_pixels_;
    std::vector<Pending> pending_;
    std::vector<Ready> ready_;
    std::unordered_map<std::string, ImageLoadState> states_;
    Statistics statistics_;
};

} // namespace hui::sdk
