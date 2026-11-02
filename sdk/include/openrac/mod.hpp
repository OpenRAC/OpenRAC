// SPDX-License-Identifier: MIT
// Copyright (c) 2026 the OpenRAC contributors
//
// The C++ layer of the mod interface (docs/port/MODS.md, section 11): header-only, C++17, compiled
// into the mod and never into the port. Everything here goes through openrac_mod.h's table, which
// stays the boundary; no exception crosses it.
//
// The header is laid out as: the includes, the error and log types, game memory, binds and the
// input state, the hook machinery (`detail`), the Mod itself, the stack frame, and last the macro
// that writes a mod's entry points. Each item says which interface version added it; the layer
// itself needs only the table of the interface the mod was built for.
//
//   #include "openrac/mod.hpp"
//   #include "openrac/games/rac1.hpp"
//
//   static void load(openrac::Mod& mod) {
//       if (mod.game() != "rac1") throw openrac::Error("this mod is for Ratchet & Clank");
//       const bool trace = mod.setting<bool>("trace");
//       mod.hook<openrac::rac1::Hero_TurnToHeading>(
//           [](auto original, int32_t m, float accel, float damping, float limit) {
//               original(m, accel, damping, limit);   // the game's own, or a mod's loaded earlier
//           });
//       mod.on_frame([] { /* once a game tick */ });
//   }
//   OPENRAC_MOD_CPP("my-mod", "0.1.0", load)
//
// One library is one mod: the layer keeps one openrac::Mod per library, and each hook point's
// callable in a static of its own (a load takes a hook point once; a later load of the same
// library starts afresh). An exception out of the
// load function becomes the mod's refusal (fail() with its message); one out of a hook or a
// callback is logged with the mod's id, and a hook then calls its original, so the game goes on
// (a hook that threw after calling the original has it called twice: do not throw there).
#ifndef OPENRAC_MOD_HPP
#define OPENRAC_MOD_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <exception>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "openrac/keys.h"
#include "openrac/openrac_mod.h"

// Keeps the layer's symbols out of the mod's exports: nothing of the layer is seen outside the mod's library, whatever the build's visibility: two mods
// that both use it never share its statics.
#if defined(_WIN32)
#define OPENRAC_MOD_HIDDEN
#else
#define OPENRAC_MOD_HIDDEN __attribute__((visibility("hidden")))
#endif

namespace openrac OPENRAC_MOD_HIDDEN {

// ----------------------------------------------------------------------------------------------
// Errors, log levels and game memory
// ----------------------------------------------------------------------------------------------

// A game address, as openrac_gaddr. Since 1.0.
using gaddr = openrac_gaddr;

// What load and the layer throw: its message is the mod's refusal, or the log's line. Since 1.0.
class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// The levels of Mod::log, from the most to the least talkative: the OPENRAC_LOG_* values. Since 1.0.
enum class LogLevel : int {
    Debug = OPENRAC_LOG_DEBUG,
    Info = OPENRAC_LOG_INFO,
    Warn = OPENRAC_LOG_WARN,
    Error = OPENRAC_LOG_ERROR,
};

// Game memory: typed reads and writes at game addresses. Reads and writes copy (any alignment);
// ref() gives an lvalue where the address is aligned for T. A view: it owns nothing, and the
// memory it views lasts for the life of the program. Since 1.0.
class Memory {
public:
    // Views the memory that begins at `base` (openrac_api.memory).
    explicit Memory(std::uint8_t* base) : m_base(base) {}

    // Returns the `T` at `address`, copied out; `T` is a plain value (trivially copyable).
    template <class T>
    T read(gaddr address) const {
        static_assert(std::is_trivially_copyable_v<T>, "game memory holds plain values");
        T value;
        std::memcpy(&value, m_base + address, sizeof value);
        return value;
    }

    // Copies `value` to `address`; `T` is a plain value (trivially copyable). It does not mark
    // anything the game caches: a value the game keeps elsewhere is the mod's to update.
    template <class T>
    void write(gaddr address, const T& value) const {
        static_assert(std::is_trivially_copyable_v<T>, "game memory holds plain values");
        std::memcpy(m_base + address, &value, sizeof value);
    }

    // Returns the `T` at `address` as an lvalue. The address must be aligned for `T`, and the
    // reference must not be kept across a level load (the level's program moves its globals).
    template <class T>
    T& ref(gaddr address) const {
        return *reinterpret_cast<T*>(m_base + address);
    }

    // Returns the host pointer to `address`.
    void* pointer(gaddr address) const { return m_base + address; }

    // Returns the host pointer to game address 0.
    std::uint8_t* base() const { return m_base; }

private:
    // The base of game memory: game address a is m_base + a. Not owned.
    std::uint8_t* m_base;
};

// ----------------------------------------------------------------------------------------------
// Binds and the input state (since 1.2)
// ----------------------------------------------------------------------------------------------

// Returns the bind (openrac/keys.h) a name is: a key, a mouse button or a wheel direction, by its
// name ("KeyW", "MouseLeft", "WheelUp"); OPENRAC_BIND_NONE (0) for a name of no bind. Since 1.2.
inline int bind_from_name(std::string_view name) {
    return openrac_bind_from_name_n(name.data(), name.size());
}

// Returns a bind's name, or an empty view for a bind of none. Since 1.2.
inline std::string_view bind_name(int bind) {
    return openrac_bind_to_name(bind);
}

// The keyboard and mouse for the tick about to run (interface 1.2, openrac_input): a view of the
// port's state, renewed each tick. Cheap to copy; do not keep it past the tick it was read in.
class Input {
public:
    // Views the port's `state` (openrac_api.input()'s result).
    explicit Input(const openrac_input* state) : m_state(state) {}

    // Returns the C state, for what the accessors do not cover.
    const openrac_input& raw() const { return *m_state; }

    // Returns whether a bind (a key's HID usage, OPENRAC_BIND_*) is held now.
    bool held(int bind) const { return openrac_bind_held(m_state, bind) != 0; }

    // Returns whether a bind went down since the last tick.
    bool pressed(int bind) const { return openrac_bind_pressed(m_state, bind) != 0; }

    // Return the mouse's motion since the last tick, in the system's pixels (raw while captured).
    float mouse_dx() const { return m_state->mouse_dx; }

    float mouse_dy() const { return m_state->mouse_dy; }

    // Return the wheel's turn since the last tick, in notches (x > 0 right, y > 0 up).
    float wheel_x() const { return m_state->wheel_x; }

    float wheel_y() const { return m_state->wheel_y; }

    // Return whether the game's window has the keyboard, whether the cursor is captured, and
    // whether the port's own overlay has the input now (leave it alone then).
    bool focused() const { return m_state->focused != 0; }

    bool captured() const { return m_state->captured != 0; }

    bool overlay() const { return m_state->overlay != 0; }

    // Returns the tick counter: a new value is a new state.
    std::uint32_t tick() const { return m_state->tick; }

    // Since 1.3: returns whether the port's state reaches the pointer fields. False on a 1.2 port,
    // whose pointer_x(), pointer_y() and pointer_inside() then give 0, 0 and false.
    bool has_pointer() const { return m_state->size >= offsetof(openrac_input, reserved2); }

    // Since 1.3: return where the cursor is over the game's picture, 0..1 across and down
    // (pointer_x() * 512, pointer_y() * 416 is the game's 2D pixel), and whether it is over it.
    float pointer_x() const { return has_pointer() ? m_state->pointer_x : 0.0f; }

    float pointer_y() const { return has_pointer() ? m_state->pointer_y : 0.0f; }

