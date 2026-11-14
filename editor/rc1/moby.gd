@tool
extends Node3D
## A moby class scene (mobys/moby_<class>.tscn), on every placed moby of the class.
##
## Light: gives its model the moby's own light, as the game lights it: the ambient colour
## (rc1_colour, red, green, blue bytes; 0x80 = 1.0) and the light word (rc1_light: set, second
## set, fade) that rc1/lit_moby.gdshader reads. A moby without them (one a cutscene adds) keeps
## what the game's CreateMoby gives: set 0 and an ambient of 0x40.
##
## Animation: a placed moby plays the sequence the game starts every moby on (InitMobyInstance:
## the class's sequence 0), looping, instead of standing in its bind pose. In the editor it
## follows the time of the level's cutscene preview, so a background moves with the scene. A
## cutscene's own actor is posed by its actor node instead. Display only.
##
## Start state: what the game does with a placed moby when the level starts, which its record
## alone does not show. The game's own code works it out: moby_states.py runs the level for a
## few seconds and writes moby_states.json beside level.json (a moby missing from it is not
## there, "hidden" one the game does not draw). Without that file, and for the reasons shown,
## its spawn flags (spawned only before or only after a mission of the planet is done) and its
## class's rule (rc1/class_rules.json: a class whose own code hides it,
## or which the game's code draws more of, as the nanotech crate's ball, rc1/nanotech.gd). The
## Inspector shows it read-only, with its reason. A moby the game keeps hidden is not drawn, or
## drawn faint with the project setting openrac/editor/show_hidden_mobys. Display only: nothing
## of it is saved.

# =============================================================================
# VARIABLES
# =============================================================================

## The model's animation player, or null for a class without sequences.
var player: AnimationPlayer

## The sequence 0 clip's name in player, or empty.
var clip: StringName = &""

## The cutscene whose preview time the editor follows (found again when it goes).
var cutscene: Node

## The last time the pose was set for, so an unmoved timeline costs nothing.
var shown: float = -1.0

## Frames until the next search for a cutscene.
var search_in: int = 0

# =============================================================================
# CONSTANTS
# =============================================================================

## Frames between searches for a cutscene in the edited scene.
const SEARCH_EVERY: int = 30

## The class rules (rc1/class_rules.json).
const RULES_PATH: String = "res://rc1/class_rules.json"

## The project setting that draws hidden mobys faint instead of not at all.
const SHOW_HIDDEN: String = "openrac/editor/show_hidden_mobys"

## How faint a hidden moby is drawn with SHOW_HIDDEN.
const GHOST: float = 0.75

## Spawn flag bits of a level moby record (load_level_gameplay): spawned while its mission is not
## done (1), once it is (2); gone for good once killed or picked up (8, or 4 without 8).
const SPAWN_BEFORE: int = 1
const SPAWN_AFTER: int = 2
const SPAWN_SAVED: int = 4
const SPAWN_ONCE: int = 8

# =============================================================================
# START STATE
# =============================================================================

## Whether the game keeps it hidden when the level starts.
var hidden_at_start: bool = false

## Whether that depends on the player's progress or on what happens in the level.
var depends_on_game: bool = false

## Whether what the player did before may change it while a cutscene plays (a mission, a
## volume Ratchet enters): the scene tree warns, so the cutscene can show or hide it outright.
var may_change: bool = false

## What the cutscene preview's hide and show events make of it at the preview's time: -1 none
## (the start state), 0 shown, 1 hidden.
var scene_hidden: int = -1

## Each spawn id's hide and show events of the cutscene ([seconds, hidden], by time), and the
## frame they were read on: read again every SEARCH_EVERY frames, once for every moby.
static var scene_events: Dictionary = {}
static var events_frame: int = -SEARCH_EVERY

## Why, one line per reason.
var start_reasons: PackedStringArray = []

## The class's looks (class_rules.json "looks"), and the one shown.
var looks: PackedStringArray = []
var look: String = ""

## The class rules, read once.
static var rules: Dictionary = {}

## Each level folder's moby_states.json: {"time": modified time, "mobys": {key: moby}}.
static var level_states: Dictionary = {}

