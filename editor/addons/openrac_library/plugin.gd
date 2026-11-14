@tool
extends EditorPlugin
## The OpenRAC assets plugin: the Assets dock (dock.gd) and placing what it gives. Editor tooling
## only. library.gd finds the assets and brings them into the project; this file puts them where
## the packer reads them:
## - a class this level has goes under Game/Mobys, Game/Ties or Game/Shrubs, as the game's own;
## - anything else goes under Game/StaticAssets as static scenery (editor/static_assets.py),
##   with the class scale its own scene gives it and collision on for mobys and ties.
## Godot's own drop (from the dock into the 3D view or the Scene dock) may put the node anywhere:
## it is moved to its group right after, keeping where it was dropped.
## behaviour_inspector.gd adds the Behaviour section to a moby's Inspector, foreign_inspector.gd the
## Imported class section to a moby that came with its own logic from another level.

const Dock = preload("dock.gd")
const Library = preload("library.gd")
const BehaviourInspector = preload("behaviour_inspector.gd")
const ForeignInspector = preload("foreign_inspector.gd")

## The groups under Game, by kind, for an asset the level has.
const GROUPS: Dictionary = {"mobys": "Mobys", "ties": "Ties", "shrubs": "Shrubs"}

## Static scenery (editor/static_assets.py GROUP).
const STATIC_GROUP: String = "StaticAssets"

## How far in front of the 3D view's camera Add places an asset, in the game's units.
const ADD_DISTANCE: float = 12.0

var dock: Dock
var behaviour_inspector: BehaviourInspector
var foreign_inspector: ForeignInspector

## What placed files are, by res:// path: the asset placed, until its node arrives.
var pending: Dictionary = {}

## Set while an asset waits for the editor to import it (ready_to_place()).
var waiting_for_import: bool = false

## Nodes added this frame, to put where they belong once the drop has finished.
var arrived: Array[Node] = []


func _enter_tree() -> void:
	dock = Dock.new()
	dock.plugin = self
	add_dock(dock)
	behaviour_inspector = BehaviourInspector.new()
	behaviour_inspector.undo_redo = get_undo_redo()
	add_inspector_plugin(behaviour_inspector)
	foreign_inspector = ForeignInspector.new()
	foreign_inspector.level_name = level_name
	add_inspector_plugin(foreign_inspector)
	scene_changed.connect(handle_scene_changed)
	get_tree().node_added.connect(handle_node_added)
	handle_scene_changed(EditorInterface.get_edited_scene_root())


func _exit_tree() -> void:
	if get_tree().node_added.is_connected(handle_node_added):
		get_tree().node_added.disconnect(handle_node_added)

	scene_changed.disconnect(handle_scene_changed)
	remove_inspector_plugin(behaviour_inspector)
	behaviour_inspector = null
	remove_inspector_plugin(foreign_inspector)
	foreign_inspector = null
	remove_dock(dock)
	dock.queue_free()
	dock = null


## Whether PATH is imported and can be placed now. When the editor is still bringing it in (a new
## folder takes a scan of its own), says so and calls AGAIN once it is imported (up to a minute).
func ready_to_place(path: String, again: Callable) -> bool:
	if waiting_for_import:
		dock.show_report("Still bringing the last asset in; try again in a moment.")
		return false

	if ResourceLoader.exists(path) or not FileAccess.file_exists(path):
		return true

	waiting_for_import = true
	dock.show_report("Importing %s; it is placed when ready..." % path.get_file())
	wait_for_import(path, again)
	return false


## Waits for PATH's import, then calls AGAIN (ready_to_place()).
func wait_for_import(path: String, again: Callable) -> void:
	var filesystem: EditorFileSystem = EditorInterface.get_resource_filesystem()

	if not filesystem.is_scanning():
		filesystem.scan()

	for _i in range(240):
		await get_tree().create_timer(0.25).timeout

		if ResourceLoader.exists(path) and not filesystem.is_scanning():
			waiting_for_import = false
			again.call()
			return

	waiting_for_import = false
	dock.show_report("The editor could not import %s." % path, true)