    bool pointer_inside() const { return has_pointer() && m_state->pointer_inside != 0; }

private:
    // The port's state; not owned.
    const openrac_input* m_state;
};

// What a menu page's callback is given each frame (openrac_menu_frame): pad 0's buttons, by the
// OPENRAC_PAD_* bits. A view of the port's state, valid during the call. Since 1.5.
class MenuFrame {
public:
    // Wraps the port's state `frame`.
    explicit MenuFrame(const openrac_menu_frame& frame) : m_frame(&frame) {}

    // Return whether any of `buttons` is down now, or went down since the last frame.
    bool held(std::uint32_t buttons) const { return (m_frame->held & buttons) != 0; }

    bool pressed(std::uint32_t buttons) const { return (m_frame->pressed & buttons) != 0; }

    // Returns whether this is the first frame since the page opened.
    bool opened() const { return m_frame->opened != 0; }

    // Returns the port's state as it is.
    const openrac_menu_frame& raw() const { return *m_frame; }

private:
    // The port's state; not owned.
    const openrac_menu_frame* m_frame;
};

// Draws a mod's menu page during its callback: rectangles and text on a page of
// OPENRAC_MENU_WIDTH by OPENRAC_MENU_HEIGHT pixels, which starts each frame black. Since 1.5.
class MenuPage {
public:
    // The page's size in its own pixels.
    static constexpr int width = OPENRAC_MENU_WIDTH;
    static constexpr int height = OPENRAC_MENU_HEIGHT;

    // Draws through `api` for the mod `handle`.
    MenuPage(const openrac_api* api, openrac_mod* handle) : m_api(api), m_handle(handle) {}

    // Fills `w` by `h` pixels from (x, y), the top left, with `rgba` (OPENRAC_RGBA), blended.
    void rect(int x, int y, int w, int h, std::uint32_t rgba) const {
        m_api->menu_rect(m_handle, x, y, w, h, rgba);
    }

    // Writes `text` (ASCII) from (x, y), its top left, at `size` (1 to 8) in `rgba`.
    void text(int x, int y, int size, std::uint32_t rgba, const std::string& text) const {
        m_api->menu_text(m_handle, x, y, size, rgba, text.c_str());
    }

    // Writes `text` centred across the page, its top at y.
    void text_centred(int y, int size, std::uint32_t rgba, const std::string& text) const {
        this->text((width - text_width(text, size)) / 2, y, size, rgba, text);
    }

    // Returns how wide `text` is at `size`, in pixels: its longest line's characters, without the
    // last one's spacing column.
    static int text_width(std::string_view text, int size) {
        int longest = 0;
        int line = 0;
        for (const char c : text) {
            // A newline starts the next line's count.
            line = c == '\n' ? 0 : line + 1;
            longest = line > longest ? line : longest;
        }
        return longest > 0 ? (longest * OPENRAC_MENU_CHAR_WIDTH - 1) * size : 0;
    }

private:
    // The port's table and the mod's handle; not owned.
    const openrac_api* m_api;
    openrac_mod* m_handle;
};

// ----------------------------------------------------------------------------------------------
// Hooks: what the layer keeps for each hook point, and its thunks
// ----------------------------------------------------------------------------------------------

// A hook's way on: the function the replacement goes on to (the game's own, or the replacement of
// a mod loaded earlier), called like the hook point. Since 1.0.
template <class Signature>
class Original;

template <class R, class... A>
class Original<R(A...)> {
public:
    using fn = R (*)(A...);

    // Wraps the function `f` that the replacement goes on to.
    explicit Original(fn f) : m_fn(f) {}

    // Calls the original with `args`, as the game would have called the hook point.
    R operator()(A... args) const { return m_fn(args...); }

    // Returns the original as a plain function pointer.
    fn get() const { return m_fn; }

private:
    // What the replacement goes on to.
    fn m_fn;
};

class Mod;

// ----------------------------------------------------------------------------------------------
// Pages of the game's own menus (since 1.7)
// ----------------------------------------------------------------------------------------------

// One item of a page (openrac_ui_item), as the mod puts it in a UiContent. Since 1.7.
struct UiItem {
    std::uint32_t kind = OPENRAC_UI_BUTTON;  // OPENRAC_UI_BUTTON, _CHOICE or _TEXT
    std::uint32_t flags = 0;                 // OPENRAC_UI_DISABLED
    std::string label;
    std::vector<std::string> choices;  // a choice's values
    std::uint32_t choice = 0;          // the one shown

    // Return a button Cross chooses (dim and refused when not `enabled`), a line of text the
    // cursor passes over, and a choice of `choices` showing `choice`, which Left and Right step.
    static UiItem button(std::string label, bool enabled = true) {
        UiItem item;
        item.label = std::move(label);
        item.flags = enabled ? 0u : OPENRAC_UI_DISABLED;
        return item;
    }

    static UiItem text(std::string label) {
        UiItem item;
        item.kind = OPENRAC_UI_TEXT;
        item.label = std::move(label);
        return item;
    }

    static UiItem choice_of(std::string label, std::vector<std::string> choices, std::uint32_t choice) {
        UiItem item;
        item.kind = OPENRAC_UI_CHOICE;
        item.label = std::move(label);
        item.choices = std::move(choices);
        item.choice = choice;
        return item;
    }
};

// What a page shows (openrac_ui_content): its title, its list, and its footer's lines (none: the
// game's own hints). Since 1.7.
struct UiContent {
    std::string title;
    std::vector<UiItem> items;
    std::vector<std::string> footer;
};

// An event of a page (openrac_ui_event). A view of the port's, valid during the call. Since 1.7.
class UiEvent {
public:
    explicit UiEvent(const openrac_ui_event& event) : m_event(&event) {}

    // Returns what happened: OPENRAC_UI_SHOW ... OPENRAC_UI_HIDE.
    std::uint32_t kind() const { return m_event->kind; }

    // Returns whether it is `kind`.
    bool is(std::uint32_t kind) const { return m_event->kind == kind; }

    // Return the item it is about (-1 for none), the new choice or the answer (1 yes), the
    // question's tag, and the item under the cursor.
    int item() const { return m_event->item; }

    int value() const { return m_event->value; }

    std::uint32_t tag() const { return m_event->tag; }

    int cursor() const { return m_event->cursor; }

    // Returns the port's event as it is.
    const openrac_ui_event& raw() const { return *m_event; }

private:
    // The port's event; not owned.
    const openrac_ui_event* m_event;
};

// One of the mod's pages of the game's own menus (Mod::ui_page): set what it shows, open it, ask a
// question on it. Copyable; the page lives as long as the program. Since 1.7.
class UiPage {
public:
    // No page (id -1): every call is refused.
    UiPage() = default;

    // The page `id` of the mod `handle`, through `api`.
    UiPage(const openrac_api* api, openrac_mod* handle, int id) : m_api(api), m_handle(handle), m_id(id) {}

    // Returns the page's id.
    int id() const { return m_id; }

    // Puts `content` on the page. Throws openrac::Error when refused (no such page).
    void set(const UiContent& content) const {
        std::vector<std::vector<const char*>> choices;
        std::vector<openrac_ui_item> items;
        choices.reserve(content.items.size());
        items.reserve(content.items.size());

        // Each item with its choices' strings, which live as long as `content`.
        for (const UiItem& from : content.items) {
            auto& values = choices.emplace_back();
            for (const std::string& c : from.choices) {
                values.push_back(c.c_str());
            }
            openrac_ui_item item{};
            item.kind = from.kind;
            item.flags = from.flags;
            item.label = from.label.c_str();
            item.choices = values.empty() ? nullptr : values.data();
            item.choice_count = static_cast<std::uint32_t>(values.size());
            item.choice = from.choice;
            items.push_back(item);
        }

        std::vector<const char*> footer;
        for (const std::string& line : content.footer) {
            footer.push_back(line.c_str());
        }

        openrac_ui_content c{};
        c.size = sizeof c;
        c.title = content.title.c_str();
        c.items = items.empty() ? nullptr : items.data();
        c.item_count = static_cast<std::uint32_t>(items.size());
        c.footer = footer.empty() ? nullptr : footer.data();
        c.footer_count = static_cast<std::uint32_t>(footer.size());

        // A page that is not this mod's is refused.
        if (m_api == nullptr || m_api->ui_set(m_handle, m_id, &c) != OPENRAC_HOOK_OK) {
            throw Error("ui_set: refused (no such page)");
        }
    }