## The level folders the states program was run for this session (run once each).
static var states_tried: Dictionary = {}

## The game's state of this moby when the level starts (moby_states.json), or empty.
var game_state: Dictionary = {}

## What the editor draws for a moby whose class's code draws it instead of its model
## (class_rules.json "drawn_by_code" and "effect"), or empty.
var code_effect: String = ""

## The effect nodes rc1/moby.gd adds, display only (not saved: they have no owner).
const EFFECT_NODES: PackedStringArray = ["NanotechEffect", "ElectricGateEffect", "GameHeightEffect"]

# =============================================================================
# METHODS
# =============================================================================

func _ready() -> void:
	apply_light()
	decide_start()
	show_start()

	# A cutscene actor's model: the actor poses it.
	if get_parent() != null and get_parent().is_class("CutsceneActor"):
		set_process(false)
		return

	find_clip()

	if not Engine.is_editor_hint():
		if clip != &"":
			player.play(clip)

		set_process(false)
		return

	# Show hidden mobys, changed in Project Settings, shows at once.
	if has_meta("rc1_index"):
		ProjectSettings.settings_changed.connect(show_start)

	if clip != &"":
		show_at(0.0)

	# Nothing to follow: no clip, and no spawn id a cutscene's event could name.
	if clip == &"" and int(get_meta("rc1_spawn_id", -1)) < 0:
		set_process(false)


func _process(_delta: float) -> void:
	# The editor only: follow the cutscene preview's time.
	if cutscene == null or not is_instance_valid(cutscene):
		search_in -= 1

		# Searching every frame would cost too much with hundreds of mobys.
		if search_in > 0:
			return

		search_in = SEARCH_EVERY
		cutscene = find_cutscene()

		# No cutscene in the scene: the first frame stays.
		if cutscene == null:
			return

	var seconds: float = cutscene.get_timeline_position()

	if clip != &"":
		show_at(seconds)

	follow_events(seconds)


## Hides or shows the moby as the cutscene's events for its spawn id say at SECONDS.
func follow_events(seconds: float) -> void:
	var frame: int = Engine.get_process_frames()

	if frame - events_frame >= SEARCH_EVERY or frame < events_frame:
		events_frame = frame
		scene_events = read_events(cutscene)

	var now: int = -1

	for event in scene_events.get(int(get_meta("rc1_spawn_id", -1)), []):
		if event[0] > seconds:
			break

		now = 1 if event[1] else 0

	if now != scene_hidden:
		scene_hidden = now
		show_start()


## The cutscene's hide, show and kill events of level mobys, by spawn id (see scene_events).
static func read_events(scene: Node) -> Dictionary:
	var found: Dictionary = {}

	for event in scene.find_children("*", "CutsceneEvent", true, false):
		var action: int = int(event.get("action"))

		# 0 hide, 1 show, 2 kill (CutsceneEvent::Action).
		if action > 2:
			continue

		var list: Array = found.get_or_add(int(event.get("spawn_id")), [])
		list.append([float(event.get("start")), action != 1])

	for list in found.values():
		list.sort_custom(func(a, b): return a[0] < b[0])

	return found


func _get_configuration_warnings() -> PackedStringArray:
	if not may_change:
		return []

	return ["Shown or not depending on the save: " + "; ".join(start_reasons)
		+ " Add a Show or Hide event for spawn id %d to be sure." % int(get_meta("rc1_spawn_id", -1))]


## Sets the moby's ambient and light word on every mesh of its model.
func apply_light() -> void:
	var colour: Array = get_meta("rc1_colour", [0x40, 0x40, 0x40])
	var word: int = int(get_meta("rc1_light", 0))
	var ambient := Color(colour[0] / 128.0, colour[1] / 128.0, colour[2] / 128.0, 1.0)

	for node in find_children("*", "GeometryInstance3D", true, false):
		var mesh := node as GeometryInstance3D
		mesh.set_instance_shader_parameter("ambient", ambient)
		mesh.set_instance_shader_parameter("light_word", word)


