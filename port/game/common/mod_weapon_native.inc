// SPDX-License-Identifier: GPL-3.0-or-later
// Native-backed custom weapons for the validated RAC1 PAL item layout.

openrac_gaddr kRac1AmmoCounts = 0x13D530;
openrac_gaddr kRac1ItemAcquired = 0x13D5F0;
openrac_gaddr kRac1HeroHand = 0x1404E0;
openrac_gaddr kRac1ItemPrices = 0x1C43B0;
openrac_gaddr kRac1ExePrices = 0x1E02B0;
openrac_gaddr kRac1VendorInfo = 0x1C2488;
openrac_gaddr kRac1VendorDialogue = 0x1CA7C0;   // the vendor menu's state (level-0 data)
openrac_gaddr kRac1PreviewPoses = 0x1E0708;     // pause item preview anchor, per native item (exe)
openrac_gaddr kRac1WeaponHudPair = 0x15FA44;    // weapon_hud_item and its animation handle (level-0 data)
const char* kRac1HudShowFunction = "func_L00_002367A8";
int kRac1HudShowTicks = 125;                    // 2.5 seconds

// NTSC-U 1.00 set of every address above and in mod_weapon_wheel.inc / kRac1VendorDialogue. Evidence per value: the NTSC-U boot executable and the NTSC decomp.
// (boot-exe MIPS signature pairing, the NTSC decomp's names_ntsc.tsv and sources, hero_layout_check.c). Selected once; PAL stays the default.
void rac1_select_weapon_addresses() {
    static bool done = false;
    if (done || g_host == nullptr) return;
    done = true;
    if (g_host->version != "ntsc") return;
    kRac1AmmoCounts = 0x13D428; kRac1ItemAcquired = 0x13D4E8; kRac1HeroHand = 0x1403E0;
    kRac1ItemPrices = 0x1C40B0; kRac1ExePrices = 0x1DFFB0; kRac1VendorInfo = 0x1C2108;
    kRac1PreviewPoses = 0x1E0408; kRac1WeaponHudPair = 0x15F96C;
    kRac1HudShowFunction = "FUN_L00_00235e18"; kRac1HudShowTicks = 150;   // 2.5 s at 60 ticks a second
    kWheelTable = 0x15FAC0; kWheelIndex = 0x15F72C; kWheelItemDefs = 0x179AC0; kWheelSlotIds = 0x141EA0;
    kRac1ItemOwned = 0x13D4C0; kRac1HeldButtons = 0x13CB00; kRac1PressedButtons = 0x13CB04;   // controller_state 0x13C940 + 0x1C0 / 0x1C4 (pad_state.h; NTSC src refers to D_0013CB04)
    kPauseMenuSystem = 0x1D5BF0; kPauseWeaponCells = 0x1CF120; kPauseItemInfo = 0x1863D0;
    kRac1VendorDialogue = 0x1CA4C0;
}
constexpr std::size_t kRac1WeaponV133Size = offsetof(openrac_rac1_weapon, weapon_preview_rotation);
constexpr std::size_t kRac1WeaponV134Size = offsetof(openrac_rac1_weapon, hud_icon);

struct WeaponRecordPatch {
    openrac_gaddr address = 0;
    unsigned size = 0;
    std::array<std::uint8_t, 0x4C> original{}, written{};
};

struct NativeWeapon {
    openrac_mod* owner = nullptr;
    openrac_rac1_weapon def{};
    std::uint32_t loads = 0;
    std::uint32_t cooldown = 0;
    bool installed = false;
    std::array<WeaponRecordPatch, 6> records{};
    std::array<std::string, 2> meshes{};
    std::array<int, 2> models{-1, -1};
    std::string name;
};
std::array<NativeWeapon, 37> g_native_weapons{};
bool g_native_fire_held = false;
bool g_native_fire_edge = false;

bool native_weapons_enabled() {
    return g_native_weapons[1].owner != nullptr || g_native_weapons[36].owner != nullptr;
}

