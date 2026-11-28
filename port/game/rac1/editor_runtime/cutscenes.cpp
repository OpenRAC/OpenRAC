// SPDX-License-Identifier: MIT
// Copyright (c) 2026 the OpenRAC contributors
// The cutscene player of editor-runtime: plays the .cutscene files OpenRAC Editor packs with a level
// (OpenRAC Editor's docs/CUTSCENE_FORMAT.md), through the port's mod interface only. It reads
// <pack>/level_NN/cutscenes/*.cutscene when a level loads, starts each one when its trigger
// happens, and drives the camera, the actors, the subtitles, the sounds and the events tick by tick.
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "openrac/mod.hpp"
#include "timeline.hpp"

namespace {

namespace fs = std::filesystem;
using namespace cutscene;

constexpr int kCameraEaseOut = 30;  // ticks from the last key back to the game's camera
constexpr int kMaxVariantHops = 8;  // variants that lead to variants: a loop stops here

// ---------------------------------------------------------------------------------------------
// The port

const openrac_api* g_api = nullptr;
openrac_mod* g_mod = nullptr;

// A UTF-8 path as the file system's.
fs::path utf8_path(const std::string& text) {
    return fs::path(std::u8string(text.begin(), text.end()));
}

// A path as UTF-8 text (path::string() throws on Windows for a name outside the code page).
std::string utf8_text(const fs::path& path) {
    const std::u8string u = path.u8string();
    return std::string(u.begin(), u.end());
}

void log(int level, const std::string& text) {
    g_api->log(g_mod, level, text.c_str());
}

// Whether the port has call `f` of the interface (newer ones are added at the table's end).
#define API_HAS(f)                                                                                 \
    (g_api->size >= offsetof(openrac_api, f) + sizeof(g_api->f) && g_api->f != nullptr)

// ---------------------------------------------------------------------------------------------
// The level's cutscenes

struct Cutscene {
    std::string name;      // the file's name without .cutscene: what "after" and "play" name
    std::string pack_dir;  // "level_03/cutscenes", for "pack:" paths of its sounds and meshes
    Value doc;
    int length = 0;
    // Trigger state.
    bool inside = false;   // volume: the hero was in it last tick
    bool present = false;  // pickup: the moby was there last tick
    bool flag_was =
        true;  // flag: last tick's value (true at load, so a flag set before is no start)
    bool pending = false;  // its trigger fired: it starts once the hero is free and no other plays
};

struct Level {
    int number = -1;
    std::uint32_t loads = 0xFFFFFFFFu;
    std::vector<Cutscene> scenes;
    bool started = false;  // level_start triggers ran

