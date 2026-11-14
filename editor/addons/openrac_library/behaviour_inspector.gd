@tool
extends EditorInspectorPlugin
## The Behaviour section in the Inspector of a moby under Game/Mobys: a preset (spin, bob, patrol,
## orbit, face the hero, follow the hero) and its numbers, kept as rc1_behaviour metadata, which
## editor/behaviours.py packs and the editor-runtime mod plays. Undoable.

## Each preset's numbers, their defaults and steps (editor/behaviours.py PRESETS), and a hint.
const PRESETS: Dictionary = {
	"spin": {"speed": [90.0, -3600.0, 3600.0, "degrees a second"]},
	"bob": {"height": [1.0, -100.0, 100.0, "units"], "period": [2.0, 0.05, 600.0, "seconds"]},
	"patrol": {"distance": [8.0, 0.0, 1000.0, "units"], "speed": [4.0, 0.0, 200.0, "units a second"]},
	"orbit": {"radius": [4.0, 0.0, 1000.0, "units"], "speed": [45.0, -3600.0, 3600.0, "degrees a second"]},
	"face_hero": {"speed": [180.0, 0.0, 3600.0, "degrees a second"]},
	"follow": {"speed": [4.0, 0.0, 200.0, "units a second"], "distance": [3.0, 0.0, 1000.0, "units"],
		"range": [30.0, 0.0, 10000.0, "units; 0: always"]},
}

const LABELS: Dictionary = {"spin": "Spin", "bob": "Bob", "patrol": "Patrol", "orbit": "Orbit",
	"face_hero": "Face the hero", "follow": "Follow the hero"}

var undo_redo: EditorUndoRedoManager


func _can_handle(object: Object) -> bool:
	return is_moby(object as Node)


## Whether NODE is a moby of a level scene: a child of Game/Mobys.
static func is_moby(node: Node) -> bool:
	if node == null or not node is Node3D:
		return false

	var group: Node = node.get_parent()
	return group != null and group.name == "Mobys" and group.get_parent() != null and group.get_parent().name == "Game"


func _parse_begin(object: Object) -> void:
	add_custom_control(section(object as Node3D))


## The section for NODE.
func section(node: Node3D) -> Control:
	var box := VBoxContainer.new()
	var title := Label.new()
	title.text = "Behaviour"
	title.tooltip_text = ("A simple move the editor mod plays in the game. The moby's own logic stops while it runs. "
		+ "Pack, then play with the mod on.")
	title.mouse_filter = Control.MOUSE_FILTER_PASS
	box.add_child(title)

	var choice := OptionButton.new()
	choice.add_item("None (its own logic)")
	choice.set_item_metadata(0, "")
	var preset: String = String(node.get_meta("rc1_behaviour", ""))

	for key: String in PRESETS:
		choice.add_item(LABELS[key])
		choice.set_item_metadata(choice.item_count - 1, key)

		if key == preset:
			choice.select(choice.item_count - 1)

	choice.item_selected.connect(func(index: int) -> void: set_preset(node, String(choice.get_item_metadata(index))))
	box.add_child(choice)

	if PRESETS.has(preset):
		var numbers: Dictionary = PRESETS[preset]

		for key: String in numbers:
			var spec: Array = numbers[key]
			box.add_child(number_row(node, key, "%s (%s)" % [key.capitalize(), spec[3]],
				float(node.get_meta("rc1_behaviour_" + key, spec[0])), spec[1], spec[2], 0.05))

		box.add_child(number_row(node, "sequence", "Animation (the class's sequence; -1: its own)",
			float(node.get_meta("rc1_behaviour_sequence", -1)), -1.0, 255.0, 1.0))

	return box


func number_row(node: Node3D, key: String, label: String, value: float, low: float, high: float, step: float) -> Control:
	var row := HBoxContainer.new()
	var name := Label.new()
	name.text = label
	name.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	name.clip_text = true
	name.tooltip_text = label
	name.mouse_filter = Control.MOUSE_FILTER_PASS
	row.add_child(name)
	var spin := SpinBox.new()
	spin.min_value = low
	spin.max_value = high
	spin.step = step
	spin.value = value
	spin.custom_minimum_size.x = 90
	spin.value_changed.connect(func(number: float) -> void: set_number(node, key, number))
	row.add_child(spin)
	return row


## Gives NODE the preset PRESET ("" for none), its numbers kept when they are the new preset's too.
func set_preset(node: Node3D, preset: String) -> void:
	var before: Dictionary = behaviour_meta(node)
	var after: Dictionary = {}

	if not preset.is_empty():
		after["rc1_behaviour"] = preset

		for key: String in PRESETS[preset]:
			after["rc1_behaviour_" + key] = before.get("rc1_behaviour_" + key, PRESETS[preset][key][0])

		after["rc1_behaviour_sequence"] = before.get("rc1_behaviour_sequence", -1)

	commit(node, "Set the behaviour of %s" % node.name, before, after, true)


func set_number(node: Node3D, key: String, value: float) -> void:
	var before: Dictionary = behaviour_meta(node)
	var after: Dictionary = before.duplicate()
	after["rc1_behaviour_" + key] = int(value) if key == "sequence" else value
	commit(node, "Set %s of %s's behaviour" % [key, node.name], before, after, false)


## NODE's rc1_behaviour metadata.
static func behaviour_meta(node: Node) -> Dictionary:
	var out: Dictionary = {}

	for key: StringName in node.get_meta_list():
		if String(key) == "rc1_behaviour" or String(key).begins_with("rc1_behaviour_"):
			out[String(key)] = node.get_meta(key)

	return out


## Replaces NODE's behaviour metadata BEFORE by AFTER, undoably (or at once without an editor);
## REBUILD: the section shows other numbers after it (an undo always rebuilds it).
func commit(node: Node3D, action: String, before: Dictionary, after: Dictionary, rebuild: bool) -> void:
	if undo_redo == null:
		replace_meta(node, after)
		return

	undo_redo.create_action(action, UndoRedo.MERGE_ENDS, node)
	undo_redo.add_do_method(self, "replace_meta", node, after)
	undo_redo.add_undo_method(self, "replace_meta", node, before)

	if rebuild:
		undo_redo.add_do_method(self, "refresh", node)

	undo_redo.add_undo_method(self, "refresh", node)
	undo_redo.commit_action()


static func replace_meta(node: Node, meta: Dictionary) -> void:
	for key: String in behaviour_meta(node):
		node.remove_meta(key)

	for key: String in meta:
		node.set_meta(key, meta[key])


## Shows NODE's section again (a preset brings other numbers).
func refresh(node: Node) -> void:
	node.notify_property_list_changed()