    // Return whether the port took it: shows this page (from another of the mod's on the screen,
    // or since 1.9 a main-menu item's page from the title's main menu), goes back a level, asks `question` on this page (answered by an OPENRAC_UI_ANSWER with
    // `tag`), plays a menu sound (OPENRAC_UI_SOUND_*).
    bool open() const { return m_api != nullptr && m_api->ui_open(m_handle, m_id) == OPENRAC_HOOK_OK; }

    bool back() const {
        return m_api != nullptr && m_api->ui_open(m_handle, OPENRAC_UI_GO_BACK) == OPENRAC_HOOK_OK;
    }

    bool confirm(const std::string& question, std::uint32_t tag = 0) const {
        return m_api != nullptr
               && m_api->ui_confirm(m_handle, m_id, question.c_str(), tag) == OPENRAC_HOOK_OK;
    }

    bool sound(int sound) const {
        return m_api != nullptr && m_api->ui_sound(m_handle, sound) == OPENRAC_HOOK_OK;
    }

    // Return the item under the cursor, after moving it to `item` (an item the cursor takes).
    int cursor() const { return m_api != nullptr ? m_api->ui_cursor(m_handle, m_id, -1) : -1; }

    int set_cursor(int item) const {
        return m_api != nullptr ? m_api->ui_cursor(m_handle, m_id, item < 0 ? -1 : item) : -1;
    }

private:
    const openrac_api* m_api = nullptr;
    openrac_mod* m_handle = nullptr;
    int m_id = -1;
};

namespace detail {

// Tells whether a hook point type declares an id (`static constexpr const char* id`).
template <class T, class = void>
struct HasId : std::false_type {};

template <class T>
struct HasId<T, std::void_t<decltype(T::id)>> : std::true_type {};

// Returns a hook point type's permanent id when it declares one, else its name.
template <class T>
constexpr const char* hook_id() {
    // The generated hook point types have an id that outlasts renames of the game's functions; a
    // hand-written one (a test's) has only a name.
    if constexpr (HasId<T>::value) {
        return T::id;
    } else {
        return T::name;
    }
}

// Returns the library's one mod (nullptr before load): made at load and never destroyed, because
// the port may call a hook or a callback while the program ends, after the library's statics
// would have gone. Only the game's thread touches it.
inline Mod*& instance() {
    static Mod* mod = nullptr;
    return mod;
}

// Logs an exception that a hook or a callback let out, under the mod's id; defined after Mod.
inline void report(const char* where, const char* what);

// Returns the list of what a load took (one release function per hook point), to be let go of
// before the library's next load: a load that the port called again on a library it opened again
// before the system had unloaded it, or a test's. Never destroyed, for the reason of instance().
inline std::deque<void (*)()>& taken() {
    static auto* list = new std::deque<void (*)()>();
    return *list;
}

// Each hook point's replacement and original, one set per library: the statics of the specialisation
// below are the state, and the game's thread is the only one that touches them. Since 1.0.
template <class HookPoint, class Fn = typename HookPoint::fn>
struct Hook;

template <class HookPoint, class R, class... A>
struct Hook<HookPoint, R (*)(A...)> {
    // What a mod's replacement is: called with the way on, then the hook point's arguments.
    using Callable = std::function<R(Original<R(A...)>, A...)>;
    // The hook point's signature as a function pointer.
    using Fp = R (*)(A...);

    // Returns the slot of the mod's replacement; null until Mod::hook takes the point. Owned: the
    // slot is deleted by release().
    static Callable*& callable() {
        static Callable* c = nullptr;
        return c;
    }

    // Returns the slot of what the replacement goes on to; null until the point is taken.
    static Fp& original() {
        static Fp o = nullptr;
        return o;
    }

    // Lets go of the replacement and forgets the original, so the next load starts afresh.
    static void release() {
        delete callable();
        callable() = nullptr;
        original() = nullptr;
    }

    // The function the port calls in the hook point's place: runs the mod's replacement and, when
    // that throws, calls the original, so the game goes on.
    static R thunk(A... args) {
        const auto go_on = original();

        // An exception must not cross into the game's frames: log it, then fall through to the
        // game's own function.
        try {
            return (*callable())(Original<R(A...)>(go_on), args...);
        } catch (const std::exception& e) {
            report(HookPoint::name, e.what());
        } catch (...) {
            report(HookPoint::name, "an exception that is not a std::exception");
        }

        return go_on(args...);
    }
};

// What a mod's on_frame callback is.
using FrameCallable = std::function<void()>;

// What a mod's on_tick_boundary callback is.
using TickBoundaryCallable = std::function<void(std::uint32_t)>;

// What a mod's on_pad callback is.
using PadCallable = std::function<void(int port, std::uint8_t* report, int size)>;

// The openrac_frame_fn the port calls: runs the FrameCallable that `user` points at, and logs what
// it throws instead of letting it into the game's frame loop.
inline void frame_thunk(void* user) {
    // An exception must not cross into the game's frame loop.
    try {
        (*static_cast<FrameCallable*>(user))();
    } catch (const std::exception& e) {
        report("on_frame", e.what());
    } catch (...) {
        report("on_frame", "an exception that is not a std::exception");
    }
}

// The openrac_tick_boundary_fn the port calls: runs the callback and logs what it throws.
inline void tick_boundary_thunk(void* user, std::uint32_t tick) {
    try {
        (*static_cast<TickBoundaryCallable*>(user))(tick);
    } catch (const std::exception& e) {
        report("on_tick_boundary", e.what());
    } catch (...) {
        report("on_tick_boundary", "an exception that is not a std::exception");
    }
}

// The openrac_pad_fn the port calls: runs the PadCallable that `user` points at, and logs what it
// throws instead of letting it into the game's pad reading.
inline void pad_thunk(void* user, int port, std::uint8_t* report_bytes, int size) {
    // An exception must not cross into the game's pad reading.
    try {
        (*static_cast<PadCallable*>(user))(port, report_bytes, size);
    } catch (const std::exception& e) {
        report("on_pad", e.what());
    } catch (...) {
        report("on_pad", "an exception that is not a std::exception");
    }
}

// What a mod's menu page is: called each frame the page is open, it draws through the MenuPage
// and returns true to close the page. Since 1.5.
using MenuCallable = std::function<bool(const MenuPage& page, const MenuFrame& frame)>;

// A menu page as the layer keeps it: the callable, and the table and handle its MenuPage draws
// through.
struct MenuEntry {
    MenuCallable callable;
    const openrac_api* api;
    openrac_mod* handle;
};

// The openrac_menu_fn the port calls: runs the MenuEntry that `user` points at. An exception is
// logged and closes the page, so that a broken page never keeps the player in it.
inline int menu_thunk(void* user, const openrac_menu_frame* frame) {
    auto* entry = static_cast<MenuEntry*>(user);

    // An exception must not cross into the game's frame loop.
    try {
        const MenuPage page(entry->api, entry->handle);
        return entry->callable(page, MenuFrame(*frame)) ? OPENRAC_MENU_CLOSE : OPENRAC_MENU_STAY;
    } catch (const std::exception& e) {
        report("menu_entry", e.what());
    } catch (...) {
        report("menu_entry", "an exception that is not a std::exception");
    }

    return OPENRAC_MENU_CLOSE;
}

// What a mod's page of the game's own menus does with its events. Since 1.7.
using UiCallable = std::function<void(const UiPage& page, const UiEvent& event)>;

// A page as the layer keeps it: the callable, and the table and handle its UiPage goes through.
struct UiEntry {
    UiCallable callable;
    const openrac_api* api;
    openrac_mod* handle;
};

// The openrac_ui_fn the port calls: runs the UiEntry that `user` points at. An exception is
// logged; the page stays as it was.
inline void ui_thunk(void* user, int page, const openrac_ui_event* event) {
    auto* entry = static_cast<UiEntry*>(user);

    // An exception must not cross into the game's frame loop.
    try {
        entry->callable(UiPage(entry->api, entry->handle, page), UiEvent(*event));
    } catch (const std::exception& e) {
        report("ui_page", e.what());
    } catch (...) {
        report("ui_page", "an exception that is not a std::exception");
    }
}

}  // namespace detail

// ----------------------------------------------------------------------------------------------
// The mod
// ----------------------------------------------------------------------------------------------

// The mod as the port loaded it: the table, the handle, and what the mod takes of the game. One per
// library, made by OPENRAC_MOD_CPP at load; the game's thread is the only one that uses it.
// Not copyable: the port holds pointers into the callbacks it keeps. Since 1.0.
class Mod {
public:
    // Wraps the port's table and this mod's handle, as openrac_mod_load received them.
    Mod(const openrac_api* api, openrac_mod* handle) : m_api(api), m_handle(handle) {}