    Cutscene* find(const std::string& name) {
        for (Cutscene& c : scenes) {
            if (c.name == name) {
                return &c;
            }
        }

        return nullptr;
    }
};

Level g_level;

// A level moby by its spawn id (the record's uid, as the editor writes it): the port finds a moby
// by its key (the record's place), so the level's uids are mapped to keys once a load. A port
// without moby_uid takes the spawn id as the key.
std::map<int, int> g_uid_keys;
std::uint32_t g_uid_loads = ~0u;

int spawn_slot(int uid) {
    if (uid < 0) {
        return -1;
    }

    if (!API_HAS(moby_uid)) {
        return g_api->moby_slot(uid);
    }

    if (g_uid_loads != g_level.loads || g_uid_keys.empty()) {
        g_uid_loads = g_level.loads;
        g_uid_keys.clear();
        std::vector<openrac_moby> list(4096);

        for (openrac_moby& m : list) {
            m.size = sizeof(openrac_moby);
        }

        const int n = std::min(
            g_api->moby_list(list.data(), static_cast<int>(list.size())),
            static_cast<int>(list.size())
        );

        for (int i = 0; i < n; ++i) {
            if (list[i].key < 0) {
                continue;
            }

            const int id = g_api->moby_uid(list[i].slot);

            if (id >= 0) {
                g_uid_keys.emplace(id, list[i].key);
            }
        }
    }

    const auto it = g_uid_keys.find(uid);
    return it != g_uid_keys.end() ? g_api->moby_slot(it->second) : -1;
}

std::string pack_folder() {
    char buf[4096];
    return g_api->pack_folder(buf, sizeof(buf)) > 0 ? std::string(buf) : std::string();
}

void load_level(int level) {
    g_level.scenes.clear();
    g_level.number = level;
    g_level.started = false;
    const std::string pack = pack_folder();

    if (pack.empty() || level < 0) {
        return;
    }

    const std::string rel = std::format("level_{:02d}/cutscenes", level);
    const fs::path dir = utf8_path(pack) / rel;
    std::error_code e;

    if (!fs::is_directory(dir, e)) {
        return;
    }

    std::vector<fs::path> files;

    for (const auto& entry : fs::directory_iterator(dir, e)) {
        if (entry.path().extension() == ".cutscene") {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());

    for (const fs::path& file : files) {
        std::ifstream in(file, std::ios::binary);
        std::stringstream text;
        text << in.rdbuf();
        std::string error;
        Value doc = Value::parse(text.str(), error);

        if (!error.empty() || doc["format"].string() != "openrac-cutscene") {
            log(OPENRAC_LOG_WARN,
                std::format(
                    "{}: not a cutscene ({})",
                    utf8_text(file.filename()),
                    error.empty() ? "no \"format\"" : error
                ));
            continue;
        }

        if (doc["version"].integer() > 1) {
            log(OPENRAC_LOG_INFO,
                std::format(
                    "{}: version {}, playing the parts this player knows",
                    utf8_text(file.filename()),
                    doc["version"].integer()
                ));
        }

        if (doc["game"].is_string() && doc["game"].string() != g_api->game) {
            continue;
        }

        Cutscene c;
        c.name = utf8_text(file.stem());
        c.pack_dir = rel;
        c.length = std::max(1, doc["length"].tick(1));
        c.doc = std::move(doc);
        g_level.scenes.push_back(std::move(c));
    }

    if (!g_level.scenes.empty()) {
        log(OPENRAC_LOG_INFO,
            std::format("level {}: {} cutscene(s)", level, g_level.scenes.size()));
    }
}

// ---------------------------------------------------------------------------------------------
// Cutscenes played once per save, in the mod's data folder

class Seen {
public:
    bool has(const std::string& key) {
        sync();
        return m_keys.count(key) != 0;
    }

    void add(const std::string& key) {
        sync();
        m_keys.insert(key);
        save();
    }

private:
    // The save in play decides the file; a game not saved yet keeps its list in memory until it is.
    void sync() {
        const int slot = g_api->save_slot_current();

        if (slot == m_slot) {
            return;
        }

        const bool was_unsaved = m_slot == -1;
        std::set<std::string> carried = was_unsaved ? m_keys : std::set<std::string>{};
        m_slot = slot;
        m_keys.clear();

        if (slot >= 0) {
            std::ifstream in(file());

            for (std::string line; std::getline(in, line);) {
                if (!line.empty()) {
                    m_keys.insert(line);
                }
            }

            m_keys.insert(carried.begin(), carried.end());

            if (!carried.empty()) {
                save();
            }
        }
    }

    fs::path file() const {
        const char* dir = g_api->data_dir(g_mod);
        return dir == nullptr ? fs::path()
                              : utf8_path(dir) / std::format("seen-slot{}.txt", m_slot);
    }

    void save() const {
        if (m_slot < 0) {
            return;
        }

        const fs::path f = file();

        if (f.empty()) {
            return;
        }

        std::ofstream out(f, std::ios::trunc);

        for (const std::string& k : m_keys) {
            out << k << '\n';
        }
    }

    int m_slot = -2;
    std::set<std::string> m_keys;
};

Seen g_seen;

std::string seen_key(const Cutscene& c) {
    return std::format("{}/{}", g_level.number, c.name);
}

bool flag_holds(const Value& cond) {
    if (!cond.is_object()) {
        return true;
    }

    std::uint8_t state[OPENRAC_MISSION_STATE_SIZE] = {};
    const int level = cond["level"].integer(g_level.number);

    if (g_api->mission_state_read(level, state) != OPENRAC_HOOK_OK) {
        return false;
    }

    const int byte = cond["byte"].integer();

    if (byte < 0 || byte >= OPENRAC_MISSION_STATE_SIZE) {
        return false;
    }

    return state[byte] == cond["is"].integer(1);
}

// ---------------------------------------------------------------------------------------------
// The level's mobys, read once a tick while a cutscene plays

std::vector<openrac_moby> g_mobys;

const openrac_moby* moby_at(int slot) {
    for (const openrac_moby& m : g_mobys) {
        if (m.slot == slot) {
            return &m;
        }
    }

    return nullptr;
}

void read_mobys() {
    g_mobys.resize(1024);

    for (openrac_moby& m : g_mobys) {
        m.size = sizeof(openrac_moby);
    }

    int n = g_api->moby_list(g_mobys.data(), static_cast<int>(g_mobys.size()));

    if (n > static_cast<int>(g_mobys.size())) {
        g_mobys.resize(static_cast<std::size_t>(n));

        for (openrac_moby& m : g_mobys) {
            m.size = sizeof(openrac_moby);
        }

        n = g_api->moby_list(g_mobys.data(), n);
    }

    g_mobys.resize(
        static_cast<std::size_t>(std::max(0, std::min(n, static_cast<int>(g_mobys.size()))))
    );
}

bool hero_pose(openrac_pose& out) {
    out = {};
    out.size = sizeof(openrac_pose);
    return g_api->hero_pose(&out) == OPENRAC_HOOK_OK;
}

// ---------------------------------------------------------------------------------------------
// Pad: Start skips; Circle talks. Buttons are active-low in the report's first two bytes.

std::uint16_t g_pressed = 0;  // newly pressed since the last tick
std::uint16_t g_held = 0;
bool g_swallow_start = false;  // Start is the player's, not the game's pause: while a cutscene
                               // plays, and after it until the press that skipped it is let go
bool g_cutscene_on = false;
bool g_talk_ready = false;  // a talk trigger is in reach: Circle is the player's
bool g_talk_pressed = false;

void on_pad(int port, std::uint8_t* report, int size) {
    if (port != 0 || report == nullptr || size < 2) {
        return;
    }

    const std::uint16_t held = static_cast<std::uint16_t>(~(report[0] | (report[1] << 8)));
    g_pressed |= static_cast<std::uint16_t>(held & ~g_held);
    g_held = held;

    if (g_swallow_start) {
        if (held & OPENRAC_PAD_START) {
            report[0] |= static_cast<std::uint8_t>(OPENRAC_PAD_START);
        } else if (!g_cutscene_on) {
            // The press that skipped is over: Start is the game's again.
            g_swallow_start = false;
        }
    }

    if (g_talk_ready && (held & OPENRAC_PAD_CIRCLE)) {
        g_talk_pressed = true;
        report[1] |= static_cast<std::uint8_t>(OPENRAC_PAD_CIRCLE >> 8);
    }
}

// ---------------------------------------------------------------------------------------------
// Playing one cutscene

struct ActorRun {
    const Value* def = nullptr;
    std::string id;
    enum class Kind {
        None,
        Moby,
        Spawned,
        Figure,
        Prop
    } kind = Kind::None;
    int slot = -1;
    int oclass =
        -1;  // the class of the moby in `slot`: another class there, the actor's moby is gone
    int figure = -1;
    int prop = -1;
    openrac_moby original{};  // a level moby as it was, for "end": "restore"
    int anim = -1;            // the anims entry playing
    int visible = -1;         // last visibility asked
    V3 position;              // where it is this tick, for look_at and sounds
    V3 rotation;
    float scale = 1;
    float base_scale = 0;  // its own size, which a track's scale multiplies (0: not read yet)
    // look_at: the yaw it last turned to, and where it last wanted to look.
    bool looking = false;
    float look_turn = 0, look_yaw = 0;
    // held, on an actor that is not a figure: a moby of the class, kept on the joint.
    int held_slot = -1;
    int held_class = -1;
    int held_failed = -1;  // a class the level could not make: not tried again this cutscene
    bool held_hidden = false;

    // layers: each one's joints and frames, and whether any offsets were set.
    struct Layer {
        const Value* def = nullptr;
        std::vector<int> joints;
        LayerFrames frames;
    };

    std::vector<Layer> layers;
    bool joints_set = false;
    // pose: the scene's own animation, every joint per frame (pose_at), set in place of the class's.
    std::string pose;
    std::size_t pose_joints = 0;
    // face: its talks' envelopes (start tick, bytes), the blink seed, and the head's share of a look.
    std::vector<std::pair<int, std::string>> talks;
    std::uint32_t blink_seed = 0;
    float head_yaw = 0;
    int gait = -1;  // the walk's sequence playing, -1 for none
    // ride: the key in force, and where its carrier was and faced when it began.
    int ride = -1;
    V3 ride_from;
    float ride_yaw = 0;

    // feet: each leg's hip, knee and ankle, how far its ankle is moved up or down (eased toward
    // the ground under it) and its three turns; how far the body is lowered.
    struct Leg {
        int joints[3] = {0, 0, 0};
        int parent = 0;  // the hip's parent joint (its turns are in the parents' axes)
        float shift = 0;
        std::array<std::array<float, 4>, 3> turns{{{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}}};
        float settled = 0;  // the ground's height under the foot as taken, against the root
        float pending = 0;  // a new one, and for how many ticks it has held
        int pending_ticks = 0;
    };

    std::vector<Leg> legs;
    float feet_drop = 0;
    V3 feet_root;  // the lowering as joint 0's move, in its parent's frame and the model's units
    // The moby the actor is in the level (a figure's too), -1 for none.
    int moby_slot() const;
};

int ActorRun::moby_slot() const {
    switch (kind) {
        case Kind::Moby:
        case Kind::Spawned:
            return slot;
        case Kind::Figure:
            return g_api->figure_slot(g_mod, figure);
        default:
            return -1;
    }
}

struct Voice {
    std::uint32_t voice = 0;
    int start = 0;
    int fade_in = 0, fade_out = 0, end = -1;
    float volume = 1;
};

// A subtitle line written as a picture (a language the game's font cannot write): its lit runs,
// `pixel` of the game's 512 x 448 picture a pixel.
struct SubPicture {
    std::vector<BitmapRun> runs;
    int width = 0, height = 0;
    float pixel = 0.5f;
};

struct Play {
    Cutscene* scene = nullptr;
    const Value* doc = nullptr;
    int t = 0;
    bool started = false;  // the hero is held and the first tick ran
    bool skipped = false;
    int ease_out = -1;  // ticks into the camera's way back, -1 before
    std::vector<ActorRun> actors;
    std::vector<char> events_done;
    std::vector<char> sounds_done;
    std::vector<Voice> voices;
    bool muted =
        false;  // a menu is over the scene: its voices are silent (the port does not pause them)

    struct Haze {
        openrac_haze patch;
        int until;
    };

    std::vector<Haze> haze;
    std::string after;  // a cutscene an event asked to play next
    int travel = -1;
    std::set<std::string> warned;
    CameraPose last_camera;  // the last picture of the cutscene's camera, for the way back
    bool has_camera = false;
    bool hero_hidden = false;
    // Subtitles drawn as pictures, decoded once: by subtitle and language.
    std::map<std::pair<std::size_t, std::string>, std::shared_ptr<const SubPicture>> pictures;
    float last_fov = 0;             // the field of view last asked for, 0 for the game's
    bool air_set = false;           // the scene's fog or background is on
    float last_stretch = 1;         // the slow motion last asked for
    std::vector<int> ties_hidden;   // ties hidden only for the scene
    std::vector<int> mobys_hidden;  // level mobys hidden only for the scene (their slots)
    // Level mobys a hide or show event set, and how: kept so each tick, as the class's own code
    // may hide or show the moby again (an amoeboid hides itself until Ratchet comes near).
    std::map<int, bool> mobys_forced;
    std::vector<int> mobys_shown;  // level mobys hidden before, shown only for the scene
    bool hud_hidden = false;
    std::string music = "keep";  // options.music, when the port can lower it
};

std::unique_ptr<Play> g_play;
// What the overlay draws, set on the game's thread and read on the window's.
std::atomic<float> g_bars{0};          // the letterbox, 0 to 1
std::atomic<std::uint32_t> g_fade{0};  // the colour over the picture, 0xRRGGBBAA
std::mutex g_pictures_lock;
std::vector<std::shared_ptr<const SubPicture>> g_pictures;  // the subtitle pictures shown now

void show_pictures(std::vector<std::shared_ptr<const SubPicture>> pictures) {
    const std::lock_guard<std::mutex> lock(g_pictures_lock);
    g_pictures = std::move(pictures);
}

bool g_has_overlay = false;  // the port draws for mods
std::string g_language;
bool g_subtitles = true;           // the "subtitles" setting
bool g_play_scenes = true;         // the "play" setting
std::atomic<int> g_host_play{-1};  // the party host's "play" (its rules), -1 for none

// Whether cutscenes play: in a party the host decides, for every game in it.
bool scenes_on() {
    return g_host_play >= 0 ? g_host_play != 0 : g_play_scenes;
}

// Holding the world (options.world "hold"): the gate skips the update of every moby but these.
bool g_holding = false;
std::set<int> g_hold_free;
bool g_has_gate = false;              // the port gave this mod the moby update gate
std::map<std::string, int> g_sounds;  // pack path -> sound id (kept until the program ends)

void warn_once(Play& p, const std::string& what) {
    if (p.warned.insert(what).second) {
        log(OPENRAC_LOG_INFO,
            std::format("{}: {} is not played by this version", p.scene->name, what));
    }
}

ActorRun* actor(Play& p, const std::string& id) {
    for (ActorRun& a : p.actors) {
        if (a.id == id) {
            return &a;
        }
    }

    return nullptr;
}

// A point the camera or an actor looks at: [x, y, z], or { "actor": id, "offset": [x, y, z] }.
bool target_point(Play& p, const Value& v, V3& out) {
    if (v.is_array()) {
        out = vec(v);
        return true;
    }

    if (!v.is_object()) {
        return false;
    }

    const std::string id = v["actor"].string();
    ActorRun* a = actor(p, id);

    // A joint of the actor (or the hero): where the port last drew it, then the offset.
    if (v.has("joint")) {
        const int slot =
            a != nullptr ? a->moby_slot() : (id == "hero" ? g_api->figure_slot(g_mod, -1) : -1);
        float at[3];

        if (slot >= 0 && API_HAS(moby_joint_position)
            && g_api->moby_joint_position(slot, v["joint"].integer(), at) == OPENRAC_HOOK_OK) {
            out = V3{at[0], at[1], at[2]} + vec(v["offset"], {0, 0, 0});
            return true;
        }

        if (!API_HAS(moby_joint_position)) {
            warn_once(p, "a joint as a look-at target (the port cannot place joints)");
        }
    }

    // "turn": the offset turns with the actor (in its own axes, x ahead), as a camera on it would.
    const V3 offset = vec(v["offset"], {0, 0, 1});
    const bool turn = v["turn"].boolean();

    if (a != nullptr) {
        out = a->position + (turn ? turn_z(offset, a->rotation.z) : offset);
        return true;
    }

    openrac_pose hero;

    if (id == "hero" && hero_pose(hero)) {
        out = V3{hero.position[0], hero.position[1], hero.position[2]}
              + (turn ? turn_z(offset, hero.rotation[2]) : offset);
        return true;
    }

    return false;
}

// Where an actor (or the hero) faces now, its yaw.
bool actor_yaw(Play& p, const std::string& id, float& out) {
    if (const ActorRun* a = actor(p, id)) {
        out = a->rotation.z;
        return true;
    }

    openrac_pose hero;

    if (id == "hero" && hero_pose(hero)) {
        out = hero.rotation[2];
        return true;
    }

    return false;
}

std::string pick_language(const Value& by_language, const Value& languages) {
    return cutscene::pick_language(by_language, languages, g_language);
}

int sound_id(Play& p, const std::string& file) {
    const std::string path = "pack:" + p.scene->pack_dir + "/" + file;
    auto it = g_sounds.find(path);

    if (it != g_sounds.end()) {
        return it->second;
    }

    const int id = g_api->sound_load(g_mod, path.c_str());

    if (id < 0) {
        log(OPENRAC_LOG_WARN, std::format("{}: cannot load {}", p.scene->name, file));
    }

    g_sounds[path] = id;
    return id;
}

// --- Actors

// A file of the cutscene's folder, read whole; empty for a path that leaves the folder or a file
// that cannot be read.
std::string pack_file(Play& p, const std::string& rel) {
    const fs::path path = utf8_path(rel);

    if (rel.empty() || path.is_absolute() || path.has_root_name()) {
        return {};
    }

    for (const fs::path& part : path) {
        if (part == "..") {
            return {};
        }
    }

    const std::string pack = pack_folder();

    if (pack.empty()) {
        return {};
    }

    std::ifstream in(utf8_path(pack) / utf8_path(p.scene->pack_dir) / path, std::ios::binary);

    if (!in) {
        return {};
    }

    std::stringstream bytes;
    bytes << in.rdbuf();
    return bytes.str();
}

void load_layers(Play& p, ActorRun& a) {
    if (!API_HAS(moby_set_joints)) {
        warn_once(p, "actor layers (the port cannot turn a moby's joints)");
        return;
    }

    for (const Value& def : (*a.def)["layers"].items()) {
        ActorRun::Layer layer;
        layer.def = &def;

        for (const Value& j : def["joints"].items()) {
            if (j.is_number() && j.integer() >= 0 && j.integer() < 256) {
                layer.joints.push_back(j.integer());
            }
        }

        const std::string file = def["frames"].string();

        if (layer.joints.empty()
            || !layer_frames(pack_file(p, file), layer.joints.size(), layer.frames)) {
            log(OPENRAC_LOG_WARN,
                std::format(
                    "{}: actor \"{}\": layer \"{}\" has no usable frames ({})",
                    p.scene->name,
                    a.id,
                    def["name"].string(),
                    file
                ));
            continue;
        }

        a.layers.push_back(std::move(layer));
    }
}

// The actor's pose file; nothing (and a warning) if it is unusable or the port cannot set joints.
void load_pose(Play& p, ActorRun& a) {
    const Value& def = (*a.def)["pose"];

    if (!API_HAS(moby_set_joint_poses)) {
        warn_once(p, "a scene pose (the port cannot set a moby's joints)");
        return;
    }

    const long long joints = def["joints"].integer();
    std::string bytes =
        joints > 0 && joints < 256 ? pack_file(p, def["frames"].string()) : std::string();

    if (pose_frames(bytes, static_cast<std::size_t>(std::max(0LL, joints))) == 0) {
        log(OPENRAC_LOG_WARN,
            std::format(
                "{}: actor \"{}\": its pose ({}) is not usable",
                p.scene->name,
                a.id,
                def["frames"].string()
            ));
        return;
    }

    if (joints > OPENRAC_JOINT_OFFSETS_MAX) {
        warn_once(p, "a pose with more joints than the port sets (the rest keep the class's)");
    }

    a.pose = std::move(bytes);
    a.pose_joints = static_cast<std::size_t>(joints);
}

// The actor's face: its talks' envelopes decoded once, and a blink of its own.
void load_face(Play& p, ActorRun& a) {
    const Value& face = (*a.def)["face"];

    if (!API_HAS(moby_set_joints)) {
        warn_once(p, "a face (the port cannot turn a moby's joints)");
        return;
    }

    for (const Value& talk : face["talk"].items()) {
        std::string bytes;

        if (!base64_decode(talk["envelope"].string(), bytes) || bytes.empty()) {
            log(OPENRAC_LOG_WARN,
                std::format("{}: actor \"{}\": a talk with no envelope", p.scene->name, a.id));
            continue;
        }

        a.talks.emplace_back(talk["t"].tick(), std::move(bytes));
    }

    a.blink_seed = static_cast<std::uint32_t>(std::hash<std::string>{}(p.scene->name + "/" + a.id));
}

// Turns of the actor's joints, in order, each joint's composed into one.
struct JointTurns {
    std::vector<int> joints;
    std::vector<std::array<float, 4>> turns;

    void add(int joint, const float q[4]) {
        if (joint < 0 || joint > 255) {
            return;
        }

        for (std::size_t i = 0; i < joints.size(); ++i) {
            if (joints[i] == joint) {
                float r[4];
                quat_mul(turns[i].data(), q, r);
                std::copy(r, r + 4, turns[i].begin());
                return;
            }
        }

        if (joints.size() >= OPENRAC_JOINT_OFFSETS_MAX) {
            return;
        }

        joints.push_back(joint);
        turns.push_back({q[0], q[1], q[2], q[3]});
    }
};

// One turn about an axis the rig names ("axis": [x, y, z], `fallback` with none).
void turn_about(
    JointTurns& out, const Value& part, int joint, float degrees, V3 fallback = {1, 0, 0}
) {
    if (degrees == 0) {
        return;
    }

    float q[4];
    axis_angle(vec(part["axis"], fallback), degrees, q);
    out.add(joint, q);
}

// The face at t: the jaw open as loud as the talk, the lids in a blink, the moods' poses, and the
// head's share of a look.
void face_turns(const ActorRun& a, int t, JointTurns& out) {
    const Value& face = (*a.def)["face"];
    const Value& rig = face["rig"];

    if (!rig.is_object()) {
        return;
    }

    for (const auto& [mood, w] : moods_at(face["moods"], t)) {
        for (const Value& j : rig["moods"][mood].items()) {
            float q[4] =
                {static_cast<float>(j["rotation"][0].number()),
                 static_cast<float>(j["rotation"][1].number()),
                 static_cast<float>(j["rotation"][2].number()),
                 static_cast<float>(j["rotation"][3].number(1))};

            // From none to the pose by the mood's weight.
            if (q[3] < 0) {
                q[0] = -q[0], q[1] = -q[1], q[2] = -q[2], q[3] = -q[3];
            }

            float r[4] = {q[0] * w, q[1] * w, q[2] * w, 1 + (q[3] - 1) * w};
            const float l = std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2] + r[3] * r[3]);

            if (!(l > 1e-6f) || !std::isfinite(l)) {
                continue;
            }

            for (float& v : r) {
                v /= l;
            }

            out.add(j["joint"].integer(-1), r);
        }
    }

