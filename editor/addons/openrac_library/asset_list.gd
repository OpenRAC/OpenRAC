@tool
extends ItemList
## The Assets dock's list: dragging an item brings its asset into the project (plugin.gd
## prepare()) and hands Godot the file to drop, as the FileSystem dock does, so the 3D view and
## the Scene dock place it.

const Library = preload("library.gd")

var dock: Control


func _get_drag_data(at_position: Vector2) -> Variant:
	# The item the press selected: Godot works the drag's start point out from the mouse's moves,
	# which can land on the next row when the editor is slow.
	var selected := get_selected_items()
	var index: int = selected[0] if not selected.is_empty() else get_item_at_position(at_position, true)

	if index < 0:
		return null

	var asset: Dictionary = dock.asset_at(index)

	if asset.is_empty():
		return null

	# Nothing is brought into the project for a scene it cannot go in.
	if dock.here.is_empty():
		dock.show_report("Open a level scene first.", true)
		return null

	var path: String = dock.plugin.prepare(asset)

	if path.is_empty():
		return null

	# The 3D view takes only an imported file: say so instead of a drop that does nothing.
	if not Library.imported(path):
		dock.show_report("Bringing %s in; drag it again in a moment." % asset["name"])
		dock.plugin.wait_until_imported(path, String(asset["name"]))
		return null

	dock.plugin.turn_drops_upright(Library.front_for(asset))

	# Its picture and name under the pointer, as the list shows it.
	var preview := VBoxContainer.new()
	var icon: Texture2D = get_item_icon(index)

	if icon != null:
		var picture := TextureRect.new()
		picture.texture = icon
		picture.custom_minimum_size = Vector2(64, 64)
		picture.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		picture.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		preview.add_child(picture)

	var label := Label.new()
	label.text = String(asset["name"])
	preview.add_child(label)
	set_drag_preview(preview)
	return {"type": "files", "files": PackedStringArray([path]), "from": self}


func _notification(what: int) -> void:
	if what == NOTIFICATION_DRAG_END and dock != null and dock.plugin != null:
		dock.plugin.stop_turning_drops()