## The name of LEVEL of the edited level's disc, as the dock read it, or "".
func level_name(level: int) -> String:
	var at: Dictionary = here()

	for entry in dock.levels if dock != null else []:
		if entry["game"] == at.get("game") and entry["disc"] == at.get("disc") and int(entry["level"]) == level:
			return String(entry["name"])

	return ""


func handle_scene_changed(root: Node) -> void:
	if dock != null:
		dock.set_here(Library.here_of(root.scene_file_path) if root != null else {})

	if root != null:
		show_skins(root)


## Each moby with a skin shows the skin, not its class's own model (which the game does not draw):
## a "SkinLook" child read from the skin's file, which the scene does not save.
func show_skins(root: Node) -> void:
	var mobys: Node = root.get_node_or_null("Game/Mobys")

	if mobys == null:
		return

	for moby in mobys.get_children():
		if not moby.has_meta("rc1_skin") or moby.has_node("SkinLook"):
			continue

		var look: Node3D = Library.scene_of(String(moby.get_meta("rc1_skin")))

		if look == null:
			continue

		var model: Node3D = moby.get_node_or_null("Model") as Node3D

		if model != null:
			model.visible = false

		var scale: float = float(moby.get_meta("rc1_skin_class_scale", 1.0))
		look.name = "SkinLook"
		look.transform = Transform3D(Basis.from_scale(Vector3(scale, scale, scale)), Vector3.ZERO)
		moby.add_child(look)


## The level the edited scene is (library.gd here_of()).
func here() -> Dictionary:
	var root: Node = EditorInterface.get_edited_scene_root()
	return Library.here_of(root.scene_file_path) if root != null else {}


## Brings ASSET in for a drag: the res:// file Godot's drop will place, or empty (the dock shows
## why).
func prepare(asset: Dictionary) -> String:
	var error: Array = []
	dock.importing = true
	var path: String = Library.prepare(asset, here(), error)
	dock.importing = false

	if path.is_empty():
		dock.show_report(" ".join(error), true)
		return ""

	pending[path] = asset
	return path


# =============================================================================
# PLACING
# =============================================================================

## Adds ASSET in front of the 3D view's camera, as an undoable edit.
func add_in_view(asset: Dictionary) -> void:
	var root: Node = EditorInterface.get_edited_scene_root()
	var game: Node3D = root.get_node_or_null("Game") as Node3D if root != null else null

	if game == null:
		dock.show_report("Open a level scene first.", true)
		return

	var path: String = prepare(asset)

	if path.is_empty():
		return

	if not ready_to_place(path, add_in_view.bind(asset)):
		return

	var scene := ResourceLoader.load(path) as PackedScene

	if scene == null:
		dock.show_report("Godot could not load %s." % path, true)
		return

	var node := scene.instantiate() as Node3D
	var camera: Camera3D = EditorInterface.get_editor_viewport_3d(0).get_camera_3d()
	var at: Vector3 = camera.global_position - camera.global_basis.z * ADD_DISTANCE if camera != null else game.global_position
	var group: Node3D = group_for(game, asset)
	var make_group: bool = group.get_parent() == null
	node.name = free_name(group, base_name(asset))
	var undo_redo := get_undo_redo()
	undo_redo.create_action("Add %s from the Assets dock" % asset["name"], UndoRedo.MERGE_DISABLE, root)

	if make_group:
		undo_redo.add_do_method(game, "add_child", group)
		undo_redo.add_do_property(group, "owner", root)
		undo_redo.add_do_reference(group)

	undo_redo.add_do_method(group, "add_child", node)
	undo_redo.add_do_property(node, "owner", root)
	undo_redo.add_do_property(node, "global_position", at)
	add_metadata(undo_redo, node, asset, path)
	undo_redo.add_do_reference(node)
	undo_redo.add_undo_method(group, "remove_child", node)

	if make_group:
		undo_redo.add_undo_method(game, "remove_child", group)

	undo_redo.commit_action()
	pending.erase(path)
	EditorInterface.get_selection().clear()
	EditorInterface.get_selection().add_node(node)
	dock.show_report(placed_report(asset, node))


