@tool
extends EditorPlugin
## test_editor_e2e.py's editor half: an editor plugin that drives the Assets dock in a headless
## editor as a person would (clicks, typing, drags through Input), checks what each action did and
## writes the results to the file named after --, then quits. Each scenario is short; the editor
## starts once for all of them.

const Library = preload("res://addons/openrac_library/library.gd")
const Dock = preload("res://addons/openrac_library/dock.gd")

## A run that has not finished by then (a scenario stopped on a script error) is written as it
## stands and the editor quits.
const MOST_SECONDS: float = 120.0

## Each a method below, run in this order.
const SCENARIOS: PackedStringArray = ["stand_up", "dock_lists", "search", "typing", "filters", "planet_and_tree",
	"details", "add_full", "add_look", "own_logic_after_pick", "class_logic", "drag_to_view", "drag_to_scene_dock",
	"drag_onto_a_selected_node", "drag_cancelled", "copies_left_alone", "foreign_switch", "previews",
	"class_menu_names", "keyboard", "many_adds", "new_group", "broken_assets", "odd_clicks", "refresh_click",
	"layout", "looks", "no_level", "closed_while_importing", "plugin_off_and_on", "speed"]

## The dock's narrowest sensible width: a dock column a person has not widened.
const NARROW: float = 280.0

## test_editor_e2e.py's STRESS_MOBYS: each of its many levels has the same classes.
const STRESS_MOBYS: int = 40

var dock: Control
var plugin: EditorPlugin
var checks: Array = []
var times: Dictionary = {}
var scenario: String = ""
var out_path: String = ""
## Only these scenarios when the runner was given a list (after the output path), all otherwise.
var only: PackedStringArray = []
## Where looks() saves the dock's pictures, when the editor draws.
var shots: String = OS.get_environment("OPENRAC_E2E_SHOTS")
var started: int = 0
var finished: bool = false


func _enter_tree() -> void:
	var args: PackedStringArray = OS.get_cmdline_user_args()
	out_path = args[0] if not args.is_empty() else ""
	only = args[1].split(",") if args.size() > 1 else PackedStringArray()
	get_tree().create_timer(MOST_SECONDS).timeout.connect(finish)
	run.call_deferred()


# =============================================================================
# THE RUN
# =============================================================================

func run() -> void:
	started = Time.get_ticks_msec()
	# Frames as fast as they come: the editor otherwise sleeps between them to save power.
	OS.low_processor_usage_mode = false
	Engine.max_fps = 0

	for _i in range(600):
		await frame()
		find_dock()
		if dock != null and EditorInterface.get_edited_scene_root() != null and not dock.levels.is_empty():
			break

	check("dock found", dock != null and plugin != null)

	if dock != null:
		# Room for the whole dock: a headless editor's window is small.
		get_tree().root.size = Vector2i(1280, 960)
		show_dock()
		await frames(2)

		for name in SCENARIOS:
			if not only.is_empty() and not only.has(name):
				continue
			scenario = name
			var began: int = Time.get_ticks_msec()
			await call(name)
			times[name] = Time.get_ticks_msec() - began

	finish()


func finish() -> void:
	if finished:
		return

	finished = true
	var file := FileAccess.open(out_path, FileAccess.WRITE)
	file.store_string(JSON.stringify({"checks": checks, "times": times, "total_ms": Time.get_ticks_msec() - started,
		"stopped_in": scenario if not times.has(scenario) else "", "editor_start_ms": started}))
	file.close()
	get_tree().quit()


func check(name: String, ok: bool, detail: Variant = "") -> void:
	checks.append({"scenario": scenario, "name": name, "ok": ok, "detail": str(detail)})


func frame() -> void:
	await get_tree().process_frame


func frames(count: int) -> void:
	for _i in range(count):
		await get_tree().process_frame


## Waits until CONDITION holds, at most FRAMES frames; whether it did.
func until(condition: Callable, most: int = 600) -> bool:
	for _i in range(most):
		if condition.call():
			return true
		await get_tree().process_frame

	return condition.call()


func find_dock() -> void:
	dock = null
	plugin = null

	for node in EditorInterface.get_base_control().find_children("*", "", true, false):
		var script: Script = node.get_script()

		if script != null and String(script.resource_path).ends_with("openrac_library/dock.gd"):
			dock = node
			plugin = node.plugin
			return


func show_dock() -> void:
	if dock.has_method("make_visible"):
		dock.make_visible()


func scene() -> Node:
	return EditorInterface.get_edited_scene_root()


func game() -> Node3D:
	return scene().get_node_or_null("Game") as Node3D if scene() != null else null


## The edited scene's undo history: its actions' names, oldest first, up to the current one.
func history() -> PackedStringArray:
	var manager: EditorUndoRedoManager = plugin.get_undo_redo()
	var undo_redo: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(scene()))
	var names := PackedStringArray()

	for i in range(undo_redo.get_current_action() + 1):
		names.append(undo_redo.get_action_name(i))

	return names


func undo() -> void:
	var manager: EditorUndoRedoManager = plugin.get_undo_redo()
	manager.get_history_undo_redo(manager.get_object_history_id(scene())).undo()
	await frame()


func redo() -> void:
	var manager: EditorUndoRedoManager = plugin.get_undo_redo()
	manager.get_history_undo_redo(manager.get_object_history_id(scene())).redo()
	await frame()


## NODE's axes in the game's: its up should be the game's z.
func up_of(node: Node3D) -> Vector3:
	return (game().global_basis.inverse() * node.global_basis).z.normalized()


func standing(node: Node3D) -> bool:
	return node != null and up_of(node).is_equal_approx(Vector3(0, 0, 1))


func children_of(group: String) -> Array:
	var node: Node = game().get_node_or_null(group)
	return node.get_children() if node != null else []


# =============================================================================
# INPUT, AS A PERSON GIVES IT
# =============================================================================

