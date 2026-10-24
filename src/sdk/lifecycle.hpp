// ps5-homebrew-ui - Explicit input-focus and render health transitions.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

namespace hui::sdk
{

// This models *observed* input interception and EGL swap failures. Input
// interception is NOT a PS5 OS suspend callback. Full system suspend/resume
// requires an additional platform event adapter and EGL restoration.
enum class LifecycleState
{
    active,
    input_intercepted,
    display_lost,
    stopping
};

class LifecycleMonitor
{
  public:
    // Returns true only on a transition. Caller can log one breadcrumb then,
    // rather than writing a line to /download0 for every intercepted frame.
    bool observe_focus(bool lost)
    {
        if (state_ == LifecycleState::display_lost || state_ == LifecycleState::stopping)
            return false;
        const auto desired = lost ? LifecycleState::input_intercepted : LifecycleState::active;
        if (desired == state_)
            return false;
        state_ = desired;
        if (lost)
            ++focus_losses_;
        else
            ++focus_returns_;
        return true;
    }

    bool display_failed()
    {
        if (state_ == LifecycleState::display_lost || state_ == LifecycleState::stopping)
            return false;
        state_ = LifecycleState::display_lost;
        ++display_errors_;
        return true;
    }

    bool display_recreated(bool focus_lost)
    {
        if (state_ != LifecycleState::display_lost)
            return false;
        state_ = focus_lost ? LifecycleState::input_intercepted : LifecycleState::active;
        ++display_resets_;
        return true;
    }

    void begin_shutdown()
    {
        state_ = LifecycleState::stopping;
    }

    LifecycleState state() const { return state_; }
    std::uint32_t focus_losses() const { return focus_losses_; }
    std::uint32_t focus_returns() const { return focus_returns_; }
    std::uint32_t display_errors() const { return display_errors_; }
    std::uint32_t display_resets() const { return display_resets_; }

  private:
    LifecycleState state_ = LifecycleState::active;
    std::uint32_t focus_losses_ = 0;
    std::uint32_t focus_returns_ = 0;
    std::uint32_t display_errors_ = 0;
    std::uint32_t display_resets_ = 0;
};

} // namespace hui::sdk