    const Value& jaw = rig["jaw"];

    if (jaw.is_object() && !a.talks.empty()) {
        float open = 0;

        for (const auto& [start, bytes] : a.talks) {
            open = std::max(open, envelope_at(bytes, start, t));
        }

        turn_about(
            out, jaw, jaw["joint"].integer(-1), static_cast<float>(jaw["open"].number(20)) * open
        );
    }

    const Value& lids = rig["lids"];

    if (lids.is_object() && face["blink"].boolean(true)) {
        const float shut = blink_at(a.blink_seed, t);

        for (const Value& j : lids["joints"].items()) {
            turn_about(
                out, lids, j.integer(-1), static_cast<float>(lids["close"].number(30)) * shut
            );
        }
    }

    const Value& head = rig["head"];

    if (head.is_object() && a.head_yaw != 0) {
        turn_about(out, head, head["joint"].integer(-1), a.head_yaw * 180.0f / kPi, {0, 0, 1});
    }
}

// The scene's pose at t on the actor: every joint's turn and place set, in place of the class's.
void pose_tick(ActorRun& a, int t) {
    const int slot = a.moby_slot();

    if (slot < 0) {
        return;
    }

    const std::size_t n = std::min<std::size_t>(a.pose_joints, OPENRAC_JOINT_OFFSETS_MAX);
    std::vector<openrac_joint_offset> joints(n);

    for (std::size_t j = 0; j < n; ++j) {
        const PoseKey k = pose_at(a.pose, a.pose_joints, j, t);
        joints[j].joint = static_cast<std::uint32_t>(j);
        offset_of(
            k.q, joints[j].rotation
        );  // the game's keys keep glTF's turn with x, y, z negated
        joints[j].translation[0] = k.t.x, joints[j].translation[1] = k.t.y,
        joints[j].translation[2] = k.t.z;
    }

    // A face on the pose (a voice line's jaw, blinks, moods): composed after the pose's own turns.
    if ((*a.def)["face"].is_object()) {
        JointTurns face;
        face_turns(a, t, face);
        std::vector<std::array<float, 4>> turns(n);

        for (std::size_t j = 0; j < n; ++j) {
            std::copy(joints[j].rotation, joints[j].rotation + 4, turns[j].begin());
        }

        face_on_pose(turns, face.joints, face.turns);

        for (std::size_t j = 0; j < n; ++j) {
            std::copy(turns[j].begin(), turns[j].end(), joints[j].rotation);
        }
    }

    a.joints_set = g_api->moby_set_joint_poses(g_mod, slot, joints.data(), static_cast<int>(n))
                   == OPENRAC_HOOK_OK;
}

// The actor's joints at t: its layers, then its face, as offsets on its moby.
// The height of the level's ground under `p`, looked for from a little above it to well below.
bool ground_below(V3 p, float& out) {
    if (!API_HAS(level_line)) {
        return false;
    }

    const float top[3] = {p.x, p.y, p.z + 0.6f};
    float to[3] = {p.x, p.y, p.z - 1.5f};
    int blocked = 0;

    if (g_api->level_line(top, to, &blocked) != OPENRAC_HOOK_OK || !blocked) {
        return false;
    }

    float hi = top[2], lo = to[2];

    for (int i = 0; i < 12; ++i) {
        to[2] = (hi + lo) / 2;
        g_api->level_line(top, to, &blocked);
        (blocked ? lo : hi) = to[2];
    }

    out = (hi + lo) / 2;
    return true;
}

// Feet on the ground: each ankle moved by the ground under it against the height the actor stands
// at (steps, slopes), the body lowered for the lower foot, the legs bent to reach. Read from the pose
// last drawn, so it follows a tick behind the animation.
void feet_tick(ActorRun& a) {
    if (a.legs.empty() || !API_HAS(moby_joint_frame)) {
        return;
    }

    const int slot = a.moby_slot();

    if (slot < 0) {
        return;
    }

    std::vector<std::array<JointFrame, 3>> frames(a.legs.size());
    std::vector<JointFrame> parents(a.legs.size());

    for (std::size_t l = 0; l < a.legs.size(); ++l) {
        float f[12];

        for (std::size_t i = 0; i < 3; ++i) {
            if (g_api->moby_joint_frame(slot, a.legs[l].joints[i], f) != OPENRAC_HOOK_OK) {
                return;
            }

            frames[l][i] = joint_frame(f);
        }

        if (g_api->moby_joint_frame(slot, a.legs[l].parent, f) != OPENRAC_HOOK_OK) {
            return;
        }

        parents[l] = joint_frame(f);
    }

    // The animation's floor is where the actor stands: its place's height.
    const float root = a.position.z;
    const float lowered = a.feet_drop;  // as last drawn
    float drop = 0;

    for (std::size_t l = 0; l < a.legs.size(); ++l) {
        ActorRun::Leg& leg = a.legs[l];
        // The ground under the ankle (the bend moves it up and down only, so where it is across
        // is the animation's), the highest of a few points round it: a foot on the edge of a step
        // stands on the step.
        const V3 ankle{frames[l][2].at.x, frames[l][2].at.y, root + 0.3f};
        float want = 0;
        bool found = false;
        constexpr float kFoot = 0.06f;
        const V3 around[5] =
            {{0, 0, 0}, {kFoot, 0, 0}, {-kFoot, 0, 0}, {0, kFoot, 0}, {0, -kFoot, 0}};

        for (const V3& o : around) {
            float under = 0;

            if (ground_below(ankle + o, under)) {
                const float w = std::clamp(under - root, -0.5f, 0.5f);
                want = found ? std::max(want, w) : w;
                found = true;
            }
        }

        // A new height counts once it holds for a few ticks, so a foot swaying over an edge does
        // not flick between the step and the ground below.
        if (std::abs(want - leg.settled) <= 0.02f) {
            leg.pending_ticks = 0;
        } else if (leg.pending_ticks > 0 && std::abs(want - leg.pending) <= 0.02f) {
            if (++leg.pending_ticks >= 8) {
                leg.settled = want, leg.pending_ticks = 0;
            }
        } else {
            leg.pending = want, leg.pending_ticks = 1;
        }

        leg.shift += (leg.settled - leg.shift) * 0.25f;
        drop = std::min(drop, leg.shift);
    }

    a.feet_drop = drop;
    // The body is lowered by its root joint (a figure stands where the game's ground puts it): the
    // drop in the root's axes, over their length (the moby's scale).
    float root_frame[12];

    if (g_api->moby_joint_frame(slot, 0, root_frame) == OPENRAC_HOOK_OK) {
        const JointFrame r = joint_frame(root_frame);
        const float size = length(r.z);

        if (size > 1e-6f) {
            const V3 down{0, 0, drop};
            a.feet_root =
                V3{dot(down, r.x), dot(down, r.y), dot(down, r.z)} * (1.0f / (size * size));
        }
    }

    for (std::size_t l = 0; l < a.legs.size(); ++l) {
        ActorRun::Leg& leg = a.legs[l];
        // The leg as animated (the turns and lowering of last tick taken off), lowered now, and
        // its ankle moved by its own shift.
        std::array<JointFrame, 3> f = leg_unturned(parents[l], frames[l], leg.turns, lowered);
        V3 target = f[2].at;
        target.z += leg.shift;

        for (JointFrame& j : f) {
            j.at.z += drop;
        }

        const LegTurns turns = leg_ik(parents[l], f[0], f[1], f[2], target);
        const float* fresh[3] = {turns.hip, turns.knee, turns.ankle};

        // Eased towards the new bend: the pose read back is the one drawn (between ticks too), so
        // taking the bend at once would chase that read's small errors into a shake.
        for (std::size_t i = 0; i < 3; ++i) {
            float* q = leg.turns[i].data();
            const float sign =
                q[0] * fresh[i][0] + q[1] * fresh[i][1] + q[2] * fresh[i][2] + q[3] * fresh[i][3]
                        < 0
                    ? -1.0f
                    : 1.0f;
            float n = 0;

            for (int k = 0; k < 4; ++k) {
                q[k] += (sign * fresh[i][k] - q[k]) * 0.4f, n += q[k] * q[k];
            }

            n = std::sqrt(n);

            for (int k = 0; k < 4; ++k) {
                q[k] = n > 1e-6f ? q[k] / n : (k == 3 ? 1.0f : 0.0f);
            }
        }
    }
}