int api_rac1_weapon_register(openrac_mod* mod, const openrac_rac1_weapon* def) {
    rac1_select_weapon_addresses();
    if (mod == nullptr || mod != g_loading || g_host == nullptr || g_host->game != "rac1" ||
        (g_host->version != "pal" && g_host->version != "ntsc") || def == nullptr || def->size < kRac1WeaponV133Size ||
        (def->size > kRac1WeaponV133Size && def->size < kRac1WeaponV134Size) ||
        (def->size > kRac1WeaponV134Size && def->size < sizeof(*def)) ||
        (def->size >= sizeof(*def) && def->hud_icon > 65535) ||
        (def->item_id != 1 && def->item_id != 36) ||
        std::ranges::find(kRac1StockWheelWeapons, def->presentation) == kRac1StockWheelWeapons.end() ||
        def->fire == nullptr || def->capacity > 999 || def->initial_ammo > def->capacity ||
        def->ammo_per_shot > def->capacity || (def->capacity != 0 && def->ammo_per_shot == 0 &&
         !(def->flags & OPENRAC_RAC1_WEAPON_EXTERNAL_AMMO)) ||
        def->ammo_price > 65535 || (def->capacity != 0 && def->ammo_price == 0) ||
        (def->capacity == 0 && def->ammo_price != 0) || def->weapon_price > 9999999 || def->icon > 65535 || def->repeat_fire > 1 ||
        (def->flags & ~(OPENRAC_RAC1_WEAPON_EXTERNAL_SHOP | OPENRAC_RAC1_WEAPON_EXTERNAL_AMMO |
                        OPENRAC_RAC1_WEAPON_NO_HELD_MODEL)) ||
        !std::isfinite(def->weapon_mesh_scale) || def->weapon_mesh_scale < 0 ||
        !std::isfinite(def->weapon_preview_scale) || def->weapon_preview_scale < 0 ||
        !std::isfinite(def->weapon_preview_z))
        return OPENRAC_HOOK_UNKNOWN;
    if (def->size >= kRac1WeaponV134Size &&
        !std::all_of(def->weapon_preview_rotation, def->weapon_preview_rotation + 3,
                     [](float r) { return std::isfinite(r); }))
        return OPENRAC_HOOK_UNKNOWN;
    auto& weapon = g_native_weapons[def->item_id];
    if (weapon.owner != nullptr) return OPENRAC_HOOK_REFUSED;
    const std::array<const char*, 2> paths{def->weapon_mesh, def->ammo_mesh};
    for (const char* path : paths) {
        if (path != nullptr && (g_api.model_create == nullptr || g_api.moby_model_at == nullptr ||
                                !folder_path(path))) return OPENRAC_HOOK_UNKNOWN;
    }
    const std::string_view name = def->display_name != nullptr ? def->display_name : "";
    if (name.size() > 40 || std::ranges::any_of(name, [](unsigned char c) {
            return c < 32 || c > 126 || c == '%';
        })) return OPENRAC_HOOK_UNKNOWN;
    const int claim = api_rac1_reserve_item_ids(mod, &def->item_id, 1);
    if (claim != OPENRAC_HOOK_OK) return claim;
    const int enroll = api_rac1_weapon_ui_add(mod, def->item_id);
    if (enroll != OPENRAC_HOOK_OK) return enroll;
    weapon.owner = mod;
    std::memcpy(&weapon.def, def, std::min<std::size_t>(def->size, sizeof weapon.def));
    weapon.def.size = static_cast<std::uint32_t>(std::min<std::size_t>(def->size, sizeof weapon.def));
    weapon.def.weapon_mesh = weapon.def.ammo_mesh = weapon.def.display_name = nullptr;
    weapon.name = name;
    for (std::size_t i = 0; i < paths.size(); ++i) {
        if (paths[i] != nullptr) weapon.meshes[i] = paths[i];
    }
    return OPENRAC_HOOK_OK;
}

// The item's own level and menu records still hold what was written: the name, the class (hand animation, model)
// and the icon. (The game writes runtime fields into them while the weapon is held, such as +0x20: not compared.)
// Another writer (a mod that borrows the record, a stale restore) can put the disc's record back while `installed`
// and the load number still say it is in place: the wheel then shows the donor's icon and the hand its animation.
bool native_item_records_intact(const NativeWeapon& weapon) {
    constexpr std::array<std::pair<unsigned, unsigned>, 3> fields{{{0x00, 4}, {0x10, 4}, {0x38, 2}}};
    for (unsigned i = 0; i < 2; ++i) {
        const auto& patch = weapon.records[i];
        if (!guest_range(patch.address, patch.size)) return false;
        for (const auto& [offset, size] : fields) {
            if (std::memcmp(G(patch.address + offset), patch.written.data() + offset, size) != 0) return false;
        }
    }
    return true;
}

