// ps5-homebrew-ui - PS5-native SDK consumer test application.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdk/async_images.hpp"
#include "sdk/demo_model.hpp"
#include "sdk/diagnostics.hpp"
#include "sdk/read_image_file.hpp"

#include "core/input.hpp"
#include "gfx/draw_list.hpp"
#include "gfx/font.hpp"
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "ui/fonts.hpp"

#include <GL/glcorearb.h>
#include <pthread.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>

namespace
{

using hui::gfx::Color;
using hui::gfx::Rect;
constexpr const char *kIconPath = "/app0/sce_sys/icon0.png";
constexpr const char *kFontPath = "/app0/assets/fonts/inter-regular.huifont";
constexpr const char *kBoldPath = "/app0/assets/fonts/inter-semibold.huifont";
constexpr std::uint32_t kWorkerStack = 512u * 1024u;

// Explicit stack size and join are essential on the PS5 native runtime.
class Worker
{
  public:
    explicit Worker(hui::sdk::JobQueue &jobs) : jobs_(jobs)
    {
    }
    Worker(const Worker &) = delete;
    Worker &operator=(const Worker &) = delete;
    ~Worker()
    {
        stop();
    }

    bool start()
    {
        pthread_attr_t attributes{};
        if (pthread_attr_init(&attributes) != 0)
            return false;
        const int stack_error = pthread_attr_setstacksize(&attributes, kWorkerStack);
        int result = -1;
        if (stack_error == 0)
            result = pthread_create(&thread_, &attributes, &Worker::run_entry, this);
        pthread_attr_destroy(&attributes);
        started_ = result == 0;
        return started_;
    }

    void stop()
    {
        if (!started_)
            return;
        stop_.store(true);
        pthread_join(thread_, nullptr);
        started_ = false;
    }

  private:
    static void *run_entry(void *context)
    {
        auto *worker = static_cast<Worker *>(context);
        while (!worker->stop_.load())
        {
            if (!worker->jobs_.process_one())
                hui::sys::sleep_us(3000);
        }
        return nullptr;
    }

