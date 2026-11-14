@tool
extends ItemList
## The Assets dock's list: dragging an item brings its asset into the project (plugin.gd
## prepare()) and hands Godot the file to drop, as the FileSystem dock does, so the 3D view and
## the Scene dock place it.

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

	var path: String = dock.plugin.prepare(asset)

	if path.is_empty():
		return null

	var preview := Label.new()
	preview.text = String(asset["name"])
	set_drag_preview(preview)
	return {"type": "files", "files": PackedStringArray([path]), "from": self}