void install_native_weapon(NativeWeapon& weapon, std::uint32_t loads) {
    const bool current = weapon.installed && weapon.loads == loads;
    if (current && native_item_records_intact(weapon)) return;
    if (current && std::getenv("OPENRAC_WEAPON_LOG") != nullptr) {
        for (unsigned i = 0; i < 2; ++i) {
            const auto& patch = weapon.records[i];
            for (unsigned b = 0; b < patch.size; ++b)
                if (GREF(std::uint8_t, patch.address + b) != patch.written[b]) {
                    log::info("weapon {}: record {} changed behind the install at +0x{:X}: {:02X} -> {:02X}, written again",
                              weapon.def.item_id, i, b, patch.written[b], GREF(std::uint8_t, patch.address + b));
                    break;
                }
        }
    }
    const auto& def = weapon.def;
    const auto defs = openrac_relocate_level_data(0, kWheelItemDefs);
    const auto info = OPENRAC_DATA(kPauseItemInfo);
    const auto vendor = openrac_relocate_level_data(0, kRac1VendorInfo);
    const auto prices = openrac_relocate_level_data(0, kRac1ItemPrices);
    const auto boot_prices = OPENRAC_DATA(kRac1ExePrices);
    const auto preview_poses = OPENRAC_DATA(kRac1PreviewPoses);  // pause item preview anchor, per native item
    const openrac_gaddr bases[] = {defs, info, vendor, prices, boot_prices, preview_poses};
    const unsigned strides[] = {0x4C, 0x4C, 0x14, 24, 24, 0x20};
    for (unsigned i = 0; i < weapon.records.size(); ++i) {
        if (current && i >= 2) continue;  // a repair of the item records only
        auto& patch = weapon.records[i];
        const auto address = bases[i] + def.item_id * strides[i];
        // Some boot tables survive a respawn unchanged. A repair keeps the original taken at the install.
        const bool already_patched = current || (weapon.installed && patch.address == address &&
            std::memcmp(G(address), patch.written.data(), strides[i]) == 0);
        if (!already_patched) std::memcpy(patch.original.data(), G(address), strides[i]);
        patch.address = address;
        patch.size = strides[i];
        std::memcpy(G(address), G(bases[i] + def.presentation * strides[i]), strides[i]);
        if (i < 2 && def.icon != 0)
            GREF(std::uint16_t, address + 0x38) = static_cast<std::uint16_t>(def.icon);
        if (i == 3 || i == 4) {
            GREF(std::uint32_t, address) = def.weapon_price;
            GREF(std::uint32_t, address + 4) = def.weapon_price;
            GREF(std::uint16_t, address + 8) = static_cast<std::uint16_t>(def.ammo_price);
            GREF(std::uint16_t, address + 10) = static_cast<std::uint16_t>(def.ammo_price);
            GREF(std::uint16_t, address + 14) = static_cast<std::uint16_t>(def.capacity);
            GREF(std::uint16_t, address + 18) = static_cast<std::uint16_t>(def.initial_ammo);
        }
        std::memcpy(patch.written.data(), G(address), strides[i]);
    }
    weapon.installed = true;
    if (current) return;
    weapon.loads = loads;
    weapon.cooldown = 0;
    weapon.models = {-1, -1};
}

struct WeaponModelPlacement {
    openrac_mod* owner = nullptr;
    openrac_gaddr address = 0;
    int model = -1;
    std::uint32_t loads = 0;
    bool hidden = false;
};
WeaponModelPlacement g_held_weapon_model;
std::array<WeaponModelPlacement, 3> g_vendor_weapon_models{};
std::array<WeaponModelPlacement, 2> g_pause_weapon_models{};

void clear_weapon_model(WeaponModelPlacement& placement) {
    if (placement.owner != nullptr && g_api.moby_model_at != nullptr) {
        openrac_moby_model clear{};
        clear.size = sizeof clear;
        clear.model = -1;
        g_api.moby_model_at(placement.owner, placement.address, &clear);
    }
    placement = {};
}

