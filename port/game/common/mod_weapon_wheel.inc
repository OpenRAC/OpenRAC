// SPDX-License-Identifier: GPL-3.0-or-later
// Included by mods.cpp so the wheel can chain the port's native hook points.

using WheelTick = void (*)(openrac_gaddr);
using WheelDraw = int (*)(openrac_gaddr);

WheelTick g_wheel_init = nullptr;
WheelTick g_wheel_update = nullptr;
WheelDraw g_wheel_draw = nullptr;
std::vector<std::pair<void**, void*>> g_wheel_hooks;
int g_weapon_wheel_page = 0;

// PAL v2.00 addresses; rac1_select_weapon_addresses() (mod_weapon_native.inc) switches them to the NTSC-U 1.00 set once, when the host is NTSC-U.
openrac_gaddr kWheelTable = 0x15FB78;
openrac_gaddr kWheelIndex = 0x15F80C;
openrac_gaddr kWheelItemDefs = 0x179BC0;
openrac_gaddr kWheelSlotIds = 0x141FA0;   // the player's eight Quick Select item ids (help_stats_base + 0x885): the ring's ammo counter reads them by slot, whatever page is shown
openrac_gaddr kRac1ItemOwned = 0x13D5C8;
openrac_gaddr kRac1HeldButtons = 0x13CC00;
openrac_gaddr kRac1PressedButtons = 0x13CC04;
constexpr unsigned kRac1WheelButton = 0x10;
constexpr unsigned kRac1PageButton = 0x02;        // R2: pages the pause menu's weapon grid (the start menus already use R1)
constexpr unsigned kRac1WheelPageButton = 0x08;   // R1: pages the Quick Select ring (as RAC3 does); the flip swallows that press so the ring does not also turn
constexpr int kWheelSlots = 8;

struct WeaponWheelSlot {
    std::uint32_t icon, flags, left, right, up, down, item;
};
static_assert(sizeof(WeaponWheelSlot) == 0x1C);

bool guest_range(openrac_gaddr address, std::size_t size) {
    return address != 0 && address <= openrac_guest_ram_size &&
           size <= openrac_guest_ram_size - address;
}

using WeaponWheelPage = std::array<WeaponWheelSlot, kWheelSlots>;

std::vector<WeaponWheelPage> weapon_wheel_pages() {
    const auto table = GREF(openrac_gaddr, openrac_relocate_level_data(0, kWheelTable));
    if (!guest_range(table, sizeof(openrac_gaddr))) return {};
    const auto slots = GREF(openrac_gaddr, table);
    if (!guest_range(slots, sizeof(WeaponWheelPage))) return {};
    WeaponWheelPage stock{};
    std::memcpy(stock.data(), G(slots), sizeof stock);
    std::vector<WeaponWheelPage> pages{stock};
    std::array<bool, 37> listed{};
    for (const auto& slot : stock) {
        if (slot.item < listed.size()) listed[slot.item] = slot.item != 0;
    }
    const auto defs = openrac_relocate_level_data(0, kWheelItemDefs);
    const auto add = [&](std::uint32_t id) {
        if (listed[id] || GREF(std::uint8_t, kRac1ItemOwned + id) == 0) return;
        const auto icon = GREF(std::uint16_t, defs + id * 0x4C + 0x38);
        if (icon == 0) return;
        auto slot = std::ranges::find_if(pages.back(), [](const auto& s) { return s.item == 0; });
        if (slot == pages.back().end()) {
            auto empty = stock;
            for (auto& row : empty) row.item = row.icon = 0;
            pages.push_back(empty);
            slot = pages.back().begin();
        }
        slot->item = id;
        slot->icon = icon;
        listed[id] = true;
    };
    for (auto id : kRac1StockWheelWeapons) add(id);
    for (const auto& entry : g_weapon_ui) add(entry.id);
    return pages;
}