func mouse(at: Vector2, pressed: bool, double: bool = false, button: MouseButton = MOUSE_BUTTON_LEFT) -> void:
	var event := InputEventMouseButton.new()
	event.button_index = button
	event.pressed = pressed
	event.double_click = double
	event.position = at
	event.global_position = at
	event.button_mask = MOUSE_BUTTON_MASK_LEFT if pressed else 0
	Input.parse_input_event(event)


func move(at: Vector2, by: Vector2, held: bool) -> void:
	var event := InputEventMouseMotion.new()
	event.position = at
	event.global_position = at
	event.relative = by
	event.button_mask = MOUSE_BUTTON_MASK_LEFT if held else 0
	Input.parse_input_event(event)


func click(at: Vector2, double: bool = false) -> void:
	move(at, Vector2.ZERO, false)
	mouse(at, true)
	mouse(at, false)

	if double:
		mouse(at, true, true)
		mouse(at, false)

	await frames(2)


func press(button: Button) -> void:
	await click(button.get_global_rect().get_center())


func key(code: Key, ctrl: bool = false, unicode: int = 0) -> void:
	for pressed in [true, false]:
		var event := InputEventKey.new()
		event.keycode = code
		event.physical_keycode = code
		event.ctrl_pressed = ctrl
		event.unicode = unicode
		event.pressed = pressed
		Input.parse_input_event(event)


func type_text(text: String) -> void:
	for character in text:
		key(OS.find_keycode_from_string(character.to_upper()) if character != " " else KEY_SPACE, false,
			character.unicode_at(0))
		await frame()


## Drags from FROM to TO in steps; lets go unless HOLD.
func drag(from: Vector2, to: Vector2, hold: bool = false) -> void:
	move(from, Vector2.ZERO, false)
	mouse(from, true)
	await frame()
	var steps: int = 5

	for i in range(1, steps + 1):
		move(from.lerp(to, float(i) / steps), (to - from) / steps, true)
		await frame()

	if not hold:
		mouse(to, false)
		await frames(3)


func item_centre(index: int) -> Vector2:
	dock.list.ensure_current_is_visible()
	return dock.list.get_global_transform() * dock.list.get_item_rect(index).get_center()


## Shows WORDS' assets (the search as a person leaves it) and picks the first, by a click.
func pick(words: String, index: int = 0) -> Dictionary:
	dock.search.text = words
	dock.search.text_submitted.emit(words)
	await frame()

	if dock.list.item_count <= index or dock.asset_at(index).is_empty():
		return {}

	await click(item_centre(index))
	await frames(2)
	return dock.asset_at(index)


func view_centre() -> Vector2:
	return (EditorInterface.get_editor_viewport_3d(0).get_parent() as Control).get_global_rect().get_center()


func reset_dock() -> void:
	dock.search.text = ""
	dock.filter.select(0)
	dock.planet.select(0)
	dock.tree.deselect_all()
	dock.refill()


# =============================================================================
# SCENARIOS
# =============================================================================

## The fixture's scene has a model an older editor dropped lying down and one standing.
func stand_up() -> void:
	await until(func() -> bool: return history().has("Stand up assets placed lying down"), 120)
	var old: Node3D = game().get_node_or_null("StaticAssets/OldDrop")
	var kept: Node3D = game().get_node_or_null("StaticAssets/Standing")
	check("stood up once, as one undo step", Array(history()).count("Stand up assets placed lying down") == 1, history())
	check("the lying one stands", standing(old), up_of(old) if old != null else null)
	check("its size and place kept", old != null and is_equal_approx(old.global_basis.get_scale().x, 2.0)
		and old.global_position.is_equal_approx(Vector3(5, 7, -6)), old.global_transform if old != null else null)
	check("the standing one is left alone", kept != null and kept.transform.basis.is_equal_approx(Basis(Vector3(0, 0, 1), 0.5)))
	check("the dock says so", dock.report.text.begins_with("Stood up 1 asset"), dock.report.text)
	check("marked done", scene().has_meta(plugin.STOOD_UP))
	await undo()
	check("Ctrl+Z lies it down again", old != null and not standing(old), up_of(old) if old != null else null)
	plugin.stand_up_lying(scene())
	await frame()
	check("not stood up a second time", Array(history()).count("Stand up assets placed lying down") <= 1, history())
	await redo()
	check("redo stands it up", standing(old))


func dock_lists() -> void:
	reset_dock()
	var games: Dictionary = {}

	for level in dock.levels:
		games[level["game"]] = games.get(level["game"], 0) + 1

	check("every level found", games.get("rac1", 0) == 4 and games.get("rac3", 0) == 1 and games.get("rac4", 0) > 0, games)
	var planets: Array = []

	for i in range(dock.planet.item_count):
		planets.append(dock.planet.get_item_text(i))

	var unique: Dictionary = {}

	for planet in planets:
		unique[planet] = true

	check("planets listed once each", planets[0] == "Every planet" and unique.size() == planets.size()
		and planets.has("Kerwan") and planets.has("Rilgar") and planets.has("Veldin") and planets.has("Blarg Station"), planets)
	check("this level's own at first", dock.shown.size() == 2 and dock.shown.all(func(asset: Dictionary) -> bool: return int(asset["level"]) == 5), names())
	check("nothing picked: buttons off", dock.add_button.disabled and dock.own_button.disabled and dock.as_button.disabled)


func names() -> Array:
	return dock.shown.map(func(asset: Dictionary) -> String: return String(asset["name"]))


