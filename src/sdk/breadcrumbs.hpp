// ps5-homebrew-ui - Bounded, allocation-free diagnostic breadcrumb ring.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace hui::sdk
{

struct Breadcrumb
{
    std::uint64_t timestamp_us = 0;
    std::array<char, 24> category{};
    std::array<char, 64> message{};
};

// UI thread only; record before/after transitions, report on *controlled*
// failure paths. No filesystem writes, dynamic allocation or signal handlers.
// This is not a native exception/crash dump or stack unwinder.
class Breadcrumbs
{
  public:
    static constexpr std::size_t kCapacity = 32;

    void record(std::uint64_t time_us, std::string_view category, std::string_view message)
    {
        auto &event = entries_[next_];
        event.timestamp_us = time_us;
        copy(event.category, category);
        copy(event.message, message);
        next_ = (next_ + 1) % kCapacity;
        if (count_ < kCapacity)
            ++count_;
    }

    std::size_t size() const
    {
        return count_;
    }

    // Oldest first; nullptr if index is out of range.
    const Breadcrumb *at(std::size_t index) const
    {
        if (index >= count_)
            return nullptr;
        return &entries_[(next_ + kCapacity - count_ + index) % kCapacity];
    }

  private:
    template <std::size_t N> static void copy(std::array<char, N> &out, std::string_view value)
    {
        const std::size_t n = value.size() < N - 1 ? value.size() : N - 1;
        for (std::size_t i = 0; i < n; ++i)
            out[i] = value[i];
        out[n] = '\0';
    }

    std::array<Breadcrumb, kCapacity> entries_{};
    std::size_t next_ = 0;
    std::size_t count_ = 0;
};

} // namespace hui::sdk