    hui::sdk::JobQueue &jobs_;
    pthread_t thread_{};
    std::atomic<bool> stop_{false};
    bool started_ = false;
};

bool load_font(hui::gfx::Renderer &renderer, const char *path, hui::gfx::Font &font,
               hui::ui::FontRef &output)
{
    std::string encoded;
    if (!hui::save::read_file(path, &encoded) || !font.load(encoded))
        return false;
    output.font = &font;
    output.texture = renderer.batch().create_font_texture(font);
    return output.texture != 0;
}

void emit(hui::gfx::DrawList &draw, const hui::ui::FontRef &face, const std::string &value, float x,
          float baseline, float size, Color tint = Color::rgb(0xe8f2ff))
{
    hui::ui::text(draw, face, value, x, baseline, size, tint);
}

const char *state_label(hui::sdk::ImageLoadState state)
{
    switch (state)
    {
    case hui::sdk::ImageLoadState::loading:
        return "DECODING";
    case hui::sdk::ImageLoadState::ready:
        return "READY";
    case hui::sdk::ImageLoadState::failed:
        return "FILE ERROR";
    case hui::sdk::ImageLoadState::cancelled:
        return "CANCELLED";
    case hui::sdk::ImageLoadState::evicted:
        return "EVICTED";
    case hui::sdk::ImageLoadState::missing:
        return "NOT LOADED";
    }
    return "UNKNOWN";
}

void draw_screen(hui::gfx::DrawList &draw, const hui::ui::FontRef &regular,
                 const hui::ui::FontRef &bold, const hui::sdk::DemoModel &model,
                 hui::sdk::AsyncImages &images, const hui::sdk::ImageCache &cpu,
                 const hui::sdk::TextureCache &gpu, const hui::sdk::FrameMonitor &monitor,
                 bool worker_running, std::uint64_t frames)
{
    draw.clear();
    const auto background = Color::rgb(0x091420);
    const auto panel = Color::rgb(0x10273a);
    const auto accent = Color::rgb(0x45d9bf);
    const auto muted = Color::rgb(0x91a9bc);
    draw.gradient_rect({0, 0, 1920, 1080}, 0, background, Color::rgb(0x122849));
    draw.rounded_rect({65, 60, 1790, 125}, 28, Color::rgb(0x112e44));
    draw.rounded_rect({65, 60, 14, 125}, 7, accent);
    emit(draw, bold, "PS5 HOME BREW UI  /  SDK LAB", 115, 119, 43);
    emit(draw, regular, "by YAHBALLON   |   SDK DEMO v0.2.0", 118, 159, 22, muted);

    const auto *route = model.pages().current();
    const std::string page = route == nullptr ? "home" : route->name;
    if (page == "home")
    {
        draw.rounded_rect({170, 290, 1580, 520}, 36, panel);
        draw.circle(960, 423, 66, accent);
        draw.circle(960, 423, 42, Color::rgb(0x10273a));
        emit(draw, bold, "SDK INTEGRATION TEST", 635, 565, 52);
        emit(draw, regular, "Screen router  +  native worker  +  PNG decode  +  GPU LRU cache", 455,
             635, 28, muted);
        emit(draw, bold, "PRESS X TO OPEN THE LIVE COVER GALLERY", 470, 750, 31, accent);
    }
    else if (page == "gallery")
    {
        emit(draw, bold, "ASYNC IMAGE GALLERY", 95, 250, 39);
        emit(draw, regular, "Use D-Pad to select a cover. X opens details.", 98, 300, 25, muted);
        for (std::size_t index = 0; index < hui::sdk::DemoModel::kCovers; ++index)
        {
            const float x = 92.0f + static_cast<float>(index % 4) * 455.0f;
            const float y = 337.0f + static_cast<float>(index / 4) * 309.0f;
            const bool selected = model.selected() == index;
            draw.bordered_rect({x, y, 426, 285}, 26, selected ? Color::rgb(0x214b62) : panel,
                               selected ? 4.0f : 1.0f, selected ? accent : Color::rgb(0x385266));
            char key[32];
            std::snprintf(key, sizeof(key), "cover-%zu", index);
            const auto texture = images.texture(key);
            if (texture != 0)
                draw.image(texture, {x + 18, y + 18, 179, 179}, hui::gfx::kFullUv,
                           Color::rgb(0xffffff), 18);
            else
                draw.rounded_rect({x + 18, y + 18, 179, 179}, 18, Color::rgb(0x294255));
            char label[40];
            std::snprintf(label, sizeof(label), "COVER %02zu", index + 1);
            emit(draw, bold, label, x + 219, y + 82, 28);
            emit(draw, regular, state_label(images.status(key)), x + 219, y + 125, 20,
                 texture ? accent : muted);
            emit(draw, regular, "PNG / IO worker", x + 219, y + 160, 18, muted);
            if (selected)
                emit(draw, bold, "SELECTED", x + 20, y + 246, 24, accent);
        }
    }
    else if (page == "detail")
    {
        const std::size_t chosen = model.selected();
        char key[32];
        std::snprintf(key, sizeof(key), "cover-%zu", chosen);
        char heading[60];
        std::snprintf(heading, sizeof(heading), "COVER %02zu  /  DETAIL VIEW", chosen + 1);
        emit(draw, bold, heading, 95, 268, 40);
        draw.rounded_rect({105, 310, 695, 620}, 35, panel);
        const auto texture = images.texture(key);
        if (texture != 0)
            draw.image(texture, {166, 367, 576, 515}, hui::gfx::kFullUv, Color::rgb(0xffffff), 30);
        else
            emit(draw, regular, "Waiting for texture", 220, 575, 31, muted);
        draw.rounded_rect({850, 310, 950, 620}, 35, panel);
        emit(draw, bold, "IMAGE PIPELINE", 910, 390, 37);
        emit(draw, regular, std::string("State: ") + state_label(images.status(key)), 912, 480, 29,
             accent);
        emit(draw, regular, "Loaded by pthread worker", 912, 548, 27);
        emit(draw, regular, "Decoded to CPU RGBA8", 912, 598, 27);
        emit(draw, regular, "Uploaded on GL render thread", 912, 648, 27);
        emit(draw, bold, "X   Open frame diagnostics", 912, 780, 29, accent);
    }
    else
    {
        const auto report = monitor.summary();
        char line[128];
        draw.rounded_rect({160, 287, 1600, 645}, 36, panel);
        emit(draw, bold, "FRAME DIAGNOSTICS", 226, 380, 46);
        std::snprintf(line, sizeof(line), "FRAMES  %llu", static_cast<unsigned long long>(frames));
        emit(draw, regular, line, 245, 463, 33, accent);
        std::snprintf(line, sizeof(line), "AVG      %.2f ms", report.average_ms);
        emit(draw, regular, line, 245, 531, 33);
        std::snprintf(line, sizeof(line), "P99      %.2f ms", report.p99_ms);
        emit(draw, regular, line, 245, 599, 33);
        std::snprintf(line, sizeof(line), "MAX      %.2f ms", report.worst_ms);
        emit(draw, regular, line, 245, 667, 33);
        std::snprintf(line, sizeof(line), "RAM CPU IMAGE CACHE    %zu KiB / %zu images",
                      cpu.used_bytes() / 1024, cpu.entries());
        emit(draw, regular, line, 910, 489, 27);
        std::snprintf(line, sizeof(line), "GPU IMAGE CACHE        %zu KiB / %zu textures",
                      gpu.used_bytes() / 1024, gpu.entries());
        emit(draw, regular, line, 910, 567, 27);
        std::snprintf(line, sizeof(line), "WORKER                  %s",
                      worker_running ? "RUNNING" : "FAILED");
        emit(draw, regular, line, 910, 646, 27);
        std::snprintf(line, sizeof(line), "FRAMES >16.7ms         %zu / %zu", report.over_budget,
                      report.samples);
        emit(draw, regular, line, 910, 730, 27);
    }

    draw.rounded_rect({65, 960, 1790, 66}, 20, Color::rgb(0x10283b));
    emit(draw, regular,
         "X  OPEN / NEXT     O  BACK     D-PAD  SELECT     SQUARE  RELOAD     "
         "TRIANGLE  CLEAR GPU     OPTIONS  EXIT",
         110, 1002, 22, muted);
}

} // namespace