func search() -> void:
	for case in [["ship red", ["red_ship_1"]], ["RED_ship", ["red_ship_1"]], ["  red   ship ", ["red_ship_1"]],
			["75", ["red_ship_1"]], ["kerwan red", ["red_ship_1"]], ["veldin", ["something"]]]:
		dock.search.text = case[0]
		dock.refill()
		check("'%s' finds %s" % case, names() == case[1], names())

	dock.search.text = "kerwan"
	dock.refill()
	check("a planet's name lists its assets", dock.shown.all(func(asset: Dictionary) -> bool: return asset["level_name"] == "Kerwan") and dock.shown.size() >= 3, names())

	for words in ["zzz", "[b]x[/b]", "%s %d", "ząb", "_", "__", "-1", "a".repeat(400)]:
		dock.search.text = words
		dock.refill()
		var ok: bool = (dock.list.item_count > 0 and not dock.note.visible) or (dock.shown.is_empty() and dock.note.visible)
		check("'%s' lists or says none" % words.left(12), ok, [dock.list.item_count, dock.note.text])

	dock.search.text = "zzz"
	dock.refill()
	check("nothing to pick, and it says so", dock.list.item_count == 0 and dock.asset_at(0).is_empty() and dock.note.visible
		and dock.note.text == "No assets match.")
	reset_dock()


## Typing waits for a pause before the list fills again, and the clear button empties it.
func typing() -> void:
	reset_dock()
	dock.search.grab_focus()
	await frame()
	var before: int = dock.list.item_count
	await type_text("red")
	check("not refilled on each key", dock.list.item_count == before and dock.search.text == "red", [dock.search.text, dock.list.item_count])
	await until(func() -> bool: return names() == ["red_ship_1"], 120)
	check("refilled once typing pauses", names() == ["red_ship_1"], names())
	key(KEY_ENTER)
	await frame()
	check("Enter keeps it", names() == ["red_ship_1"], names())
	dock.search.clear()
	dock.search.text_changed.emit("")
	await until(func() -> bool: return dock.shown.size() == 2, 120)
	check("cleared: this level's own again", dock.shown.size() == 2, names())
	dock.search.release_focus()


func filters() -> void:
	reset_dock()
	var expect: Dictionary = {Dock.SHOW_ALL: 2, Dock.SHOW_HERE: 2, Dock.SHOW_THIS_GAME: 2}

	for id in expect:
		dock.filter.select(dock.filter.get_item_index(id))
		dock.refill()
		check("filter %d with nothing picked" % id, dock.shown.size() == expect[id], names())

	dock.filter.select(dock.filter.get_item_index(Dock.SHOW_IMPORTS))
	dock.refill()
	check("from other levels: every level's importable mobys", names().has("red_ship_1") and names().has("codebot_slot")
		and dock.shown.all(func(asset: Dictionary) -> bool: return Library.import_refusal(asset, dock.here).is_empty()), names())
	dock.filter.select(dock.filter.get_item_index(Dock.SHOW_OTHER_GAMES))
	dock.refill()
	check("other games: every other game's", not dock.shown.is_empty() and dock.shown.all(func(asset: Dictionary) -> bool: return asset["game"] != "rac1"), names().slice(0, 5))
	dock.search.text = "zzz"
	dock.refill()
	check("a filter that hides all says so", dock.list.item_count == 0 and dock.note.visible and dock.note.text == "No assets match.")
	reset_dock()


func planet_and_tree() -> void:
	reset_dock()
	var kerwan: int = -1

	for i in range(dock.planet.item_count):
		if dock.planet.get_item_text(i) == "Kerwan":
			kerwan = i

	dock.planet.select(kerwan)
	dock.planet.item_selected.emit(kerwan)
	check("a planet shows its assets", not dock.shown.is_empty() and dock.shown.all(func(asset: Dictionary) -> bool: return asset["level_name"] == "Kerwan"), names())

	# A level of another planet picked in the tree: the tree wins and the planet goes back to all.
	var level_item: TreeItem = null
	var item: TreeItem = dock.tree.get_root()

	while item != null:
		var meta: Variant = item.get_metadata(0)
		if meta is Dictionary and meta.has("level") and not meta.has("kind") and meta["level"]["name"] == "Rilgar":
			level_item = item
		item = item.get_next_in_tree()

	level_item.select(0)
	await frame()
	check("a level in the tree resets the planet", dock.planet.selected == 0 and dock.shown.size() == 2, [dock.planet.selected, names()])
	dock.planet.select(kerwan)
	dock.planet.item_selected.emit(kerwan)
	check("a planet clears the tree's level", dock.tree.get_selected() == null and not dock.shown.is_empty(), names())
	dock.rescan()
	check("refresh keeps the planet", dock.planet.selected == kerwan, dock.planet.selected)

	# Kinds under a level, and collapsing and opening the tree, keep the list right.
	level_item = null
	item = dock.tree.get_root()

	while item != null:
		var meta: Variant = item.get_metadata(0)
		if meta is Dictionary and meta.get("kind", "") == "ties" and meta["level"]["game"] == "rac1" and meta["level"]["level"] == 3 \
				and meta["level"]["disc"] == "ntsc":
			level_item = item
		item = item.get_next_in_tree()

	level_item.get_parent().get_parent().collapsed = false
	level_item.get_parent().collapsed = false
	level_item.select(0)
	await frame()
	check("a level's ties", not dock.shown.is_empty() and dock.shown.all(func(asset: Dictionary) -> bool: return asset["kind"] == "ties"),
		[names(), level_item.get_text(0), dock.tree.get_selected() == level_item, dock.planet.selected, dock.search.text, dock.filter.selected])
	level_item.get_parent().get_parent().collapsed = true
	await frame()
	check("the tree sizes to its rows", dock.tree.custom_minimum_size.y > 0)
	reset_dock()


## What the selected asset can do here, by its colour.
func details() -> void:
	reset_dock()
	var full: Dictionary = await pick("vendor")
	check("the game's own: added with its logic", not dock.add_button.disabled and dock.add_button.text.ends_with("(with its logic)"), dock.add_button.text)
	check("the game's own: no import", dock.own_button.disabled and dock.own_button.tooltip_text == "This level has the class already.", dock.own_button.tooltip_text)
	check("the game's own: no other class's logic", dock.as_button.disabled)
	check("details name it", dock.details.text.contains(String(full.get("name", "?"))), dock.details.text)
	await pick("red_ship_1")
	check("another level's moby: import and class logic", not dock.own_button.disabled and not dock.as_button.disabled
		and dock.add_button.text.ends_with("(look only)"))
	dock.filter.select(dock.filter.get_item_index(Dock.SHOW_ALL))
	var pal: Dictionary = await pick("turret_gaspar_2")
	check("another disc's: no import", not pal.is_empty() and dock.own_button.disabled
		and dock.own_button.tooltip_text == "Only between levels of the same disc.", [pal.get("name"), dock.own_button.tooltip_text])
	await pick("682")
	check("a tie: no logic of its own", dock.own_button.disabled and dock.own_button.tooltip_text == "Only mobys have logic of their own.")
	check("a tie: no class logic", dock.as_button.disabled)
	reset_dock()