// Supply a single page around the native call. A guest stack frame owns both
// the pointer table and its slots. The original game table is never enlarged.
class ScopedWeaponWheelPage {
public:
    explicit ScopedWeaponWheelPage(const WeaponWheelPage& page)
        : table_(openrac_relocate_level_data(0, kWheelTable)),
          index_(openrac_relocate_level_data(0, kWheelIndex)),
          previous_table_(GREF(openrac_gaddr, table_)),
          previous_index_(GREF(int, index_)),
          frame_before_(openrac_guest_sp) {
        // The selector uses negative focus sentinels before a direction is picked.
        constexpr std::uint32_t prefix = 64;
        const auto frame = openrac_guest_frame(prefix + sizeof page);
        const auto slots = frame + prefix;
        std::memset(G(frame), 0, prefix);
        GREF(openrac_gaddr, frame) = slots;
        std::memcpy(G(slots), page.data(), sizeof page);
        GREF(openrac_gaddr, table_) = frame;
        GREF(int, index_) = 0;
    }
    ~ScopedWeaponWheelPage() {
        GREF(openrac_gaddr, table_) = previous_table_;
        GREF(int, index_) = previous_index_;
        openrac_guest_sp = frame_before_;
    }

    ScopedWeaponWheelPage(const ScopedWeaponWheelPage&) = delete;
    ScopedWeaponWheelPage& operator=(const ScopedWeaponWheelPage&) = delete;

private:
    openrac_gaddr table_, index_, previous_table_;
    int previous_index_;
    openrac_gaddr frame_before_;
};

void weapon_wheel_init(openrac_gaddr obj) {
    g_weapon_wheel_page = 0;
    g_wheel_init(obj);
}

void weapon_wheel_update(openrac_gaddr obj) {
    const auto pages = weapon_wheel_pages();
    if (pages.empty()) {
        g_wheel_update(obj);
        return;
    }
    const int count = static_cast<int>(pages.size());
    if (g_weapon_wheel_page >= count) g_weapon_wheel_page = 0;
    if (std::getenv("OPENRAC_WHEEL_LOG") != nullptr) {
        static int calls = 0;
        if (++calls % 120 == 1) {
            for (std::size_t pg = 0; pg < pages.size(); ++pg) {
                std::string line = "wheel page " + std::to_string(pg) + ":";
                for (const auto& sl : pages[pg]) line += " " + std::to_string(sl.item) + "/" + std::to_string(sl.icon) + "{f" + std::to_string(sl.flags) + " l" + std::to_string(static_cast<int>(sl.left)) + " r" + std::to_string(static_cast<int>(sl.right)) + " u" + std::to_string(static_cast<int>(sl.up)) + " d" + std::to_string(static_cast<int>(sl.down)) + "}";
                log::info("{}", line);
            }
        }
    }
    if (count > 1 && (GREF(unsigned, kRac1HeldButtons) & kRac1WheelButton) != 0 &&
        (GREF(unsigned, kRac1PressedButtons) & kRac1WheelPageButton) != 0) {
        g_weapon_wheel_page = (g_weapon_wheel_page + 1) % count;
        if (std::getenv("OPENRAC_WHEEL_LOG") != nullptr) log::info("wheel flip -> page {}", g_weapon_wheel_page);
        const auto& page = pages[g_weapon_wheel_page];
        const auto first = std::ranges::find_if(page, [](const auto& s) { return s.item != 0; });
        GREF(int, obj + 0x74) = static_cast<int>(first - page.begin());
        GREF(unsigned, kRac1PressedButtons) &= ~kRac1WheelPageButton;   // the same R1 press would otherwise turn the ring one slot as well
    }
    const ScopedWeaponWheelPage page(pages[g_weapon_wheel_page]);
    g_wheel_update(obj);
}

std::string_view registered_weapon_name(int item);

struct ActiveWeaponName {
    openrac_gaddr text = 0;
    int item = -1;
} g_active_weapon_name;

class ScopedWeaponName {
public:
    explicit ScopedWeaponName(int item)
        : previous_(g_active_weapon_name), stack_(openrac_guest_sp) {
        const auto name = registered_weapon_name(item);
        g_active_weapon_name = {};
        if (name.empty()) return;
        const auto text = openrac_guest_frame(static_cast<std::uint32_t>(name.size() + 1));
        std::memcpy(G(text), name.data(), name.size());
        GREF(char, text + static_cast<openrac_gaddr>(name.size())) = 0;
        g_active_weapon_name = {text, item};
    }
    ~ScopedWeaponName() {
        g_active_weapon_name = previous_;
        openrac_guest_sp = stack_;
    }
    ScopedWeaponName(const ScopedWeaponName&) = delete;
    ScopedWeaponName& operator=(const ScopedWeaponName&) = delete;

private:
    ActiveWeaponName previous_;
    openrac_gaddr stack_;
};

