// ps5-homebrew-ui - Diagnostics receipt placement regression tests.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdk/diagnostic_export.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

namespace
{

TEST(SdkDiagnosticExport, PrefersBrowsableDataDirectory)
{
    std::vector<std::string> visited;
    const auto receipt = hui::sdk::export_diagnostics(
        "joined=1\n", [](const std::string &) { return true; },
        [&visited](const std::string &path, std::string_view contents)
        {
            visited.push_back(path);
            EXPECT_EQ(contents, "joined=1\n");
            return std::string{};
        });
    EXPECT_TRUE(receipt.saved);
    EXPECT_EQ(receipt.path, "/data/PS5UI_SDK_Consumer/logs/shutdown.txt");
    ASSERT_EQ(visited.size(), 1u);
}

TEST(SdkDiagnosticExport, FallsBackToUsbIfDataIsUnavailable)
{
    const auto receipt = hui::sdk::export_diagnostics(
        "state", [](const std::string &path) { return path.find("/data/") != 0; },
        [](const std::string &, std::string_view) { return std::string{}; });
    EXPECT_TRUE(receipt.saved);
    EXPECT_EQ(receipt.path, "/mnt/usb0/PS5UI_SDK_Consumer/logs/shutdown.txt");
}

TEST(SdkDiagnosticExport, FallsBackWhenFirstWriteFails)
{
    const auto receipt = hui::sdk::export_diagnostics(
        "state", [](const std::string &) { return true; },
        [](const std::string &path, std::string_view)
        { return path.find("/data/") == 0 ? std::string{"permission denied"} : std::string{}; });
    EXPECT_TRUE(receipt.saved);
    EXPECT_EQ(receipt.path, "/mnt/usb0/PS5UI_SDK_Consumer/logs/shutdown.txt");
}

TEST(SdkDiagnosticExport, SignalsFailureWhenAllLocationsAreInaccessible)
{
    const auto receipt = hui::sdk::export_diagnostics(
        "state", [](const std::string &) { return false; },
        [](const std::string &, std::string_view) { return std::string{}; });
    EXPECT_FALSE(receipt.saved);
    EXPECT_TRUE(receipt.path.empty());
}

} // namespace
