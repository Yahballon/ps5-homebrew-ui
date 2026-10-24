// ps5-homebrew-ui - Dependency-free SDK integration smoke test.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdk/sdk.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace
{
constexpr std::array<std::uint8_t, 70> kPng = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48,
    0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00,
    0x00, 0x1f, 0x15, 0xc4, 0x89, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x44, 0x41, 0x54, 0x78,
    0x9c, 0x63, 0x10, 0x50, 0x30, 0xf8, 0x0f, 0x00, 0x02, 0x04, 0x01, 0x60, 0x8d, 0xbc,
    0xbb, 0x71, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};
} // namespace

int main()
{
    hui::sdk::Navigator nav;
    assert(nav.set_root("home"));
    assert(nav.push("covers"));
    assert(nav.back());
    assert(nav.current() && nav.current()->name == "home");

    hui::sdk::StoragePaths paths;
    assert(paths.resolve(hui::sdk::StorageRoot::save, "covers/icon.png"));
    assert(!paths.resolve(hui::sdk::StorageRoot::save, "../escape"));

    hui::sdk::Translations i18n;
    i18n.set("en", "loading", "Loading {name}");
    assert(i18n.format("fr", "loading", {{"name", "cover"}}) == "Loading cover");

    hui::sdk::FrameMonitor frames;
    frames.record(16.0f);
    frames.record(32.0f);
    assert(frames.summary().over_budget == 1);

    auto decoded = hui::sdk::decode_image(kPng);
    assert(decoded && decoded->valid());
    assert(decoded->width == 1 && decoded->height == 1);
    assert(decoded->rgba[0] == 0x10);

    hui::sdk::JobQueue jobs;
    hui::sdk::ImageCache cpu(4096);
    std::uint32_t next_texture = 100;
    std::size_t destroyed = 0;
    hui::sdk::TextureCache gpu(
        4096, [&next_texture](const hui::sdk::Image &) { return ++next_texture; },
        [&destroyed](std::uint32_t) { ++destroyed; });
    hui::sdk::AsyncImages covers(jobs, cpu, gpu, [](const std::string &, std::size_t)
                                 { return std::vector<std::uint8_t>(kPng.begin(), kPng.end()); });

    assert(covers.request("test", "dummy.png"));
    assert(covers.status("test") == hui::sdk::ImageLoadState::loading);
    std::thread worker([&jobs]() { assert(jobs.process_one()); });
    worker.join();
    assert(covers.pump(0) == 0);
    assert(covers.pump(1) == 1);
    assert(covers.texture("test") == 101);
    assert(covers.status("test") == hui::sdk::ImageLoadState::ready);
    gpu.clear();
    assert(destroyed == 1);
    return 0;
}