## Adds ASSET's model, as it is (animated), on the logic of HOST_CLASS, a moby class this level
## has: a moby of that class under Game/Mobys, which the game draws as ASSET's model (a skin:
## editor/skins.py packs it, the port draws it). Undoable.
func add_as(asset: Dictionary, host_class: int) -> void:
	var root: Node = EditorInterface.get_edited_scene_root()
	var game: Node3D = root.get_node_or_null("Game") as Node3D if root != null else null
	var level: Dictionary = here()

	if game == null or level.is_empty():
		dock.show_report("Open a level scene first.", true)
		return

	var host_path: String = String(level["classes"]["mobys"].get(host_class, ""))
	var error: Array = []
	dock.importing = true
	var skin_path: String = Library.prepare_skin(asset, error)
	dock.importing = false

	if host_path.is_empty() or skin_path.is_empty():
		dock.show_report(" ".join(error) if not error.is_empty() else "This level has no moby class %d." % host_class, true)
		return

	var host_scene := ResourceLoader.load(host_path) as PackedScene

	if host_scene == null:
		dock.show_report("Godot could not load %s." % host_path, true)
		return

	var host: Dictionary = {"name": "Class %d" % host_class, "scale": 1.0}
	Library.read_class_scene(host, FileAccess.get_file_as_string(host_path))
	var node := host_scene.instantiate() as Node3D
	var scale: float = float(asset["scale"])
	var camera: Camera3D = EditorInterface.get_editor_viewport_3d(0).get_camera_3d()
	var at: Vector3 = camera.global_position - camera.global_basis.z * ADD_DISTANCE if camera != null else game.global_position
	var group: Node3D = game.get_node_or_null(GROUPS["mobys"]) as Node3D
	var make_group: bool = group == null

	if make_group:
		group = Node3D.new()
		group.name = GROUPS["mobys"]

	var label: String = "%s_as_%s" % [String(host["name"]).capitalize().replace(" ", ""),
		String(asset["name"]).capitalize().replace(" ", "")]
	node.name = free_name(group, label.validate_node_name())
	var undo_redo := get_undo_redo()
	undo_redo.create_action("Add %s on the logic of %s" % [asset["name"], host["name"]], UndoRedo.MERGE_DISABLE, root)

	if make_group:
		undo_redo.add_do_method(game, "add_child", group)
		undo_redo.add_do_property(group, "owner", root)
		undo_redo.add_do_reference(group)

	undo_redo.add_do_method(group, "add_child", node)
	undo_redo.add_do_property(node, "owner", root)
	undo_redo.add_do_property(node, "global_position", at)
	undo_redo.add_do_method(node, "set_meta", "rc1_skin", skin_path)
	undo_redo.add_do_method(node, "set_meta", "rc1_skin_asset", "%s/%s/level_%02d/%s/%d" % [asset["game"],
		asset["disc"], int(asset["level"]), asset["kind"], int(asset["class"])])
	# The game scales the moby by its own class's scale: the skin's is that times this.
	undo_redo.add_do_method(node, "set_meta", "rc1_skin_scale", scale / maxf(float(host["scale"]), 0.0001))
	undo_redo.add_do_method(node, "set_meta", "rc1_skin_class_scale", scale)
	undo_redo.add_do_method(node, "set_meta", "rc1_skin_animations", [])
	undo_redo.add_do_method(self, "show_skins", root)
	undo_redo.add_do_reference(node)
	undo_redo.add_undo_method(group, "remove_child", node)

	if make_group:
		undo_redo.add_undo_method(game, "remove_child", group)

	undo_redo.commit_action()
	EditorInterface.get_selection().clear()
	EditorInterface.get_selection().add_node(node)
	dock.show_report(("Added %s: a %s of this level that looks like %s from %s. "
		+ "Set its animations in the Inspector (rc1_skin_animations). Pack, then play.") % [node.name,
		host["name"], asset["name"], asset["level_name"] if not String(asset["level_name"]).is_empty() else "its level"])