    Mod(const Mod&) = delete;
    Mod& operator=(const Mod&) = delete;

    // Returns the C table, for what the layer does not wrap.
    const openrac_api& api() const { return *m_api; }

    // Returns the port's handle for this mod, the first argument of the table's own functions.
    openrac_mod* handle() const { return m_handle; }

    // Return where the mod runs: the game's id ("rac1"), the disc's version ("pal", "ntsc"), the
    // platform ("macos-arm64", ...) and OpenRAC's version. Valid for the life of the program.
    std::string_view game() const { return m_api->game; }

    std::string_view version() const { return m_api->version; }

    std::string_view platform() const { return m_api->platform; }

    std::string_view openrac_version() const { return m_api->openrac_version; }

    // Return the port's interface version: at least what the mod was built for, else it is not
    // loaded. Check api_minor() before using anything newer than the minor the mod was built for.
    unsigned api_major() const { return m_api->api_major; }

    unsigned api_minor() const { return m_api->api_minor; }

    // Writes `text` to the session's log at `level`, after the mod's id.
    void log(LogLevel level, const std::string& text) const {
        m_api->log(m_handle, static_cast<int>(level), text.c_str());
    }

    // Write `text` to the log at the level the name says.
    void debug(const std::string& text) const { log(LogLevel::Debug, text); }

    void info(const std::string& text) const { log(LogLevel::Info, text); }

    void warn(const std::string& text) const { log(LogLevel::Warn, text); }

    void error(const std::string& text) const { log(LogLevel::Error, text); }

    // Says why the mod refuses to load, for status.json and the launcher. Only during load;
    // throwing an openrac::Error from load does the same.
    void fail(const std::string& reason) const { m_api->fail(m_handle, reason.c_str()); }

    // Returns a setting of the manifest's, as T: bool for "bool"; an integer type for "int"; float
    // or double for "float"; std::string for "choice" and "string". A key the manifest does not
    // declare, or of another type, gives 0, 0.0, false or "".
    template <class T>
    T setting(const char* key) const {
        // One branch per kind of setting the manifest has; the others are compiled out, and a
        // type that is none of them does not compile.
        if constexpr (std::is_same_v<T, bool>) {
            return m_api->setting_bool(m_handle, key) != 0;
        } else if constexpr (std::is_integral_v<T>) {
            return static_cast<T>(m_api->setting_int(m_handle, key));
        } else if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(m_api->setting_float(m_handle, key));
        } else if constexpr (std::is_same_v<T, std::string>) {
            const char* text = m_api->setting_string(m_handle, key);

            return text != nullptr ? std::string(text) : std::string();
        } else {
            static_assert(
                sizeof(T) == 0, "setting<T>: bool, an integer, a floating type or std::string"
            );
        }
    }

    // Takes the hook point (a type of openrac/games/<game>.hpp) for `callable`, called as
    // callable(original, args...). Throws openrac::Error when the port has no such hook point, a
    // different signature, or the mod already took it. Only during load.
    template <class HookPoint, class F>
    void hook(F&& callable) {
        using H = detail::Hook<HookPoint>;

        // Ask by the permanent id where the type has one (the generated games/<game>.hpp do),
        // which lasts through renames of the game's functions; else by its name
        // (docs/port/MODS.md, section 4).
        const char* const asked = detail::hook_id<HookPoint>();

        // The statics of Hook hold one replacement per hook point, so a second take would
        // overwrite the first.
        if (H::callable() != nullptr) {
            throw Error(std::string(HookPoint::name) + ": this mod has already taken it");
        }

        H::callable() = new typename H::Callable(std::forward<F>(callable));

        void* original = nullptr;
        const int result = m_api->hook(
            m_handle, asked, HookPoint::signature, reinterpret_cast<void*>(&H::thunk), &original
        );

        // The port refused: drop the replacement so a later take can try again.
        if (result != OPENRAC_HOOK_OK) {
            delete H::callable();
            H::callable() = nullptr;
            throw Error(std::string(HookPoint::name) + ": " + hook_error(result));
        }

        H::original() = reinterpret_cast<typename H::Fp>(original);
        detail::taken().push_back(&H::release);
    }

    // Returns what a hook point this mod took goes on to (as the callable's `original`), for
    // calling it from elsewhere in the mod (another hook, a callback); nullptr before the hook is
    // taken.
    template <class HookPoint>
    typename HookPoint::fn original() const {
        return detail::Hook<HookPoint>::original();
    }

    // Calls `callable()` after each game tick, in load order. Throws openrac::Error when refused.
    // Only during load.
    template <class F>
    void on_frame(F&& callable) {
        // A deque keeps the stored callable where it is as more are added: the port holds its
        // address.
        auto& stored = m_frames.emplace_back(std::forward<F>(callable));

        // The port refuses a callback once load is over.
        if (m_api->on_frame(m_handle, &detail::frame_thunk, &stored) != 0) {
            throw Error("on_frame: refused (only during load)");
        }
    }

    // Calls `callable(tick)` after vertical-blank handlers. Snapshot calls are valid only here.
    // Since 1.6; only during load.
    template <class F>
    void on_tick_boundary(F&& callable) {
        if (!has_savestates()) {
            throw Error("on_tick_boundary: requires interface 1.6");
        }
        auto& stored = m_tick_boundaries.emplace_back(std::forward<F>(callable));
        if (m_api->on_tick_boundary(m_handle, &detail::tick_boundary_thunk, &stored) != 0) {
            throw Error("on_tick_boundary: refused (only during load)");
        }
    }

    // Since 1.6: returns whether the port offers tick-boundary and warm-snapshot calls.
    bool has_savestates() const {
        return m_api->api_minor >= 6
               && m_api->size >= offsetof(openrac_api, savestate_load) + sizeof(void*);
    }

    // The exact size of this build's warm, same-process snapshot. Since 1.6.
    std::size_t savestate_size() const {
        if (!has_savestates()) {
            throw Error("savestate_size: requires interface 1.6");
        }
        return m_api->savestate_size();
    }

