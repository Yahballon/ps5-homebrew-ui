// ps5-homebrew-ui - PS5 SDK demo controller navigation tests.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdk/demo_model.hpp"

#include <gtest/gtest.h>

namespace
{

TEST(SdkDemoModel, NavigatesGalleryDetailsMetricsAndBack)
{
    hui::sdk::DemoModel model;
    hui::InputFrame input;
    input.pressed = hui::action_bit(hui::Action::confirm);
    EXPECT_EQ(model.update(input), hui::sdk::DemoCommand::request_images);
    ASSERT_NE(model.pages().current(), nullptr);
    EXPECT_EQ(model.pages().current()->name, "gallery");
    input.pressed = 0;
    input.nav = hui::Direction::right;
    EXPECT_EQ(model.update(input), hui::sdk::DemoCommand::none);
    EXPECT_EQ(model.selected(), 1u);
    input.nav = hui::Direction::none;
    input.pressed = hui::action_bit(hui::Action::confirm);
    model.update(input);
    EXPECT_EQ(model.pages().current()->name, "detail");
    model.update(input);
    EXPECT_EQ(model.pages().current()->name, "metrics");
    input.pressed = hui::action_bit(hui::Action::back);
    model.update(input);
    EXPECT_EQ(model.pages().current()->name, "detail");
    model.update(input);
    EXPECT_EQ(model.pages().current()->name, "gallery");
    model.update(input);
    EXPECT_EQ(model.pages().current()->name, "home");
}

TEST(SdkDemoModel, SupportsCacheReloadAndShutdown)
{
    hui::sdk::DemoModel model;
    hui::InputFrame input;
    input.pressed = hui::action_bit(hui::Action::confirm);
    model.update(input);
    input.pressed = hui::action_bit(hui::Action::west);
    EXPECT_EQ(model.update(input), hui::sdk::DemoCommand::request_images);
    input.pressed = hui::action_bit(hui::Action::north);
    EXPECT_EQ(model.update(input), hui::sdk::DemoCommand::reset_gpu_cache);
    input.pressed = hui::action_bit(hui::Action::menu);
    EXPECT_EQ(model.update(input), hui::sdk::DemoCommand::quit);
    input.focus_lost = true;
    EXPECT_EQ(model.update(input), hui::sdk::DemoCommand::none);
}

} // namespace
