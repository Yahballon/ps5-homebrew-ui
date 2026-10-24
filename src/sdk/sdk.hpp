// ps5-homebrew-ui - Public portable extensions for the homebrew UI kit.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sdk/assets.hpp"
#include "sdk/breadcrumbs.hpp"
#include "sdk/async_images.hpp"
#include "sdk/bmp.hpp"
#include "sdk/diagnostics.hpp"
#include "sdk/i18n.hpp"
#include "sdk/image_decode.hpp"
#include "sdk/image_stress.hpp"
#include "sdk/jobs.hpp"
#include "sdk/lifecycle.hpp"
#include "sdk/navigation.hpp"
#include "sdk/paths.hpp"
#include "sdk/soak_monitor.hpp"
#include "sdk/read_image_file.hpp"
#include "sdk/texture_cache.hpp"
#include "sdk/worker_thread.hpp"

// Existing graphics and components remain opt-in for portability:
// #include "gfx/renderer.hpp"
// #include "ui/components.hpp"
// #include "platform/ps5/display_egl.hpp"