void joints_tick(ActorRun& a, int t) {
    if (!a.pose.empty()) {
        pose_tick(a, t);
        return;
    }

    if (a.layers.empty() && a.legs.empty() && !(*a.def)["face"].is_object()) {
        return;
    }

    if (!API_HAS(moby_set_joints)) {
        return;
    }

    const int slot = a.moby_slot();

    if (slot < 0) {
        return;
    }

    JointTurns turns;

    for (const ActorRun::Layer& layer : a.layers) {
        const float w = weight_at((*layer.def)["weight"], t);

        for (std::size_t j = 0; j < layer.joints.size(); ++j) {
            float q[4];
            layer_rotation(layer.frames, j, t, w, q);
            turns.add(layer.joints[j], q);
        }
    }

    face_turns(a, t, turns);

    for (const ActorRun::Leg& leg : a.legs) {
        for (std::size_t i = 0; i < 3; ++i) {
            float q[4];
            offset_of(leg.turns[i].data(), q);
            turns.add(leg.joints[i], q);
        }
    }

    if (!a.legs.empty()) {
        const float none[4] = {0, 0, 0, 1};
        turns.add(0, none);  // the root, moved below
    }

    std::vector<openrac_joint_offset> offsets(turns.joints.size());

    for (std::size_t i = 0; i < offsets.size(); ++i) {
        offsets[i].joint = static_cast<std::uint32_t>(turns.joints[i]);
        std::copy(turns.turns[i].begin(), turns.turns[i].end(), offsets[i].rotation);

        if (turns.joints[i] == 0 && !a.legs.empty()) {
            offsets[i].translation[0] = a.feet_root.x;
            offsets[i].translation[1] = a.feet_root.y;
            offsets[i].translation[2] = a.feet_root.z;
        }
    }

    if (offsets.empty() && !a.joints_set) {
        return;
    }

    a.joints_set = g_api->moby_set_joints(
                       g_mod,
                       slot,
                       offsets.empty() ? nullptr : offsets.data(),
                       static_cast<int>(offsets.size())
                   )
                   == OPENRAC_HOOK_OK;
}

// What an actor holds at t: a figure's pieces through the port; on a moby, a moby of the class kept
// on the joint.
void held_tick(Play& p, ActorRun& a, int t) {
    const Value& list = (*a.def)["held"];

    if (!list.is_array() || list.size() == 0) {
        return;
    }

    const int at = anim_at(list, t);
    const Value* e = at >= 0 ? &list[static_cast<std::size_t>(at)] : nullptr;
    const bool none = e == nullptr || (!e->has("class") && (*e)["from"].string() != "hero");

    if (a.kind == ActorRun::Kind::Figure) {
        openrac_held held{};
        held.size = sizeof(held);

        if (!none && (*e)["from"].string() == "hero") {
            if (g_api->hero_held(&held) != OPENRAC_HOOK_OK) {
                held.count = 0;
            }
        } else if (!none) {
            held.count = 1;
            openrac_held_piece& piece = held.pieces[0];
            piece.slot = (*e)["slot"].integer(0);
            piece.item = (*e)["item"].integer(-1);
            piece.oclass = (*e)["class"].integer();
            piece.joint = (*e)["joint"].integer();
        }

        held.size = sizeof(held);
        g_api->figure_held(g_mod, a.figure, &held);
        return;
    }

    if (a.kind != ActorRun::Kind::Moby && a.kind != ActorRun::Kind::Spawned) {
        return;
    }

    if (!none && (*e)["from"].string() == "hero") {
        warn_once(p, "\"held\" from the hero on an actor that is not a figure");
        return;
    }

    int cls = none ? -1 : (*e)["class"].integer();

    if (cls == a.held_failed) {
        cls = -1;
    }

    if (cls != a.held_class && a.held_slot >= 0) {
        g_api->moby_kill(g_mod, a.held_slot);
        a.held_slot = -1;
    }

    a.held_class = cls;

    if (cls < 0) {
        return;
    }

    // At the joint as the port last drew it, else in the actor's middle.
    V3 where = a.position + V3{0, 0, 1};
    float joint_at[3];

    if (API_HAS(moby_joint_position) && a.slot >= 0
        && g_api->moby_joint_position(a.slot, (*e)["joint"].integer(), joint_at)
               == OPENRAC_HOOK_OK) {
        where = {joint_at[0], joint_at[1], joint_at[2]};
    }

    openrac_moby pose{};
    pose.size = sizeof(pose);
    std::memcpy(pose.position, &where, sizeof(float) * 3);
    std::memcpy(pose.rotation, &a.rotation, sizeof(float) * 3);

    if (a.held_slot < 0) {
        a.held_slot = g_api->moby_spawn(g_mod, cls, &pose);

        if (a.held_slot < 0) {
            warn_once(p, std::format("a held moby of class {} (the level cannot make one)", cls));
            a.held_failed = cls;
            a.held_class = -1;
            return;
        }
    }

    pose.slot = a.held_slot;
    g_api->moby_pose(g_mod, a.held_slot, &pose);
}

// Riding another actor (or the hero, or one's joint): from a "ride" key on, the actor's own place
// and turn are kept as they were against its carrier when the key began, so it goes where the
// carrier goes and turns as it turns.
void ride_tick(Play& p, ActorRun& a, int t) {
    const Value& rides = (*a.def)["ride"];

    if (!rides.is_array()) {
        return;
    }

    const int i = ride_at(rides, t);

    if (i < 0) {
        a.ride = -1;
        return;
    }

    const Value& key = rides[static_cast<std::size_t>(i)];
    const std::string carrier_id = key["actor"].string();

    if (carrier_id == a.id) {
        return;
    }

    ActorRun* carrier = actor(p, carrier_id);
    V3 at;
    float yaw = 0;

    if (carrier != nullptr) {
        yaw = carrier->rotation.z;
    } else if (openrac_pose hero; carrier_id == "hero" && hero_pose(hero)) {
        yaw = hero.rotation[2];
    }

    if (!target_point(p, key, at)) {
        warn_once(p, std::format("a ride on \"{}\", who is not there", carrier_id));
        return;
    }

    if (a.ride != i) {
        a.ride = i;
        a.ride_from = at;
        a.ride_yaw = yaw;
    }

    const float turned = yaw - a.ride_yaw;
    a.position = at + turn_z(a.position - a.ride_from, turned);
    a.rotation.z += turned;
}

// Turns the actor's body towards what its look_at keys say, by their weight.
void look_tick(Play& p, ActorRun& a, int t) {
    const Value& keys = (*a.def)["look_at"];

    if (!keys.is_array() || keys.size() == 0) {
        return;
    }

    const Look look = look_at(keys, t);

    if (look.key == -1) {
        a.looking = false;
        a.head_yaw = 0;
        return;
    }

    if (look.key >= 0) {
        V3 target;

        if (target_point(p, keys[static_cast<std::size_t>(look.key)]["target"], target)) {
            a.look_yaw = yaw_to(a.position, target, a.look_yaw);
        }
    }

    const float goal = angle_lerp(a.rotation.z, a.look_yaw, look.weight);
    // A new target turns the body round over a few ticks, not at once.
    a.look_turn = a.looking ? turn_toward(a.look_turn, goal, 0.2f) : goal;
    a.looking = true;
    // With a head in the face's rig, the head turns first, up to its limit, and the body the rest.
    const Value& head = (*a.def)["face"]["rig"]["head"];

    if (head.is_object()) {
        float body = 0;
        head_share(
            a.look_turn - a.rotation.z,
            static_cast<float>(head["limit"].number(60)) * kPi / 180.0f,
            a.head_yaw,
            body
        );
        a.rotation.z += body;
    } else {
        a.rotation.z = a.look_turn;
    }
}

