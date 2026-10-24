// ps5-homebrew-ui - Standalone SDK demo navigation model.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/input.hpp"
#include "sdk/navigation.hpp"

#include <cstddef>

namespace hui::sdk
{

enum class DemoCommand
{
    none,
    request_images,
    reset_gpu_cache,
    quit
};

// Portable controller model, separate from the PS5 renderer so it is unit-testable.
class DemoModel
{
  public:
    DemoModel()
    {
        pages_.set_root("home");
    }

    DemoCommand update(const InputFrame &input)
    {
        if (input.focus_lost)
            return DemoCommand::none;
        if (input.is_pressed(Action::menu))
            return DemoCommand::quit;
        if (input.is_pressed(Action::back) || input.is_pressed(Action::page_prev))
        {
            pages_.back();
            return DemoCommand::none;
        }
        const auto *page = pages_.current();
        if (page == nullptr)
            return DemoCommand::none;

        if (page->name == "gallery" || page->name == "detail")
        {
            if (input.is_pressed(Action::west))
                return DemoCommand::request_images;
            if (input.is_pressed(Action::north))
                return DemoCommand::reset_gpu_cache;
            if (input.nav == Direction::right)
                selection_ = (selection_ + 1) % kCovers;
            if (input.nav == Direction::left)
                selection_ = (selection_ + kCovers - 1) % kCovers;
            if (input.nav == Direction::down)
                selection_ = (selection_ + 4) % kCovers;
            if (input.nav == Direction::up)
                selection_ = (selection_ + kCovers - 4) % kCovers;
        }

        if (input.is_pressed(Action::confirm))
        {
            if (page->name == "home")
            {
                pages_.push("gallery");
                return DemoCommand::request_images;
            }
            if (page->name == "gallery")
                pages_.push("detail");
            else if (page->name == "detail")
                pages_.push("metrics");
            else
                pages_.pop_to("gallery");
        }
        if (input.is_pressed(Action::page_next))
        {
            if (page->name == "home")
            {
                pages_.push("gallery");
                return DemoCommand::request_images;
            }
            if (page->name == "gallery")
                pages_.push("metrics");
        }
        return DemoCommand::none;
    }

    const Navigator &pages() const
    {
        return pages_;
    }
    std::size_t selected() const
    {
        return selection_;
    }
    static constexpr std::size_t kCovers = 8;

  private:
    Navigator pages_;
    std::size_t selection_ = 0;
};

} // namespace hui::sdk