void place_weapon_model(WeaponModelPlacement& placement, NativeWeapon* weapon,
                        openrac_gaddr address, bool ammo, bool pause = false, bool hide = false) {
    if (weapon == nullptr || !guest_range(address, 0x100)) {
        clear_weapon_model(placement);
        return;
    }
    if (hide) {
        if (placement.owner == weapon->owner && placement.address == address &&
            placement.loads == weapon->loads && placement.hidden) return;
        clear_weapon_model(placement);
        openrac_moby_model value{};
        value.size = sizeof value;
        value.flags = OPENRAC_MODEL_HIDE_NATIVE;
        value.model = -1;
        value.matrix[0] = value.matrix[5] = value.matrix[10] = value.matrix[15] = 1;
        if (g_api.moby_model_at(weapon->owner, address, &value) == OPENRAC_HOOK_OK)
            placement = {weapon->owner, address, -1, weapon->loads, true};
        return;
    }
    if (weapon->meshes[ammo].empty()) {
        clear_weapon_model(placement);
        return;
    }
    int& model = weapon->models[ammo];
    if (model == -1) {
        model = g_api.model_create(weapon->owner, weapon->meshes[ammo].c_str());
        if (model < 0) model = -2;  // retry only on a new level
    }
    if (model < 0) {
        clear_weapon_model(placement);
        return;
    }
    if (placement.owner == weapon->owner && placement.address == address &&
        placement.model == model && placement.loads == weapon->loads && !placement.hidden) return;
    clear_weapon_model(placement);
    openrac_moby_model value{};
    value.size = sizeof value;
    value.model = model;
    float scale = !ammo && weapon->def.weapon_mesh_scale > 0 ? weapon->def.weapon_mesh_scale : 1;
    if (pause && !ammo) {
        if (weapon->def.weapon_preview_scale > 0) scale = weapon->def.weapon_preview_scale;
        value.matrix[14] = weapon->def.weapon_preview_z;
        const auto* r = weapon->def.weapon_preview_rotation;
        const float cx = std::cos(r[0]), sx = std::sin(r[0]);
        const float cy = std::cos(r[1]), sy = std::sin(r[1]);
        const float cz = std::cos(r[2]), sz = std::sin(r[2]);
        // Column-major Rz * Ry * Rx: local X, then Y, then Z.
        value.matrix[0] = (cz * cy) * scale;
        value.matrix[4] = (cz * sy * sx - sz * cx) * scale;
        value.matrix[8] = (cz * sy * cx + sz * sx) * scale;
        value.matrix[1] = (sz * cy) * scale;
        value.matrix[5] = (sz * sy * sx + cz * cx) * scale;
        value.matrix[9] = (sz * sy * cx - cz * sx) * scale;
        value.matrix[2] = (-sy) * scale;
        value.matrix[6] = (cy * sx) * scale;
        value.matrix[10] = (cy * cx) * scale;
    } else {
        value.matrix[0] = value.matrix[5] = value.matrix[10] = scale;
    }
    value.matrix[15] = 1;
    if (g_api.moby_model_at(weapon->owner, address, &value) == OPENRAC_HOOK_OK)
        placement = {weapon->owner, address, model, weapon->loads, false};
}

void restore_native_weapon_records(NativeWeapon& weapon) {
    for (const auto& record : weapon.records) {
        if (openrac_guest_base != nullptr && guest_range(record.address, record.size) &&
            std::memcmp(G(record.address), record.written.data(), record.size) == 0)
            std::memcpy(G(record.address), record.original.data(), record.size);
    }
    weapon.installed = false;
}

bool external_weapon_shop(const NativeWeapon& weapon) {
    return weapon.owner != nullptr && (weapon.def.flags & OPENRAC_RAC1_WEAPON_EXTERNAL_SHOP);
}

unsigned g_external_vendor_grace = 0;
bool g_external_vendor_menu = false;
bool g_native_vendor_menu = false;

void release_native_weapons(const openrac_mod* mod) {
    if (g_held_weapon_model.owner == mod) clear_weapon_model(g_held_weapon_model);
    for (auto& p : g_vendor_weapon_models) if (p.owner == mod) clear_weapon_model(p);
    for (auto& p : g_pause_weapon_models) if (p.owner == mod) clear_weapon_model(p);
    for (auto& weapon : g_native_weapons) {
        if (weapon.owner != mod) continue;
        restore_native_weapon_records(weapon);
        weapon = {};
    }
    if (!native_weapons_enabled()) {
        g_external_vendor_grace = 0;
        g_external_vendor_menu = false;
        g_native_vendor_menu = false;
        g_native_fire_held = g_native_fire_edge = false;
    }
}

bool playing_status(openrac_play_status& status) {
    status.size = sizeof status;
    return g_api.play_status != nullptr && g_api.play_status(&status) == OPENRAC_HOOK_OK &&
        status.phase == OPENRAC_PLAY_PLAYING;
}