void actors_begin(Play& p) {
    const Value& list = (*p.doc)["actors"];

    for (const Value& def : list.items()) {
        ActorRun a;
        a.def = &def;
        a.id = def["id"].string();
        const Value& src = def["source"];
        // A first pose for actors made here: the track's first key.
        openrac_moby first{};
        first.size = sizeof(openrac_moby);
        const Value& key0 = def["track"][0];
        const V3 pos = vec(key0["position"]), rot = vec(key0["rotation"]);
        std::memcpy(first.position, &pos, sizeof(float) * 3);
        std::memcpy(first.rotation, &rot, sizeof(float) * 3);
        first.scale =
            0;  // the class's own size; the track's scale multiplies it from the first tick

        if (src.has("spawn_id")) {
            a.slot = spawn_slot(src["spawn_id"].integer());
            const bool taken =
                std::any_of(p.actors.begin(), p.actors.end(), [&](const ActorRun& o) {
                    return o.kind == ActorRun::Kind::Moby && o.slot == a.slot;
                });

            if (a.slot >= 0 && taken) {
                // Two actors on one level moby: the first has it.
                log(
                    OPENRAC_LOG_WARN,
                    std::format("{}: actor \"{}\" is a moby another actor has", p.scene->name, a.id)
                );
                a.slot = -1;
            } else if (a.slot >= 0) {
                if (const openrac_moby* m = moby_at(a.slot)) {
                    a.original = *m;
                }

                a.kind = g_api->moby_drive(g_mod, a.slot, 1) == OPENRAC_HOOK_OK
                             ? ActorRun::Kind::Moby
                             : ActorRun::Kind::None;
            } else if (def["missing"].string_or("spawn") == "spawn" && src.has("class")) {
                a.slot = g_api->moby_spawn(g_mod, src["class"].integer(), &first);
                a.kind = a.slot >= 0 ? ActorRun::Kind::Spawned : ActorRun::Kind::None;
            }
        } else if (src.has("class")) {
            a.slot = g_api->moby_spawn(g_mod, src["class"].integer(), &first);
            a.kind = a.slot >= 0 ? ActorRun::Kind::Spawned : ActorRun::Kind::None;
        } else if (src.has("figure")) {
            a.figure = g_api->figure_create(g_mod);
            a.kind = a.figure >= 0 ? ActorRun::Kind::Figure : ActorRun::Kind::None;
        } else if (src.has("prop")) {
            const std::string path = "pack:" + p.scene->pack_dir + "/" + src["prop"].string();
            a.prop = g_api->prop_create(g_mod, path.c_str());
            a.kind = a.prop >= 0 ? ActorRun::Kind::Prop : ActorRun::Kind::None;
        }

        if (a.kind == ActorRun::Kind::None) {
            log(OPENRAC_LOG_WARN,
                std::format("{}: actor \"{}\" is missing, skipped", p.scene->name, a.id));
        }

        const bool jointed = a.kind != ActorRun::Kind::None && a.kind != ActorRun::Kind::Prop;

        // A scene pose moves every joint: layers and feet on it would be undone each tick. A face is
        // turned on top of it (pose_tick), so a new voice line moves the jaw of a game scene's actor.
        if (def.has("pose") && jointed) {
            load_pose(p, a);
        }

        if (!a.pose.empty() && (def.has("layers") || def.has("feet"))) {
            warn_once(p, "layers or feet on an actor with a pose (the pose has every joint)");
        }

        if (def.has("layers") && jointed && a.pose.empty()) {
            load_layers(p, a);
        }

        for (const Value& leg : def["feet"]["legs"].items()) {
            if ((leg.size() != 3 && leg.size() != 4) || !jointed || !a.pose.empty()) {
                continue;
            }

            ActorRun::Leg l;

            for (std::size_t i = 0; i < 3; ++i) {
                l.joints[i] = static_cast<int>(leg[i].integer());
            }

            l.parent = leg.size() == 4 ? static_cast<int>(leg[3].integer()) : 0;
            a.legs.push_back(l);
        }

        if (def.has("face") && jointed) {
            load_face(p, a);
        }

        p.actors.push_back(std::move(a));
    }

    // The new mobys' own size (their class's), for actor_tick's scale 1, and their class.
    read_mobys();

    for (ActorRun& a : p.actors) {
        if (a.kind != ActorRun::Kind::Moby && a.kind != ActorRun::Kind::Spawned) {
            continue;
        }

        const openrac_moby* m = moby_at(a.slot);
        a.oclass = m != nullptr ? m->oclass : -1;
    }
}

// Whether the moby in `slot` is still the one of class `oclass` (the game may kill one, and give its
// slot to another).
bool still_there(int slot, int oclass) {
    const openrac_moby* m = moby_at(slot);
    return m != nullptr && (oclass < 0 || m->oclass == oclass);
}

void actor_tick(Play& p, ActorRun& a, int t) {
    if (a.kind == ActorRun::Kind::None) {
        return;
    }

    if ((a.kind == ActorRun::Kind::Moby || a.kind == ActorRun::Kind::Spawned)
        && !still_there(a.slot, a.oclass)) {
        log(OPENRAC_LOG_WARN,
            std::format("{}: actor \"{}\" was taken out of the level", p.scene->name, a.id));
        a.kind = ActorRun::Kind::None;
        a.slot = -1;
        return;
    }

    if (a.held_slot >= 0 && !still_there(a.held_slot, a.held_class)) {
        a.held_slot = -1;
    }

    const Value& def = *a.def;
    track_at(def["track"], t, a.position, a.rotation, a.scale);
    path_at(def["path"], t, a.position, a.rotation);
    ride_tick(p, a, t);

    if (!a.pose.empty()) {
        a.rotation = {};  // the pose's root joint has the actor's turn
    }

    if (!a.looking) {
        a.look_yaw = a.rotation.z;
    }

    if (a.pose.empty()) {
        look_tick(p, a, t);
    }

    feet_tick(a);

    const int visible = visible_at(def["visible"], t) ? 1 : 0;

    if (a.kind == ActorRun::Kind::Prop) {
        openrac_prop_pose pose{};
        pose.size = sizeof(pose);
        std::memcpy(pose.position, &a.position, sizeof(float) * 3);
        quaternion(a.rotation, pose.rotation);
        pose.scale = a.scale;
        g_api->prop_pose(g_mod, a.prop, &pose);
        // A skinned .glb: its anims entry's animation (seq: its index in the file), from the
        // entry's start at its speed, looped unless the entry says not.
        const int anim = anim_at(def["anims"], t);

        if (anim >= 0 && API_HAS(prop_animate)) {
            const Value& e = def["anims"].items()[static_cast<std::size_t>(anim)];
            const double seconds =
                (t - e["t"].tick()) / 60.0 * e["speed"].number(1) + e["frame"].tick() / 30.0;
            g_api->prop_animate(
                g_mod,
                a.prop,
                static_cast<int>(e["seq"].integer()),
                static_cast<float>(seconds),
                e["loop"].boolean(true) ? 1 : 0
            );
        } else if (anim >= 0) {
            warn_once(p, "an animated prop (needs a newer port)");
        } else if (a.anim >= 0 && API_HAS(prop_animate)) {
            g_api->prop_animate(g_mod, a.prop, -1, 0.0f, 0);
        }

        a.anim = anim;

        if (visible != a.visible) {
            g_api->prop_hide(g_mod, a.prop, !visible);
        }

        a.visible = visible;
        return;
    }

    // Animation: a new anims entry starts its sequence at its frame; after that the game plays it
    // on by itself, so the pose hands back what the moby has (no pull back to a key).
    const auto& anims = def["anims"].items();
    int anim = anim_at(def["anims"], t);
    // A walk with gaits plays them (walking, then standing), unless an anims entry started after
    // the walk did.
    const Value& path = def["path"];
    int gait = -1;

    if (path["gaits"].is_array()
        && (anim < 0 || anims[static_cast<std::size_t>(anim)]["t"].tick() < path["t"].tick())) {
        gait = gait_at(path["gaits"], walk_state(path, t), path["speed"].number(8));
    }

    const int slot =
        a.kind == ActorRun::Kind::Figure ? g_api->figure_slot(g_mod, a.figure) : a.slot;
    const openrac_moby* now = moby_at(slot);

    if (a.base_scale <= 0) {
        // Scale 1 is the actor's own size: a level moby's, a new moby's, the hero's for a figure.
        openrac_pose hero;

        if (a.kind == ActorRun::Kind::Moby && a.original.scale > 0) {
            a.base_scale = a.original.scale;
        } else if (a.kind == ActorRun::Kind::Figure && hero_pose(hero) && hero.scale > 0) {
            a.base_scale = hero.scale;
        } else if (now != nullptr && now->scale > 0) {
            a.base_scale = now->scale;
        }
    }

    // Size unknown yet (0): the moby keeps the one it has.
    const float size = a.base_scale > 0 ? a.scale * a.base_scale : 0.0f;
    std::uint32_t seq = now ? now->seq : 0, frame = now ? now->frame : 0;
    std::uint32_t next = now ? now->next_frame : 0;
    float anim_time = now ? now->anim_time : 0, speed = now ? now->anim_speed : 1;

    if (!a.pose.empty()) {
        // The class's own animation plays on under the pose, which sets every joint (the port
        // evaluates a moby's joints only once it has a frame of its own).
        speed = 1;
        gait = -1;
    } else if (gait >= 0) {
        speed = 1;

        if (gait != a.gait) {
            seq = static_cast<std::uint32_t>(gait), frame = 0, next = 1, anim_time = 0;
        }

        anim = a.anim;  // kept: an anims entry after the walk still starts as new
    } else if (anim >= 0) {
        const Value& e = anims[static_cast<std::size_t>(anim)];
        speed = static_cast<float>(e["speed"].number(1));

        if (anim != a.anim) {
            seq = static_cast<std::uint32_t>(e["seq"].integer());
            frame = static_cast<std::uint32_t>(e["frame"].tick());
            next = frame + 1;
            anim_time = 0;

            if (e.has("blend") && e["blend"].tick() != 4) {
                warn_once(p, "a blend other than 4 ticks");
            }
        }
    }

    a.anim = anim;
    a.gait = gait;
    const V3 body = a.position;

    if (a.kind == ActorRun::Kind::Figure) {
        openrac_pose pose{};
        pose.size = sizeof(pose);
        pose.level = g_level.number;
        std::memcpy(pose.position, &body, sizeof(float) * 3);
        std::memcpy(pose.rotation, &a.rotation, sizeof(float) * 3);
        pose.scale = size;
        pose.item = -1;
        pose.seq = seq, pose.frame = frame, pose.next_frame = next;
        pose.anim_time = anim_time, pose.anim_speed = speed;
        pose.flags = OPENRAC_POSE_ON_GROUND;
        g_api->figure_pose(g_mod, a.figure, &pose);
    } else {
        openrac_moby pose{};
        pose.size = sizeof(pose);
        pose.slot = a.slot;
        std::memcpy(pose.position, &body, sizeof(float) * 3);
        std::memcpy(pose.rotation, &a.rotation, sizeof(float) * 3);
        pose.scale = size;
        pose.seq = seq, pose.frame = frame, pose.next_frame = next;
        pose.anim_time = anim_time, pose.anim_speed = speed;
        g_api->moby_pose(g_mod, a.slot, &pose);
    }

    if (visible != a.visible && slot >= 0) {
        g_api->moby_hide(g_mod, slot, !visible);
    }

    a.visible = visible;
    held_tick(p, a, t);
    joints_tick(a, t);

    if (a.held_slot >= 0 && a.held_hidden != !visible) {
        g_api->moby_hide(g_mod, a.held_slot, !visible);
        a.held_hidden = !visible;
    }
}

void actors_end(Play& p) {
    for (ActorRun& a : p.actors) {
        const std::string end = (*a.def)["end"].string_or("keep");

        // A skip puts every actor where the cutscene leaves it.
        if (p.skipped) {
            actor_tick(p, a, p.scene->length);
        }

        if (a.joints_set) {
            g_api->moby_set_joints(g_mod, a.moby_slot(), nullptr, 0);
        }

        if (a.held_slot >= 0) {
            if (end == "keep") {
                g_api->moby_drive(g_mod, a.held_slot, 0);
            } else {
                g_api->moby_kill(g_mod, a.held_slot);
            }
        }

        switch (a.kind) {
            case ActorRun::Kind::Moby:
                if (end == "remove") {
                    g_api->moby_kill(g_mod, a.slot);
                    break;
                }

                if (end == "restore") {
                    openrac_moby back = a.original;
                    back.size = sizeof(back);
                    g_api->moby_pose(g_mod, a.slot, &back);

                    if (!(a.original.flags & OPENRAC_MOBY_HIDDEN)) {
                        g_api->moby_hide(g_mod, a.slot, 0);
                    }
                }

                g_api->moby_drive(g_mod, a.slot, 0);
                break;
            case ActorRun::Kind::Spawned:
                // A moby made for the cutscene has no update of the game's to go back to: it stays
                // as a still pose ("keep") or goes.
                if (end != "keep") {
                    g_api->moby_kill(g_mod, a.slot);
                }

                break;
            case ActorRun::Kind::Figure:
                if (end != "keep") {
                    g_api->figure_remove(g_mod, a.figure);
                }

                break;
            case ActorRun::Kind::Prop:
                if (end != "keep") {
                    g_api->prop_remove(g_mod, a.prop);
                }

                break;
            case ActorRun::Kind::None:
                break;
        }
    }
}

