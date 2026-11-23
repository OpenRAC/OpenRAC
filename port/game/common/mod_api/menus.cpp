// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "menus": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

// The longest label an entry keeps, in characters (openrac_api.menu_entry).
constexpr std::size_t kMenuLabelMax = 31;

// The main menu's own items a mod's may go after (openrac_api.ui_main_item).
constexpr std::array<const char*, 3> kUiMainItems =
    {OPENRAC_UI_NEW_GAME, OPENRAC_UI_LOAD_GAME, OPENRAC_UI_OPTIONS};

int api_developer_menu(
    openrac_mod* mod,
    const char* title,
    const openrac_developer_menu_item* items,
    std::uint32_t count
) {
    if (mod == nullptr || mod != g_loading) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (title == nullptr || title[0] == '\0' || (count != 0 && items == nullptr)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    constexpr std::array<const char*, 5> built_in_titles =
        {"Game", "View", "Graphics", "Sound", "Developer"};
    if (std::find(built_in_titles.begin(), built_in_titles.end(), std::string_view(title))
        != built_in_titles.end()) {
        return OPENRAC_HOOK_TAKEN;
    }
    if (std::any_of(
            g_developer_menus.begin(), g_developer_menus.end(), [title](const DeveloperMenu& menu) {
                return menu.title == title;
            }
        )) {
        return OPENRAC_HOOK_TAKEN;
    }
    DeveloperMenu menu;
    menu.mod = mod;
    menu.title = title;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (items[i].label == nullptr || items[i].action == 0) {
            return OPENRAC_HOOK_UNKNOWN;
        }
        if (std::any_of(menu.items.begin(), menu.items.end(), [&](const DeveloperMenuItem& item) {
                return item.action == items[i].action;
            })) {
            return OPENRAC_HOOK_TAKEN;
        }
        const bool action_taken = std::any_of(
            g_developer_menus.begin(), g_developer_menus.end(), [&](const DeveloperMenu& entry) {
                return entry.mod == mod
                       && std::any_of(
                           entry.items.begin(),
                           entry.items.end(),
                           [&](const DeveloperMenuItem& item) {
                               return item.action == items[i].action;
                           }
                       );
            }
        );
        if (action_taken) {
            return OPENRAC_HOOK_TAKEN;
        }
        DeveloperMenuItem item;
        item.label = items[i].label;
        item.shortcut = items[i].shortcut != nullptr ? items[i].shortcut : "";
        item.group = items[i].group != nullptr ? items[i].group : "";
        item.action = items[i].action;
        menu.items.push_back(std::move(item));
    }
    g_developer_menus.push_back(std::move(menu));
    return OPENRAC_HOOK_OK;
}

std::uint32_t api_take_developer_menu_action(openrac_mod* mod) {
    if (mod == nullptr || mod != g_tick_mod || !g_in_tick_boundary) {
        return 0;
    }
    const auto menu = std::find_if(
        g_developer_menus.begin(), g_developer_menus.end(), [mod](const DeveloperMenu& entry) {
            return entry.mod == mod;
        }
    );
    if (menu == g_developer_menus.end() || menu->actions.empty()) {
        return 0;
    }
    const std::uint32_t action = menu->actions.front();
    menu->actions.pop_front();
    return action;
}

int api_developer_menu_set_label(openrac_mod* mod, std::uint32_t action, const char* label) {
    if (mod == nullptr || mod != g_tick_mod || !g_in_tick_boundary || label == nullptr) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    for (DeveloperMenu& menu : g_developer_menus) {
        if (menu.mod != mod) {
            continue;
        }
        const auto item = std::find_if(
            menu.items.begin(), menu.items.end(), [action](const DeveloperMenuItem& entry) {
                return entry.action == action;
            }
        );
        if (item != menu.items.end()) {
            item->label = label;
            return OPENRAC_HOOK_OK;
        }
    }
    return OPENRAC_HOOK_UNKNOWN;
}