## Finds the model's player and its sequence 0 clip (seq_00, the import may drop a "_loop").
func find_clip() -> void:
	var players: Array[Node] = find_children("*", "AnimationPlayer", true, false)

	# A class without a skeleton has no player.
	if players.is_empty():
		return

	player = players[0] as AnimationPlayer

	for name in player.get_animation_list():
		if name == "seq_00" or name == "seq_00_loop":
			clip = name
			return


## The first cutscene in the edited scene, or null.
func find_cutscene() -> Node:
	var root: Node = get_tree().edited_scene_root if is_inside_tree() else null

	# Outside an edited scene there is no preview to follow.
	if root == null:
		return null

	var found: Array[Node] = root.find_children("*", "Cutscene", true, false)
	return null if found.is_empty() else found[0]


## Poses the model at seconds into its clip, looping.
func show_at(seconds: float) -> void:
	# The same time as last frame: the pose is already there.
	if is_equal_approx(seconds, shown):
		return

	shown = seconds
	var animation: Animation = player.get_animation(clip)
	var at: float = fmod(seconds, animation.length) if animation.length > 0.0 else 0.0

	if player.assigned_animation != clip:
		player.assigned_animation = clip

	player.seek(at, true)


func _get_property_list() -> Array[Dictionary]:
	# A moby the level places: its start state, read-only, and its class's looks.
	if not has_meta("rc1_index"):
		return []

	var properties: Array[Dictionary] = [
		{"name": "Start state", "type": TYPE_NIL, "usage": PROPERTY_USAGE_GROUP},
		{"name": "start_state", "type": TYPE_STRING, "hint": PROPERTY_HINT_MULTILINE_TEXT,
			"usage": PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY},
	]

	# One click: a cutscene event that shows or hides it from the preview's time on.
	if int(get_meta("rc1_spawn_id", -1)) >= 0:
		properties.append({"name": "scene_show", "type": TYPE_CALLABLE, "hint": PROPERTY_HINT_TOOL_BUTTON,
			"hint_string": "Show in the cutscene from now,GuiVisibilityVisible", "usage": PROPERTY_USAGE_EDITOR})
		properties.append({"name": "scene_hide", "type": TYPE_CALLABLE, "hint": PROPERTY_HINT_TOOL_BUTTON,
			"hint_string": "Hide in the cutscene from now,GuiVisibilityHidden", "usage": PROPERTY_USAGE_EDITOR})

	# The class's rule, edited here for every moby of the class (class_rules.json).
	properties.append({"name": "class_rule", "type": TYPE_STRING, "hint": PROPERTY_HINT_EXPRESSION,
		"usage": PROPERTY_USAGE_EDITOR})

	if not looks.is_empty():
		properties.append({"name": "preview_look", "type": TYPE_STRING, "hint": PROPERTY_HINT_ENUM,
			"hint_string": ",".join(looks), "usage": PROPERTY_USAGE_EDITOR})

	return properties


func _get(property: StringName) -> Variant:
	if property == &"start_state":
		var head: String = "Hidden" if hidden_at_start else "Shown"

		if depends_on_game:
			head += " (depends on the game)"


		if not game_state.is_empty():
			head += ", state %d" % int(game_state["state"])
		if scene_hidden != -1:
			head += "; the cutscene %s it now" % ("hides" if scene_hidden == 1 else "shows")

		return head + ("\n" + "\n".join(start_reasons) if not start_reasons.is_empty() else "")

	if property == &"preview_look":
		return look

	if property == &"scene_show":
		return add_scene_event.bind(1)

	if property == &"scene_hide":
		return add_scene_event.bind(0)

	if property == &"class_rule":
		var rule: Dictionary = class_rules().get(str(int(get_meta("rc1_class", -1))), {})
		return JSON.stringify(rule, "  ") if not rule.is_empty() else ""

	return null


func _set(property: StringName, value: Variant) -> bool:
	if property == &"class_rule":
		return set_class_rule(String(value))

	if property != &"preview_look" or not String(value) in looks:
		return false

	look = String(value)
	show_start()
	return true