    // Save/load only inside on_tick_boundary. Loading may be refused if the runtime allocated new
    // guest statics or strings after capture, since translated call sites cache their addresses.
    // Throws when called outside that boundary or when the port refuses the supplied blob. Since 1.6.
    void savestate_save(void* out, std::size_t size) const {
        if (!has_savestates()) {
            throw Error("savestate_save: requires interface 1.6");
        }
        if (m_api->savestate_save(out, size) != 0) {
            throw Error("savestate_save: refused");
        }
    }
    void savestate_load(const void* data, std::size_t size) const {
        if (!has_savestates()) {
            throw Error("savestate_load: requires interface 1.6");
        }
        if (m_api->savestate_load(data, size) != 0) {
            throw Error("savestate_load: refused");
        }
    }

    // Calls `callable(port, report, size)` for each pad report, before the game reads it: the
    // callable may change the report's bytes. Throws openrac::Error when refused. Only during load.
    template <class F>
    void on_pad(F&& callable) {
        // As in on_frame, the port holds the address of the stored callable.
        auto& stored = m_pads.emplace_back(std::forward<F>(callable));

        // The port refuses a callback once load is over.
        if (m_api->on_pad(m_handle, &detail::pad_thunk, &stored) != 0) {
            throw Error("on_pad: refused (only during load)");
        }
    }

    // Returns a game function by its name in this disc's build, as Fn (a function pointer type);
    // nullptr when there is none. required_function throws openrac::Error instead.
    template <class Fn>
    Fn function(const char* name) const {
        static_assert(
            std::is_pointer_v<Fn> && std::is_function_v<std::remove_pointer_t<Fn>>,
            "function<Fn>: Fn is a function pointer type"
        );

        return reinterpret_cast<Fn>(m_api->function(name));
    }

    template <class Fn>
    Fn required_function(const char* name) const {
        Fn f = function<Fn>(name);

        // The disc's build has no such function: the mod cannot run on it, so refuse the load.
        if (f == nullptr) {
            throw Error(std::string("this disc's build has no function ") + name);
        }

        return f;
    }

    // Returns the function at a code address of the level loaded now, as Fn; nullptr when there is
    // none.
    template <class Fn>
    Fn function_at(gaddr address) const {
        return reinterpret_cast<Fn>(m_api->function_at(address));
    }

    // Since 1.2: returns whether the port offers the keyboard and mouse (input(), take_input(),
    // capture_mouse()): a mod built for 1.2 is loaded only by a port that does, so a mod that
    // wants to run on 1.1 checks this first.
    bool has_input() const {
        return m_api->api_minor >= 2
               && m_api->size >= offsetof(openrac_api, capture_mouse) + sizeof(void*);
    }

    // Since 1.2: returns the keyboard and mouse for the tick about to run.
    Input input() const { return Input(m_api->input()); }

    // Since 1.2: takes the keyboard (OPENRAC_INPUT_KEYBOARD) and or the mouse from the port's own
    // use of them in play: the mod presses pad 0's buttons through on_pad. Throws openrac::Error
    // when refused. Only during load.
    void take_input(std::uint32_t devices) {
        // The port hands the devices over during load only.
        if (m_api->take_input(m_handle, devices) != 0) {
            throw Error("take_input: refused (only during load)");
        }
    }

    // Since 1.2: asks for the cursor captured (hidden, motion relative) or let go. The port may let
    // it go anyway (window not focused, overlay open). Any time.
    void capture_mouse(bool capture) const { m_api->capture_mouse(m_handle, capture ? 1 : 0); }

    // Since 1.4: returns whether the port offers the graphics context, views
    // and the Vulkan device (graphics(), on_views(), setup_vulkan()): a mod
    // that wants to run on 1.3 checks this first.
    bool has_views() const {
      return m_api->api_minor >= 4 &&
             m_api->size >=
                 offsetof(openrac_api, setup_vulkan) + sizeof(void *);
    }

    // Since 1.4: returns what the port draws with (openrac_graphics); its
    // backend is OPENRAC_GRAPHICS_NONE before the window opens.
    const openrac_graphics &graphics() const { return *m_api->graphics(); }

    // Since 1.4: has the port draw the views `source` asks for each frame
    // (openrac_view_source, sdk/docs/views.md). Throws openrac::Error when
    // refused. Only during load.
    void on_views(const openrac_view_source &source) {
      openrac_view_source sized = source;
      sized.size = sizeof sized;

      // One mod has the views, and only from its load.
      if (const int result = m_api->on_views(m_handle, &sized);
          result != OPENRAC_HOOK_OK) {
        throw Error(std::string("on_views: ") + views_error(result));
      }
    }

    // Since 1.4: has the port make its Vulkan device through `setup`
    // (openrac_vulkan_setup); the manifest says "vulkan_device": true. Throws
    // openrac::Error when refused. Only during load.
    void setup_vulkan(const openrac_vulkan_setup &setup) {
      openrac_vulkan_setup sized = setup;
      sized.size = sizeof sized;

      // One mod makes the device, and only from its load.
      if (const int result = m_api->setup_vulkan(m_handle, &sized);
          result != OPENRAC_HOOK_OK) {
        throw Error(std::string("setup_vulkan: ") + views_error(result));
      }
    }

    // Since 1.5: returns whether the port offers menu entries and pages (menu_entry()): a mod
    // that wants to run on 1.4 checks this first.
    bool has_menu() const {
        return m_api->api_minor >= 5
               && m_api->size >= offsetof(openrac_api, menu_text) + sizeof(void*);
    }

    // Since 1.5: adds an entry named `label` to the game's front-end menu that opens a page drawn
    // by `callable(page, frame)` each frame, which returns true to close it (MenuCallable).
    // Throws openrac::Error when refused. Only during load.
    template <class F>
    void menu_entry(const char* label, F&& callable) {
        // As in on_frame, the port holds the address of the stored entry.
        auto& stored = m_menus.emplace_back(
            detail::MenuEntry{detail::MenuCallable(std::forward<F>(callable)), m_api, m_handle}
        );

        // The port refuses an entry once load is over, and past its most.
        if (const int result = m_api->menu_entry(m_handle, label, &detail::menu_thunk, &stored);
            result != OPENRAC_HOOK_OK) {
            throw Error(
                result == OPENRAC_HOOK_TOO_LATE ? "menu_entry: refused (only during load)"
                                                : "menu_entry: refused (no label, or too many)"
            );
        }
    }

    // Since 1.7: returns whether the port offers pages of the game's own menus (ui_page(),
    // ui_main_item()): a mod that wants to run on 1.6 checks this first.
    bool has_ui() const {
        return m_api->api_minor >= 7 && m_api->size >= offsetof(openrac_api, ui_sound) + sizeof(void*);
    }

    // Since 1.7: makes a page of the game's own menus whose events go to `callable(page, event)`
    // (UiCallable), with `flags` (OPENRAC_UI_OWN_BACK). Throws openrac::Error when refused. Only
    // during load.
    template <class F>
    UiPage ui_page(F&& callable, std::uint32_t flags = 0) {
        if (!has_ui()) {
            throw Error("ui_page: requires interface 1.7");
        }

        // As in on_frame, the port holds the address of the stored entry.
        auto& stored = m_pages.emplace_back(
            detail::UiEntry{detail::UiCallable(std::forward<F>(callable)), m_api, m_handle}
        );
        const int id = m_api->ui_page(m_handle, flags, &detail::ui_thunk, &stored);

        // The port refuses a page once load is over, and past its most.
        if (id < 0) {
            m_pages.pop_back();
            throw Error(
                id == OPENRAC_HOOK_TOO_LATE ? "ui_page: refused (only during load)"
                                            : "ui_page: refused (too many pages)"
            );
        }

        return UiPage(m_api, m_handle, id);
    }