// The ring's centre ammo counter is looked up through the player's own slot-id array (page 0's items), not through the table, so on another page it
// showed the page-0 weapon of the same slot. Around the draw the array carries the shown page's items; it is put back afterwards (it is saved with the game).
class ScopedWheelSlotIds {
public:
    explicit ScopedWheelSlotIds(const WeaponWheelPage& page) {
        if (!guest_range(kWheelSlotIds, sizeof saved_)) return;
        std::memcpy(saved_.data(), G(kWheelSlotIds), sizeof saved_);
        for (int i = 0; i < kWheelSlots; ++i) GREF(std::int32_t, kWheelSlotIds + i * 4) = static_cast<std::int32_t>(page[i].item);
        active_ = true;
    }
    ~ScopedWheelSlotIds() {
        if (active_) std::memcpy(G(kWheelSlotIds), saved_.data(), sizeof saved_);
    }
    ScopedWheelSlotIds(const ScopedWheelSlotIds&) = delete;
    ScopedWheelSlotIds& operator=(const ScopedWheelSlotIds&) = delete;

private:
    std::array<std::int32_t, kWheelSlots> saved_{};
    bool active_ = false;
};

int weapon_wheel_draw(openrac_gaddr obj) {
    const auto pages = weapon_wheel_pages();
    if (pages.empty()) return g_wheel_draw(obj);
    const int selected = g_weapon_wheel_page < static_cast<int>(pages.size()) ? g_weapon_wheel_page : 0;
    const ScopedWeaponWheelPage page(pages[selected]);
    std::optional<ScopedWheelSlotIds> slot_ids;
    if (selected != 0) slot_ids.emplace(pages[selected]);
    const int focus = GREF(int, obj + 0x74);
    const ScopedWeaponName name(focus >= 0 && focus < kWheelSlots
                                ? static_cast<int>(pages[selected][focus].item) : -1);
    return g_wheel_draw(obj);
}

openrac_gaddr kPauseMenuSystem = 0x1D5F70;
openrac_gaddr kPauseWeaponCells = 0x1CF4A0;
openrac_gaddr kPauseItemInfo = 0x1864D0;
constexpr int kPauseWeaponSlots = 15;

struct WeaponGridCell {
    std::uint16_t icon;
    std::int16_t frame, kind, item, stream;
};
static_assert(sizeof(WeaponGridCell) == 10);

using WeaponGridPage = std::array<WeaponGridCell, kPauseWeaponSlots>;
WheelDraw g_weapon_grid_update = nullptr;
struct WeaponGridState {
    openrac_gaddr grid = 0, cells = 0;
    int page = 0;
    WeaponGridPage original{}, published{};
} g_weapon_grid;

void restore_weapon_grid() {
    if (openrac_guest_base != nullptr && guest_range(g_weapon_grid.cells, sizeof(WeaponGridPage))) {
        auto* cells = static_cast<WeaponGridCell*>(G(g_weapon_grid.cells));
        for (int i = 0; i < kPauseWeaponSlots; ++i) {
            if (std::memcmp(cells + i, &g_weapon_grid.published[i], sizeof(WeaponGridCell)) == 0)
                std::memcpy(cells + i, &g_weapon_grid.original[i], sizeof(WeaponGridCell));
        }
    }
    g_weapon_grid = {};
}

openrac_gaddr weapon_grid_focus() {
    const auto menu = OPENRAC_DATA(kPauseMenuSystem);
    const auto current = GREF(openrac_gaddr, menu + 4);
    if (!guest_range(current, 0x44)) return 0;
    return GREF(openrac_gaddr, current + 0x40);
}

bool weapon_grid_is_active() {
    const auto grid = weapon_grid_focus();
    return play_phase() == OPENRAC_PLAY_MENU && guest_range(grid, 0x50) &&
        GREF(openrac_gaddr, grid + 0x48) == OPENRAC_DATA(kPauseWeaponCells) &&
        GREF(int, grid + 0x40) == 3 && GREF(int, grid + 0x44) == 5;
}

void restore_weapon_grid_if_left() {
    if (g_weapon_grid.grid != 0 &&
        (!weapon_grid_is_active() || weapon_grid_focus() != g_weapon_grid.grid))
        restore_weapon_grid();
}