## The class rules (rc1/class_rules.json "classes"), read once.
static func class_rules() -> Dictionary:
	# The setting shows in Project Settings (Advanced) once a moby is read; reopen the scene
	# after changing it.
	if not ProjectSettings.has_setting(SHOW_HIDDEN):
		ProjectSettings.set_setting(SHOW_HIDDEN, false)
		ProjectSettings.set_initial_value(SHOW_HIDDEN, false)
		ProjectSettings.add_property_info({"name": SHOW_HIDDEN, "type": TYPE_BOOL})

	if rules.is_empty() and FileAccess.file_exists(RULES_PATH):
		var parsed: Variant = JSON.parse_string(FileAccess.get_file_as_string(RULES_PATH))
		rules = parsed.get("classes", {}) if parsed is Dictionary else {}

	return rules


## Adds a CutsceneEvent to the edited scene's cutscene that hides (ACTION 0) or shows (1) this
## moby from the preview's time on, so the game does the same whatever the player did before.
func add_scene_event(action: int) -> void:
	var scene: Node = find_cutscene()

	if scene == null:
		push_warning("This scene has no cutscene.")
		return

	var event: Node = ClassDB.instantiate("CutsceneEvent")
	event.set("action", action)
	event.set("spawn_id", int(get_meta("rc1_spawn_id")))
	event.set("start", scene.get_timeline_position())
	event.name = "%s%s" % ["Show" if action == 1 else "Hide", name]
	scene.add_child(event, true)
	event.owner = get_tree().edited_scene_root
	events_frame = -SEARCH_EVERY
	var editor: Object = Engine.get_singleton("EditorInterface")
	editor.mark_scene_as_unsaved()
	editor.edit_node(event)


## Saves TEXT (JSON, empty for none) as the rule of this moby's class in class_rules.json and
## works out the start state again. Other mobys of the class take it when the scene is reopened.
func set_class_rule(text: String) -> bool:
	var rule: Variant = JSON.parse_string(text) if text.strip_edges() != "" else {}

	if not rule is Dictionary:
		push_warning("The class rule is not a JSON object; nothing was saved.")
		return false

	var parsed: Variant = JSON.parse_string(FileAccess.get_file_as_string(RULES_PATH)) if FileAccess.file_exists(RULES_PATH) else {}
	var file: Dictionary = parsed if parsed is Dictionary else {}
	var classes: Dictionary = file.get_or_add("classes", {})
	var key: String = str(int(get_meta("rc1_class", -1)))

	if rule.is_empty():
		classes.erase(key)
	else:
		classes[key] = rule

	var out := FileAccess.open(RULES_PATH, FileAccess.WRITE)

	if out == null:
		push_warning("Cannot write " + RULES_PATH)
		return false

	out.store_string(JSON.stringify(file, "  ") + "\n")
	out.close()
	rules = classes
	decide_start()
	show_start()
	update_configuration_warnings()
	notify_property_list_changed()
	return true