int hui_sdk_demo_main()
{
    using namespace hui;
    sys::log("[HUI SDK DEMO] start version 0.2.0");
    ps5::Display display;
    if (!display.open(1920, 1080))
    {
        sys::log("[HUI SDK DEMO] display failed");
        sys::park();
    }
    gfx::Renderer renderer;
    gfx::Font regular;
    gfx::Font semibold;
    ui::FontRef regular_ref;
    ui::FontRef bold_ref;
    if (!renderer.init() || !load_font(renderer, kFontPath, regular, regular_ref) ||
        !load_font(renderer, kBoldPath, semibold, bold_ref))
    {
        sys::log("[HUI SDK DEMO] fonts or renderer failed");
        sys::park();
    }

    sdk::JobQueue jobs;
    sdk::ImageCache cpu(24u * 1024u * 1024u);
    sdk::TextureCache gpu(
        24u * 1024u * 1024u, [&renderer](const sdk::Image &image)
        { return renderer.batch().create_texture(image.width, image.height, image.rgba.data()); },
        [&renderer](std::uint32_t handle) { renderer.batch().destroy_texture(handle); });
    sdk::AsyncImages images(jobs, cpu, gpu, sdk::read_image_file);
    Worker worker(jobs);
    const bool worker_started = worker.start();
    sys::log("[HUI SDK DEMO] worker=%d (stack=%u)", worker_started ? 1 : 0, kWorkerStack);

    const auto request_covers = [&]()
    {
        for (std::size_t index = 0; index < sdk::DemoModel::kCovers; ++index)
        {
            char key[32];
            std::snprintf(key, sizeof(key), "cover-%zu", index);
            images.request(key, kIconPath);
        }
    };

    ps5::Pad pad;
    pad.open();
    InputTracker tracker;
    sdk::DemoModel model;
    sdk::FrameMonitor timings;
    gfx::DrawList list;
    PadSample samples[64];
    std::uint64_t frames = 0;
    std::int64_t last = sys::monotonic_us();
    bool splash_hidden = false;
    for (;;)
    {
        const auto now = sys::monotonic_us();
        InputFrame input = tracker.update(
            std::span<const PadSample>(samples, pad.read(std::span<PadSample>(samples))),
            static_cast<std::uint64_t>(now));
        const sdk::DemoCommand command = model.update(input);
        if (command == sdk::DemoCommand::request_images && worker_started)
            request_covers();
        if (command == sdk::DemoCommand::reset_gpu_cache)
        {
            gpu.clear();
            if (worker_started)
                request_covers();
        }
        if (command == sdk::DemoCommand::quit)
        {
            images.cancel_all();
            jobs.cancel_pending();
            worker.stop();
            gpu.clear();
            renderer.release();
            display.close();
            pad.close();
            sys::log("[HUI SDK DEMO] clean shutdown after %llu frames",
                     static_cast<unsigned long long>(frames));
            sys::quit();
        }

        const std::size_t uploaded = images.pump(1); // at most one GL upload each frame
        if (uploaded != 0)
            sys::log("[HUI SDK DEMO] texture uploaded count=%zu/%zu", gpu.entries(),
                     sdk::DemoModel::kCovers);

        draw_screen(list, regular_ref, bold_ref, model, images, cpu, gpu, timings, worker_started,
                    frames);
        renderer.begin();
        renderer.draw(list);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.present(0, display.width(), display.height());
        if (!display.swap())
        {
            sys::log("[HUI SDK DEMO] display swap failed");
            images.cancel_all();
            jobs.cancel_pending();
            worker.stop();
            gpu.clear();
            sys::park();
        }
        if (!splash_hidden)
        {
            splash_hidden = sys::hide_splash_screen();
            sys::log("[HUI SDK DEMO] first frame ready splash=%d", splash_hidden ? 1 : 0);
        }
        const auto current = sys::monotonic_us();
        timings.record(static_cast<float>(current - last) / 1000.0f);
        last = current;
        ++frames;
        if (frames % 300 == 0)
        {
            const auto report = timings.summary();
            sys::log(
                "[HUI SDK DEMO] frame=%llu avg=%.2f p99=%.2f max=%.2f pending=%zu cpu=%zu gpu=%zu",
                static_cast<unsigned long long>(frames), report.average_ms, report.p99_ms,
                report.worst_ms, images.pending_count(), cpu.used_bytes(), gpu.used_bytes());
        }
    }
}
