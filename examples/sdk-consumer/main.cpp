// ps5-homebrew-ui - Independent PS5 consumer linked to the packaged static UI SDK.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

// No project implementation .cpp is built into this title. All UI, graphics,
// controller, file and image-decoding code comes from dist/sdk/lib/libps5hui.a.
#include "sdk/async_images.hpp"
#include "sdk/breadcrumbs.hpp"
#include "sdk/lifecycle.hpp"
#include "sdk/image_stress.hpp"
#include "sdk/soak_monitor.hpp"
#include "sdk/worker_thread.hpp"
#include "sdk/diagnostics.hpp"
#include "sdk/diagnostic_export.hpp"
#include "sdk/context_restart.hpp"
#include "sdk/navigation.hpp"
#include "sdk/read_image_file.hpp"
#include "sdk/texture_cache.hpp"

#include "core/input.hpp"
#include "core/save_file.hpp"
#include "gfx/draw_list.hpp"
#include "gfx/font.hpp"
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"

#include <GL/glcorearb.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <string_view>

namespace
{

using hui::gfx::Color;
using hui::gfx::DrawList;
using hui::gfx::Rect;
constexpr const char *kImagePath = "/app0/sce_sys/icon0.png";
constexpr const char *kFontPath = "/app0/assets/fonts/inter-regular.huifont";
constexpr const char *kFontBold = "/app0/assets/fonts/inter-semibold.huifont";
constexpr std::size_t kTiles = 8;

struct ShutdownReceipt
{
    bool ready = false;
    bool joined = false;
    bool saved = false;
    std::size_t cancelled = 0;
    std::uint64_t processed = 0;
    std::string path;
};

struct RestartStatus
{
    std::uint32_t attempts = 0;
    std::uint32_t successful = 0;
    std::uint64_t last_duration_us = 0;
    hui::sdk::ContextRestartStage last = hui::sdk::ContextRestartStage::succeeded;
};

struct Face
{
    hui::gfx::Font font;
    std::uint32_t texture = 0;
};

bool load_face(hui::gfx::Renderer &renderer, Face &face, const char *path)
{
    std::string data;
    if (!hui::save::read_file(path, &data) || !face.font.load(data))
        return false;
    face.texture = renderer.batch().create_font_texture(face.font);
    return face.texture != 0;
}

void label(DrawList &draw, const Face &face, std::string_view value, float x, float y, float size,
           Color color)
{
    draw.text(face.font, face.texture, value, x, y, size, color);
}

const char *image_state(hui::sdk::ImageLoadState state)
{
    switch (state)
    {
    case hui::sdk::ImageLoadState::ready:
        return "READY";
    case hui::sdk::ImageLoadState::loading:
        return "LOADING";
    case hui::sdk::ImageLoadState::failed:
        return "FAILED";
    case hui::sdk::ImageLoadState::evicted:
        return "EVICTED";
    case hui::sdk::ImageLoadState::cancelled:
        return "CANCELLED";
    case hui::sdk::ImageLoadState::missing:
        return "WAITING";
    }
    return "UNKNOWN";
}

void draw_page(DrawList &draw, const Face &regular, const Face &bold,
               const hui::sdk::Navigator &navigation, hui::sdk::AsyncImages &images,
               const hui::sdk::ImageCache &cpu, const hui::sdk::TextureCache &gpu,
               const hui::sdk::FrameMonitor &frames, bool worker_running,
               const hui::sdk::LifecycleMonitor &lifecycle, const hui::sdk::ImageStress &stress,
               const hui::sdk::SoakMonitor &soak, const RestartStatus &restart,
               const ShutdownReceipt &receipt, std::uint64_t processed_jobs, std::size_t selected,
               std::uint64_t frame_count)
{
    const Color foreground = Color::rgb(0xffffff);
    const Color dim = Color::rgb(0x9fb1c6);
    const Color mint = Color::rgb(0x3de3c6);
    const Color card = Color::rgb(0x132c44);
    draw.clear();
    draw.gradient_rect({0, 0, 1920, 1080}, 0, Color::rgb(0x06101c), Color::rgb(0x11294e));
    draw.rounded_rect({72, 53, 1776, 140}, 24, card);
    draw.rounded_rect({72, 53, 13, 140}, 6, mint);
    label(draw, bold, "PS5 UI SDK  /  ARCHIVE CONSUMER", 110, 117, 42, foreground);
    label(draw, regular, "by YAHBALLON  |  v0.7.0  |  libps5hui.a linked", 114, 161, 23, dim);

    const auto *route = navigation.current();
    const std::string current = route ? route->name : "home";
    if (receipt.ready)
    {
        draw.rounded_rect({154, 254, 1610, 662}, 32, card);
        label(draw, bold, "WORKER SHUTDOWN RECEIPT", 215, 340, 44, foreground);
        label(draw, regular, "The worker has stopped. You can photograph this screen.", 218, 400,
              26, dim);
        label(draw, bold, receipt.joined ? "WORKER JOINED: YES" : "WORKER JOINED: NO", 225, 505, 40,
              receipt.joined ? mint : Color::rgb(0xff7e8b));
        char report[160];
        std::snprintf(report, sizeof(report), "Pending jobs cancelled: %zu    Processed jobs: %llu",
                      receipt.cancelled, static_cast<unsigned long long>(receipt.processed));
        label(draw, regular, report, 225, 569, 28, foreground);
        label(draw, bold,
              receipt.saved ? "DIAGNOSTIC FILE: SAVED" : "DIAGNOSTIC FILE: NO WRITABLE LOCATION",
              225, 670, 31, receipt.saved ? mint : Color::rgb(0xffb37d));
        if (receipt.saved)
            label(draw, regular, receipt.path, 225, 735, 24, foreground);
        else
            label(draw, regular,
                  "Use a screenshot of this screen; the shutdown status is shown above.", 225, 735,
                  23, dim);
        label(draw, bold, "PRESS X TO EXIT", 225, 843, 32, mint);
    }
    else if (current == "home")
    {
        draw.rounded_rect({165, 295, 1590, 539}, 35, card);
        draw.circle(960, 450, 76, mint);
        draw.circle(960, 450, 49, card);
        label(draw, bold, "STANDALONE SDK CONSUMER", 525, 591, 51, foreground);
        label(draw, regular, "No UI implementation sources were compiled into this app.", 525, 669,
              28, dim);
        label(draw, bold, "X  START IMAGE & NAVIGATION TEST", 530, 763, 29, mint);
    }
    else if (current == "covers")
    {
        label(draw, bold, "PNG DECODING  /  GPU CACHE", 110, 271, 37, foreground);
        if (stress.target())
        {
            char progress[80];
            std::snprintf(progress, sizeof(progress), "STRESS %u/%u %s", stress.completed(),
                          stress.target(),
                          stress.running()  ? "RUNNING"
                          : stress.failed() ? "FAIL"
                                            : "PASS");
            label(draw, bold, progress, 1320, 270, 22,
                  stress.failed() ? Color::rgb(0xff7e8b) : mint);
        }
        label(draw, regular, "D-Pad selects covers; Square rereads PNG; Triangle clears GPU.", 110,
              317, 24, dim);
        for (std::size_t index = 0; index < kTiles; ++index)
        {
            const float x = 108.0f + static_cast<float>(index % 4) * 441.0f;
            const float y = 360.0f + static_cast<float>(index / 4) * 286.0f;
            const bool focus = index == selected;
            draw.bordered_rect({x, y, 411, 254}, 24, focus ? Color::rgb(0x245269) : card,
                               focus ? 4.0f : 1.0f, focus ? mint : Color::rgb(0x35506a));
            char key[32];
            std::snprintf(key, sizeof(key), "arch-%zu", index);
            const std::uint32_t texture = images.texture(key);
            if (texture)
                draw.image(texture, {x + 13, y + 19, 189, 190}, hui::gfx::kFullUv, foreground, 15);
            else
                draw.rounded_rect({x + 13, y + 19, 189, 190}, 17, Color::rgb(0x29445e));
            char name[40];
            std::snprintf(name, sizeof(name), "IMAGE %02zu", index + 1);
            label(draw, bold, name, x + 220, y + 88, 27, foreground);
            label(draw, regular, image_state(images.status(key)), x + 220, y + 135, 22,
                  texture ? mint : dim);
            if (focus)
                label(draw, bold, "SELECTED", x + 18, y + 240, 21, mint);
        }
    }
    else
    {
        const auto perf = frames.summary();
        draw.rounded_rect({156, 294, 1614, 594}, 32, card);
        label(draw, bold, "SDK LINK & FRAME TELEMETRY", 210, 373, 42, foreground);
        char recovery_text[160];
        std::snprintf(recovery_text, sizeof(recovery_text),
                      "GL restart: %u/%u OK  last=%s  %.1f ms", restart.successful,
                      restart.attempts,
                      restart.attempts == 0 ? "NOT TESTED"
                                            : hui::sdk::context_restart_stage_name(restart.last),
                      static_cast<double>(restart.last_duration_us) / 1000.0);
        label(draw, regular, recovery_text, 815, 295, 22,
              restart.successful == restart.attempts ? mint : Color::rgb(0xff7e8b));
        char number[160];
        std::snprintf(number, sizeof(number), "Frames:  %llu",
                      static_cast<unsigned long long>(frame_count));
        label(draw, regular, number, 231, 460, 31, mint);
        std::snprintf(number, sizeof(number), "Average: %.2f ms", perf.average_ms);
        label(draw, regular, number, 231, 519, 31, foreground);
        std::snprintf(number, sizeof(number), "P99: %.2f ms", perf.p99_ms);
        label(draw, regular, number, 231, 578, 31, foreground);
        std::snprintf(number, sizeof(number), "Max: %.2f ms", perf.worst_ms);
        label(draw, regular, number, 231, 637, 31, foreground);
        std::snprintf(number, sizeof(number), "CPU: %zu KiB / %zu images", cpu.used_bytes() / 1024,
                      cpu.entries());
        label(draw, regular, number, 925, 465, 28, foreground);
        std::snprintf(number, sizeof(number), "GPU: %zu KiB / %zu textures",
                      gpu.used_bytes() / 1024, gpu.entries());
        label(draw, regular, number, 925, 532, 28, foreground);
        label(draw, regular, worker_running ? "Worker: RUNNING" : "Worker: UNAVAILABLE", 925, 597,
              28, worker_running ? mint : dim);
        label(draw, regular, "UI implementations: STATIC ARCHIVE", 925, 669, 26, mint);
        std::snprintf(number, sizeof(number), "Background jobs: %llu",
                      static_cast<unsigned long long>(processed_jobs));
        label(draw, regular, number, 925, 728, 26, foreground);
        std::snprintf(number, sizeof(number), "Focus losses / returns: %u / %u",
                      lifecycle.focus_losses(), lifecycle.focus_returns());
        label(draw, regular, number, 925, 788, 23, dim);
        const auto &stats = images.statistics();
        std::snprintf(number, sizeof(number), "Uploads %llu  Errors: decode %llu / GPU %llu",
                      static_cast<unsigned long long>(stats.uploaded),
                      static_cast<unsigned long long>(stats.decode_failures),
                      static_cast<unsigned long long>(stats.upload_failures));
        label(draw, regular, number, 225, 723, 21, dim);
        std::snprintf(number, sizeof(number), "STRESS %u/%u %s", stress.completed(),
                      stress.target(),
                      stress.running()  ? "RUNNING"
                      : stress.failed() ? "FAILED"
                                        : "IDLE / PASS");
        label(draw, regular, number, 225, 784, 23, stress.failed() ? Color::rgb(0xff7e8b) : mint);
        std::snprintf(number, sizeof(number), "Peak cache CPU/GPU %zu/%zu KiB",
                      soak.peak_cpu_bytes() / 1024u, soak.peak_gpu_bytes() / 1024u);
        label(draw, regular, number, 225, 845, 21, dim);
        std::snprintf(number, sizeof(number), "Slow frames (>33ms) %llu   Worst %.2f ms",
                      static_cast<unsigned long long>(soak.slow_frames()), soak.worst_frame_ms());
        label(draw, regular, number, 925, 846, 21, dim);
    }
    draw.rounded_rect({73, 966, 1775, 65}, 20, Color::rgb(0x102639));
    if (receipt.ready)
        label(draw, regular, "SHUTDOWN COMPLETE     X  EXIT TO PS5 HOME", 106, 1008, 24, mint);
    else
        label(draw, regular,
              "X  OPEN / NEXT    O  BACK    SQUARE  RELOAD    TRIANGLE  CLEAR GPU    "
              "L1  GL RESTART    R1  20    R2  200    OPTIONS  RECEIPT",
              106, 1008, 21, dim);
}

} // namespace