std::vector<WeaponGridPage> weapon_grid_pages() {
    std::vector<WeaponGridPage> pages{g_weapon_grid.original};
    std::array<bool, 37> listed{};
    for (const auto& cell : pages.front()) {
        if (cell.item > 0 && static_cast<std::size_t>(cell.item) < listed.size()) listed[cell.item] = true;
    }
    const auto defs = OPENRAC_DATA(kPauseItemInfo);
    for (const auto& entry : g_weapon_ui) {
        const auto id = entry.id;
        if (listed[id] || GREF(std::uint8_t, kRac1ItemOwned + id) == 0) continue;
        const auto icon = GREF(std::uint16_t, defs + id * 0x4C + 0x38);
        if (icon == 0) continue;
        // Every position on the retail grid belongs to the game, including
        // visually empty cells. Only populate pages we own.
        if (pages.size() == 1) pages.emplace_back();
        auto cell = std::ranges::find_if(pages.back(), [](auto row) { return row.item == 0; });
        if (cell == pages.back().end()) {
            pages.emplace_back();
            cell = pages.back().begin();
        }
        *cell = {icon, 0, 0, static_cast<std::int16_t>(id), 0};
        listed[id] = true;
    }
    return pages;
}

int weapon_grid_update(openrac_gaddr grid) {
    const auto cells = GREF(openrac_gaddr, grid + 0x48);
    if (grid != weapon_grid_focus() || cells != OPENRAC_DATA(kPauseWeaponCells) ||
        GREF(int, grid + 0x40) != 3 || GREF(int, grid + 0x44) != 5 ||
        !guest_range(cells, sizeof(WeaponGridPage)))
        return g_weapon_grid_update(grid);
    if (g_weapon_grid.grid != grid || g_weapon_grid.cells != cells) {
        restore_weapon_grid();
        g_weapon_grid.grid = grid;
        g_weapon_grid.cells = cells;
        std::memcpy(g_weapon_grid.original.data(), G(cells), sizeof(WeaponGridPage));
        g_weapon_grid.published = g_weapon_grid.original;
    }
    const auto pages = weapon_grid_pages();
    const int count = static_cast<int>(pages.size());
    if (g_weapon_grid.page >= count) g_weapon_grid.page = 0;
    if (count > 1 && (GREF(unsigned, kRac1PressedButtons) & kRac1PageButton) != 0) {
        g_weapon_grid.page = (g_weapon_grid.page + 1) % count;
        GREF(int, grid + 0x3C) = 0;
    }
    const auto& page = pages[g_weapon_grid.page];
    std::memcpy(G(cells), page.data(), sizeof(WeaponGridPage));
    g_weapon_grid.published = page;
    return g_weapon_grid_update(grid);
}

template <class Fn>
void chain_weapon_wheel_hook(const char* name, Fn replacement, Fn& previous) {
    for (int i = 0; i < g_host->hook_count; ++i) {
        const auto& p = g_host->hooks[i];
        if (p.name == nullptr || std::strcmp(hook_id(*g_host, p.name), name) != 0) continue;
        previous = reinterpret_cast<Fn>(*p.slot != nullptr ? *p.slot : p.original);
        g_wheel_hooks.emplace_back(p.slot, *p.slot);
        *p.slot = reinterpret_cast<void*>(replacement);
        return;
    }
}

void remove_weapon_wheel_hooks() {
    for (auto it = g_wheel_hooks.rbegin(); it != g_wheel_hooks.rend(); ++it) *it->first = it->second;
    g_wheel_hooks.clear();
    restore_weapon_grid();
}

void install_weapon_wheel_hooks() {
    if (g_weapon_ui.empty()) return;
    chain_weapon_wheel_hook("rac1.hud.quick_select.init", weapon_wheel_init, g_wheel_init);
    chain_weapon_wheel_hook("rac1.hud.quick_select.update", weapon_wheel_update, g_wheel_update);
    chain_weapon_wheel_hook("rac1.hud.quick_select.draw", weapon_wheel_draw, g_wheel_draw);
    chain_weapon_wheel_hook("rac1.menu.item_grid.update", weapon_grid_update, g_weapon_grid_update);
    if (g_wheel_hooks.size() != 4) {
        remove_weapon_wheel_hooks();
        log::warn("RAC1 weapon paging could not attach to the native Quick Select");
    }
}
