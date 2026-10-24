// ps5-homebrew-ui - Verified diagnostics receipt path selection.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <string>
#include <string_view>

namespace hui::sdk
{

struct DiagnosticExportResult
{
    bool saved = false;
    std::string path;
};

// Try paths users can actually browse before falling back to an app sandbox.
// Both callbacks are supplied by the caller; no filesystem writes happen
// until the caller explicitly exports a bounded diagnostic receipt.
template <typename EnsureDirectory, typename WriteAtomic>
DiagnosticExportResult export_diagnostics(std::string_view contents, EnsureDirectory ensure,
                                          WriteAtomic write)
{
    struct Candidate
    {
        const char *parent;
        const char *directory;
    };
    constexpr std::array<Candidate, 3> kTargets{{
        {"/data/PS5UI_SDK_Consumer", "/data/PS5UI_SDK_Consumer/logs"},
        {"/mnt/usb0/PS5UI_SDK_Consumer", "/mnt/usb0/PS5UI_SDK_Consumer/logs"},
        {"/download0/hui", "/download0/hui/dev"},
    }};
    for (const auto &target : kTargets)
    {
        if (!ensure(target.parent) || !ensure(target.directory))
            continue;
        const std::string path = std::string(target.directory) + "/shutdown.txt";
        if (write(path, contents).empty())
            return {true, path};
    }
    return {false, {}};
}

} // namespace hui::sdk