int main()
{
    using namespace hui;
    sys::log("[HUI ARCHIVE CONSUMER] start v0.7.0 libps5hui.a");
    ps5::Display display;
    if (!display.open(1920, 1080))
    {
        sys::log("[HUI ARCHIVE CONSUMER] EGL startup failed");
        sys::park();
    }
    gfx::Renderer renderer;
    Face regular;
    Face bold;
    if (!renderer.init() || !load_face(renderer, regular, kFontPath) ||
        !load_face(renderer, bold, kFontBold))
    {
        sys::log("[HUI ARCHIVE CONSUMER] renderer/font initialization failed");
        sys::park();
    }

    sdk::JobQueue queue;
    sdk::ImageCache cpu(24u * 1024u * 1024u);
    sdk::TextureCache gpu(
        24u * 1024u * 1024u, [&renderer](const sdk::Image &image)
        { return renderer.batch().create_texture(image.width, image.height, image.rgba.data()); },
        [&renderer](std::uint32_t id) { renderer.batch().destroy_texture(id); });
    sdk::AsyncImages images(queue, cpu, gpu, sdk::read_image_file);
    sdk::WorkerThread worker(queue, &sys::sleep_us);
    const bool worker_ok = worker.start();
    sdk::LifecycleMonitor lifecycle;
    sdk::Breadcrumbs breadcrumbs;
    breadcrumbs.record(static_cast<std::uint64_t>(sys::monotonic_us()), "boot",
                       worker_ok ? "worker running" : "worker failed");

    ps5::Pad pad;
    pad.open();
    InputTracker controller;
    PadSample pad_samples[64];
    sdk::Navigator nav;
    nav.set_root("home");
    sdk::FrameMonitor frametimes;
    sdk::ImageStress stress;
    sdk::SoakMonitor soak;
    ShutdownReceipt receipt;
    RestartStatus restart;
    gfx::DrawList frame;
    std::uint64_t frame_count = 0;
    std::size_t selected = 0;
    std::int64_t previous = sys::monotonic_us();

    const auto load_covers = [&]()
    {
        if (!worker_ok)
            return;
        for (std::size_t i = 0; i < kTiles; ++i)
        {
            char key[32];
            std::snprintf(key, sizeof(key), "arch-%zu", i);
            images.request(key, kImagePath);
        }
    };

    const auto reload_all = [&]()
    {
        images.reset();
        cpu.clear();
        gpu.clear();
        load_covers();
    };

    for (;;)
    {
        const std::int64_t now = sys::monotonic_us();
        const std::size_t samples = pad.read(std::span<PadSample>(pad_samples));
        const auto input = controller.update(std::span<const PadSample>(pad_samples, samples),
                                             static_cast<std::uint64_t>(now));

        if (lifecycle.observe_focus(input.focus_lost))
        {
            breadcrumbs.record(static_cast<std::uint64_t>(now), "input",
                               input.focus_lost ? "intercepted" : "restored");
            sys::log("[HUI ARCHIVE CONSUMER] input focus %s",
                     input.focus_lost ? "intercepted" : "restored");
        }
        if (receipt.ready)
        {
            if (!input.focus_lost && input.is_pressed(Action::confirm))
            {
                gpu.clear();
                renderer.release();
                display.close();
                pad.close();
                sys::log("[HUI ARCHIVE CONSUMER] clean exit after receipt frame=%llu",
                         static_cast<unsigned long long>(frame_count));
                sys::quit();
            }
        }
        else if (!input.focus_lost)
        {
            if (input.is_pressed(Action::menu))
            {
                lifecycle.begin_shutdown();
                stress.cancel();
                soak.finish(static_cast<std::uint64_t>(now));
                breadcrumbs.record(static_cast<std::uint64_t>(now), "exit", "user requested");
                images.cancel_all();
                const auto shutdown = worker.stop();
                receipt.joined = shutdown.joined;
                receipt.cancelled = shutdown.cancelled_queued;
                receipt.processed = shutdown.processed_after_join;
                sys::log("[HUI ARCHIVE CONSUMER] shutdown worker joined=%d queued_cancelled=%zu "
                         "processed=%llu",
                         receipt.joined ? 1 : 0, receipt.cancelled,
                         static_cast<unsigned long long>(receipt.processed));

                // Export once after the worker joins; never write from the
                // 60 FPS frame loop or pretend a failed export succeeded.
                std::string report = "PS5 UI SDK Consumer v0.7.0 - shutdown receipt\n";
                char line[256];
                std::snprintf(
                    line, sizeof(line),
                    "worker.joined=%d\nworker.cancelled_queued=%zu\nworker.processed=%llu\n",
                    receipt.joined ? 1 : 0, receipt.cancelled,
                    static_cast<unsigned long long>(receipt.processed));
                report += line;
                const auto &stats = images.statistics();
                std::snprintf(
                    line, sizeof(line),
                    "stress=%u/"
                    "%u\nimage.uploads=%llu\nimage.decode_errors=%llu\nimage.gpu_errors=%llu\n",
                    stress.completed(), stress.target(),
                    static_cast<unsigned long long>(stats.uploaded),
                    static_cast<unsigned long long>(stats.decode_failures),
                    static_cast<unsigned long long>(stats.upload_failures));
                report += line;
                std::snprintf(line, sizeof(line),
                              "cache.cpu_kib=%zu\ncache.gpu_kib=%zu\nsoak.peak_cpu_kib=%zu\nsoak."
                              "peak_gpu_kib=%zu\n",
                              cpu.used_bytes() / 1024, gpu.used_bytes() / 1024,
                              soak.peak_cpu_bytes() / 1024, soak.peak_gpu_bytes() / 1024);
                report += line;
                std::snprintf(line, sizeof(line),
                              "gl.restart_attempts=%u\ngl.restart_success=%u\ngl.restart_last=%"
                              "s\ngl.restart_duration_us=%llu\n",
                              restart.attempts, restart.successful,
                              restart.attempts == 0 ? "not_tested"
                                                    : sdk::context_restart_stage_name(restart.last),
                              static_cast<unsigned long long>(restart.last_duration_us));
                report += line;
                for (std::size_t i = 0; i < breadcrumbs.size(); ++i)
                {
                    const auto *event = breadcrumbs.at(i);
                    std::snprintf(line, sizeof(line), "breadcrumb=%llu %s %s\n",
                                  static_cast<unsigned long long>(event->timestamp_us),
                                  event->category.data(), event->message.data());
                    report += line;
                }
                const auto export_result = sdk::export_diagnostics(
                    report,
                    [](const std::string &directory) { return save::ensure_directory(directory); },
                    [](const std::string &filepath, std::string_view contents)
                    { return save::write_atomic(filepath, contents); });
                receipt.saved = export_result.saved;
                receipt.path = export_result.path;
                receipt.ready = true;
                nav.push("shutdown");
                sys::log("[HUI ARCHIVE CONSUMER] receipt saved=%d path=%s", receipt.saved ? 1 : 0,
                         receipt.saved ? receipt.path.c_str() : "NONE");
            }
            if (input.is_pressed(Action::back))
                nav.back();
            const auto *page = nav.current();
            if (page && input.is_pressed(Action::confirm))
            {
                if (page->name == "home")
                {
                    nav.push("covers");
                    load_covers();
                }
                else if (page->name == "covers")
                    nav.push("stats");
                else
                    nav.pop_to("covers");
            }
            page = nav.current();
            if (page && page->name == "covers")
            {
                if (input.nav == Direction::right)
                    selected = (selected + 1) % kTiles;
                if (input.nav == Direction::left)
                    selected = (selected + kTiles - 1) % kTiles;
                if (input.nav == Direction::down)
                    selected = (selected + 4) % kTiles;
                if (input.nav == Direction::up)
                    selected = (selected + kTiles - 4) % kTiles;
            }
            if (input.is_pressed(Action::page_prev))
            {
                // A deliberate, orderly GL/EGL teardown while the old
                // context is healthy; NOT automatic context-loss recovery.
                if (stress.running())
                {
                    stress.cancel();
                    soak.finish(static_cast<std::uint64_t>(now));
                }
                ++restart.attempts;
                const std::int64_t begin = sys::monotonic_us();
                breadcrumbs.record(static_cast<std::uint64_t>(now), "gl",
                                   "controlled context restart requested");
                const auto outcome = sdk::restart_context(
                    [&]()
                    {
                        images.reset(); // drop stale handles before releasing GL
                        gpu.clear();    // GL context is still valid here
                        regular.texture = 0;
                        bold.texture = 0;
                    },
                    [&]() { renderer.release(); }, [&]() { display.close(); },
                    [&]() { return display.open(1920, 1080); }, [&]() { return renderer.init(); },
                    [&]() {
                        return load_face(renderer, regular, kFontPath) &&
                               load_face(renderer, bold, kFontBold);
                    },
                    [&]() { load_covers(); });
                const std::int64_t done = sys::monotonic_us();
                restart.last_duration_us =
                    static_cast<std::uint64_t>(done >= begin ? done - begin : 0);
                restart.last = outcome.stage;
                if (outcome.ok())
                {
                    ++restart.successful;
                    previous = done;
                    breadcrumbs.record(static_cast<std::uint64_t>(done), "gl",
                                       "controlled context restart succeeded");
                    sys::log("[HUI ARCHIVE CONSUMER] GL context restart PASS %u/%u in %llu us",
                             restart.successful, restart.attempts,
                             static_cast<unsigned long long>(restart.last_duration_us));
                }
                else
                {
                    // No current display to render an error screen. Persist
                    // one controlled failure receipt before parking.
                    breadcrumbs.record(static_cast<std::uint64_t>(done), "gl",
                                       sdk::context_restart_stage_name(outcome.stage));
                    images.cancel_all();
                    const auto stopped = worker.stop();
                    char failure[512];
                    std::snprintf(failure, sizeof(failure),
                                  "PS5 UI SDK Consumer v0.7.0 GL restart failure\n"
                                  "stage=%s\nworker.joined=%d\nattempts=%u\nsuccess=%u\n",
                                  sdk::context_restart_stage_name(outcome.stage),
                                  stopped.joined ? 1 : 0, restart.attempts, restart.successful);
                    const auto saved = sdk::export_diagnostics(
                        failure, [](const std::string &dir) { return save::ensure_directory(dir); },
                        [](const std::string &file, std::string_view data)
                        { return save::write_atomic(file, data); });
                    sys::log("[HUI ARCHIVE CONSUMER] GL restart FAIL stage=%s saved=%d",
                             sdk::context_restart_stage_name(outcome.stage), saved.saved ? 1 : 0);
                    sys::park();
                }
            }
            if (input.is_pressed(Action::north))
            {
                stress.cancel();
                soak.finish(static_cast<std::uint64_t>(now));
                images.reset();
                gpu.clear();
                load_covers();
                breadcrumbs.record(static_cast<std::uint64_t>(now), "cache",
                                   "GPU cleared and requeued");
                sys::log("[HUI ARCHIVE CONSUMER] GPU cache cleared");
            }
            if (input.is_pressed(Action::west))
            {
                stress.cancel();
                soak.finish(static_cast<std::uint64_t>(now));
                reload_all();
                breadcrumbs.record(static_cast<std::uint64_t>(now), "cache",
                                   "CPU/GPU reload requested");
                sys::log("[HUI ARCHIVE CONSUMER] cache re-request");
            }
            if ((input.is_pressed(Action::page_next) || input.is_pressed(Action::jump_next)) &&
                worker_ok)
            {
                const std::uint32_t rounds = input.is_pressed(Action::jump_next) ? 200u : 20u;
                if (stress.running())
                {
                    stress.cancel();
                    soak.finish(static_cast<std::uint64_t>(now));
                    sys::log("[HUI ARCHIVE CONSUMER] stress cancelled at %u/%u", stress.completed(),
                             stress.target());
                }
                else if (stress.start(rounds, static_cast<std::uint64_t>(now)))
                {
                    soak.begin(static_cast<std::uint64_t>(now));
                    images.reset_statistics();
                    reload_all();
                    nav.pop_to("covers");
                    sys::log("[HUI ARCHIVE CONSUMER] stress %u rounds started", rounds);
                }
            }
        }

        if (!receipt.ready)
            images.pump(1);
        sdk::StressEvent terminal_event = sdk::StressEvent::none;
        if (stress.running())
        {
            bool all_ready = true;
            bool any_failed = false;
            for (std::size_t i = 0; i < kTiles; ++i)
            {
                char key[32];
                std::snprintf(key, sizeof(key), "arch-%zu", i);
                const auto state = images.status(key);
                all_ready &= state == sdk::ImageLoadState::ready;
                any_failed |=
                    state == sdk::ImageLoadState::failed || state == sdk::ImageLoadState::cancelled;
            }
            const auto event = stress.tick(all_ready, any_failed, static_cast<std::uint64_t>(now));
            if (event == sdk::StressEvent::reload)
            {
                reload_all();
                // Avoid console filesystem stalls from one log entry per wave.
                if (stress.completed() % 20u == 0u)
                    sys::log("[HUI ARCHIVE CONSUMER] stress progress %u/%u", stress.completed(),
                             stress.target());
            }
            else if (event == sdk::StressEvent::completed || event == sdk::StressEvent::failed)
            {
                // The GPU can upload the eighth cover on the final frame.
                // Defer finishing and the terminal log until after the frame's
                // cache and frame-time observation to capture the actual peak.
                terminal_event = event;
            }
        }
        draw_page(frame, regular, bold, nav, images, cpu, gpu, frametimes, worker_ok, lifecycle,
                  stress, soak, restart, receipt, worker.processed_jobs(), selected, frame_count);
        renderer.begin();
        renderer.draw(frame);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.present(0, display.width(), display.height());
        if (!display.swap())
        {
            lifecycle.display_failed();
            breadcrumbs.record(static_cast<std::uint64_t>(now), "display", "EGL swap failed");
            sys::log("[HUI ARCHIVE CONSUMER] EGL swap failure");
            images.cancel_all();
            const auto report = worker.stop();
            sys::log("[HUI ARCHIVE CONSUMER] display failure worker joined=%d "
                     "queued_cancelled=%zu processed=%llu",
                     report.joined ? 1 : 0, report.cancelled_queued,
                     static_cast<unsigned long long>(report.processed_after_join));
            for (std::size_t index = 0; index < breadcrumbs.size(); ++index)
            {
                const auto *event = breadcrumbs.at(index);
                sys::log("[HUI ARCHIVE CONSUMER] breadcrumb %llu %s %s",
                         static_cast<unsigned long long>(event->timestamp_us),
                         event->category.data(), event->message.data());
            }
            gpu.context_lost();
            sys::park();
        }
        if (frame_count == 0)
        {
            sys::hide_splash_screen();
            sys::log("[HUI ARCHIVE CONSUMER] first swap success");
        }

        const auto time = sys::monotonic_us();
        const float delta_ms = static_cast<float>(time - previous) / 1000.0f;
        frametimes.record(delta_ms);
        soak.observe(static_cast<std::uint64_t>(time), cpu.used_bytes(), gpu.used_bytes(),
                     images.pending_count(), delta_ms);
        previous = time;
        // The final upload must be included in peak-cache telemetry and
        // the final displayed frame must be included in slow-frame statistics.
        if (terminal_event != sdk::StressEvent::none)
        {
            soak.finish(static_cast<std::uint64_t>(time));
            const auto &stats = images.statistics();
            const bool pass = terminal_event == sdk::StressEvent::completed;
            sys::log("[HUI ARCHIVE CONSUMER] soak elapsed_ms=%llu peak_cpu=%zu peak_gpu=%zu "
                     "slow_frames=%llu worst_ms=%.2f",
                     static_cast<unsigned long long>(soak.elapsed_ms()), soak.peak_cpu_bytes(),
                     soak.peak_gpu_bytes(), static_cast<unsigned long long>(soak.slow_frames()),
                     soak.worst_frame_ms());
            sys::log("[HUI ARCHIVE CONSUMER] stress %s %u/%u uploads=%llu "
                     "decode_errors=%llu gpu_errors=%llu",
                     pass ? "PASS" : "FAIL", stress.completed(), stress.target(),
                     static_cast<unsigned long long>(stats.uploaded),
                     static_cast<unsigned long long>(stats.decode_failures),
                     static_cast<unsigned long long>(stats.upload_failures));
            breadcrumbs.record(static_cast<std::uint64_t>(time), "stress", pass ? "PASS" : "FAIL");
        }
        ++frame_count;
        if (frame_count % 300 == 0)
        {
            const auto result = frametimes.summary();
            sys::log("[HUI ARCHIVE CONSUMER] frames=%llu p99=%.2f avg=%.2f CPU=%zu GPU=%zu "
                     "pending=%zu jobs=%llu focus=%u/%u",
                     static_cast<unsigned long long>(frame_count), result.p99_ms, result.average_ms,
                     cpu.used_bytes(), gpu.used_bytes(), images.pending_count(),
                     static_cast<unsigned long long>(worker.processed_jobs()),
                     lifecycle.focus_losses(), lifecycle.focus_returns());
        }
    }
}
