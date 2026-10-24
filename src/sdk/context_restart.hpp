// ps5-homebrew-ui - Controlled render-context teardown and restart order.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace hui::sdk
{

enum class ContextRestartStage
{
    succeeded,
    display_open_failed,
    renderer_init_failed,
    fonts_restore_failed
};

struct ContextRestartResult
{
    ContextRestartStage stage = ContextRestartStage::display_open_failed;

    bool ok() const
    {
        return stage == ContextRestartStage::succeeded;
    }
};

// A deliberate, orderly restart of a *still valid* EGL/GL context. All
// callbacks run on the rendering thread. Never use for an already-lost
// context: glDelete* and release_renderer need the old context to be valid.
// The worker may keep decoding but cannot access any GL context.
template <typename ClearGpu, typename ReleaseRenderer, typename CloseDisplay, typename OpenDisplay,
          typename InitializeRenderer, typename RestoreFonts, typename RequestImages>
ContextRestartResult restart_context(ClearGpu clear_gpu, ReleaseRenderer release_renderer,
                                     CloseDisplay close_display, OpenDisplay open_display,
                                     InitializeRenderer initialize_renderer,
                                     RestoreFonts restore_fonts, RequestImages request_images)
{
    // Invalidate image handles and clear GPU textures BEFORE releasing
    // programs, font atlases, and finally the EGL context.
    clear_gpu();
    release_renderer();
    close_display();

    if (!open_display())
        return {ContextRestartStage::display_open_failed};
    if (!initialize_renderer())
    {
        release_renderer();
        close_display();
        return {ContextRestartStage::renderer_init_failed};
    }
    if (!restore_fonts())
    {
        release_renderer();
        close_display();
        return {ContextRestartStage::fonts_restore_failed};
    }
    request_images();
    return {ContextRestartStage::succeeded};
}

const char *context_restart_stage_name(ContextRestartStage stage);

} // namespace hui::sdk