int api_rac1_weapon_grant(openrac_mod* mod, std::uint32_t id) {
    if (mod == nullptr || id >= g_native_weapons.size() || g_native_weapons[id].owner != mod)
        return OPENRAC_HOOK_REFUSED;
    openrac_play_status status{};
    if (!playing_status(status)) return OPENRAC_HOOK_UNKNOWN;
    auto& weapon = g_native_weapons[id];
    install_native_weapon(weapon, status.loads);
    auto& owned = GREF(std::uint8_t, kRac1ItemOwned + id);
    if (owned == 0) {
        GREF(std::int32_t, kRac1AmmoCounts + id * 4) = static_cast<int>(weapon.def.initial_ammo);
        GREF(std::uint8_t, kRac1ItemAcquired + id) = 1;
        owned = 1;
    }
    return OPENRAC_HOOK_OK;
}

NativeWeapon* held_native_weapon() {
    const auto id = GREF(std::uint32_t, kRac1HeroHand + 0x28);
    if (id >= g_native_weapons.size() || !g_native_weapons[id].installed ||
        GREF(std::uint8_t, kRac1ItemOwned + id) == 0 ||
        GREF(openrac_gaddr, kRac1HeroHand) == 0)
        return nullptr;
    return &g_native_weapons[id];
}

void native_weapon_pad(int port, unsigned char* report, int size) {
    if (port != 0 || report == nullptr || size < 2) return;
    openrac_play_status status{};
    if (!native_weapons_enabled() || !playing_status(status) || held_native_weapon() == nullptr) {
        g_native_fire_held = g_native_fire_edge = false;
        return;
    }
    constexpr auto circle = static_cast<unsigned char>(OPENRAC_PAD_CIRCLE >> 8);
    const bool down = (report[1] & circle) == 0;
    g_native_fire_edge |= down && !g_native_fire_held;
    g_native_fire_held = down;
    report[1] |= circle;
}