// --- Camera

std::vector<CamKey> camera_keys(Play& p) {
    const auto& all = (*p.doc)["camera"]["keys"].items();
    std::vector<CamKey> keys;

    for (std::size_t i : by_time(all)) {
        const Value& k = all[i];
        V3 target, eye;
        const bool has_target = target_point(p, k["look_at"], target);
        // An eye on an actor ({"actor", "joint", "offset", "turn"}) goes with it.
        const bool on_actor = k["eye"].is_object() && target_point(p, k["eye"], eye);
        CamKey c = camera_key(
            k,
            keys.empty() ? nullptr : &keys.back(),
            has_target ? &target : nullptr,
            on_actor ? &eye : nullptr
        );
        // "turn": the key's turn is the actor's own, so the camera turns as the actor does.
        float yaw = 0;

        if (on_actor && k["eye"]["turn"].boolean() && !has_target
            && actor_yaw(p, k["eye"]["actor"].string(), yaw)) {
            if (k.has("rotation")) {
                c.forward = turn_z(c.forward, yaw);
            }

            c.in = turn_z(c.in, yaw);
            c.out = turn_z(c.out, yaw);
        }

        keys.push_back(c);
    }

    return keys;
}

openrac_camera make_camera(V3 eye, V3 forward, float roll) {
    const CameraAxes axes = camera_axes(forward, roll);
    openrac_camera cam{};
    cam.size = sizeof(cam);
    std::memcpy(cam.eye, &eye, sizeof(float) * 3);
    std::memcpy(cam.forward, &axes.forward, sizeof(float) * 3);
    std::memcpy(cam.left, &axes.left, sizeof(float) * 3);
    std::memcpy(cam.up, &axes.up, sizeof(float) * 3);
    return cam;
}

bool camera_at(Play& p, int t, CameraPose& out) {
    if (!camera_pose(camera_keys(p), t, out)) {
        return false;
    }

    out.eye = out.eye + shake_at((*p.doc)["camera"]["shake"], t);
    return true;
}

// The camera's field of view, once the camera is the cutscene's (0 gives the game's back).
void fov_set(Play& p, float fov) {
    if (fov == p.last_fov) {
        return;
    }

    if (!API_HAS(camera_fov)) {
        if (fov > 0) {
            warn_once(p, "a camera field of view (the port cannot set one)");
        }

        return;
    }

    if (g_api->camera_fov(g_mod, fov) == OPENRAC_HOOK_OK) {
        p.last_fov = fov;
    }
}

// The game's music, lowered as options.music says, `k` of the way in.
void music_set(Play& p, int t, float k) {
    if (p.music == "keep") {
        return;
    }

    g_api->music_volume(g_mod, music_gain(p.music, t, k));
}

// --- Over the picture

// The letterbox and the fade at t, `k` of the way in (less on the way back to the game).
void overlay_tick(Play& p, int t, float k) {
    if (!g_has_overlay) {
        return;
    }

    g_bars = (*p.doc)["options"]["letterbox"].boolean(true) ? letterbox_at(t) * k : 0.0f;
    Rgba fade = fade_at((*p.doc)["events"], t);
    fade.a *= k;
    g_fade = pack_rgba(fade);
}

void draw_overlay(void*, const openrac_overlay* o) {
    constexpr float kWidth = 512, kHeight = 448, kBar = kHeight * 0.12f;
    const float bars = g_bars.load();

    if (bars > 0) {
        o->rect(o->context, 0, 0, kWidth, kBar * bars, 0x000000FFu);
        o->rect(o->context, 0, kHeight - kBar * bars, kWidth, kBar * bars, 0x000000FFu);
    }

    // Subtitle pictures, one above the other, centred where the HUD's lines go, with a shadow.
    {
        const std::lock_guard<std::mutex> lock(g_pictures_lock);
        float total = 0;

        for (const auto& pic : g_pictures) {
            total += pic->height * pic->pixel;
        }

        float y = kHeight * 0.80f - total / 2;

        for (const auto& pic : g_pictures) {
            const float x = (kWidth - pic->width * pic->pixel) / 2;

            for (const std::uint32_t colour : {0x000000C0u, 0xF0F0F0FFu}) {
                const float shift = colour == 0xF0F0F0FFu ? 0.0f : 1.0f;

                for (const BitmapRun& r : pic->runs) {
                    o->rect(
                        o->context,
                        x + r.x * pic->pixel + shift,
                        y + r.y * pic->pixel + shift,
                        r.length * pic->pixel,
                        pic->pixel,
                        colour
                    );
                }
            }

            y += pic->height * pic->pixel;
        }
    }
    const std::uint32_t fade = g_fade.load();

    if ((fade & 0xFFu) != 0) {
        o->rect(o->context, 0, 0, kWidth, kHeight, fade);
    }
}

// --- Subtitles

void subtitles_tick(Play& p, int t) {
    std::vector<openrac_hud_text> lines;
    std::vector<std::shared_ptr<const SubPicture>> pictures;
    // Subtitles off: none (and the lines already up go).
    static const std::vector<Value> kNone;
    const auto& list = g_subtitles ? (*p.doc)["subtitles"].items() : kNone;

    for (std::size_t i = 0; i < list.size(); ++i) {
        const Value& s = list[i];

        if (t < s["t"].tick() || t >= s["end"].tick(s["t"].tick() + 120)) {
            continue;
        }

        const Value& text = s["text"];
        const std::string lang =
            text.is_string() ? std::string() : pick_language(text, (*p.doc)["languages"]);
        // A line the editor wrote as a picture (Japanese, and any the game's font cannot write).
        const Value& bitmap = s["bitmap"][lang];

        if (!lang.empty() && bitmap.is_object() && g_has_overlay) {
            auto& pic = p.pictures[{i, lang}];

            if (!pic) {
                auto made = std::make_shared<SubPicture>();

                if (bitmap_runs(bitmap, made->runs)) {
                    made->width = bitmap["width"].integer();
                    made->height = bitmap["height"].integer();
                    made->pixel =
                        static_cast<float>(std::clamp(bitmap["pixel"].number(0.5), 0.1, 4.0));
                } else {
                    log(OPENRAC_LOG_WARN,
                        std::format(
                            "{}: subtitle {} has a broken {} picture", p.scene->name, i, lang
                        ));
                }

                pic = std::move(made);
            }

            if (!pic->runs.empty() && pictures.size() < 4) {
                pictures.push_back(pic);
            }

            continue;
        }

        if (lang == "ja") {
            warn_once(p, "Japanese subtitles without a picture (the HUD font has only ASCII)");
            continue;
        }

        // Lines of up to 40 letters, broken at spaces, at most three.
        const std::vector<std::string> rows =
            wrap(ascii(text.is_string() ? text.string() : text[lang].string()));

        for (std::size_t r = 0; r < rows.size(); ++r) {
            openrac_hud_text line{};
            std::snprintf(line.text, sizeof(line.text), "%s", rows[r].c_str());
            line.x = 0.5f;
            line.y = 0.80f + 0.055f * static_cast<float>(r)
                     - 0.0275f * static_cast<float>(rows.size() - 1);
            line.scale = 0.75f;
            line.rgba[0] = line.rgba[1] = line.rgba[2] =
                190;  // white, as the picture lines (the font is grey at 128)
            line.rgba[3] = 255;
            line.flags = OPENRAC_HUD_SHADOW;
            lines.push_back(line);
        }
    }

    if (lines.size() > 8) {
        lines.resize(8);
    }

    g_api->hud_text_set(
        g_mod, lines.empty() ? nullptr : lines.data(), static_cast<int>(lines.size())
    );
    show_pictures(std::move(pictures));
}

// --- Sounds

void sounds_tick(Play& p, int t) {
    const auto& list = (*p.doc)["sounds"].items();

    for (std::size_t i = 0; i < list.size(); ++i) {
        const Value& s = list[i];

        if (p.sounds_done[i] || t < s["t"].tick()) {
            continue;
        }

        p.sounds_done[i] = 1;

        if (p.skipped) {
            continue;
        }

        const float volume = std::clamp(static_cast<float>(s["volume"].number(1)), 0.0f, 4.0f);

        if (s.has("game_sound")) {
            const std::string who = s["actor"].string_or("hero");
            ActorRun* a = who == "hero" ? nullptr : actor(p, who);
            const int slot = a != nullptr ? a->moby_slot() : -1;

            if (who == "hero" && actor(p, "hero") == nullptr) {
                g_api->hero_sound_play(g_mod, s["game_sound"].integer(), 0);
            } else if (slot >= 0 && API_HAS(moby_sound_play)) {
                g_api->moby_sound_play(g_mod, slot, s["game_sound"].integer(), 0);
            } else {
                warn_once(p, "a game sound on an actor the port cannot play one on");
            }

            continue;
        }

        const Value& file = s["file"];
        std::string path = file.is_string() ? file.string() : std::string();

        if (file.is_object()) {
            const std::string lang = pick_language(file, (*p.doc)["languages"]);
            path = file[lang].string();
        }

        if (path.empty()) {
            continue;
        }

        const int id = sound_id(p, path);

        if (id < 0) {
            continue;
        }

        Voice v;
        v.fade_in = s["fade_in"].tick();
        v.fade_out = s["fade_out"].tick();
        v.start = t;
        v.end =
            s.has("end") ? s["end"].tick() : (s["kind"].string() == "music" ? p.scene->length : -1);
        v.volume = volume;
        v.voice = g_api->sound_play(g_mod, id, v.fade_in > 0 ? 0.0f : volume, 1.0f, 0);

        if (v.voice != 0) {
            p.voices.push_back(v);
        }
    }

    // Fades.
    for (Voice& v : p.voices) {
        if (v.end >= 0 && t >= v.end) {
            g_api->sound_stop(g_mod, v.voice);
            v.voice = 0;
            continue;
        }

        if (v.fade_in > 0 || v.fade_out > 0 || p.muted) {
            g_api->sound_set(
                g_mod, v.voice, v.volume * fade_gain(v.start, v.fade_in, v.fade_out, v.end, t), 1.0f
            );
        }
    }

    std::erase_if(p.voices, [](const Voice& v) { return v.voice == 0; });
    p.muted = false;
}

// --- Air and slow motion

// The scene's fog and background, eased by `k` (1 in the scene, to 0 on the way out).
void air_tick(Play& p, int t, float k) {
    if (!API_HAS(scene_air)) {
        return;
    }

    const Air a = air_at((*p.doc)["events"], t);

    if (a.weight * k <= 0 || (!a.fog && !a.background)) {
        if (p.air_set) {
            g_api->scene_air(g_mod, nullptr);
        }

        p.air_set = false;
        return;
    }

    openrac_air air{};
    air.size = sizeof(air);
    air.flags = (a.fog ? OPENRAC_AIR_FOG : 0u) | (a.background ? OPENRAC_AIR_BACKGROUND : 0u);
    std::memcpy(air.fog_colour, a.fog_colour, sizeof(air.fog_colour));
    air.fog_near = a.fog_near;
    air.fog_far = a.fog_far;
    air.fog_thickness = a.fog_thickness;
    std::memcpy(air.background, a.back, sizeof(air.background));
    air.weight = a.weight * k;
    p.air_set = g_api->scene_air(g_mod, &air) == OPENRAC_HOOK_OK;
}