## Works out the start state of a moby the level places.
func decide_start() -> void:
	hidden_at_start = false
	depends_on_game = false
	may_change = false
	start_reasons.clear()
	game_state = {}
	code_effect = ""

	if not has_meta("rc1_index"):
		return

	var flags: int = int(get_meta("rc1_spawn_flags", 0))
	var mission: int = int(get_meta("rc1_unknown_4", -1))

	if flags & (SPAWN_BEFORE | SPAWN_AFTER) == SPAWN_AFTER:
		hidden_at_start = true
		depends_on_game = true
		may_change = true
		start_reasons.append("Appears after mission %d of this planet (spawn flags %d); not in a new game." % [mission, flags])
	elif flags & (SPAWN_BEFORE | SPAWN_AFTER) == SPAWN_BEFORE:
		depends_on_game = true
		may_change = true
		start_reasons.append("Gone after mission %d of this planet (spawn flags %d)." % [mission, flags])

	if flags & SPAWN_ONCE or flags & (SPAWN_SAVED | SPAWN_ONCE) == SPAWN_SAVED:
		depends_on_game = true
		start_reasons.append("Gone for good once killed, broken or picked up (the save keeps it).")

	var rule: Dictionary = class_rules().get(str(int(get_meta("rc1_class", -1))), {})

	var states: Variant = game_states()

	# The game's own run, when there is one and it had this moby; the class rules only explain
	# it. A moby added in the editor since (no record, or a name the packed level did not have)
	# goes by the rules alone until the level is packed again.
	var by_game: bool = states != null and game_key(states) != null

	if by_game:
		decide_by_game(states, rule)
	elif rule.has("drawn_by_code"):
		hidden_at_start = true
		start_reasons.append(String(rule["drawn_by_code"]))

	# What its class's code draws instead of its model: while it is there, in the states listed.
	if rule.has("drawn_by_code") and (not by_game or (not game_state.is_empty() and int(game_state["dead"]) == 0)):
		var states_drawn: Array = rule.get("effect_states", [])

		if states_drawn.is_empty() or game_state.is_empty() or states_drawn.has(float(game_state["state"])) \
				or states_drawn.has(int(game_state["state"])):
			code_effect = String(rule.get("effect", ""))

		# A class whose code brings it out once Ratchet enters a volume.
		if hidden_at_start and rule.has("trigger_volume_pvar"):
			var volume: int = int(pvar_word(String(rule["trigger_volume_pvar"]).hex_to_int()))

			if volume != -1:
				depends_on_game = true
				may_change = true
				start_reasons.append("Shown once Ratchet enters volume %d." % volume)
	elif rule.has("hidden_unless_pvar"):
		decide_by_pvars(rule)

	# A pvar word the class's code hides it by in a way the run does not count as hidden.
	if rule.has("hidden_if_pvar") and not hidden_at_start and int(pvar_word(String(rule["hidden_if_pvar"]).hex_to_int())) != 0:
		hidden_at_start = true
		depends_on_game = true
		may_change = true
		start_reasons.append(String(rule.get("hidden_if_pvar_reason", "Hidden by its class's code (pvar %s)." % rule["hidden_if_pvar"])))

	# Volumes the camera must be inside, or outside, for its code to draw it.
	if rule.has("camera_volumes"):
		decide_by_camera_volumes(rule["camera_volumes"])

	if rule.has("progress"):
		depends_on_game = true
		may_change = true
		start_reasons.append(String(rule["progress"]))

	if rule.has("source"):
		start_reasons.append("Rule from " + String(rule["source"]) + ".")

	looks = PackedStringArray(rule.get("looks", []))
	look = looks[0] if not looks.is_empty() else ""


## The level's moby states from the game's run (moby_states.json), read again when the file
## changes, or null without one.
func game_states() -> Variant:
	var path: String = scene_file_path.get_base_dir().get_base_dir().path_join("moby_states.json")

	if not FileAccess.file_exists(path) and not states_tried.has(path):
		states_tried[path] = true
		run_states(path)

	if not FileAccess.file_exists(path):
		return null

	var time: int = FileAccess.get_modified_time(path)
	var known: Dictionary = level_states.get(path, {})

	if known.get("time", -1) != time:
		var parsed: Variant = JSON.parse_string(FileAccess.get_file_as_string(path))
		var mobys: Dictionary = {}
		var packed: bool = parsed is Dictionary and bool(parsed.get("packed", false))

		# From the disc's level a moby is its record (rc1_index); from a packed one, its node's
		# name, as the packed table's order is not the scene's (moby_states.py after_pack).
		if parsed is Dictionary:
			for moby in parsed.get("mobys", []):
				if not packed:
					mobys[int(moby["key"])] = moby
				elif moby.get("name") is String:
					mobys[String(moby["name"])] = moby

		known = {"time": time, "mobys": mobys, "packed": packed,
			"names": PackedStringArray(parsed.get("names", [])) if packed else PackedStringArray(),
			"ticks": int(parsed.get("ticks", 0)) if parsed is Dictionary else 0}
		level_states[path] = known

	return known