func add_full() -> void:
	reset_dock()
	var count: int = children_of("Mobys").size()
	var asset: Dictionary = await pick("vendor")
	await click(item_centre(0), true)
	await until(func() -> bool: return children_of("Mobys").size() == count + 1, 120)
	var node: Node3D = children_of("Mobys").back() if children_of("Mobys").size() == count + 1 else null
	check("a double-click adds the game's own under Mobys", node != null and node.scene_file_path.ends_with("moby_11.tscn"), node)
	check("standing", standing(node), up_of(node) if node != null else null)

	if node != null:
		var camera: Camera3D = EditorInterface.get_editor_viewport_3d(0).get_camera_3d()
		var front: Vector3 = node.global_basis * Library.front_for(asset)
		var to_eye: Vector3 = camera.global_position - node.global_position
		check("its front to the camera", Vector2(front.x, front.z).normalized().dot(Vector2(to_eye.x, to_eye.z).normalized()) > 0.99)
		check("in front of the camera", node.global_position.distance_to(camera.global_position) > plugin.ADD_DISTANCE - 0.01
			and node.global_position.distance_to(camera.global_position) < plugin.ADD_DISTANCE + 0.01)

	check("one undo step", history()[-1].begins_with("Add vendor"), history())
	await undo()
	check("undo takes it out", children_of("Mobys").size() == count)
	await redo()
	check("redo puts it back", children_of("Mobys").size() == count + 1)
	check("the report names it", dock.report.text.begins_with("Added"), dock.report.text)


## Another level's model as static scenery: copied, imported, then placed (by the Add button).
func add_look() -> void:
	reset_dock()
	var asset: Dictionary = await pick("682")
	await press(dock.add_button)
	await until(func() -> bool: return children_of("StaticAssets").any(func(node: Node) -> bool: return node.has_meta("rc1_asset") and String(node.get_meta("rc1_asset")).ends_with("ties/682")), 900)
	var placed: Array = children_of("StaticAssets").filter(func(node: Node) -> bool: return node.has_meta("rc1_asset") and String(node.get_meta("rc1_asset")).ends_with("ties/682"))
	check("added under StaticAssets once", placed.size() == 1, [dock.report.text, dock.add_button.get_global_rect(), get_viewport().get_visible_rect(), dock.add_button.disabled, asset.get("name")])

	if placed.size() == 1:
		check("standing", standing(placed[0]))
		check("solid (a tie)", placed[0].get_meta("rc1_static_collision", false) == true)
		check("from the project's library copy", String(placed[0].scene_file_path).begins_with(Library.LIBRARY_DIR), placed[0].scene_file_path)

	check("no drop settled after it", not history().has("Place assets from the Assets dock"), history())
	check("not left waiting", not plugin.waiting_for_import)


## A regression: picking the asset brings its static copy in, and "Add with its own logic" placed
## that copy, which the drop handling then moved to StaticAssets as scenery.
func own_logic_after_pick() -> void:
	reset_dock()
	await pick("red_ship_1")
	await frames(5)
	await press(dock.own_button)
	await until(func() -> bool: return children_of("Mobys").any(func(node: Node) -> bool: return node.has_meta("rc1_foreign_level")), 600)
	var foreign: Array = children_of("Mobys").filter(func(node: Node) -> bool: return node.has_meta("rc1_foreign_level"))
	check("a moby with its own logic under Mobys", foreign.size() == 1, children_of("Mobys").map(func(node: Node) -> String: return node.name))
	check("not moved to StaticAssets", not children_of("StaticAssets").any(func(node: Node) -> bool: return node.has_meta("rc1_foreign_level")))
	check("no scenery metadata", foreign.size() == 1 and not foreign[0].has_meta("rc1_asset"))
	check("standing", foreign.size() == 1 and standing(foreign[0]))
	check("one undo step", history()[-1].ends_with("with its own logic"), history())
	check("the report says own logic", dock.report.text.contains("with its own logic"), dock.report.text)


func class_logic() -> void:
	reset_dock()
	await pick("red_ship_1")
	await press(dock.as_button)
	await frame()
	check("the class menu opens", dock.classes_menu.visible and dock.classes_menu.item_count == 1)
	var index: int = dock.classes_menu.get_item_index(11)
	dock.classes_menu.id_pressed.emit(11)
	dock.classes_menu.hide()
	await frames(2)
	var node: Node = children_of("Mobys").back()
	check("a moby of that class with the model as its skin", node.has_meta("rc1_skin") and node.scene_file_path.ends_with("moby_11.tscn"), node.get_meta_list())
	check("the skin is drawn", node.has_node("SkinLook"))
	check("standing", standing(node))
	await undo()
	check("undo takes it out", not children_of("Mobys").has(node))
	await redo()
	await frame()
	check("redo puts it back, drawn", children_of("Mobys").has(node) and node.has_node("SkinLook"))
	check("menu entry names the class once", index >= 0 and not dock.classes_menu.get_item_text(index).begins_with("Class 11 (class"), dock.classes_menu.get_item_text(index))


