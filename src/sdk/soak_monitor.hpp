// ps5-homebrew-ui - Bounded render-thread cache soak metrics.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace hui::sdk
{

// Cache budget telemetry, NOT process-heap or total physical VRAM usage.
// No allocations, locks or disk writes; call once per frame on the UI thread.
class SoakMonitor
{
  public:
    void begin(std::uint64_t timestamp_us)
    {
        active_ = true;
        finished_ = false;
        started_us_ = timestamp_us;
        elapsed_us_ = 0;
        frames_ = 0;
        slow_frames_ = 0;
        cpu_peak_ = 0;
        gpu_peak_ = 0;
        pending_peak_ = 0;
        worst_frame_ms_ = 0.0f;
    }

    void observe(std::uint64_t timestamp_us, std::size_t cpu_bytes, std::size_t gpu_bytes,
                 std::size_t pending, float frame_ms)
    {
        if (!active_)
            return;
        if (timestamp_us >= started_us_)
            elapsed_us_ = timestamp_us - started_us_;
        cpu_peak_ = std::max(cpu_peak_, cpu_bytes);
        gpu_peak_ = std::max(gpu_peak_, gpu_bytes);
        pending_peak_ = std::max(pending_peak_, pending);
        if (frame_ms >= 0.0f && frame_ms < 60000.0f)
        {
            worst_frame_ms_ = std::max(worst_frame_ms_, frame_ms);
            if (frame_ms > 33.334f)
                ++slow_frames_;
        }
        ++frames_;
    }

    void finish(std::uint64_t timestamp_us)
    {
        if (!active_)
            return;
        if (timestamp_us >= started_us_)
            elapsed_us_ = timestamp_us - started_us_;
        active_ = false;
        finished_ = true;
    }

    bool active() const
    {
        return active_;
    }
    bool finished() const
    {
        return finished_;
    }
    std::uint64_t elapsed_ms() const
    {
        return elapsed_us_ / 1000u;
    }
    std::uint64_t frames() const
    {
        return frames_;
    }
    std::uint64_t slow_frames() const
    {
        return slow_frames_;
    }
    std::size_t peak_cpu_bytes() const
    {
        return cpu_peak_;
    }
    std::size_t peak_gpu_bytes() const
    {
        return gpu_peak_;
    }
    std::size_t peak_pending() const
    {
        return pending_peak_;
    }
    float worst_frame_ms() const
    {
        return worst_frame_ms_;
    }

  private:
    std::uint64_t started_us_ = 0;
    std::uint64_t elapsed_us_ = 0;
    std::uint64_t frames_ = 0;
    std::uint64_t slow_frames_ = 0;
    std::size_t cpu_peak_ = 0;
    std::size_t gpu_peak_ = 0;
    std::size_t pending_peak_ = 0;
    float worst_frame_ms_ = 0.0f;
    bool active_ = false;
    bool finished_ = false;
};

} // namespace hui::sdk