## Runs the port's states program (moby_states.py does the same from a shell) for the level
## whose moby_states.json is PATH, when the program, the game's data and the port's levels are
## found. The launcher names all three on the editor's command line, after -- (--openrac-data,
## --openrac-levels, --openrac-states); without them the data comes from OPENRAC_DATA and the rest
## from the checkout the project sits in (assets/levels/rac1/<version>/level_NN).
static func run_states(path: String) -> void:
	var folder: String = ProjectSettings.globalize_path(path.get_base_dir())
	var stats: Variant = JSON.parse_string(FileAccess.get_file_as_string(folder.path_join("level.json")))

	if not stats is Dictionary or not stats.has("level"):
		return

	var version: String = String(stats.get("version", "pal"))
	var root: String = ProjectSettings.globalize_path("res://").path_join("../../../../..").simplify_path()
	var data: String = user_arg("--openrac-data", OS.get_environment("OPENRAC_DATA"))
	var levels: String = user_arg("--openrac-levels", root.path_join("assets/port/rac1").path_join(version))
	var program: String = user_arg("--openrac-states",
		root.path_join("port/build/release/openrac-rac1-%s-states" % version))

	if not FileAccess.file_exists(program) and FileAccess.file_exists(program + ".exe"):
		program += ".exe"

	if data == "" or not FileAccess.file_exists(program) or not DirAccess.dir_exists_absolute(levels):
		return

	var output: Array = []
	var code: int = OS.execute(program, ["--data", data, "--levels", levels, "--level", str(int(stats["level"])),
		"--moby-states", ProjectSettings.globalize_path(path)], output, true)

	if code != 0:
		push_warning("The moby states program failed for %s: %s" % [path, "".join(output).right(400)])


## The value after NAME on the editor's command line (after --), or FALLBACK.
static func user_arg(name: String, fallback: String) -> String:
	var args: PackedStringArray = OS.get_cmdline_user_args()
	var at: int = args.find(name)
	return args[at + 1] if at >= 0 and at + 1 < args.size() else fallback


## What the game's run calls this moby: its node's name from a packed level, else its record;
## null when the run had no such moby.
func game_key(states: Dictionary) -> Variant:
	if states["packed"]:
		return String(name) if states["names"].has(String(name)) else null

	var index: int = int(get_meta("rc1_index", -1))
	return index if index >= 0 and owns_record(index) else null


## Whether this node is the moby of record INDEX, as pack.py (pack_level) decides: Godot's copy of
## a moby keeps its index, so of the Mobys' nodes with it the first named after it (_NNNN) is the
## moby, else the first one.
func owns_record(index: int) -> bool:
	var suffix: String = "_%04d" % index
	var parent: Node = get_parent()

	if parent == null:
		return true

	var first: Node = null

	for sibling in parent.get_children():
		if int(sibling.get_meta("rc1_index", -1)) != index:
			continue

		if String(sibling.name).ends_with(suffix):
			return sibling == self

		if first == null:
			first = sibling

	return first == self


## The start state the game's run gave.
func decide_by_game(states: Dictionary, rule: Dictionary) -> void:
	var seconds: float = states["ticks"] / 60.0
	game_state = states["mobys"].get(game_key(states), {})

	if game_state.is_empty():
		hidden_at_start = true
		start_reasons.append("Gone %.0f s after the level starts (moby_states.json)." % seconds)
	elif int(game_state["dead"]) != 0:
		hidden_at_start = true
		start_reasons.append("Dead %.0f s after the level starts (moby_states.json)." % seconds)
	elif int(game_state["hidden"]) != 0 and rule.has("game_hidden"):
		start_reasons.append(String(rule["game_hidden"]))
	elif int(game_state["hidden"]) != 0 and rule.has("drawn_by_code"):
		hidden_at_start = true
		start_reasons.append(String(rule["drawn_by_code"]))
	elif int(game_state["hidden"]) != 0:
		hidden_at_start = true
		start_reasons.append("Not drawn %.0f s after the level starts (moby_states.json)." % seconds)


## A copy of the model where the game's run puts the moby (class_rules.json effect "game_height":
## Rilgar's water, which its code raises or lowers), the moby itself left where its record has it.
func add_game_height_model() -> void:
	var model: Node3D = get_node_or_null("Model") as Node3D

	if model == null:
		return

	# The game's place (its record's without a run), in the moby's own frame: the mobys' parent
	# is the game's frame.
	var at: Vector3 = position
	var place: Variant = game_state.get("pos")

	if place is Array and place.size() == 3:
		at = Vector3(place[0], place[1], place[2])

	var copy: Node3D = model.duplicate() as Node3D
	copy.name = "GameHeightEffect"
	copy.position = model.position + transform.affine_inverse() * at
	add_child(copy)