## A real drag from the list into the 3D view: Godot's drop, then the dock puts it in its group.
func drag_to_view() -> void:
	reset_dock()
	await pick("red_ship_1")
	await until(func() -> bool: return Library.imported("res://library/rac1/ntsc/level_03/mobys/moby_75.glb"), 600)
	var before: int = history().size()
	var count: int = children_of("StaticAssets").size()
	await drag(item_centre(0), view_centre(), true)
	check("dragging the asset's file", get_viewport().gui_is_dragging() and get_viewport().gui_get_drag_data() is Dictionary)
	check("the 3D view stands it up meanwhile", EditorInterface.has_meta(plugin.DROP_BASIS))
	mouse(view_centre(), false)
	await until(func() -> bool: return children_of("StaticAssets").size() == count + 1, 120)
	await frames(2)
	check("dropped under StaticAssets", children_of("StaticAssets").size() == count + 1)
	var node: Node3D = children_of("StaticAssets").back()
	check("standing", standing(node))
	check("named for the asset", String(node.name).begins_with("RedShip1"), node.name)
	check("drag over: the turn is off", not EditorInterface.has_meta(plugin.DROP_BASIS))
	check("two undo steps: Godot's drop and the placing", history().size() == before + 2, history())
	await undo()
	await undo()
	check("undo twice takes it out", not children_of("StaticAssets").has(node))
	var steps: int = plugin.get_undo_redo().get_history_undo_redo(plugin.get_undo_redo().get_object_history_id(scene())).get_history_count()
	await redo()
	await redo()
	await frames(3)
	check("redo puts it back where it was", children_of("StaticAssets").has(node) and standing(node))
	check("undo and redo add no steps", plugin.get_undo_redo().get_history_undo_redo(plugin.get_undo_redo().get_object_history_id(scene())).get_history_count() == steps)


## Onto the Scene dock's root row: Godot drops it at the root's origin, lying; the dock moves it in
## front of the camera, standing.
func drag_to_scene_dock() -> void:
	reset_dock()
	await pick("red_ship_1")
	var tree: Tree = null

	for node in EditorInterface.get_base_control().find_children("*", "SceneTreeEditor", true, false):
		tree = node.find_children("*", "Tree", true, false)[0] if not node.find_children("*", "Tree", true, false).is_empty() else null
		if tree != null and tree.is_visible_in_tree():
			break

	if tree == null or tree.get_root() == null:
		check("the Scene dock is there", false)
		return

	tree.scroll_to_item(tree.get_root(), true)
	await frame()
	var row: Vector2 = tree.get_global_transform() * tree.get_item_area_rect(tree.get_root()).get_center()
	var count: int = children_of("StaticAssets").size()
	await drag(item_centre(0), row)
	await until(func() -> bool: return children_of("StaticAssets").size() == count + 1, 120)
	await frames(2)
	var node: Node3D = children_of("StaticAssets").back() if children_of("StaticAssets").size() == count + 1 else null
	check("dropped under StaticAssets", node != null, [row, tree.get_global_rect(), history(), dock.report.text, game().get_children()])
	check("standing", standing(node))
	var camera: Camera3D = EditorInterface.get_editor_viewport_3d(0).get_camera_3d()
	check("in front of the camera, not at the origin", node != null and absf(node.global_position.distance_to(camera.global_position) - plugin.ADD_DISTANCE) < 0.01,
		node.global_position if node != null else null)


func drag_cancelled() -> void:
	reset_dock()
	await pick("red_ship_1")
	var count: int = children_of("StaticAssets").size()
	var steps: PackedStringArray = history()
	await drag(item_centre(0), view_centre(), true)
	key(KEY_ESCAPE)
	await frames(2)
	mouse(view_centre(), false)
	await frames(3)
	check("Escape drops nothing", children_of("StaticAssets").size() == count and history() == steps, history())
	check("the turn is off", not EditorInterface.has_meta(plugin.DROP_BASIS))

	# Let go over the dock itself, and over the search line.
	for target in [dock.list.get_global_rect().get_center() + Vector2(0, 20), dock.search.get_global_rect().get_center()]:
		await drag(item_centre(0), target)
		check("dropping on the dock adds nothing", children_of("StaticAssets").size() == count and history() == steps)
		check("and turns nothing", not EditorInterface.has_meta(plugin.DROP_BASIS))

	check("the search line takes no drop", not dock.search.text.contains("res://"), dock.search.text)


## A copy of a placed asset (duplicate, paste) is not settled again as a drop.
func copies_left_alone() -> void:
	var original: Node3D = children_of("StaticAssets").back()
	var copy: Node3D = original.duplicate()
	var group: Node = original.get_parent()
	var manager: EditorUndoRedoManager = plugin.get_undo_redo()
	manager.create_action("Duplicate Node(s)", UndoRedo.MERGE_DISABLE, scene())
	manager.add_do_method(group, "add_child", copy, true)
	manager.add_do_property(copy, "owner", scene())
	manager.add_do_reference(copy)
	manager.add_undo_method(group, "remove_child", copy)
	manager.commit_action()
	await frames(3)
	check("no placing step after a duplicate", history()[-1] == "Duplicate Node(s)", history())
	check("the copy stays where Godot put it", copy.get_parent() == group and copy.global_transform.is_equal_approx(original.global_transform))
	await undo()


func foreign_switch() -> void:
	var foreign: Array = children_of("Mobys").filter(func(node: Node) -> bool: return node.has_meta("rc1_foreign_level"))

	if foreign.is_empty():
		check("a moby with its own logic to switch", false)
		return

	EditorInterface.edit_node(foreign[0])
	await frames(3)
	var switch: CheckBox = null

	for node in EditorInterface.get_inspector().find_children("*", "CheckBox", true, false):
		if node.text.begins_with("Import without"):
			switch = node

	check("the Inspector shows the switch", switch != null)

	if switch == null:
		return

	await press(switch)
	check("switching on sets the metadata", foreign[0].get_meta("rc1_foreign_partial", false) == true)
	check("as an undo step", history()[-1].ends_with("without what does not fit"), history())
	await undo()
	check("Ctrl+Z takes it back", not foreign[0].has_meta("rc1_foreign_partial"))
	EditorInterface.edit_node(null)


