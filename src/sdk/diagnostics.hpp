// ps5-homebrew-ui - Rolling frame-time monitor without frame-loop allocations.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

namespace hui::sdk
{

struct FrameSummary
{
    std::size_t samples = 0;
    float average_ms = 0.0f;
    float worst_ms = 0.0f;
    float p99_ms = 0.0f;
    std::size_t over_budget = 0;
};

class FrameMonitor
{
  public:
    static constexpr std::size_t kCapacity = 240;

    void record(float duration_ms)
    {
        if (!(duration_ms >= 0.0f) || duration_ms > 60000.0f)
            return;
        samples_[head_] = duration_ms;
        head_ = (head_ + 1) % kCapacity;
        if (count_ < kCapacity)
            ++count_;
    }

    FrameSummary summary(float budget_ms = 16.667f) const
    {
        FrameSummary out;
        out.samples = count_;
        if (!count_)
            return out;
        std::array<float, kCapacity> ordered{};
        float total = 0.0f;
        for (std::size_t i = 0; i < count_; ++i)
        {
            ordered[i] = samples_[i];
            total += ordered[i];
            if (ordered[i] > budget_ms)
                ++out.over_budget;
            if (ordered[i] > out.worst_ms)
                out.worst_ms = ordered[i];
        }
        std::sort(ordered.begin(), ordered.begin() + static_cast<std::ptrdiff_t>(count_));
        out.average_ms = total / static_cast<float>(count_);
        const auto rank = (99 * count_ + 99) / 100;
        out.p99_ms = ordered[rank - 1];
        return out;
    }

  private:
    std::array<float, kCapacity> samples_{};
    std::size_t head_ = 0;
    std::size_t count_ = 0;
};

} // namespace hui::sdk