    // Since 1.7: adds an item named `label` to the title's main menu after its own `after`
    // (OPENRAC_UI_NEW_GAME, _LOAD_GAME, _OPTIONS), opening `page`. Throws openrac::Error when
    // refused. Only during load.
    void ui_main_item(const char* after, const char* label, const UiPage& page) {
        if (!has_ui()) {
            throw Error("ui_main_item: requires interface 1.7");
        }

        // Refused once load is over, for an unknown item or another mod's page, and past the most.
        if (const int result = m_api->ui_main_item(m_handle, after, label, page.id());
            result != OPENRAC_HOOK_OK) {
            throw Error(
                result == OPENRAC_HOOK_TOO_LATE
                    ? "ui_main_item: refused (only during load)"
                    : "ui_main_item: refused (unknown item, no label, not the mod's page, or too many)"
            );
        }
    }

    // Since 1.8: returns whether the port offers play (play_status() ... save_read()), multiplayer's
    // calls, for this game: a mod that wants to run on 1.7, or on a game without them, checks this
    // first. Without it the play calls below answer nothing (a status of OPENRAC_PLAY_NONE, no pose,
    // no figure, no save).
    bool has_play() const {
        if (m_api->api_minor < 8 || m_api->size < offsetof(openrac_api, save_read) + sizeof(void*)) {
            return false;
        }
        openrac_play_status status{};
        status.size = sizeof status;
        return m_api->play_status(&status) == OPENRAC_HOOK_OK;
    }

    // Since 1.8: what the game is doing now (openrac_play_status: its phase, level, loads).
    openrac_play_status play_status() const {
        openrac_play_status status{};
        status.size = sizeof status;
        status.level = -1;
        if (m_api->api_minor >= 8) {
            m_api->play_status(&status);
        }
        return status;
    }

    // Since 1.8: the name of the game's level `level`, empty for none.
    std::string level_name(int level) const {
        const char* name = m_api->api_minor >= 8 ? m_api->level_name(level) : nullptr;
        return name != nullptr ? name : "";
    }

    // Since 1.8: the hero's pose into `out`; false when there is no hero now.
    bool hero_pose(openrac_pose& out) const {
        out = openrac_pose{};
        out.size = sizeof out;
        return m_api->api_minor >= 8 && m_api->hero_pose(&out) == OPENRAC_HOOK_OK;
    }

    // Since 1.8: a figure that looks like the hero, in the level (figure_create): its id, or -1
    // when there is none to have (no level, no room).
    int figure_create() const {
        const int id = m_api->api_minor >= 8 ? m_api->figure_create(m_handle) : OPENRAC_HOOK_UNKNOWN;
        return id >= 0 ? id : -1;
    }

    // Since 1.8: puts figure `id` in `pose` (its size set here); false for a figure gone with its
    // level (make another).
    bool figure_pose(int id, const openrac_pose& pose) const {
        openrac_pose p = pose;
        p.size = sizeof p;
        return m_api->api_minor >= 8 && m_api->figure_pose(m_handle, id, &p) == OPENRAC_HOOK_OK;
    }

    // Since 1.8: takes figure `id` out of the level; false for one gone already.
    bool figure_remove(int id) const {
        return m_api->api_minor >= 8 && m_api->figure_remove(m_handle, id) == OPENRAC_HOOK_OK;
    }

    // Since 1.8: from the front end, a new game in `level` (no save), or `save` continued (in its
    // own level, or `level` when 0 or more), after this frame. Returns OPENRAC_HOOK_OK,
    // OPENRAC_HOOK_TAKEN (one is pending) or OPENRAC_HOOK_UNKNOWN (refused).
    int play_start(int level, const std::vector<std::uint8_t>& save = {}) const {
        if (m_api->api_minor < 8) {
            return OPENRAC_HOOK_UNKNOWN;
        }
        return m_api->play_start(
            m_handle, level, save.empty() ? nullptr : save.data(), save.size()
        );
    }

    // Since 1.8: in a level, leaves it for `level` after this frame; as play_start's results.
    int play_travel(int level) const {
        return m_api->api_minor >= 8 ? m_api->play_travel(m_handle, level) : OPENRAC_HOOK_UNKNOWN;
    }

    // Since 1.8: the game's progress now as a save, empty for none.
    std::vector<std::uint8_t> save_capture() const {
        if (m_api->api_minor < 8) {
            return {};
        }
        std::vector<std::uint8_t> bytes(m_api->save_capture(nullptr, 0));
        // The size asked first; the progress the same a moment later, on the same thread.
        if (!bytes.empty() && m_api->save_capture(bytes.data(), bytes.size()) != bytes.size()) {
            bytes.clear();
        }
        return bytes;
    }

    // Since 1.8: what the memory card's save `slot` holds, into `out`: 1 a save, 0 an empty slot,
    // -1 past the last.
    int save_slot(int slot, openrac_save_info& out) const {
        out = openrac_save_info{};
        out.size = sizeof out;
        return m_api->api_minor >= 8 ? m_api->save_slot(slot, &out) : -1;
    }

    // Since 1.8: the bytes of the memory card's save `slot`, for play_start; empty for none.
    std::vector<std::uint8_t> save_read(int slot) const {
        if (m_api->api_minor < 8) {
            return {};
        }
        std::vector<std::uint8_t> bytes(m_api->save_read(slot, nullptr, 0));
        if (!bytes.empty() && m_api->save_read(slot, bytes.data(), bytes.size()) != bytes.size()) {
            bytes.clear();
        }
        return bytes;
    }

    // Whether the port has interface 1.11's calls (data_dir, save_slot_current).
    bool has_api_11() const {
        return m_api->api_minor >= 11
               && m_api->size >= offsetof(openrac_api, save_slot_current) + sizeof(void*);
    }

    // Since 1.11: the mod's own folder for the files it keeps (beside the memory cards, made by the
    // port), empty for none.
    std::string data_dir() const {
        const char* dir = has_api_11() ? m_api->data_dir(m_handle) : nullptr;
        return dir != nullptr ? dir : "";
    }

    // Since 1.11: the memory card slot the game in progress saves into (the one it was loaded from
    // or last saved to), -1 for none.
    int save_slot_current() const { return has_api_11() ? m_api->save_slot_current() : -1; }

    // Since 1.9: whether the port offers the world calls (moby_list() ... bolts()).
    bool has_world() const {
        return m_api->api_minor >= 9
               && m_api->size >= offsetof(openrac_api, play_skip) + sizeof(void*)
               && has_play();
    }

    // Since 1.9: the level's live mobys (each's size set), all of them.
    std::vector<openrac_moby> moby_list() const {
        if (m_api->api_minor < 9) {
            return {};
        }
        std::vector<openrac_moby> out(256);
        int n = m_api->moby_list(out.data(), static_cast<int>(out.size()));
        // More than room for: again with room for all.
        if (n > static_cast<int>(out.size())) {
            out.resize(static_cast<std::size_t>(n));
            n = m_api->moby_list(out.data(), n);
        }
        out.resize(static_cast<std::size_t>(std::max(0, std::min(n, static_cast<int>(out.size())))));
        return out;
    }

    // Since 1.9: the slot of placed moby `key`, -1 for none.
    int moby_slot(int key) const { return m_api->api_minor >= 9 ? m_api->moby_slot(key) : -1; }

    // Since 1.9: drives the moby in `slot` (or lets it go); false when refused.
    bool moby_drive(int slot, bool on = true) const {
        return m_api->api_minor >= 9 && m_api->moby_drive(m_handle, slot, on ? 1 : 0) == OPENRAC_HOOK_OK;
    }