func previews() -> void:
	var mesh := ArrayMesh.new()
	var arrays: Array = []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = PackedVector3Array([Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)])
	arrays[Mesh.ARRAY_NORMAL] = PackedVector3Array([Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)])
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)

	for side in [Vector3(1, 0, 0), Vector3(-1, 0, 0), Vector3(0, 1, 0), Vector3(0, -1, 0)]:
		var turned: Mesh = Dock.standing(mesh, side)
		var up: Vector3 = turned.surface_get_arrays(0)[Mesh.ARRAY_VERTEX][2]
		var face: Vector3 = Basis.IDENTITY * Vector3.ZERO
		var vertices: PackedVector3Array = turned.surface_get_arrays(0)[Mesh.ARRAY_VERTEX]
		# The front side's vertex (or the opposite's, negated) lands on the camera's z.
		var on_side: Vector3 = vertices[0] * side.x + vertices[1] * side.y
		check("front %s to the camera, up stays up" % side, up.is_equal_approx(Vector3(0, 1, 0)) and on_side.is_equal_approx(Vector3(0, 0, 1)), [up, on_side, face])

	var flat: Dictionary = dock.assets_of(dock.levels.filter(func(level: Dictionary) -> bool: return level["disc"] == "ntsc" and int(level["level"]) == 3)[0]) \
		.filter(func(asset: Dictionary) -> bool: return int(asset["class"]) == 1430)[0]
	check("the names table turns codebot_slot to -y", Library.front_for(flat).is_equal_approx(Vector3(0, -1, 0)))
	var plain: Dictionary = flat.duplicate()
	plain["class"] = 75
	check("a named front gets its own cached picture", dock.cache_path(flat) != dock.cache_path(plain))
	check("the waiting picture is sharp at the list's size", dock.placeholder != null and dock.placeholder.get_width() == 80)


func class_menu_names() -> void:
	await pick("red_ship_1")
	dock.show_classes()
	await frame()
	var labels: Array = []

	for i in range(dock.classes_menu.item_count):
		labels.append(dock.classes_menu.get_item_text(i))

	dock.classes_menu.hide()
	check("the dock's names in the class menu", labels == ["vendor (class 11)"], labels)


## A scene that is not a level, then none: nothing is brought in, and the dock says why.
func no_level() -> void:
	var path: String = "res://not_a_level.tscn"
	var root := Node3D.new()
	root.name = "NotALevel"
	var packed := PackedScene.new()
	packed.pack(root)
	ResourceSaver.save(packed, path)
	root.free()
	EditorInterface.open_scene_from_path(path)
	await until(func() -> bool: return scene() != null and scene().scene_file_path == path, 300)
	await frames(2)
	reset_dock()
	check("no level here", dock.here.is_empty())
	check("the list says what to do", dock.note.visible and dock.note.text == "Open a level scene, search, or pick a level above.", dock.note.text)
	await pick("red_ship_1")
	check("no button works", dock.add_button.disabled and dock.own_button.disabled and dock.as_button.disabled)
	var files: int = DirAccess.get_files_at("res://library/rac1/ntsc/level_03/ties").size() if DirAccess.dir_exists_absolute("res://library/rac1/ntsc/level_03/ties") else 0
	check("a drag carries nothing", dock.list._get_drag_data(dock.list.get_local_mouse_position()) == null and dock.report.text == "Open a level scene first.", dock.report.text)
	await click(item_centre(0), true)
	check("a double-click adds nothing", scene().get_children().all(func(node: Node) -> bool: return node.owner == null)
		and dock.report.text == "Open a level scene first.")
	# Every scene closed: the dock still lists and says what to do.
	while scene() != null:
		EditorInterface.close_scene()
		await frames(2)
	check("no scene: the dock still lists", dock.here.is_empty() and (dock.list.item_count > 0 or dock.note.visible), dock.note.text)
	check("no files copied meanwhile", (DirAccess.get_files_at("res://library/rac1/ntsc/level_03/ties").size() if DirAccess.dir_exists_absolute("res://library/rac1/ntsc/level_03/ties") else 0) == files)
	EditorInterface.open_scene_from_path("res://levels/level_05/level_05.tscn")
	await until(func() -> bool: return scene() != null and not dock.here.is_empty(), 300)
	check("back on the level", int(dock.here.get("level", -1)) == 5)


## Switched off while an asset waits to be imported, and on again: the editor keeps going.
func plugin_off_and_on() -> void:
	var name: String = "openrac_library"
	reset_dock()
	dock.filter.select(dock.filter.get_item_index(Dock.SHOW_ALL))
	var other: Dictionary = await pick("turret_gaspar_2")

	if not other.is_empty():
		plugin.add_in_view(other)

	for i in range(3):
		EditorInterface.set_plugin_enabled(name, false)
		await frames(2)
		EditorInterface.set_plugin_enabled(name, true)
		await frames(2)

	await until(func() -> bool:
		find_dock()
		return dock != null and dock.list.item_count > 0, 120)
	check("the dock is back", dock != null and plugin != null and dock.list.item_count > 0)
	check("still on the level", dock != null and int(dock.here.get("level", -1)) == 5)
	show_dock()
	await frame()


func speed() -> void:
	reset_dock()
	var began: int = Time.get_ticks_usec()

	for words in ["a", "e", "r", "ship", "kerwan", ""]:
		dock.search.text = words
		dock.refill()

	var each: float = (Time.get_ticks_usec() - began) / 6000.0
	check("a refill of every level takes under 50 ms", each < 50.0, "%.2f ms" % each)
	dock.search.text = "thing"
	dock.refill()
	check("a search over many levels finds each class once", dock.shown.filter(func(asset: Dictionary) -> bool: return String(asset["name"]).begins_with("thing_")).size() == STRESS_MOBYS, dock.shown.size())
	reset_dock()