// Whether the mod may make or set a switch or a choice now (1.19): while it loads, or at its tick
// boundary, as for the labels.
bool may_set_developer_menu(openrac_mod* mod) {
    return mod != nullptr && (mod == g_loading || (mod == g_tick_mod && g_in_tick_boundary));
}

int api_developer_menu_switch(openrac_mod* mod, std::uint32_t action, int on) {
    if (!may_set_developer_menu(mod)) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    DeveloperMenuItem* item = developer_menu_item(mod, action);
    if (item == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    item->kind = MenuItemKind::Switch;
    item->value = on != 0 ? 1 : 0;
    item->values.clear();
    return OPENRAC_HOOK_OK;
}

int api_developer_menu_choice(
    openrac_mod* mod,
    std::uint32_t action,
    const char* const* values,
    std::uint32_t count,
    std::uint32_t index
) {
    if (!may_set_developer_menu(mod)) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    DeveloperMenuItem* item = developer_menu_item(mod, action);
    if (item == nullptr || values == nullptr || count == 0 || index >= count
        || std::any_of(values, values + count, [](const char* v) { return v == nullptr; })) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    item->kind = MenuItemKind::Choice;
    item->values.assign(values, values + count);
    item->value = static_cast<int>(index);
    return OPENRAC_HOOK_OK;
}

int api_developer_menu_look(
    openrac_mod* mod, std::uint32_t action, const openrac_developer_menu_look* look
) {
    if (!may_set_developer_menu(mod)) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    DeveloperMenuItem* item = developer_menu_item(mod, action);
    if (item == nullptr
        || (look != nullptr
            && (look->size < offsetof(openrac_developer_menu_look, icon) + sizeof(look->icon)
                || std::isnan(look->progress)))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    item->info = look != nullptr && (look->flags & OPENRAC_DEVELOPER_MENU_INFO) != 0;
    item->locked = look != nullptr && (look->flags & OPENRAC_DEVELOPER_MENU_LOCKED) != 0;
    item->look_value = look != nullptr && look->value != nullptr ? look->value : "";
    item->progress =
        look != nullptr && look->progress >= 0.0f ? std::min(look->progress, 1.0f) : -1.0f;
    item->icon = look != nullptr && look->icon != nullptr ? look->icon : "";
    return OPENRAC_HOOK_OK;
}

int api_developer_menu_value(openrac_mod* mod, std::uint32_t action) {
    const DeveloperMenuItem* item = mod != nullptr ? developer_menu_item(mod, action) : nullptr;
    return item != nullptr && item->kind != MenuItemKind::Press ? item->value : -1;
}

int api_menu_entry(openrac_mod* mod, const char* label, openrac_menu_fn fn, void* user) {
    if (mod == nullptr || mod != g_loading) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    // An entry needs a name to show and a page to open, and the menu has room for so many.
    if (label == nullptr || label[0] == '\0' || fn == nullptr
        || g_menus.size() >= OPENRAC_MENU_ENTRIES_MAX) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::string text(label, std::min(std::strlen(label), kMenuLabelMax));
    g_menus.push_back({std::move(text), fn, user, mod});
    return OPENRAC_HOOK_OK;
}

void api_menu_rect(openrac_mod* mod, int x, int y, int width, int height, uint32_t rgba) {
    // Only the open page's own mod draws, and only while its callback runs.
    if (mod == nullptr || mod != g_drawing || g_host == nullptr || g_host->menu_rect == nullptr) {
        return;
    }
    g_host->menu_rect(x, y, width, height, rgba);
}

void api_menu_text(openrac_mod* mod, int x, int y, int size, uint32_t rgba, const char* text) {
    // As menu_rect: the open page's mod, during its callback.
    if (mod == nullptr || mod != g_drawing || text == nullptr || g_host == nullptr
        || g_host->menu_text == nullptr) {
        return;
    }
    g_host->menu_text(x, y, size, rgba, text);
}

// A text of a mod's page: at most OPENRAC_UI_TEXT_MAX bytes, none for null.
std::string ui_text(const char* text) {
    return text == nullptr ? std::string() : std::string(text, strnlen(text, OPENRAC_UI_TEXT_MAX));
}

// Whether the page on the screen is `mod`'s.
bool shows_mod(openrac_mod* mod) {
    return g_ui_shown >= 0 && mod_page(mod, g_ui_shown) != nullptr;
}

// Whether the title's main menu is on the screen and `page` is one of `mod`'s main-menu items'.
bool main_item_opens(openrac_mod* mod, int page) {
    return g_ui_main_menu && g_ui_shown < 0 && mod_page(mod, page) != nullptr
           && std::any_of(
               g_ui_main.begin(),
               g_ui_main.end(),
               [&](const UiMainItem& item) { return item.page == page; }
           );
}

int api_ui_page(openrac_mod* mod, uint32_t flags, openrac_ui_fn fn, void* user) {
    if (mod == nullptr || mod != g_loading) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    // A page needs a callback for its events, and the pages have room.
    if (fn == nullptr || g_ui_pages.size() >= OPENRAC_UI_PAGES_MAX) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    UiPageCallback page{UiPage{}, fn, user, mod};
    page.page.flags = flags & OPENRAC_UI_OWN_BACK;
    g_ui_pages.push_back(std::move(page));
    return static_cast<int>(g_ui_pages.size()) - 1;
}

int api_ui_main_item(openrac_mod* mod, const char* after, const char* label, int page) {
    if (mod == nullptr || mod != g_loading) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    const bool known = after != nullptr
                       && std::any_of(kUiMainItems.begin(), kUiMainItems.end(), [&](const char* n) {
                              return std::strcmp(n, after) == 0;
                          });
    // A known item to go after, a name, a page of the mod's own, and room in the menu.
    if (!known || label == nullptr || label[0] == '\0' || mod_page(mod, page) == nullptr
        || g_ui_main.size() >= OPENRAC_UI_MAIN_ITEMS_MAX) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_ui_main.push_back({after, ui_text(label), page});
    return OPENRAC_HOOK_OK;
}

int api_ui_pause_item(openrac_mod* mod, const char* label, int page) {
    if (mod == nullptr || mod != g_loading) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    // A name, a page of the mod's own, and room on the page.
    if (label == nullptr || label[0] == '\0' || mod_page(mod, page) == nullptr
        || g_ui_pause.size() >= OPENRAC_UI_PAUSE_ITEMS_MAX) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_ui_pause.push_back({ui_text(label), page});
    return OPENRAC_HOOK_OK;
}

int api_ui_set(openrac_mod* mod, int page, const openrac_ui_content* content) {
    UiPageCallback* p = mod_page(mod, page);
    // The mod's own page, and content at least as large as its first version.
    if (p == nullptr || content == nullptr
        || content->size < offsetof(openrac_ui_content, footer_count) + sizeof(uint32_t)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    UiPage& to = p->page;
    to.title = ui_text(content->title);
    to.items.clear();
    const std::uint32_t items =
        content->items != nullptr
            ? std::min<std::uint32_t>(content->item_count, OPENRAC_UI_ITEMS_MAX)
            : 0;
    for (std::uint32_t i = 0; i < items; ++i) {
        const openrac_ui_item& from = content->items[i];
        UiItem item;
        item.kind = from.kind <= OPENRAC_UI_TEXT ? from.kind : OPENRAC_UI_TEXT;
        item.flags = from.flags & OPENRAC_UI_DISABLED;
        item.label = ui_text(from.label);
        // A choice with none to choose from is a line of text.
        const std::uint32_t choices =
            from.choices != nullptr
                ? std::min<std::uint32_t>(from.choice_count, OPENRAC_UI_CHOICES_MAX)
                : 0;
        if (item.kind == OPENRAC_UI_CHOICE && choices == 0) {
            item.kind = OPENRAC_UI_TEXT;
        }
        for (std::uint32_t c = 0; item.kind == OPENRAC_UI_CHOICE && c < choices; ++c) {
            item.choices.push_back(ui_text(from.choices[c]));
        }
        item.choice = item.choices.empty() ? 0 : std::min<std::uint32_t>(from.choice, choices - 1);
        to.items.push_back(std::move(item));
    }
    to.footer.clear();
    const std::uint32_t footer =
        content->footer != nullptr
            ? std::min<std::uint32_t>(content->footer_count, OPENRAC_UI_FOOTER_MAX)
            : 0;
    for (std::uint32_t i = 0; i < footer; ++i) {
        to.footer.push_back(ui_text(content->footer[i]));
    }
    to.cursor = settle_cursor(to, to.cursor);
    ++to.revision;
    return OPENRAC_HOOK_OK;
}

int api_ui_open(openrac_mod* mod, int page) {
    // From one of the mod's pages on the screen, to one of its pages or back; or from the title's
    // main menu, to one of its main-menu items' pages.
    if ((!shows_mod(mod) || (page != OPENRAC_UI_GO_BACK && mod_page(mod, page) == nullptr))
        && !main_item_opens(mod, page)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_ui_requests.open = true;
    g_ui_requests.page = page;
    return OPENRAC_HOOK_OK;
}

int api_ui_confirm(openrac_mod* mod, int page, const char* question, uint32_t tag) {
    // The mod's page on the screen, without a question open or asked already.
    if (mod_page(mod, page) == nullptr || g_ui_shown != page || g_ui_question
        || g_ui_requests.confirm) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_ui_requests.confirm = true;
    g_ui_requests.confirm_page = page;
    g_ui_requests.question = ui_text(question);
    g_ui_requests.tag = tag;
    return OPENRAC_HOOK_OK;
}

int api_ui_cursor(openrac_mod* mod, int page, int item) {
    UiPageCallback* p = mod_page(mod, page);
    if (p == nullptr) {
        return -1;
    }
    UiPage& to = p->page;
    // Moved only onto an item the cursor takes.
    if (item >= 0 && item < static_cast<int>(to.items.size())
        && takes_cursor(to.items[static_cast<std::size_t>(item)])) {
        to.cursor = item;
    }
    return to.cursor;
}

int api_ui_sound(openrac_mod* mod, int sound) {
    // The game's five, while one of the mod's pages is on the screen.
    if (!shows_mod(mod) || sound < OPENRAC_UI_SOUND_SELECT || sound > OPENRAC_UI_SOUND_PAGE) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_ui_requests.sounds.push_back(sound);
    return OPENRAC_HOOK_OK;
}

// openrac_api.menu_option_set. Since 1.38.
namespace {

// What the mods set the menu's options to (openrac_api.menu_option_set): one state an option.
struct MenuOption {
    const openrac_mod* mod;
    std::string name;
    int state;
};

std::mutex g_menu_options_lock;
std::vector<MenuOption> g_menu_options;

}  // namespace

// Interface 1.36: a mod turns an option of the port's menu on or off.
int api_menu_option_set(openrac_mod* mod, const char* option, int state) {
    if (mod == nullptr || option == nullptr || option[0] == '\0'
        || state < OPENRAC_MENU_OPTION_DEFAULT || state > OPENRAC_MENU_OPTION_OFF) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::lock_guard<std::mutex> lock(g_menu_options_lock);
    std::erase_if(g_menu_options, [&](const MenuOption& o) { return o.name == option; });
    if (state != OPENRAC_MENU_OPTION_DEFAULT) {
        g_menu_options.push_back({mod, option, state});
    }
    return OPENRAC_HOOK_OK;
}

void drop_menu_options(const openrac_mod* mod) {
    const std::lock_guard<std::mutex> lock(g_menu_options_lock);
    std::erase_if(g_menu_options, [&](const MenuOption& o) {
        return mod == nullptr || o.mod == mod;
    });
}

}  // namespace openrac::mods::detail

namespace openrac::mods {

int menu_option(std::string_view name) {
    const std::lock_guard<std::mutex> lock(detail::g_menu_options_lock);
    for (const detail::MenuOption& o : detail::g_menu_options) {
        if (o.name == name) {
            return o.state;
        }
    }
    return OPENRAC_MENU_OPTION_DEFAULT;
}

}  // namespace openrac::mods
