// ps5-homebrew-ui - Controlled render-context restart diagnostics.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdk/context_restart.hpp"

namespace hui::sdk
{

const char *context_restart_stage_name(ContextRestartStage stage)
{
    switch (stage)
    {
    case ContextRestartStage::succeeded:
        return "succeeded";
    case ContextRestartStage::display_open_failed:
        return "display_open_failed";
    case ContextRestartStage::renderer_init_failed:
        return "renderer_init_failed";
    case ContextRestartStage::fonts_restore_failed:
        return "fonts_restore_failed";
    }
    return "unknown";
}

} // namespace hui::sdk