## A level's model, or a scene, picked in the Scene dock: Godot drops the asset as its child; the
## dock still puts it in its group.
func drag_onto_a_selected_node() -> void:
	reset_dock()
	await pick("red_ship_1")
	var vendor: Node = game().get_node("Mobys/Vendor_0000")
	EditorInterface.get_selection().clear()
	EditorInterface.get_selection().add_node(vendor)
	await frame()
	var count: int = children_of("StaticAssets").size()
	await drag(item_centre(0), view_centre() + Vector2(30, 40))
	await until(func() -> bool: return children_of("StaticAssets").size() == count + 1, 120)
	await frames(2)
	check("moved to StaticAssets, not left under the selected moby", children_of("StaticAssets").size() == count + 1
		and vendor.get_children().all(func(node: Node) -> bool: return node.owner != scene()),
		vendor.get_children().map(func(node: Node) -> String: return node.name))
	check("standing", standing(children_of("StaticAssets").back()))
	EditorInterface.get_selection().clear()


## Up and down in the list, then Enter, adds the asset as a double-click does.
func keyboard() -> void:
	reset_dock()
	dock.list.grab_focus()
	dock.list.select(1)
	await frame()
	key(KEY_LEFT)
	await frame()
	check("Left picks the asset before", dock.list.get_selected_items() == PackedInt32Array([0]),
		[dock.list.get_selected_items(), dock.list.has_focus(), get_viewport().gui_get_focus_owner()])
	var count: int = children_of("Mobys").size()
	key(KEY_ENTER)
	await frames(2)
	check("Enter adds it", children_of("Mobys").size() == count + 1, dock.report.text)
	dock.list.release_focus()


## Add pressed again and again: each its own node, name and undo step.
func many_adds() -> void:
	reset_dock()
	await pick("vendor")
	var count: int = children_of("Mobys").size()
	var steps: int = history().size()

	for _i in range(5):
		await press(dock.add_button)

	var added: Array = children_of("Mobys").slice(count)
	var named: Dictionary = {}

	for node in added:
		named[String(node.name)] = true

	check("five added", added.size() == 5, added.size())
	check("each its own name", named.size() == 5, named.keys())
	check("five undo steps", history().size() == steps + 5, history().size() - steps)

	for _i in range(5):
		await undo()

	check("undone, all gone", children_of("Mobys").size() == count)

	for _i in range(5):
		await redo()

	check("redone, all back", children_of("Mobys").size() == count + 5)


## The first tie of the game's own makes Game/Ties; undo takes the group out with it.
func new_group() -> void:
	reset_dock()
	check("no Ties group yet", game().get_node_or_null("Ties") == null)
	await pick("orange_tank")
	await press(dock.add_button)
	var ties: Node = game().get_node_or_null("Ties")
	check("Ties made, with the tie", ties != null and ties.get_child_count() == 1 and ties.owner == scene())
	await undo()
	check("undo takes the group out", game().get_node_or_null("Ties") == null)
	await redo()
	check("redo puts it back", game().get_node_or_null("Ties") != null)


## A model whose texture is gone and a file that is no model: refused with a reason, nothing added.
func broken_assets() -> void:
	reset_dock()
	var count: int = children_of("StaticAssets").size()
	var asset: Dictionary = await pick("blarg_turret")
	check("listed", not asset.is_empty())
	await press(dock.add_button)
	await frames(2)
	check("a missing texture: says so", dock.report.text.contains("is missing"), dock.report.text)
	asset = await pick("machinegun_turret_oltanis")
	check("a file that is no model: listed with its waiting picture", not asset.is_empty() and dock.list.get_item_icon(0) == dock.placeholder)
	await press(dock.add_button)
	await frames(2)
	check("no model: says so", dock.report.text.contains("not a glTF model"), dock.report.text)
	check("no drag for it", dock.list._get_drag_data(item_centre(0)) == null)
	check("nothing added", children_of("StaticAssets").size() == count)
	check("not left waiting", not plugin.waiting_for_import)
	await press(dock.own_button)
	check("own logic: refused too", children_of("Mobys").all(func(node: Node) -> bool: return not String(node.name).begins_with("MachinegunTurret")))


## Clicks that do nothing must break nothing: the "none match" note, a right-click, empty space.
func odd_clicks() -> void:
	dock.search.text = "zzz"
	dock.refill()
	var steps: int = history().size()
	await click(dock.note.get_global_rect().get_center(), true)
	check("the none-match note adds nothing", history().size() == steps)
	reset_dock()
	var at: Vector2 = item_centre(0)
	move(at, Vector2.ZERO, false)
	mouse(at, true, false, MOUSE_BUTTON_RIGHT)
	mouse(at, false, false, MOUSE_BUTTON_RIGHT)
	await frames(2)
	await click(dock.list.get_global_rect().end - Vector2(8, 8), true)
	check("a right-click and empty space add nothing", history().size() == steps)
	await press(dock.add_button)
	check("Add with nothing picked adds nothing", history().size() == steps)


## The refresh button, clicked, reads the levels again and keeps the search and filter.
func refresh_click() -> void:
	reset_dock()
	dock.search.text = "red"
	dock.filter.select(dock.filter.get_item_index(Dock.SHOW_IMPORTS))
	dock.refill()
	var levels: int = dock.levels.size()
	await press(dock.refresh_button)
	check("levels read again", dock.levels.size() == levels and dock.report.text.begins_with("Levels found: %d" % levels), dock.report.text)
	check("search and filter kept", dock.search.text == "red" and dock.filter.get_selected_id() == Dock.SHOW_IMPORTS and names() == ["red_ship_1"], [names(), dock.search.text, dock.filter.get_selected_id()])
	reset_dock()


## The scene closed while an asset waits for its import: no crash, and it is not placed anywhere.
func closed_while_importing() -> void:
	reset_dock()
	dock.filter.select(dock.filter.get_item_index(Dock.SHOW_OTHER_GAMES))
	# Not picked first (a pick brings it in at once): only Add, as from the keyboard.
	dock.search.text = "something"
	dock.refill()
	var asset: Dictionary = dock.asset_at(0)
	plugin.add_in_view(asset)
	var waited: bool = plugin.waiting_for_import
	EditorInterface.close_scene()
	await until(func() -> bool: return not plugin.waiting_for_import, 600)
	check("it waited, then stopped", waited and not plugin.waiting_for_import, dock.report.text)
	check("and says why it did not place it", dock.report.text == "Open a level scene first." or scene() != null, dock.report.text)
	EditorInterface.open_scene_from_path("res://levels/level_05/level_05.tscn")
	await until(func() -> bool: return scene() != null and not dock.here.is_empty(), 300)
	check("back on the level", int(dock.here.get("level", -1)) == 5)