    // Since 1.9: poses, kills or makes (driven; its slot, -1 for none) a moby.
    bool moby_pose(int slot, const openrac_moby& pose) const {
        openrac_moby p = pose;
        p.size = sizeof p;
        return m_api->api_minor >= 9 && m_api->moby_pose(m_handle, slot, &p) == OPENRAC_HOOK_OK;
    }

    bool moby_kill(int slot) const {
        return m_api->api_minor >= 9 && m_api->moby_kill(m_handle, slot) == OPENRAC_HOOK_OK;
    }

    int moby_spawn(int oclass, const openrac_moby& pose) const {
        openrac_moby p = pose;
        p.size = sizeof p;
        const int slot = m_api->api_minor >= 9 ? m_api->moby_spawn(m_handle, oclass, &p) : -1;
        return slot >= 0 ? slot : -1;
    }

    // Since 1.9: the slot of figure `id` (-1: the hero's), -1 for none.
    int figure_slot(int id) const { return m_api->api_minor >= 9 ? m_api->figure_slot(m_handle, id) : -1; }

    // Since 1.9: the gate around each moby's update, `fn(slot, phase)` returning true to skip it
    // (before), and the watcher of the game's hits, `fn(hit)` returning true to take it back. A
    // plain function each (the port keeps no state for a lambda's capture): `user` is passed on.
    // Only during load; throws openrac::Error when refused.
    void on_moby_update(openrac_moby_update_fn fn, void* user = nullptr) {
        if (m_api->api_minor < 9 || m_api->on_moby_update(m_handle, fn, user) != OPENRAC_HOOK_OK) {
            throw Error("on_moby_update: refused (only during load, one mod's, interface 1.9)");
        }
    }

    void on_moby_hit(openrac_hit_fn fn, void* user = nullptr) {
        if (m_api->api_minor < 9 || m_api->on_moby_hit(m_handle, fn, user) != OPENRAC_HOOK_OK) {
            throw Error("on_moby_hit: refused (only during load, one mod's, interface 1.9)");
        }
    }

    // Since 1.9: in the gate's call before an update, the hero stands in at `position` with
    // figure `figure` as his moby (-1: his own); false outside it.
    bool hero_stand_in(const float position[3], int figure) const {
        return m_api->api_minor >= 9 && m_api->hero_stand_in(m_handle, position, figure) == OPENRAC_HOOK_OK;
    }

    // Since 1.9: posts `hit` (its size set here); false for no victim.
    bool moby_hit(const openrac_hit& hit) const {
        openrac_hit h = hit;
        h.size = sizeof h;
        return m_api->api_minor >= 9 && m_api->moby_hit(m_handle, &h) == OPENRAC_HOOK_OK;
    }

    // Since 1.9: the eyes besides the camera (x, y, z each), and the bolts after adding `add`.
    void play_eyes(const std::vector<float>& xyz) const {
        if (m_api->api_minor >= 9) {
            m_api->play_eyes(m_handle, xyz.data(), static_cast<int>(xyz.size() / 3));
        }
    }

    int bolts(int add = 0) const { return m_api->api_minor >= 9 ? m_api->bolts(add) : 0; }

    // Since 1.9: skips the scene playing as the game's own skip does; false for a game without.
    bool play_skip() const {
        return has_world() && m_api->play_skip(m_handle) == OPENRAC_HOOK_OK;
    }

    // Since 1.9: whether the port offers the held pieces and the shots (hero_held() ...
    // figure_shot()).
    bool has_held() const {
        return has_world() && m_api->size >= offsetof(openrac_api, figure_shot) + sizeof(void*);
    }

    // Since 1.9: what the hero wears and holds into `out`; false when there is no hero now.
    bool hero_held(openrac_held& out) const {
        out = openrac_held{};
        out.size = sizeof out;
        return has_held() && m_api->hero_held(&out) == OPENRAC_HOOK_OK;
    }

    // Since 1.9: has figure `id` wear and hold `held` (its size set here), each tick it is posed;
    // false for a figure gone with its level.
    bool figure_held(int id, const openrac_held& held) const {
        openrac_held h = held;
        h.size = sizeof h;
        return has_held() && m_api->figure_held(m_handle, id, &h) == OPENRAC_HOOK_OK;
    }

    // Since 1.9: the shots the hero fired since the last call, oldest first.
    std::vector<openrac_shot> hero_shots() const {
        if (!has_held()) {
            return {};
        }
        std::vector<openrac_shot> shots(OPENRAC_SHOTS_MAX);
        const int n = m_api->hero_shots(shots.data(), static_cast<int>(shots.size()));
        shots.resize(static_cast<std::size_t>(n > 0 ? n : 0));
        return shots;
    }

    // Since 1.9: has figure `id` fire `shot` (its size set here), hitting nothing in this game;
    // false for a figure gone or a weapon the port cannot fire for one.
    bool figure_shot(int id, const openrac_shot& shot) const {
        openrac_shot s = shot;
        s.size = sizeof s;
        return has_held() && m_api->figure_shot(m_handle, id, &s) == OPENRAC_HOOK_OK;
    }

    // Since 1.9: whether the port offers play_notice().
    bool has_notice() const {
        return has_world() && m_api->size >= offsetof(openrac_api, play_notice) + sizeof(void*);
    }

    // Since 1.9: a line at the top of the game's picture until replaced or cleared (""); false for
    // a port without it.
    bool play_notice(const std::string& text) const {
        return has_notice() && m_api->play_notice(m_handle, text.c_str()) == OPENRAC_HOOK_OK;
    }

    // Since 1.9: whether the port offers items of the pause menu's Options page (ui_pause_item()).
    bool has_pause_items() const {
        return has_ui() && m_api->api_minor >= 9
               && m_api->size >= offsetof(openrac_api, ui_pause_item) + sizeof(void*);
    }

    // Since 1.9: adds an item named `label` to the Options page of the pause menu in a level,
    // opening `page`. Throws openrac::Error when refused. Only during load.
    void ui_pause_item(const char* label, const UiPage& page) {
        if (!has_pause_items()) {
            throw Error("ui_pause_item: requires interface 1.9");
        }

        // Refused once load is over, with no label or another mod's page, and past the most.
        if (const int result = m_api->ui_pause_item(m_handle, label, page.id());
            result != OPENRAC_HOOK_OK) {
            throw Error(
                result == OPENRAC_HOOK_TOO_LATE
                    ? "ui_pause_item: refused (only during load)"
                    : "ui_pause_item: refused (no label, not the mod's page, or too many)"
            );
        }
    }

    // Since 1.9: whether the port offers the level's scenes (scene_playing(), play_scene()).
    bool has_scenes() const {
        return has_world() && m_api->size >= offsetof(openrac_api, play_scene) + sizeof(void*);
    }

    // Since 1.9: the in-level scene playing (its number in the level), -1 for none; `frame` (when
    // not null) gets how many of its frames have played.
    int scene_playing(int* frame = nullptr) const {
        int played = 0;
        const int scene = has_scenes() ? m_api->scene_playing(&played) : -1;
        if (frame != nullptr) {
            *frame = scene >= 0 ? played : 0;
        }
        return scene;
    }

    // Since 1.9: starts the level's scene `scene` from its start as the game's own trigger does,
    // once the player is in control (-1 drops a request still waiting); false outside a level or
    // for a port without scenes.
    bool play_scene(int scene) const {
        return has_scenes() && m_api->play_scene(m_handle, scene) == OPENRAC_HOOK_OK;
    }

    // Since 1.9: whether the port offers moby_hide(), moby_deletion() and camera_position().
    bool has_moby_detail() const {
        return has_world() && m_api->size >= offsetof(openrac_api, camera_position) + sizeof(void*);
    }