void update_native_weapons() {
    if (!native_weapons_enabled()) return;
    openrac_play_status status{};
    status.size = sizeof status;
    const bool have_status = g_api.play_status != nullptr &&
        g_api.play_status(&status) == OPENRAC_HOOK_OK;
    if (std::getenv("OPENRAC_WEAPON_LOG") != nullptr) {
        static std::array<std::int64_t, 10> last{};
        for (const auto& w : g_native_weapons) {
            if (w.owner == nullptr) continue;
            const auto defs = openrac_relocate_level_data(0, kWheelItemDefs) + w.def.item_id * 0x4C;
            const std::array<std::int64_t, 10> now{status.phase, status.loads, w.installed, w.loads,
                GREF(std::uint8_t, kRac1ItemOwned + w.def.item_id), GREF(std::int32_t, defs + 0x10),
                GREF(std::uint16_t, defs + 0x38), GREF(std::uint32_t, kRac1HeroHand + 0x28),
                g_external_vendor_grace, g_external_vendor_menu};
            if (now != last) {
                last = now;
                log::info("weapon {}: phase {} loads {} installed {} (loads {}) owned {} defs class {} icon {} held {} grace {} vendor_menu {}",
                          w.def.item_id, now[0], now[1], now[2], now[3], now[4], now[5], now[6], now[7], now[8], now[9]);
            }
        }
    }
    if (!have_status || status.phase != OPENRAC_PLAY_PLAYING) {
        if (status.phase == OPENRAC_PLAY_MENU && g_external_vendor_grace != 0)
            g_external_vendor_menu = true;
        if (g_native_vendor_menu) {
            for (auto& weapon : g_native_weapons) {
                if (weapon.owner == nullptr || (g_external_vendor_menu && external_weapon_shop(weapon))) continue;
                install_native_weapon(weapon, status.loads);
            }
            g_native_vendor_menu = false;
        } else if (status.phase == OPENRAC_PLAY_MENU && weapon_grid_is_active()) {
            for (auto& weapon : g_native_weapons) {
                if (weapon.owner != nullptr) install_native_weapon(weapon, status.loads);
            }
        } else {
            clear_weapon_model(g_held_weapon_model);
            for (auto& p : g_vendor_weapon_models) clear_weapon_model(p);
            for (auto& p : g_pause_weapon_models) clear_weapon_model(p);
            restore_weapon_grid();
            for (auto& weapon : g_native_weapons)
                if (weapon.owner != nullptr && weapon.installed) restore_native_weapon_records(weapon);
            // A frame of the vendor menu the game's menu update did not run in is still that menu: only
            // leaving the menus ends it, or the next frames would write the weapon over the mod's rows.
            if (status.phase != OPENRAC_PLAY_MENU) {
                g_external_vendor_menu = false;
                g_external_vendor_grace = 0;
            }
        }
        g_native_fire_held = g_native_fire_edge = false;
        return;
    }
    g_native_vendor_menu = false;
    // The mod that sells the weapon puts its borrowed records back a few ticks after the menu closes: the
    // grace runs from the first tick of play (it used to end there), and the weapon is written after it.
    if (g_external_vendor_menu) g_external_vendor_menu = false;
    else if (g_external_vendor_grace != 0) --g_external_vendor_grace;
    for (auto& weapon : g_native_weapons) {
        if (weapon.owner == nullptr) continue;
        if (external_weapon_shop(weapon) && (g_external_vendor_grace != 0 ||
            (!weapon.installed && GREF(std::uint8_t, kRac1ItemOwned + weapon.def.item_id) == 0)))
            continue;
        install_native_weapon(weapon, status.loads);
        if (weapon.cooldown > 0) --weapon.cooldown;
    }
    for (auto& p : g_vendor_weapon_models) clear_weapon_model(p);
    for (auto& p : g_pause_weapon_models) clear_weapon_model(p);
    const bool edge = std::exchange(g_native_fire_edge, false);
    auto* weapon = held_native_weapon();
    place_weapon_model(g_held_weapon_model, weapon, GREF(openrac_gaddr, kRac1HeroHand), false,
                       false, weapon != nullptr && (weapon->def.flags & OPENRAC_RAC1_WEAPON_NO_HELD_MODEL));
    if (weapon == nullptr || weapon->cooldown != 0 ||
        !(edge || (weapon->def.repeat_fire && g_native_fire_held))) return;
    const auto& def = weapon->def;
    auto& ammo = GREF(std::int32_t, kRac1AmmoCounts + def.item_id * 4);
    const int cost = static_cast<int>(def.ammo_per_shot);
    const bool consume = def.capacity != 0 && !(def.flags & OPENRAC_RAC1_WEAPON_EXTERNAL_AMMO);
    if (consume && ammo < cost) return;
    if (!def.fire(def.user, def.item_id)) return;
    if (consume) ammo = std::max(0, ammo - cost);
    weapon->cooldown = def.cooldown_ticks;
    if (def.capacity != 0) {
        const auto pair = openrac_relocate_level_data(0, kRac1WeaponHudPair);
        const int handle = GREF(int, pair + 4);
        if (GREF(std::uint32_t, pair) == def.item_id && handle >= 0) {
            const auto show = reinterpret_cast<void (*)(int, int)>(g_api.function(kRac1HudShowFunction));
            if (show != nullptr) show(handle, kRac1HudShowTicks);  // 2.5 seconds
        }
    }
}

using NativeVendorBuild = void (*)(int);
NativeVendorBuild g_native_vendor_build = nullptr;
constexpr int kRac1VendorRows = 16;

void native_weapon_vendor_build(int skip_unflagged) {
    // The retail loop writes owned ammunition rows without a capacity check.
    // Keep the custom IDs out of it, then add their rows within the native limit.
    constexpr std::array<std::uint32_t, 2> ids{1, 36};
    std::array<std::uint8_t, ids.size()> owned{};
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (!g_native_weapons[ids[i]].installed ||
            (g_native_weapons[ids[i]].def.flags & OPENRAC_RAC1_WEAPON_EXTERNAL_SHOP)) continue;
        auto& flag = GREF(std::uint8_t, kRac1ItemOwned + ids[i]);
        owned[i] = std::exchange(flag, 0);
    }
    g_native_vendor_build(skip_unflagged);
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (g_native_weapons[ids[i]].installed &&
            !(g_native_weapons[ids[i]].def.flags & OPENRAC_RAC1_WEAPON_EXTERNAL_SHOP)) GREF(std::uint8_t, kRac1ItemOwned + ids[i]) = owned[i];
    }

    const auto menu = openrac_relocate_level_data(0, kRac1VendorDialogue);
    const auto vendor = openrac_relocate_level_data(0, kRac1VendorInfo);
    auto& count = GREF(int, menu + 0x210);
    if (count < 0 || count >= kRac1VendorRows) return;
    for (std::size_t i = 0; i < ids.size() && count < kRac1VendorRows; ++i) {
        const auto id = ids[i];
        const auto& weapon = g_native_weapons[id];
        if (!weapon.installed || (weapon.def.flags & OPENRAC_RAC1_WEAPON_EXTERNAL_SHOP)) continue;
        const bool ammo = owned[i] != 0;
        if (ammo ? weapon.def.capacity == 0 : (skip_unflagged || weapon.def.weapon_price == 0))
            continue;
        bool duplicate = false;
        for (int row = 0; row < count; ++row)
            duplicate |= GREF(std::uint32_t, menu + 0xD0 + row * 20) == id;
        if (duplicate) continue;
        const auto entry = menu + 0xD0 + count * 20;
        const std::int32_t words[] = {
            static_cast<int>(id), ammo, 0, 0, GREF(std::int32_t, vendor + id * 0x14)
        };
        std::memcpy(G(entry), words, sizeof words);
        ++count;
    }
}