## Nothing spills out of a narrow dock: names fit their two lines, and every control fits.
func layout() -> void:
	reset_dock()
	dock.filter.select(dock.filter.get_item_index(Dock.SHOW_ALL))
	dock.refill()
	var font: Font = dock.list.get_theme_font("font")
	var size: int = dock.list.get_theme_font_size("font_size")
	var long: Array = []

	for asset in dock.shown:
		var text := TextParagraph.new()
		text.width = dock.list.fixed_column_width
		text.break_flags = TextServer.BREAK_MANDATORY | TextServer.BREAK_WORD_BOUND | TextServer.BREAK_GRAPHEME_BOUND
		text.add_string(String(asset["name"]), font, size)
		if text.get_line_count() > dock.list.max_text_lines:
			long.append(asset["name"])

	check("every name fits two lines", not dock.shown.is_empty() and long.is_empty(), long)
	var wide: Array = []

	for i in range(dock.planet.item_count):
		dock.planet.select(i)
		dock.planet.item_selected.emit(i)
		await frame()
		if dock.get_combined_minimum_size().x > NARROW:
			wide.append([dock.planet.get_item_text(i), dock.get_combined_minimum_size().x])

	for i in range(dock.filter.item_count):
		dock.filter.select(i)
		dock.filter.item_selected.emit(i)
		await frame()
		if dock.get_combined_minimum_size().x > NARROW:
			wide.append([dock.filter.get_item_text(i), dock.get_combined_minimum_size().x])

	dock.filter.select(dock.filter.get_item_index(Dock.SHOW_ALL))
	check("every planet and filter fits a narrow dock", wide.is_empty(), wide)
	dock.planet.select(0)
	var longest: String = ""

	for asset in dock.shown:
		if String(asset["name"]).length() > longest.length():
			longest = asset["name"]

	await pick(longest)
	await frames(2)
	check("a long name's details fit a narrow dock", dock.get_combined_minimum_size().x <= NARROW, [longest, dock.get_combined_minimum_size().x])
	dock.report.text = "Could not bring this asset in: its model failed to import. Pick it again after the import ends."
	await frame()
	check("a long report fits a narrow dock", dock.get_combined_minimum_size().x <= NARROW, dock.get_combined_minimum_size().x)
	var hidden: Array = []

	for button in [dock.add_button, dock.own_button, dock.as_button]:
		if not dock.get_global_rect().grow(1).encloses(button.get_global_rect()) or button.size.x < 1:
			hidden.append(button.text)

	check("every button shows in the dock", hidden.is_empty(), hidden)
	check("one whole row of assets shows", dock.list.item_count > 0
		and dock.list.size.y >= dock.list.get_item_rect(0).size.y, [dock.list.size, dock.list.get_item_rect(0)])
	var hint: String = dock.search.placeholder_text
	check("the search hint fits", dock.search.get_theme_font("font").get_string_size(hint, HORIZONTAL_ALIGNMENT_LEFT, -1,
		dock.search.get_theme_font_size("font_size")).x <= dock.search.size.x - 40, [hint, dock.search.size.x])
	var outside: Array = []
	var area: Rect2 = dock.get_global_rect().grow(1)

	for control in dock.find_children("*", "Control", true, false):
		if control.is_visible_in_tree() and not (control is Window) and control.get_parent() is Control \
				and not area.encloses(control.get_global_rect()):
			outside.append([control.get_class(), control.get_global_rect()])

	check("nothing outside the dock", outside.is_empty(), [dock.get_global_rect(), outside])
	dock.report.text = ""
	reset_dock()


## The dock as drawn (only when the editor draws, under a display): something is on screen in each
## state, and each picture is saved for a person to look at when OPENRAC_E2E_SHOTS names a folder.
func looks() -> void:
	if DisplayServer.get_name() == "headless":
		return

	var states: Dictionary = {
		"this_level": func() -> void: reset_dock(),
		"picked": func() -> void:
			dock.filter.select(dock.filter.get_item_index(Dock.SHOW_ALL))
			# Picked by code: a window without a window manager gets no clicks, and the clicks are
			# the headless run's to check.
			dock.search.text = "red_ship_1"
			dock.refill()
			dock.list.select(0)
			dock.list.item_selected.emit(0)
			check("picked: shown in the details", dock.details.text.contains("red_ship_1"), dock.details.text),
		"no_match": func() -> void:
			dock.search.text = "zzz nothing"
			dock.search.text_submitted.emit(dock.search.text),
	}

	for state in states:
		reset_dock()
		await states[state].call()
		# Previews are made in the background: wait for this view's.
		await until(func() -> bool: return dock.preview_queue.is_empty(), 300)
		await frames(4)
		var picture: Image = dock.get_viewport().get_texture().get_image()
		var rect := Rect2i(dock.get_global_rect()).intersection(Rect2i(Vector2i.ZERO, picture.get_size()))
		var part: Image = picture.get_region(rect)
		var colours: Dictionary = {}

		for y in range(0, part.get_height(), 2):
			for x in range(0, part.get_width(), 2):
				colours[part.get_pixel(x, y).to_rgba32()] = true

		check("%s: the dock is drawn" % state, colours.size() > 64, colours.size())
		check("%s: the list's note breaks between words" % state, not dock.note.visible
			or dock.note.get_line_count() <= dock.note.text.split(" ").size(), [dock.note.text, dock.note.get_line_count()])
		if not shots.is_empty():
			DirAccess.make_dir_recursive_absolute(shots)
			part.save_png(shots.path_join("dock_%s.png" % state))

	reset_dock()