    // Since 1.9: hides the moby in `slot` from drawing (`hidden`) or shows it again; false for no
    // moby there or a port without it.
    bool moby_hide(int slot, bool hidden) const {
        return has_moby_detail()
               && m_api->moby_hide(m_handle, slot, hidden ? 1 : 0) == OPENRAC_HOOK_OK;
    }

    // Since 1.9: the last deletion the game made of the moby in `slot` in this level load into
    // `out`; false for none seen or a port without it.
    bool moby_deletion(int slot, openrac_moby_deletion& out) const {
        out = {};
        out.size = sizeof out;
        return has_moby_detail() && m_api->moby_deletion(slot, &out) == OPENRAC_HOOK_OK;
    }

    // Since 1.9: where the game's camera is into `out` (x, y, z); false outside a level or for a
    // port without it.
    bool camera_position(float (&out)[3]) const {
        return has_moby_detail() && m_api->camera_position(out) == OPENRAC_HOOK_OK;
    }

    // Since 1.9: whether the port offers moby_run().
    bool has_moby_run() const {
        return has_world() && m_api->size >= offsetof(openrac_api, moby_run) + sizeof(void*);
    }

    // Since 1.9: how the game ran the level's mobys in the last tick, and the moby in `slot` (-1:
    // none) in it, into `out`; false outside a level or for a port without it.
    bool moby_run(int slot, openrac_moby_run& out) const {
        out = {};
        out.size = sizeof out;
        return has_moby_run() && m_api->moby_run(slot, &out) == OPENRAC_HOOK_OK;
    }

    // Since 1.9: the slot of the moby at game address `moby` (a hook point's argument), -1 none.
    int moby_slot_of(gaddr moby) const { return has_world() ? m_api->moby_slot_of(moby) : -1; }

    // Since 1.9: the world keeps running under the game's menus while `on`; false for a game
    // without it.
    bool play_overlay_pause(bool on) const {
        return has_world()
               && m_api->play_overlay_pause(m_handle, on ? 1 : 0) == OPENRAC_HOOK_OK;
    }

    // Returns a view of game memory.
    Memory memory() const { return Memory(m_api->memory); }

    // Return a global's address for the code loaded now: the executable's (`relocate`) or level
    // `level`'s program's (`relocate_level`). Ask each time, because a level moves its globals.
    gaddr relocate(gaddr address) const { return m_api->relocate_data(address); }

    gaddr relocate_level(int level, gaddr address) const {
        return m_api->relocate_level_data(level, address);
    }

    // Returns the level whose program is loaded, -1 for none (the executable's).
    int loaded_level() const { return m_api->loaded_level(); }

private:
    // Returns the words for a failed openrac_api.hook, for the Error that load throws.
    static const char* hook_error(int result) {
        // Each OPENRAC_HOOK_* failure has its own words; any other code is the port's and unnamed.
        switch (result) {
            case OPENRAC_HOOK_UNKNOWN:
                return "this OpenRAC has no such hook point on this disc";
            case OPENRAC_HOOK_SIGNATURE:
                return "the hook point's signature is another";
            case OPENRAC_HOOK_TOO_LATE:
                return "hooks are taken during load only";
            default:
                return "refused";
            }
    }

    // Returns the words for a failed openrac_api.on_views or setup_vulkan.
    static const char *views_error(int result) {
      // Each failure has its own words; any other code is the port's and
      // unnamed.
      switch (result) {
      case OPENRAC_HOOK_TAKEN:
        return "another mod has it already";
      case OPENRAC_HOOK_TOO_LATE:
        return "refused (only during load)";
      case OPENRAC_HOOK_UNKNOWN:
        return "the structure is incomplete";
      default:
        return "refused";
      }
    }

    // The port's table and this mod's handle; not owned, valid for the life of the program.
    const openrac_api* m_api;
    openrac_mod* m_handle;

    // The callbacks the mod gave on_frame and on_pad, at stable addresses (a deque): the port holds
    // pointers to these.
    std::deque<detail::FrameCallable> m_frames;
    std::deque<detail::TickBoundaryCallable> m_tick_boundaries;
    std::deque<detail::PadCallable> m_pads;

    // The menu pages the mod gave menu_entry, likewise at stable addresses (since 1.5).
    std::deque<detail::MenuEntry> m_menus;

    // The pages of the game's own menus the mod made, likewise (since 1.7).
    std::deque<detail::UiEntry> m_pages;
};

// Room on the game's stack for a call that takes the address of a local, given back when it goes.
// Not copyable; frames nest and must end innermost first, which scopes do. Since 1.0.
class StackFrame {
public:
    // Pushes `size` bytes (aligned to 16) on the game's stack through `mod`'s table.
    StackFrame(const Mod& mod, std::uint32_t size) : m_api(&mod.api()) {
        m_address = m_api->stack_push(size, &m_saved);
    }

    // Pops what the constructor pushed.
    ~StackFrame() { m_api->stack_pop(m_saved); }

    StackFrame(const StackFrame&) = delete;
    StackFrame& operator=(const StackFrame&) = delete;

    // Returns the game address of the room.
    gaddr address() const { return m_address; }

private:
    // The port's table; not owned.
    const openrac_api* m_api;

    // The stack as it was before the push, for the pop.
    gaddr m_saved = 0;

    // The room's game address.
    gaddr m_address = 0;
};

// ----------------------------------------------------------------------------------------------
// Entry points
// ----------------------------------------------------------------------------------------------

namespace detail {

// Logs an exception that a hook or a callback let out, as an error under the mod's id.
inline void report(const char* where, const char* what) {
    // Before load made the mod there is nobody to log under.
    if (Mod* mod = instance()) {
        mod->error(std::string("exception in ") + where + ": " + what);
    }
}

// Runs `load_fn(mod)` for openrac_mod_load: makes the library's Mod and turns an exception out of
// `load_fn` into the mod's refusal. Returns 0 to load, 1 to refuse.
template <class Load>
int load(const openrac_api* api, openrac_mod* handle, Load&& load_fn) {
    // The port may load a library again before the system has unloaded it: let go of what the
    // earlier load took first.
    for (auto release : taken()) {
        release();
    }

    taken().clear();

    Mod* mod = nullptr;

    // A throw out of load_fn refuses the load with its message, as the interface has it.
    try {
        mod = new Mod(api, handle);
        instance() = mod;
        load_fn(*mod);

        return 0;
    } catch (const std::exception& e) {
        api->fail(handle, e.what());
    } catch (...) {
        api->fail(handle, "the mod's load threw an exception that is not a std::exception");
    }

    return 1;
}

}  // namespace detail

}  // namespace openrac OPENRAC_MOD_HIDDEN

// Writes the two entry points of a C++ mod: openrac_mod_query (the id and version, as its
// manifest's) and openrac_mod_load, which calls load_fn(openrac::Mod&). A throw out of load_fn
// refuses the load with its message. `mod_id` and `mod_version` are string literals. Since 1.0.
#define OPENRAC_MOD_CPP(mod_id, mod_version, load_fn)                                              \
    extern "C" OPENRAC_MOD_EXPORT void openrac_mod_query(openrac_mod_info* info) {                 \
        info->api_major = OPENRAC_MOD_API_MAJOR;                                                   \
        info->api_minor = OPENRAC_MOD_TARGET_MINOR;                                                \
        info->id = (mod_id);                                                                       \
        info->version = (mod_version);                                                             \
    }                                                                                              \
    extern "C" OPENRAC_MOD_EXPORT int openrac_mod_load(const openrac_api* api, openrac_mod* mod) { \
        return ::openrac::detail::load(api, mod, load_fn);                                         \
    }

#endif  // OPENRAC_MOD_HPP
