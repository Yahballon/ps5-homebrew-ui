// ps5-homebrew-ui - Translation dictionary with locale fallback and substitutions.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hui::sdk
{

class Translations
{
  public:
    explicit Translations(std::string fallback = "en") : fallback_(std::move(fallback))
    {
    }

    void set(std::string language, std::string key, std::string value)
    {
        entries_[std::move(language)][std::move(key)] = std::move(value);
    }

    // The returned view lives until that entry is modified.
    std::string_view lookup(std::string_view language, std::string_view key) const
    {
        const auto *translation = find(language, key);
        if (!translation)
            translation = find(fallback_, key);
        return translation ? std::string_view(*translation) : key;
    }

    std::string format(std::string_view language, std::string_view key,
                       const std::vector<std::pair<std::string, std::string>> &values) const
    {
        const auto pattern = lookup(language, key);
        std::string output;
        for (std::size_t i = 0; i < pattern.size();)
        {
            if (pattern[i] != '{')
            {
                output.push_back(pattern[i++]);
                continue;
            }
            const auto end = pattern.find('}', i + 1);
            if (end == std::string_view::npos)
            {
                output.push_back(pattern[i++]);
                continue;
            }
            const auto placeholder = pattern.substr(i + 1, end - i - 1);
            bool replaced = false;
            for (const auto &pair : values)
            {
                if (placeholder == pair.first)
                {
                    output += pair.second;
                    replaced = true;
                    break;
                }
            }
            if (!replaced)
                output.append(pattern.substr(i, end - i + 1));
            i = end + 1;
        }
        return output;
    }

  private:
    const std::string *find(std::string_view language, std::string_view key) const
    {
        const auto lang = entries_.find(std::string(language));
        if (lang == entries_.end())
            return nullptr;
        const auto item = lang->second.find(std::string(key));
        return item == lang->second.end() ? nullptr : &item->second;
    }

    std::string fallback_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> entries_;
};

} // namespace hui::sdk
