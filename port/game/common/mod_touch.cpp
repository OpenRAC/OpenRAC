// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "mod_touch.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <iterator>
#include <mutex>

#include "common/json.h"

namespace openrac::mods::touch {
namespace {

struct Pushed {
    std::string mod;
    std::string context;
};

struct Icon {
    std::string mod;
    std::string name;
    std::string svg;
};

struct State {
    std::mutex lock;
    unsigned generation = 0;
    std::vector<std::pair<std::string, std::string>> sets;
    std::vector<Icon> icons;
    std::vector<Pushed> pushed;
    std::vector<std::pair<std::string, std::string>> hints;  // mod, hint
};

State& state() {
    static State s;
    return s;
}

bool read_text(const std::filesystem::path& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

// A path inside the mod: relative, with /, no empty, "." or ".." part.
bool inside(std::string_view p) {
    if (p.empty() || p.front() == '/' || p.find('\\') != std::string_view::npos
        || p.find(':') != std::string_view::npos) {
        return false;
    }
    std::size_t start = 0;
    while (start <= p.size()) {
        const std::size_t end = std::min(p.find('/', start), p.size());
        const std::string_view part = p.substr(start, end - start);
        if (part.empty() || part == "." || part == "..") {
            return false;
        }
        start = end + 1;
    }
    return true;
}

}  // namespace

bool load_file(
    std::string_view mod,
    const std::filesystem::path& root,
    std::string_view file,
    std::string& error
) {
    std::string text;
    if (!inside(file) || !read_text(root / std::filesystem::path(std::string(file)), text)) {
        error = std::format("{}: touch file {} cannot be read", mod, file);
        return false;
    }
    json::Value doc;
    if (!json::parse(text, doc, error)) {
        error = std::format("{}: {}: {}", mod, file, error);
        return false;
    }
    if (!define(mod, text, error)) {
        return false;
    }
    error.clear();
    for (const auto& [name, path] : doc["icons"].members()) {
        std::string svg;
        std::string e;
        if (!path.is_string() || !inside(path.string()) || !path.string().ends_with(".svg")
            || !read_text(root / std::filesystem::path(path.string()), svg)
            || !icon(mod, name, svg, e)) {
            error += std::format(
                "{}{}: touch icon {} cannot be read", error.empty() ? "" : "; ", mod, name
            );
        }
    }
    return true;
}

bool define(std::string_view mod, std::string_view json, std::string& error) {
    json::Value doc;
    if (!json::parse(json, doc, error)) {
        error = std::format("{}: touch set: {}", mod, error);
        return false;
    }
    if (!doc["contexts"].is_object()) {
        error = std::format("{}: touch set: no \"contexts\" object", mod);
        return false;
    }
    State& s = state();
    const std::lock_guard guard(s.lock);
    s.sets.emplace_back(std::string(mod), std::string(json));
    ++s.generation;
    return true;
}

bool icon(std::string_view mod, std::string_view name, std::string_view svg, std::string& error) {
    if (name.empty() || svg.find("<svg") == std::string_view::npos) {
        error = std::format("{}: touch icon {}: not an SVG", mod, name);
        return false;
    }
    State& s = state();
    const std::lock_guard guard(s.lock);
    std::erase_if(s.icons, [&](const Icon& i) { return i.name == name; });
    s.icons.push_back(Icon{std::string(mod), std::string(name), std::string(svg)});
    ++s.generation;
    return true;
}

void push(std::string_view mod, std::string_view context) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    std::erase_if(s.pushed, [&](const Pushed& p) { return p.mod == mod; });
    if (!context.empty()) {
        s.pushed.push_back(Pushed{std::string(mod), std::string(context)});
    }
}

void hint(std::string_view mod, std::string_view hint, bool on) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    std::erase_if(s.hints, [&](const auto& h) { return h.first == mod && h.second == hint; });
    if (on && !hint.empty()) {
        s.hints.emplace_back(std::string(mod), std::string(hint));
    }
}

Snapshot snapshot(unsigned known) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    Snapshot out;
    out.generation = s.generation;
    out.files = known != s.generation;
    if (out.files) {
        out.sets = s.sets;
        for (const Icon& i : s.icons) {
            out.icons.emplace_back(i.name, i.svg);
        }
    }
    out.context = s.pushed.empty() ? std::string() : s.pushed.back().context;
    for (const auto& [mod, h] : s.hints) {
        if (std::find(out.hints.begin(), out.hints.end(), h) == out.hints.end()) {
            out.hints.push_back(h);
        }
    }
    return out;
}

void forget(std::string_view mod) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    const std::size_t before = s.sets.size() + s.icons.size();
    std::erase_if(s.sets, [&](const auto& p) { return p.first == mod; });
    std::erase_if(s.icons, [&](const Icon& i) { return i.mod == mod; });
    std::erase_if(s.pushed, [&](const Pushed& p) { return p.mod == mod; });
    std::erase_if(s.hints, [&](const auto& h) { return h.first == mod; });
    if (s.sets.size() + s.icons.size() != before) {
        ++s.generation;
    }
}

void clear() {
    State& s = state();
    const std::lock_guard guard(s.lock);
    const bool had = !s.sets.empty() || !s.icons.empty();
    s.sets.clear();
    s.icons.clear();
    s.pushed.clear();
    s.hints.clear();
    if (had) {
        ++s.generation;
    }
}

}  // namespace openrac::mods::touch
