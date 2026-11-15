@tool
extends EditorInspectorPlugin
## The Imported class section in the Inspector of a moby that came with its own logic from another
## level (rc1_foreign_level metadata, the Assets dock's "Add with its own logic"): where its logic
## runs from, and what to expect of it. editor/foreign_classes.py packs it.

const BehaviourInspector = preload("behaviour_inspector.gd")

## Where the switch's changes go, so Ctrl+Z takes them back (plugin.gd sets it).
var undo_redo: EditorUndoRedoManager

## A level's name by its number, or "" (the dock knows the levels read).
var level_name: Callable = func(_level: int) -> String: return ""


func _can_handle(object: Object) -> bool:
	var node := object as Node
	return BehaviourInspector.is_moby(node) and node.has_meta("rc1_foreign_level")


func _parse_begin(object: Object) -> void:
	add_custom_control(section(object as Node3D))


## The section for NODE.
func section(node: Node3D) -> Control:
	var panel := PanelContainer.new()
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.55, 0.78, 1.0, 0.12)
	style.border_color = Color(0.55, 0.78, 1.0, 0.6)
	style.border_width_left = 3
	style.content_margin_left = 8
	style.content_margin_right = 6
	style.content_margin_top = 4
	style.content_margin_bottom = 4
	panel.add_theme_stylebox_override("panel", style)

	var text := RichTextLabel.new()
	text.bbcode_enabled = true
	text.fit_content = true
	text.scroll_active = false
	text.custom_minimum_size = Vector2(160, 0)
	var level: int = int(node.get_meta("rc1_foreign_level", -1))
	var name: String = String(level_name.call(level))
	text.text = ("[b]Imported class[/b]\nRuns its own logic from level %02d%s. "
		+ "[color=#9aa4b2]Its paths, zones, cameras and needed mobys come with it. "
		+ "Sounds and texts are this level's. Costs about 0.5 ms a frame. Pack, then play.[/color]") % [level, "" if name.is_empty() else " (%s)" % name]
	var box := VBoxContainer.new()
	box.add_child(text)
	box.add_child(partial_toggle(node))
	panel.add_child(box)
	return panel


## The switch for a level short of class slots (rc1_foreign_partial): by default the pack refuses a
## class whose classes do not all fit, and names the levels where they do; switched on, the class
## comes without the ones it makes that do not fit (shots, effects, parts it adds), which then do
## not appear.
func partial_toggle(node: Node) -> Control:
	var check := CheckBox.new()
	check.text = "Import without what does not fit"
	check.tooltip_text = ("A level has room for only so many classes. If this class and what it makes do not fit, "
		+ "the pack refuses it. Turn this on to import it anyway: the extra classes it makes "
		+ "(shots, effects, parts) will be missing.")
	check.button_pressed = bool(node.get_meta("rc1_foreign_partial", false))
	check.toggled.connect(func(on: bool) -> void:
		if undo_redo == null:
			return
		undo_redo.create_action("Import %s %s" % [node.name, "without what does not fit" if on else "whole"])
		if on:
			undo_redo.add_do_method(node, "set_meta", "rc1_foreign_partial", true)
			undo_redo.add_undo_method(node, "remove_meta", "rc1_foreign_partial")
		else:
			undo_redo.add_do_method(node, "remove_meta", "rc1_foreign_partial")
			undo_redo.add_undo_method(node, "set_meta", "rc1_foreign_partial", true)
		# The Inspector draws the switch again, as it now is.
		undo_redo.add_do_method(node, "notify_property_list_changed")
		undo_redo.add_undo_method(node, "notify_property_list_changed")
		undo_redo.commit_action())
	return check