## The volumes its code checks the camera against (class_rules.json "camera_volumes": the pvar
## offsets of two volume numbers each, -1 for none): drawn only inside a "show" one, or not
## inside a "hide" one, so a cutscene's camera can bring it out or hide it.
func decide_by_camera_volumes(volumes: Dictionary) -> void:
	for kind in ["show", "hide"]:
		if not volumes.has(kind):
			continue

		var at: int = String(volumes[kind]).hex_to_int()

		# Its code goes by the first word: -1 there, the kind is not used.
		if int(pvar_word(at)) == -1:
			continue

		var found: Array = [int(pvar_word(at)), int(pvar_word(at + 4))].filter(func(v: int) -> bool: return v != -1)

		may_change = true
		var listed: String = " or ".join(found.map(func(v: int) -> String: return str(v)))
		start_reasons.append(("Drawn only while the camera is inside volume %s." if kind == "show"
			else "Not drawn while the camera is inside volume %s.") % listed)
		# The first kind set is the one its code goes by.
		return


## A class whose code hides the moby unless one of some pvar words is set (amoeboids: state 12,
## or 13 until Ratchet enters the volume another word names).
func decide_by_pvars(rule: Dictionary) -> void:
	for offset in rule["hidden_unless_pvar"]:
		if pvar_word(String(offset).hex_to_int()) != 0:
			return

	hidden_at_start = true

	if not rule.has("trigger_volume_pvar"):
		start_reasons.append("Hidden by its class's code.")
		return

	var volume: int = int(pvar_word(String(rule["trigger_volume_pvar"]).hex_to_int()))

	if volume == -1:
		start_reasons.append("Hidden by its class's code, and nothing shows it.")
	else:
		depends_on_game = true
		may_change = true
		start_reasons.append("Hidden until Ratchet enters volume %d." % volume)


## The pvar word at a byte offset (0 when it has none).
func pvar_word(offset: int) -> Variant:
	var words: Array = get_meta("rc1_pvar", [])
	var at: int = offset / 4
	return words[at] if at < words.size() else 0


## Draws the moby as it starts, or as the cutscene's events have it at the preview's time:
## hidden (or faint), in its look, with the code-drawn effect of its class.
func show_start() -> void:
	if not has_meta("rc1_index"):
		return

	for called in EFFECT_NODES:
		var effect: Node = get_node_or_null(NodePath(called))

		if effect != null:
			remove_child(effect)
			effect.queue_free()

	var rule: Dictionary = class_rules().get(str(int(get_meta("rc1_class", -1))), {})
	# A cutscene event at the preview's time wins over the start state.
	var hidden: bool = scene_hidden == 1 or (scene_hidden == -1 and hidden_at_start)
	var gone: bool = hidden or look == "broken"

	if rule.get("effect", "") == "nanotech" and not hidden:
		var directory: String = scene_file_path.get_base_dir().get_base_dir().path_join("effects")
		var Nanotech: Script = load("res://rc1/nanotech.gd")
		Nanotech.attach(self, directory, look)

	# What the class's code draws instead of the model, whatever a cutscene does with the moby
	# (the game's code draws it whether the moby is shown or not).
	if code_effect == "electric_gate":
		var ElectricGate: Script = load("res://rc1/electric_gate.gd")
		ElectricGate.attach(self, scene_file_path.get_base_dir().get_base_dir().path_join("effects"))
	elif code_effect == "game_height":
		add_game_height_model()

	var ghost: bool = ProjectSettings.get_setting(SHOW_HIDDEN, false)
	var effects: Array[Node] = []

	for called in EFFECT_NODES:
		if has_node(NodePath(called)):
			effects.append(get_node(NodePath(called)))

	for node in find_children("*", "GeometryInstance3D", true, false):
		# An effect is drawn whole, in its look.
		if effects.any(func(effect: Node) -> bool: return effect.is_ancestor_of(node)):
			continue

		var instance: RID = (node as GeometryInstance3D).get_instance()
		RenderingServer.instance_set_visible(instance, (node as Node3D).is_visible_in_tree() and (not gone or ghost))
		RenderingServer.instance_geometry_set_transparency(instance, GHOST if gone and ghost else 0.0)