NativeWeapon* installed_weapon(int id) {
    if (id < 0 || static_cast<std::size_t>(id) >= g_native_weapons.size()) return nullptr;
    auto& weapon = g_native_weapons[id];
    return weapon.installed ? &weapon : nullptr;
}

using NativeVendorPreview = void (*)();
NativeVendorPreview g_native_vendor_preview = nullptr;
WheelDraw g_native_pause_weapon = nullptr;
WheelDraw g_native_pause_ammo = nullptr;

void native_vendor_preview() {
    g_native_vendor_preview();
    const auto menu = openrac_relocate_level_data(0, kRac1VendorDialogue);
    const int index = GREF(int, menu + 0x58);
    const int count = GREF(int, menu + 0x210);
    NativeWeapon* weapon = nullptr;
    if (index >= 0 && index < count && index < kRac1VendorRows)
        weapon = installed_weapon(GREF(int, menu + 0xD0 + index * 20));
    if (weapon != nullptr && (weapon->def.flags & OPENRAC_RAC1_WEAPON_EXTERNAL_SHOP)) weapon = nullptr;
    const auto preview = GREF(openrac_gaddr, menu + 0x20);
    const bool valid = guest_range(preview, 0x400);
    for (unsigned i = 0; i < g_vendor_weapon_models.size(); ++i)
        place_weapon_model(g_vendor_weapon_models[i], weapon,
                           valid ? preview + (i + 1) * 0x100 : 0, i == 1);
}

NativeWeapon* selected_pause_weapon() {
    if (g_weapon_grid.grid == 0 || weapon_grid_focus() != g_weapon_grid.grid) return nullptr;
    const int selected = GREF(int, g_weapon_grid.grid + 0x3C);
    if (selected < 0 || selected >= kPauseWeaponSlots) return nullptr;
    return installed_weapon(g_weapon_grid.published[selected].item);
}

int native_pause_preview(openrac_gaddr screen, bool ammo, WheelDraw draw) {
    const auto moby = GREF(openrac_gaddr, screen + 0x44);
    place_weapon_model(g_pause_weapon_models[ammo], selected_pause_weapon(), moby, ammo, true);
    return draw(screen);
}

int native_pause_draw_weapon(openrac_gaddr screen) {
    return native_pause_preview(screen, false, g_native_pause_weapon);
}

int native_pause_draw_ammo(openrac_gaddr screen) {
    return native_pause_preview(screen, true, g_native_pause_ammo);
}

WheelTick g_native_vendor_open = nullptr;
using NativeVendorUpdate = void (*)();
NativeVendorUpdate g_native_vendor_update = nullptr;

void native_vendor_open(openrac_gaddr vendor) {
    bool suspend = false;
    for (auto& weapon : g_native_weapons) {
        if (!external_weapon_shop(weapon)) continue;
        suspend = true;
        if (weapon.installed) restore_native_weapon_records(weapon);
        if (g_held_weapon_model.owner == weapon.owner) clear_weapon_model(g_held_weapon_model);
    }
    if (suspend) {
        g_external_vendor_grace = 30;
        g_external_vendor_menu = true;
    }
    g_native_vendor_menu = true;
    g_native_vendor_open(vendor);
}

void native_vendor_menu_update() {
    g_native_vendor_menu = true;
    g_native_vendor_update();
}

std::string_view registered_weapon_name(int item) {
    const auto* weapon = installed_weapon(item);
    return weapon == nullptr ? std::string_view{} : std::string_view(weapon->name);
}