void stretch_set(Play& p, float stretch) {
    if (stretch == p.last_stretch || !API_HAS(time_stretch)) {
        return;
    }

    if (g_api->time_stretch(g_mod, stretch) == OPENRAC_HOOK_OK) {
        p.last_stretch = stretch;
    }
}

void air_check(Play& p) {
    for (const Value& e : (*p.doc)["events"].items()) {
        const std::string what = e["do"].string();

        if (what == "air" && !API_HAS(scene_air)) {
            warn_once(p, "fog or background (needs a newer port)");
        }

        if (what == "slow" && !API_HAS(time_stretch)) {
            warn_once(p, "slow motion (needs a newer port)");
        }

        if (what == "tie_hide" && !API_HAS(tie_hide)) {
            warn_once(p, "hiding ties (needs a newer port)");
        }
    }
}

// --- Events

void event_run(Play& p, const Value& e) {
    const std::string what = e["do"].string();
    const int level = e["level"].integer(g_level.number);

    if (what == "moby_hide" || what == "moby_kill") {
        const int slot = spawn_slot(e["spawn_id"].integer());

        if (slot < 0) {
            return;
        }

        if (what == "moby_kill") {
            g_api->moby_kill(g_mod, slot);
            p.mobys_forced.erase(slot);
            std::erase(p.mobys_shown, slot);
            std::erase(p.mobys_hidden, slot);
        } else {
            const bool on = e["on"].boolean(true);
            const openrac_moby* before = moby_at(slot);
            const bool was_hidden = before != nullptr && (before->flags & OPENRAC_MOBY_HIDDEN) != 0;

            if (g_api->moby_hide(g_mod, slot, on ? 1 : 0) != OPENRAC_HOOK_OK) {
                return;
            }

            p.mobys_forced[slot] = on;
            // Not persistent: as it was when the scene ends.
            const bool restore = !e["persistent"].boolean();

            if (!std::count(p.mobys_hidden.begin(), p.mobys_hidden.end(), slot)
                && !std::count(p.mobys_shown.begin(), p.mobys_shown.end(), slot) && restore) {
                if (on && !was_hidden) {
                    p.mobys_hidden.push_back(slot);
                }

                if (!on && was_hidden) {
                    p.mobys_shown.push_back(slot);
                }
            } else if (!restore) {
                std::erase(p.mobys_hidden, slot);
                std::erase(p.mobys_shown, slot);
            }
        }
    } else if (what == "mission_flag") {
        std::uint8_t flags[OPENRAC_MISSION_STATE_SIZE] = {};
        const int byte = e["byte"].integer();

        if (byte < 0 || byte >= OPENRAC_MISSION_STATE_SIZE) {
            return;
        }

        const int value = e["value"].integer(1);

        if (value == 0) {
            flags[byte] = 1;
            g_api->mission_state_clear(level, flags);
        } else {
            g_api->mission_state_read(level, flags);
            flags[byte] = static_cast<std::uint8_t>(value);
            g_api->mission_state_write(level, flags);
        }
    } else if (what == "haze") {
        Play::Haze h{};
        const V3 pos = vec(e["position"]);
        std::memcpy(h.patch.position, &pos, sizeof(float) * 3);
        h.patch.radius = static_cast<float>(e["radius"].number(4));
        h.patch.strength = static_cast<float>(e["strength"].number(0.1));
        h.until = e["t"].tick() + e["length"].tick(60);
        p.haze.push_back(h);
    } else if (what == "fade") {
        // Drawn from the events each tick (overlay_tick); without the port's overlay, not at all.
        if (!g_has_overlay) {
            warn_once(p, "a fade (needs the port's overlay)");
        }
    } else if (what == "tie_hide") {
        if (!API_HAS(tie_hide)) {
            return;
        }

        const int tie = e["tie"].integer(-1);
        const bool on = e["on"].boolean(true);

        if (g_api->tie_hide(g_mod, tie, on ? 1 : 0) != OPENRAC_HOOK_OK) {
            return;
        }

        // Not persistent: shown again when the scene ends.
        std::erase(p.ties_hidden, tie);

        if (on && !e["persistent"].boolean()) {
            p.ties_hidden.push_back(tie);
        }
    } else if (what == "air" || what == "slow") {
        // Read each tick (air_tick, stretch_at).
    } else if (what == "play") {
        p.after = e["cutscene"].string();
    } else if (what == "travel") {
        p.travel = level;
    } else {
        warn_once(p, std::format("the event \"{}\"", what));
    }
}

void events_tick(Play& p, int t) {
    const auto& list = (*p.doc)["events"].items();

    for (std::size_t i = 0; i < list.size(); ++i) {
        if (p.events_done[i] || t < list[i]["t"].tick()) {
            continue;
        }

        p.events_done[i] = 1;
        // A skip runs only what must last: persistent events, a next cutscene, a travel, and what
        // cannot be undone (a kill, a mission flag), so a skip leaves the world as watching does.
        const std::string what = list[i]["do"].string();

        if (p.skipped && !list[i]["persistent"].boolean() && what != "play" && what != "travel"
            && what != "moby_kill" && what != "mission_flag") {
            continue;
        }

        event_run(p, list[i]);
    }

    std::erase_if(p.haze, [t](const Play::Haze& h) { return t >= h.until; });
    std::vector<openrac_haze> patches;

    for (const Play::Haze& h : p.haze) {
        patches.push_back(h.patch);
    }

    g_api->haze_set(
        g_mod, patches.empty() ? nullptr : patches.data(), static_cast<int>(patches.size())
    );
}

// --- Start and end

void start(Cutscene& c, int hops = 0) {
    // A variant whose condition holds plays instead (not round in a loop).
    for (const Value& v : c.doc["variants"].items()) {
        if (hops >= kMaxVariantHops || !flag_holds(v["if"])) {
            continue;
        }

        if (Cutscene* other = g_level.find(v["play"].string()); other != nullptr && other != &c) {
            if (c.doc["trigger"]["once"].boolean(true)) {
                g_seen.add(seen_key(c));
            }

            start(*other, hops + 1);
            return;
        }
    }

    auto p = std::make_unique<Play>();
    p->scene = &c;
    p->doc = &c.doc;
    p->events_done.assign(c.doc["events"].size(), 0);
    p->sounds_done.assign(c.doc["sounds"].size(), 0);
    const Value& opts = c.doc["options"];

    if (opts["letterbox"].boolean(true) && !g_has_overlay) {
        warn_once(*p, "the letterbox (needs the port's overlay)");
    }

    if (opts["world"].string_or("run") == "hold" && !g_has_gate) {
        warn_once(*p, "holding the world (another mod has the moby update gate)");
    }

    p->music = opts["music"].string_or("duck");

    if (p->music != "keep" && !API_HAS(music_volume)) {
        warn_once(*p, "lowering the game's music (the port cannot)");
        p->music = "keep";
    }

    if (opts["hide_hud"].boolean(true)) {
        if (API_HAS(hud_hide)) {
            p->hud_hidden = g_api->hud_hide(g_mod, 1) == OPENRAC_HOOK_OK;
        } else {
            warn_once(*p, "hiding the HUD (the port cannot)");
        }
    }

    // Off (the "play" setting, or the party host's): it ends at once, as a skip, so what it leaves in
    // the world still happens.
    if (!scenes_on()) {
        p->skipped = true;
    }

    // Held, the hero stands (his sequence 0) or plays the file's hero.seq; the grab's own animation
    // (-1) is the one of being caught, arms up, which looked like a fall.
    g_api->hero_grab(
        g_mod, std::clamp(static_cast<int>(c.doc["hero"]["seq"].integer(0)), 0, 255), 0
    );
    g_play = std::move(p);
    g_swallow_start = true;
    g_cutscene_on = true;
    log(OPENRAC_LOG_INFO, std::format("playing {}", c.name));
}

// Ends the cutscene: what it leaves stays, the rest goes back. With `level_gone` the level it played
// in was reloaded or left, and its mobys with it: only the port's state is put back.
std::set<std::string> g_chain;  // the cutscenes played one after the other since the hero was free

void finish(bool level_gone) {
    Play& p = *g_play;

    if (!level_gone && p.skipped) {
        // Everything still to come that lasts.
        std::fill(p.sounds_done.begin(), p.sounds_done.end(), 1);
        events_tick(p, p.scene->length);
    }

    for (Voice& v : p.voices) {
        g_api->sound_stop(g_mod, v.voice);
    }

    p.voices.clear();
    g_api->hud_text_set(g_mod, nullptr, 0);
    show_pictures({});
    g_api->haze_set(g_mod, nullptr, 0);

    if (p.last_fov > 0) {
        fov_set(p, 0);
    }

    if (p.air_set) {
        g_api->scene_air(g_mod, nullptr);
    }

    stretch_set(p, 1);

    if (!level_gone) {
        for (int tie : p.ties_hidden) {
            g_api->tie_hide(g_mod, tie, 0);
        }

        for (int slot : p.mobys_hidden) {
            g_api->moby_hide(g_mod, slot, 0);
        }

        for (int slot : p.mobys_shown) {
            g_api->moby_hide(g_mod, slot, 1);
        }
    }

    g_api->camera_set(g_mod, nullptr);
    g_api->hero_release(g_mod);

    if (p.hud_hidden) {
        g_api->hud_hide(g_mod, 0);
    }

    if (p.music != "keep") {
        g_api->music_volume(g_mod, 1.0f);
    }

    g_holding = false;
    g_hold_free.clear();

    if (!level_gone) {
        actors_end(p);

        if (p.hero_hidden) {
            g_api->moby_hide(g_mod, g_api->figure_slot(g_mod, -1), 0);
        }

        const Value& end = (*p.doc)["hero"]["end"];

        if (end.is_object()) {
            const V3 pos = vec(end["position"]);
            const float at[3] = {pos.x, pos.y, pos.z};
            g_api->hero_place(
                g_mod, at, static_cast<float>(end["yaw"].number()), OPENRAC_PLACE_GROUND
            );
        }
    }

    const std::string done = p.scene->name, next = p.after;
    const int travel = p.travel;

    // Seen only once it ended or was skipped: a game closed in the middle plays it again.
    if (!level_gone && (*p.doc)["trigger"]["once"].boolean(true)) {
        g_seen.add(seen_key(*p.scene));
    }

    g_play.reset();
    g_cutscene_on = false;
    g_bars = 0;
    g_fade = 0;
    log(OPENRAC_LOG_INFO, std::format("{} ended", done));
    g_chain.insert(done);

    if (level_gone) {
        g_chain.clear();
        return;
    }

    if (travel >= 0) {
        g_chain.clear();
        g_api->play_travel(g_mod, travel);
        return;
    }

    // One that played in this chain already does not play again (a loop of "play" or "after").
    if (!next.empty()) {
        if (Cutscene* c = g_level.find(next); c != nullptr && g_chain.count(next) == 0) {
            start(*c);
            return;
        } else if (c != nullptr) {
            log(OPENRAC_LOG_WARN,
                std::format(
                    "{}: \"play\" of {}, which played just before: a loop, stopped", done, next
                ));
        }
    }

    // Cutscenes that start after this one.
    for (Cutscene& c : g_level.scenes) {
        const Value& tr = c.doc["trigger"];

        if (tr["kind"].string() == "after" && tr["cutscene"].string() == done
            && flag_holds(tr["flag"]) && !(tr["once"].boolean(true) && g_seen.has(seen_key(c)))
            && g_chain.count(c.name) == 0) {
            start(c);
            return;
        }
    }

    g_chain.clear();
}

