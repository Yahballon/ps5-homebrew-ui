// ps5-homebrew-ui - Deterministic, bounded image-cache stress-test state machine.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

namespace hui::sdk
{

enum class StressEvent
{
    none,
    reload,
    completed,
    failed
};

class ImageStress
{
  public:
    bool start(std::uint32_t cycles, std::uint64_t now_us,
               std::uint64_t round_timeout_us = 15000000)
    {
        if (cycles == 0 || cycles > 1000 || round_timeout_us == 0)
            return false;
        completed_ = 0;
        target_ = cycles;
        started_us_ = now_us;
        timeout_us_ = round_timeout_us;
        running_ = true;
        failed_ = false;
        return true;
    }

    StressEvent tick(bool all_ready, bool any_failed, std::uint64_t now_us)
    {
        if (!running_)
            return StressEvent::none;
        if (any_failed || now_us < started_us_ || now_us - started_us_ > timeout_us_)
        {
            failed_ = true;
            running_ = false;
            return StressEvent::failed;
        }
        if (!all_ready)
            return StressEvent::none;
        ++completed_;
        if (completed_ >= target_)
        {
            running_ = false;
            return StressEvent::completed;
        }
        started_us_ = now_us;
        return StressEvent::reload;
    }

    void cancel()
    {
        running_ = false;
    }

    bool running() const { return running_; }
    bool failed() const { return failed_; }
    std::uint32_t completed() const { return completed_; }
    std::uint32_t target() const { return target_; }

  private:
    std::uint32_t completed_ = 0;
    std::uint32_t target_ = 0;
    std::uint64_t started_us_ = 0;
    std::uint64_t timeout_us_ = 0;
    bool running_ = false;
    bool failed_ = false;
};

} // namespace hui::sdk
