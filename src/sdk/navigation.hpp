// ps5-homebrew-ui - Bounded page navigation state and back stack.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace hui::sdk
{

struct Route
{
    std::string name;
    std::string state;
};

class Navigator
{
  public:
    explicit Navigator(std::size_t max_depth = 64) : max_depth_(max_depth)
    {
    }

    bool set_root(std::string name, std::string state = {})
    {
        if (name.empty() || !max_depth_)
            return false;
        pages_.clear();
        pages_.push_back({std::move(name), std::move(state)});
        ++revision_;
        return true;
    }

    bool push(std::string name, std::string state = {})
    {
        if (name.empty() || pages_.empty() || pages_.size() >= max_depth_)
            return false;
        pages_.push_back({std::move(name), std::move(state)});
        ++revision_;
        return true;
    }

    bool replace(std::string name, std::string state = {})
    {
        if (name.empty() || pages_.empty())
            return false;
        pages_.back() = {std::move(name), std::move(state)};
        ++revision_;
        return true;
    }

    bool back()
    {
        if (pages_.size() < 2)
            return false;
        pages_.pop_back();
        ++revision_;
        return true;
    }

    bool pop_to(std::string_view name)
    {
        for (std::size_t i = pages_.size(); i; --i)
        {
            if (pages_[i - 1].name != name)
                continue;
            if (i != pages_.size())
            {
                pages_.resize(i);
                ++revision_;
            }
            return true;
        }
        return false;
    }

    const Route *current() const
    {
        return pages_.empty() ? nullptr : &pages_.back();
    }

    bool can_back() const
    {
        return pages_.size() > 1;
    }
    std::size_t depth() const
    {
        return pages_.size();
    }
    std::uint64_t revision() const
    {
        return revision_;
    }

  private:
    std::vector<Route> pages_;
    std::size_t max_depth_;
    std::uint64_t revision_ = 0;
};

} // namespace hui::sdk