void play_tick() {
    Play& p = *g_play;
    read_mobys();

    // What the scene's events set wins over the class's code while the scene plays.
    for (const auto& [slot, hidden] : p.mobys_forced) {
        const openrac_moby* m = moby_at(slot);

        if (m != nullptr && ((m->flags & OPENRAC_MOBY_HIDDEN) != 0) != hidden) {
            g_api->moby_hide(g_mod, slot, hidden ? 1 : 0);
        }
    }

    if (!p.started) {
        // The first tick: the hero hidden if asked, the actors taken or made.
        p.started = true;
        const std::string mode = (*p.doc)["hero"]["mode"].string_or("held");

        if (mode == "hidden" || mode == "actor") {
            p.hero_hidden =
                g_api->moby_hide(g_mod, g_api->figure_slot(g_mod, -1), 1) == OPENRAC_HOOK_OK;
        }

        actors_begin(p);
        air_check(p);

        if ((*p.doc)["options"]["world"].string_or("run") == "hold" && g_has_gate) {
            // Every moby but the actors, the hero and what the mods' figures are holds still.
            g_hold_free.clear();
            g_hold_free.insert(g_api->figure_slot(g_mod, -1));

            for (const ActorRun& a : p.actors) {
                if (a.moby_slot() >= 0) {
                    g_hold_free.insert(a.moby_slot());
                }
            }

            g_holding = true;
        }
    }

    if (p.ease_out >= 0) {
        // Back to the game's camera, then the end.
        openrac_camera game{};
        game.size = sizeof(game);
        const float u = float(p.ease_out) / float(kCameraEaseOut);
        const float s = u * u * (3 - 2 * u);

        if (p.has_camera && g_api->camera_view(&game) == OPENRAC_HOOK_OK) {
            const V3 eye = lerp(p.last_camera.eye, V3{game.eye[0], game.eye[1], game.eye[2]}, s);
            const V3 f = lerp(
                p.last_camera.forward, V3{game.forward[0], game.forward[1], game.forward[2]}, s
            );
            openrac_camera cam =
                make_camera(eye, normal(f, p.last_camera.forward), p.last_camera.roll * (1 - s));
            g_api->camera_set(g_mod, &cam);

            if (p.last_fov > 0) {
                const float from = fov_at((*p.doc)["camera"]["keys"], p.scene->length);
                fov_set(p, from + (kGameFov - from) * s);
            }
        }

        overlay_tick(p, p.scene->length, 1 - s);
        music_set(p, p.scene->length, 1 - s);
        air_tick(p, p.scene->length, 1 - s);
        stretch_set(p, 1 + (stretch_at((*p.doc)["events"], p.scene->length) - 1) * (1 - s));

        if (++p.ease_out > kCameraEaseOut) {
            finish(false);
        }

        return;
    }

    const bool skippable = (*p.doc)["options"]["skippable"].boolean(true);

    if (skippable && (g_pressed & OPENRAC_PAD_START)) {
        p.skipped = true;
    }

    if (p.skipped) {
        finish(false);
        return;
    }

    const int t = p.t;

    // Carriers first, so a rider goes where its carrier is this tick.
    for (ActorRun& a : p.actors) {
        if (!(*a.def)["ride"].is_array()) {
            actor_tick(p, a, t);
        }
    }

    for (ActorRun& a : p.actors) {
        if ((*a.def)["ride"].is_array()) {
            actor_tick(p, a, t);
        }
    }

    CameraPose pose;

    if (camera_at(p, t, pose)) {
        const openrac_camera cam = make_camera(pose.eye, pose.forward, pose.roll);
        g_api->camera_set(g_mod, &cam);
        p.last_camera = pose;
        p.has_camera = true;
        fov_set(p, fov_at((*p.doc)["camera"]["keys"], t));
    }

    music_set(p, t, 1);
    subtitles_tick(p, t);
    sounds_tick(p, t);
    events_tick(p, t);
    overlay_tick(p, t, 1);
    air_tick(p, t, 1);
    stretch_set(p, stretch_at((*p.doc)["events"], t));

    if (p.travel >= 0) {
        finish(false);
        return;
    }

    if (++p.t > p.scene->length) {
        g_api->hud_text_set(g_mod, nullptr, 0);
        show_pictures({});

        if (p.has_camera && (*p.doc)["camera"]["end"].string_or("ease") == "ease") {
            g_api->hero_release(g_mod);
            p.ease_out = 0;
        } else {
            finish(false);
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Triggers

// The game would start a scene of its own now: the player in control, on foot, alive; on a port that
// says when he stands steady (not hurt, not on a ride, a lift or in water), once he does.
bool g_port_steady = false;  // the port has said the hero stands steady

bool can_start(const openrac_play_status& s, const openrac_pose& hero) {
    if (hero.flags & OPENRAC_POSE_STEADY) {
        g_port_steady = true;
    }

    return s.phase == OPENRAC_PLAY_PLAYING && g_api->scene_playing(nullptr) < 0 && hero.health > 0
           && !(hero.flags & OPENRAC_POSE_AIRBORNE)
           && (!g_port_steady || (hero.flags & OPENRAC_POSE_STEADY));
}

// Each trigger's start is kept until it can play: two at once play one after the other, and one
// that fires while the hero is busy (in the air, hurt, a scene of the game's) plays once he is free.
void triggers_tick(const openrac_play_status& s) {
    openrac_pose hero;
    const bool has_hero = hero_pose(hero);
    const V3 at = has_hero ? V3{hero.position[0], hero.position[1], hero.position[2]} : V3{};
    const bool ready = has_hero && can_start(s, hero);
    bool talk_ready = false;
    bool mobys_read = false;

    for (Cutscene& c : g_level.scenes) {
        const Value& tr = c.doc["trigger"];
        const std::string kind = tr["kind"].string();

        if (tr["once"].boolean(true) && g_seen.has(seen_key(c))) {
            c.pending = false;
            continue;
        }

        bool now = false;

        if (kind == "level_start") {
            now = !g_level.started;
        } else if (kind == "volume") {
            const bool in = has_hero && inside(tr, at);
            now = in && !c.inside;
            c.inside = in;
        } else if (kind == "talk") {
            const int slot = spawn_slot(tr["spawn_id"].integer(-1));

            if (!mobys_read) {
                read_mobys(), mobys_read = true;
            }

            if (const openrac_moby* m = moby_at(slot); m && has_hero) {
                const V3 mp{m->position[0], m->position[1], m->position[2]};

                if (length(mp - at) <= tr["radius"].number(4)) {
                    talk_ready = ready;
                    now = g_talk_pressed;
                }
            }
        } else if (kind == "pickup") {
            const bool here = spawn_slot(tr["spawn_id"].integer(-1)) >= 0;
            now = c.present && !here;
            c.present = here;
        } else if (kind == "flag") {
            const bool holds = flag_holds(tr["flag"]);
            now = holds && !c.flag_was;
            c.flag_was = holds;
        }

        if (now && (kind == "flag" || flag_holds(tr["flag"]))) {
            c.pending = true;
        }
    }

    g_talk_ready = talk_ready;
    g_talk_pressed = false;
    g_level.started = true;

    if (!ready) {
        return;
    }

    for (Cutscene& c : g_level.scenes) {
        if (c.pending) {
            c.pending = false;
            start(c);
            return;
        }
    }
}

// ---------------------------------------------------------------------------------------------

void frame() {
    openrac_play_status s{};
    s.size = sizeof(s);

    if (g_api->play_status(&s) != OPENRAC_HOOK_OK) {
        return;
    }

    if (s.loads != g_level.loads || s.level != g_level.number) {
        if (g_play) {
            finish(true);  // everything of it went with the level
        }

        g_level.loads = s.loads;

        if (s.level != g_level.number) {
            load_level(s.level);
        } else {
            g_level.started = false;
        }

        for (Cutscene& c : g_level.scenes) {
            c.inside = false;
            c.pending = false;
            c.present = spawn_slot(c.doc["trigger"]["spawn_id"].integer(-1)) >= 0;
            c.flag_was = flag_holds(c.doc["trigger"]["flag"]);
        }

        char code[8] = "en";
        g_api->game_language(code, sizeof(code));
        g_language = code;
    }

    if (g_play) {
        // A menu over the cutscene pauses it, and silences its voices until it goes.
        if (s.phase == OPENRAC_PLAY_PLAYING) {
            play_tick();
        } else if (!g_play->muted) {
            for (const Voice& v : g_play->voices) {
                g_api->sound_set(g_mod, v.voice, 0.0f, 1.0f);
            }

            g_play->muted = true;
        }
    } else if (!g_level.scenes.empty()) {
        triggers_tick(s);
    }

    g_pressed = 0;
}

int moby_gate(void*, int slot, int phase) {
    return phase == OPENRAC_MOBY_BEFORE && g_holding && g_hold_free.count(slot) == 0 ? 1 : 0;
}

void load(openrac::Mod& mod) {
    g_api = &mod.api();
    g_mod = mod.handle();

    if (g_api->api_minor < 35 || g_api->size < offsetof(openrac_api, game_language) + sizeof(void*)
        || g_api->pack_folder == nullptr || g_api->game_language == nullptr) {
        throw openrac::Error(
            "this player needs a newer OpenRAC (one with pack_folder and game_language)"
        );
    }

    g_subtitles = mod.setting<bool>("subtitles");
    g_play_scenes = mod.setting<bool>("play");

    // "play" is the host's in a party: published as this game's rules, taken from the host's.
    if (API_HAS(party_rules_publish) && API_HAS(on_party_rules)) {
        const std::uint8_t rule = g_play_scenes ? 1 : 0;
        g_api->party_rules_publish(
            g_mod, &rule, 1, g_play_scenes ? "Cutscenes on" : "Cutscenes off"
        );
        g_api->on_party_rules(
            g_mod,
            [](void*, const void* data, std::uint32_t size) {
                g_host_play =
                    data != nullptr && size >= 1 ? *static_cast<const std::uint8_t*>(data) : -1;
            },
            nullptr
        );
    }

    g_has_overlay = mod.on_overlay(draw_overlay);
    // The gate holds the world (options.world "hold"); it lets every update run otherwise.
    g_has_gate = g_api->on_moby_update(g_mod, moby_gate, nullptr) == OPENRAC_HOOK_OK;
    mod.on_frame(frame);
    mod.on_pad(on_pad);
}

}  // namespace

namespace editor_runtime {
void cutscenes_load(openrac::Mod& mod) {
    load(mod);
}
}  // namespace editor_runtime
