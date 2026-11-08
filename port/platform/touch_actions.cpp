// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/touch_actions.h"

#include <algorithm>
#include <format>
#include <set>

#include "common/json.h"
#include "platform/pad.h"

namespace openrac::platform {
namespace {

struct Name {
    std::string_view name;
    Button button;
};

constexpr Name kButtons[] = {
    {"select", Button::Select},
    {"l3", Button::L3},
    {"r3", Button::R3},
    {"start", Button::Start},
    {"up", Button::Up},
    {"right", Button::Right},
    {"down", Button::Down},
    {"left", Button::Left},
    {"l2", Button::L2},
    {"r2", Button::R2},
    {"l1", Button::L1},
    {"r1", Button::R1},
    {"triangle", Button::Triangle},
    {"circle", Button::Circle},
    {"cross", Button::Cross},
    {"square", Button::Square},
};

// The bits of a list of button names; false for a name that is not a button.
bool bits(const json::Value& list, std::uint16_t& out, std::string& bad) {
    out = 0;
    for (const json::Value& item : list.items()) {
        const std::uint16_t bit = touch_button_bits(item.string());
        if (bit == 0) {
            bad = item.string();
            return false;
        }
        out = static_cast<std::uint16_t>(out | bit);
    }
    return true;
}

bool read_action(const json::Value& v, TouchAction& a, std::string& error) {
    a.id = v["id"].string();
    a.icon = v["icon"].string();
    if (a.id.empty() || a.icon.empty()) {
        error = "an action without an id or an icon";
        return false;
    }
    std::string bad;
    if (!bits(v["press"], a.press, bad) || !bits(v["then"], a.then, bad)) {
        error = std::format("action {}: no button \"{}\"", a.id, bad);
        return false;
    }
    if (a.press == 0 && a.then == 0) {
        error = std::format("action {} presses nothing", a.id);
        return false;
    }
    const std::string& size = v["size"].string();
    if (size == "large") {
        a.size = TouchSize::Large;
    } else if (size == "small") {
        a.size = TouchSize::Small;
    } else if (size.empty() || size == "medium") {
        a.size = TouchSize::Medium;
    } else {
        error = std::format("action {}: no size \"{}\"", a.id, size);
        return false;
    }
    const std::string& side = v["side"].string();
    if (side == "left") {
        a.side = TouchSide::Left;
    } else if (side == "top") {
        a.side = TouchSide::Top;
    } else if (side == "hint" || (side.empty() && !v["when"].string().empty())) {
        a.side = TouchSide::Hint;
    } else if (side.empty() || side == "arc") {
        a.side = TouchSide::Arc;
    } else {
        error = std::format("action {}: no side \"{}\"", a.id, side);
        return false;
    }
    const json::Value& at = v["at"];
    if (!at.is_null()) {
        const double n = at.number(-1.0);
        if (!at.is_number() || n < 0.0 || n > 7.0 || n != static_cast<int>(n)) {
            error = std::format("action {}: \"at\" is an arc place, 0 to 7", a.id);
            return false;
        }
        a.at = static_cast<int>(n);
    }
    a.when = v["when"].string();
    a.instead = v["instead"].string();
    if (!a.instead.empty() && a.when.empty()) {
        error = std::format("action {}: \"instead\" needs a \"when\"", a.id);
        return false;
    }
    a.toggle = v["toggle"].boolean();
    a.wide = v["wide"].boolean();
    a.stick = v["stick"].boolean();
    return true;
}

// Puts `add` over `into`: an action of the same id takes the old one's place, a new one goes last.
void merge(std::vector<TouchAction>& into, const std::vector<TouchAction>& add) {
    for (const TouchAction& a : add) {
        const auto same = std::find_if(into.begin(), into.end(), [&](const TouchAction& b) {
            return b.id == a.id;
        });
        if (same != into.end()) {
            *same = a;
        } else {
            into.push_back(a);
        }
    }
}

}  // namespace

std::uint16_t touch_button_bits(std::string_view name) {
    for (const Name& n : kButtons) {
        if (n.name == name) {
            return static_cast<std::uint16_t>(n.button);
        }
    }
    return 0;
}

bool TouchActionSets::read(std::string_view text, std::string_view source, std::string& error) {
    json::Value doc;
    if (!json::parse(text, doc, error)) {
        error = std::format("{}: {}", source, error);
        return false;
    }
    if (!doc["contexts"].is_object()) {
        error = std::format("{}: no \"contexts\" object", source);
        return false;
    }
    // Read whole before anything changes, so that a bad set leaves the others as they were.
    std::map<std::string, Context, std::less<>> read;
    std::map<std::string, bool, std::less<>> replace;
    for (const auto& [name, v] : doc["contexts"].members()) {
        Context c;
        c.extends = v["extends"].string();
        c.dpad = v["dpad"].boolean();
        for (const json::Value& item : v["actions"].items()) {
            TouchAction a;
            if (!read_action(item, a, error)) {
                error = std::format("{}: context {}: {}", source, name, error);
                return false;
            }
            merge(c.actions, {a});
        }
        replace[name] = v["replace"].boolean();
        read[name] = std::move(c);
    }
    for (auto& [name, c] : read) {
        const auto old = m_contexts.find(name);
        if (old == m_contexts.end() || replace[name]) {
            m_contexts[name] = std::move(c);
            continue;
        }
        merge(old->second.actions, c.actions);
        old->second.dpad = old->second.dpad || c.dpad;
        if (!c.extends.empty()) {
            old->second.extends = c.extends;
        }
    }
    return true;
}

std::vector<const TouchActionSets::Context*> TouchActionSets::chain(
    std::string_view context
) const {
    // From `context` to its farthest base, each once (a loop ends where it repeats).
    std::vector<const Context*> chain;
    std::set<std::string, std::less<>> seen;
    std::string_view name = context;
    while (!name.empty() && !seen.contains(name)) {
        const auto it = m_contexts.find(name);
        if (it == m_contexts.end()) {
            break;
        }
        seen.emplace(name);
        chain.push_back(&it->second);
        name = it->second.extends;
    }
    return chain;
}

bool TouchActionSets::dpad(std::string_view context) const {
    const auto contexts = chain(context);
    return std::any_of(contexts.begin(), contexts.end(), [](const Context* c) { return c->dpad; });
}

std::vector<TouchAction> TouchActionSets::actions(std::string_view context) const {
    const std::vector<const Context*> chain = this->chain(context);
    std::vector<TouchAction> out;
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        merge(out, (*it)->actions);
    }
    return out;
}

}  // namespace openrac::platform
