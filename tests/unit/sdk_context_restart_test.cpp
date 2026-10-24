// ps5-homebrew-ui - Controlled EGL/GL restart sequencing.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdk/context_restart.hpp"

#include <gtest/gtest.h>

#include <string>

namespace
{
using hui::sdk::ContextRestartStage;

TEST(SdkContextRestart, SuccessPreservesGpuAndDisplayDestructionOrder)
{
    std::string sequence;
    const auto result = hui::sdk::restart_context(
        [&] { sequence += "G"; }, [&] { sequence += "R"; },
        [&] { sequence += "C"; }, [&] { sequence += "O"; return true; },
        [&] { sequence += "I"; return true; }, [&] { sequence += "F"; return true; },
        [&] { sequence += "Q"; });
    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.stage, ContextRestartStage::succeeded);
    EXPECT_EQ(sequence, "GRCOIFQ");
}

TEST(SdkContextRestart, DisplayFailureNeverUsesOldGpuHandles)
{
    std::string sequence;
    const auto result = hui::sdk::restart_context(
        [&] { sequence += "G"; }, [&] { sequence += "R"; },
        [&] { sequence += "C"; }, [&] { sequence += "O"; return false; },
        [&] { sequence += "I"; return true; }, [&] { sequence += "F"; return true; },
        [&] { sequence += "Q"; });
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.stage, ContextRestartStage::display_open_failed);
    EXPECT_EQ(sequence, "GRCO");
}

TEST(SdkContextRestart, RendererAndFontFailuresCleanUpWithoutReloading)
{
    for (bool renderer_failure : {true, false})
    {
        std::string sequence;
        const auto result = hui::sdk::restart_context(
            [&] { sequence += "G"; }, [&] { sequence += "R"; },
            [&] { sequence += "C"; }, [&] { sequence += "O"; return true; },
            [&] { sequence += "I"; return !renderer_failure; },
            [&] { sequence += "F"; return false; },
            [&] { sequence += "Q"; });
        EXPECT_FALSE(result.ok());
        EXPECT_EQ(result.stage, renderer_failure ? ContextRestartStage::renderer_init_failed
                                                : ContextRestartStage::fonts_restore_failed);
        EXPECT_EQ(sequence, renderer_failure ? "GRCOIRC" : "GRCOIFRC");
    }
}
} // namespace