WheelDraw g_native_pause_name = nullptr;
using NativeMessage = int (*)(int);
NativeMessage g_native_weapon_message = nullptr;

int native_weapon_message(int id) {
    if (g_active_weapon_name.text != 0) {
        const auto item = static_cast<unsigned>(g_active_weapon_name.item);
        const auto defs = openrac_relocate_level_data(0, kWheelItemDefs) + item * 0x4C;
        const auto info = OPENRAC_DATA(kPauseItemInfo) + item * 0x4C;
        if (id == GREF(std::uint16_t, defs + 0x46) || id == GREF(std::uint16_t, defs + 0x48) ||
            id == GREF(std::uint16_t, info + 0x40) || id == GREF(std::uint16_t, info + 0x42) ||
            id == GREF(int, info))
            return static_cast<int>(g_active_weapon_name.text);
    }
    return g_native_weapon_message(id);
}

int native_pause_draw_name(openrac_gaddr screen) {
    const auto* weapon = selected_pause_weapon();
    const ScopedWeaponName name(weapon == nullptr ? -1 : static_cast<int>(weapon->def.item_id));
    return g_native_pause_name(screen);
}

void restore_native_weapon_runtime() {
    clear_weapon_model(g_held_weapon_model);
    for (auto& placement : g_vendor_weapon_models) clear_weapon_model(placement);
    for (auto& placement : g_pause_weapon_models) clear_weapon_model(placement);
    restore_weapon_grid();
    for (auto& weapon : g_native_weapons)
        if (weapon.owner != nullptr && weapon.installed) restore_native_weapon_records(weapon);
    g_native_vendor_menu = false;
    g_external_vendor_menu = false;
    g_external_vendor_grace = 0;
    g_native_fire_held = g_native_fire_edge = false;
}

using NativeShipTransition = void (*)(int);
NativeShipTransition g_native_ship_transition = nullptr;
NativeShipTransition g_native_ship_travel = nullptr;
using NativeSaveRestore = void (*)(std::uint32_t);
NativeSaveRestore g_native_save_restore = nullptr;
using NativeNewGame = void (*)();
NativeNewGame g_native_new_game = nullptr;
NativeShipTransition g_native_new_game_plus = nullptr;

void native_ship_transition(int level) {
    restore_native_weapon_runtime();
    g_native_ship_transition(level);
}

void native_ship_travel(int level) {
    restore_native_weapon_runtime();
    g_native_ship_travel(level);
}

void native_save_restore(std::uint32_t slot) {
    restore_native_weapon_runtime();
    g_native_save_restore(slot);
}

void native_new_game() {
    restore_native_weapon_runtime();
    g_native_new_game();
}

void native_new_game_plus(int mode) {
    restore_native_weapon_runtime();
    g_native_new_game_plus(mode);
}

void install_native_weapon_hooks() {
    if (!native_weapons_enabled()) return;
    chain_weapon_wheel_hook("rac1.vendor.open_menu", native_vendor_open, g_native_vendor_open);
    chain_weapon_wheel_hook("rac1.vendor.menu_update", native_vendor_menu_update, g_native_vendor_update);
    chain_weapon_wheel_hook("rac1.vendor.build_item_list", native_weapon_vendor_build, g_native_vendor_build);
    chain_weapon_wheel_hook("rac1.vendor.initialize_selected", native_vendor_preview, g_native_vendor_preview);
    chain_weapon_wheel_hook("rac1.menu.preview.draw_available", native_pause_draw_weapon, g_native_pause_weapon);
    chain_weapon_wheel_hook("rac1.text.msg_string", native_weapon_message, g_native_weapon_message);
    chain_weapon_wheel_hook("rac1.menu.item_text.draw", native_pause_draw_name, g_native_pause_name);
    chain_weapon_wheel_hook("rac1.menu.preview.draw_selected", native_pause_draw_ammo, g_native_pause_ammo);
    chain_weapon_wheel_hook("rac1.ship.init_transition", native_ship_transition, g_native_ship_transition);
    chain_weapon_wheel_hook("rac1.ship.travel_to", native_ship_travel, g_native_ship_travel);
    chain_weapon_wheel_hook("rac1.save.restore", native_save_restore, g_native_save_restore);
    chain_weapon_wheel_hook("rac1.save.new_game", native_new_game, g_native_new_game);
    chain_weapon_wheel_hook("rac1.save.new_game_plus", native_new_game_plus, g_native_new_game_plus);
}