## Adds ASSET, a moby of another level of this disc, with its own class and logic: a moby of its
## class under Game/Mobys with rc1_foreign_level, which editor/foreign_classes.py packs and the
## port runs in its own level's program. Undoable.
func add_foreign(asset: Dictionary) -> void:
	var root: Node = EditorInterface.get_edited_scene_root()
	var game: Node3D = root.get_node_or_null("Game") as Node3D if root != null else null

	if game == null:
		dock.show_report("Open a level scene first.", true)
		return

	var refusal: String = Library.import_refusal(asset, here())

	if not refusal.is_empty():
		dock.show_report(refusal, true)
		return

	var error: Array = []
	dock.importing = true
	var path: String = Library.prepare(asset, here(), error)
	dock.importing = false

	if not path.is_empty() and not ready_to_place(path, add_foreign.bind(asset)):
		return

	var scene := ResourceLoader.load(path) as PackedScene if not path.is_empty() else null

	if scene == null:
		dock.show_report(" ".join(error) if not error.is_empty() else "Godot could not load %s." % path, true)
		return

	var node := scene.instantiate() as Node3D
	var camera: Camera3D = EditorInterface.get_editor_viewport_3d(0).get_camera_3d()
	var at: Vector3 = camera.global_position - camera.global_basis.z * ADD_DISTANCE if camera != null else game.global_position
	var group: Node3D = game.get_node_or_null(GROUPS["mobys"]) as Node3D
	var make_group: bool = group == null

	if make_group:
		group = Node3D.new()
		group.name = GROUPS["mobys"]

	node.name = free_name(group, base_name(asset))
	var undo_redo := get_undo_redo()
	undo_redo.create_action("Add %s with its own logic" % asset["name"], UndoRedo.MERGE_DISABLE, root)

	if make_group:
		undo_redo.add_do_method(game, "add_child", group)
		undo_redo.add_do_property(group, "owner", root)
		undo_redo.add_do_reference(group)

	undo_redo.add_do_method(group, "add_child", node)
	undo_redo.add_do_property(node, "owner", root)
	undo_redo.add_do_property(node, "global_position", at)
	# Drawn at its class's size, as in the game: the packed moby's own scale is this over that.
	var scale: float = float(asset["scale"])
	undo_redo.add_do_property(node, "scale", Vector3(scale, scale, scale))
	undo_redo.add_do_method(node, "set_meta", "rc1_foreign_level", int(asset["level"]))
	undo_redo.add_do_method(node, "set_meta", "rc1_foreign_class_scale", scale)
	undo_redo.add_do_reference(node)
	undo_redo.add_undo_method(group, "remove_child", node)

	if make_group:
		undo_redo.add_undo_method(game, "remove_child", group)

	undo_redo.commit_action()
	EditorInterface.get_selection().clear()
	EditorInterface.get_selection().add_node(node)
	dock.show_report("Added %s with its own logic from %s. Pack, then play." % [
		node.name, asset["level_name"] if not String(asset["level_name"]).is_empty() else "level %02d" % int(asset["level"])])


## The group ASSET goes in under GAME; a new, parentless Node3D for a static group not made yet.
func group_for(game: Node3D, asset: Dictionary) -> Node3D:
	var full: bool = Library.status(asset, here()) == Library.FULL
	var name: String = GROUPS[asset["kind"]] if full else STATIC_GROUP
	var group: Node3D = game.get_node_or_null(name) as Node3D

	if group == null:
		group = Node3D.new()
		group.name = name

	return group


## A node name of the asset: Moby_0012-like for the game's own (pack.py reads the class from the
## scene, not the name), else its name in the library ("Sentrybot_rac1_pal_L03").
func base_name(asset: Dictionary) -> String:
	var name: String = String(asset["name"]).capitalize().replace(" ", "").validate_node_name()

	if Library.status(asset, here()) == Library.FULL:
		return name

	return "%s_%s_%s_L%02d" % [name, asset["game"], asset["disc"], int(asset["level"])]


func free_name(group: Node, base: String) -> String:
	var name: String = base
	var suffix: int = 2

	while group.has_node(NodePath(name)):
		name = "%s_%d" % [base, suffix]
		suffix += 1

	return name


## What a static asset carries: its source, the class scale and whether it is solid.
func add_metadata(undo_redo: EditorUndoRedoManager, node: Node3D, asset: Dictionary, path: String) -> void:
	if not path.begins_with(Library.LIBRARY_DIR + "/"):
		return

	var scale: float = float(asset["scale"])
	undo_redo.add_do_property(node, "scale", Vector3(scale, scale, scale))
	undo_redo.add_do_method(node, "set_meta", "rc1_static_collision", asset["kind"] != "shrubs")
	undo_redo.add_do_method(node, "set_meta", "rc1_asset", "%s/%s/level_%02d/%s/%d" % [asset["game"], asset["disc"],
		int(asset["level"]), asset["kind"], int(asset["class"])])


func placed_report(asset: Dictionary, node: Node) -> String:
	if Library.status(asset, here()) == Library.FULL:
		return "Added %s, a %s of this level. Move it, then pack." % [node.name, asset["kind"].trim_suffix("s")]

	return ("Added %s as static scenery from %s, no logic. "
		+ "Collision is %s (rc1_static_collision). Pack bakes it in.") % [node.name,
		asset["level_name"] if not String(asset["level_name"]).is_empty() else "its level",
		"on" if asset["kind"] != "shrubs" else "off"]


# =============================================================================
# GODOT'S DROP
# =============================================================================

## A node of a file the dock prepared: put it where it belongs once the drop has finished.
func handle_node_added(node: Node) -> void:
	if pending.is_empty() or not node.scene_file_path in pending:
		return

	var root: Node = EditorInterface.get_edited_scene_root()

	if root == null or not root.is_ancestor_of(node):
		return

	arrived.append(node)

	if arrived.size() == 1:
		settle.call_deferred()


## Moves each dropped node to its group under Game, keeping where it was dropped, as one undoable
## edit after Godot's own.
func settle() -> void:
	var nodes: Array[Node] = arrived.duplicate()
	arrived.clear()
	var root: Node = EditorInterface.get_edited_scene_root()
	var game: Node3D = root.get_node_or_null("Game") as Node3D if root != null else null

	if game == null:
		return

	var undo_redo := get_undo_redo()
	undo_redo.create_action("Place assets from the Assets dock", UndoRedo.MERGE_DISABLE, root)
	var made: Dictionary = {}
	var moved: Array[Node] = []

	for node in nodes:
		if not is_instance_valid(node) or not node is Node3D or not node.is_inside_tree():
			continue

		var asset: Dictionary = pending.get(node.scene_file_path, {})

		if asset.is_empty():
			continue

		var group: Node3D = made.get(GROUPS.get(asset["kind"], "") if Library.status(asset, here()) == Library.FULL
			else STATIC_GROUP, null)

		if group == null:
			group = group_for(game, asset)

			if group.get_parent() == null:
				undo_redo.add_do_method(game, "add_child", group)
				undo_redo.add_do_property(group, "owner", root)
				undo_redo.add_do_reference(group)
				undo_redo.add_undo_method(game, "remove_child", group)

			made[String(group.name)] = group

		var old_parent: Node = node.get_parent()
		var at: Transform3D = (node as Node3D).global_transform

		if old_parent != group:
			var name: String = free_name(group, base_name(asset))
			undo_redo.add_do_method(old_parent, "remove_child", node)
			undo_redo.add_do_property(node, "name", name)
			undo_redo.add_do_method(group, "add_child", node)
			undo_redo.add_do_property(node, "owner", root)
			undo_redo.add_do_property(node, "global_transform", at)
			undo_redo.add_undo_method(group, "remove_child", node)
			undo_redo.add_undo_property(node, "name", node.name)
			undo_redo.add_undo_method(old_parent, "add_child", node)
			undo_redo.add_undo_property(node, "owner", root)
			undo_redo.add_undo_property(node, "global_transform", at)

		add_metadata(undo_redo, node, asset, node.scene_file_path)
		moved.append(node)

	undo_redo.commit_action()

	if not moved.is_empty():
		var asset: Dictionary = pending.get(moved[0].scene_file_path, {})
		dock.show_report(placed_report(asset, moved[0]))
