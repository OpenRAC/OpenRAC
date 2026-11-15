@tool
extends EditorDock
## The Assets dock: the mobys, ties and shrubs of every level of every disc read (library.gd), to
## drag onto the level or add in front of the 3D view's camera (plugin.gd places them). Each
## shows what it does here: green for a class the level has (the game's own, with its logic), blue
## for another level's moby that can come with its own logic, white for the rest (its look only, as
## static scenery, or on a class's logic).

const Library = preload("library.gd")
const AssetList = preload("asset_list.gd")

## The filter's choices.
const SHOW_ALL: int = 0
const SHOW_HERE: int = 1
const SHOW_THIS_GAME: int = 2
const SHOW_OTHER_GAMES: int = 3
const SHOW_IMPORTS: int = 4

## The names of assets this level has (placed with the game's logic).
const FULL_COLOUR: Color = Color(0.55, 0.9, 0.55)
## Another level's moby that can come with its own logic (library.gd import_refusal()).
const IMPORT_COLOUR: Color = Color(0.55, 0.78, 1.0)
## The rest: its look only.
const LOOK_COLOUR: Color = Color(0.92, 0.92, 0.92)

## Rows the level tree grows to before it scrolls (fit_tree()).
const TREE_ROWS: int = 8

## Previews made at most this many a frame, the rest in later frames.
const PREVIEWS_PER_FRAME: int = 2

## Previews kept on disk, so the dock shows them at once next time.
const PREVIEW_CACHE: String = "user://asset_previews"

var plugin: EditorPlugin

## The level the edited scene is (library.gd here_of()), or empty.
var here: Dictionary = {}

## Every level found: library.gd levels_in()'s entries.
var levels: Array = []

## Each level's assets once read, by its folder.
var assets_by_level: Dictionary = {}

## The assets the list shows, in its order.
var shown: Array = []

## Previews already made, by model file.
var previews: Dictionary = {}

## Assets whose model is still to preview, one each.
var preview_queue: Array = []

## The models in preview_queue, to find one at once.
var queued: Dictionary = {}

## Set by plugin.gd while it brings files in.
var importing: bool = false

## Shown until an asset's preview is made, so the list is a grid of pictures from the start.
var placeholder: Texture2D

var search: LineEdit
var filter: OptionButton
var planet: OptionButton
var tree: Tree
var list: AssetList
var report: Label
var add_button: Button
var as_button: Button
var own_button: Button
var refresh_button: Button
var details: RichTextLabel
## What the list says when it has no assets to show.
var note: Label
var classes_menu: PopupMenu


func _init() -> void:
	title = "Assets"
	default_slot = EditorDock.DOCK_SLOT_LEFT_BR
	# Wide enough for two columns of assets and a button's label on one line or two, never wider:
	# the dock's long labels wrap instead.
	custom_minimum_size.x = 250.0 * EditorInterface.get_editor_scale()
	var box := VBoxContainer.new()
	box.size_flags_vertical = Control.SIZE_EXPAND_FILL
	add_child(box)

	var row := HBoxContainer.new()
	box.add_child(row)
	search = LineEdit.new()
	search.placeholder_text = "Name or class number"
	search.clear_button_enabled = true
	search.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	# Refilled once typing pauses: a list of every level takes a moment to fill.
	var typing := Timer.new()
	typing.one_shot = true
	typing.wait_time = 0.2
	typing.timeout.connect(refill)
	add_child(typing)
	search.text_changed.connect(func(_text: String) -> void: typing.start())
	search.text_submitted.connect(func(_text: String) -> void:
		typing.stop()
		refill())
	row.add_child(search)
	refresh_button = Button.new()
	refresh_button.tooltip_text = "Look for levels again (after reading another disc)."
	refresh_button.flat = true
	refresh_button.pressed.connect(rescan)
	row.add_child(refresh_button)

	filter = OptionButton.new()
	# A long choice is cut short rather than making the whole dock that wide.
	filter.fit_to_longest_item = false
	filter.clip_text = true
	filter.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	filter.add_item("Everything", SHOW_ALL)
	filter.add_item("In this level (with the game's logic)", SHOW_HERE)
	filter.add_item("From other levels, with their own logic", SHOW_IMPORTS)
	filter.add_item("This game", SHOW_THIS_GAME)
	filter.add_item("Other games", SHOW_OTHER_GAMES)
	filter.item_selected.connect(func(_index: int) -> void: refill())
	box.add_child(filter)
	planet = OptionButton.new()
	planet.tooltip_text = "Only the assets of one planet (each level's name)."
	planet.fit_to_longest_item = false
	planet.clip_text = true
	planet.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	planet.item_selected.connect(func(_index: int) -> void:
		# A level picked in the tree is another planet's: the planet wins.
		var picked: TreeItem = tree.get_selected()
		if picked != null and picked.get_metadata(0).has("level"):
			tree.deselect_all()
		refill())
	box.add_child(planet)

	# What each colour means, one line each.
	var legend := VBoxContainer.new()
	legend.add_theme_constant_override("separation", 0)
	for entry in [[FULL_COLOUR, "In this level: works as in the game."],
			[IMPORT_COLOUR, "From another level: can bring its logic."],
			[LOOK_COLOUR, "Look only, or on another class's logic."]]:
		var line := Label.new()
		line.text = "\u25A0 " + entry[1]
		line.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		line.custom_minimum_size = Vector2(160, 0)
		line.add_theme_color_override("font_color", entry[0])
		line.add_theme_font_size_override("font_size", 12)
		legend.add_child(line)
	box.add_child(legend)

	var split := VSplitContainer.new()
	split.size_flags_vertical = Control.SIZE_EXPAND_FILL
	box.add_child(split)
	tree = Tree.new()
	tree.hide_root = true
	tree.item_selected.connect(func() -> void:
		# A level picked here is its own planet.
		if tree.get_selected().get_metadata(0).has("level"):
			planet.select(0)
		refill())
	tree.item_collapsed.connect(func(_item: TreeItem) -> void: fit_tree())
	split.add_child(tree)

	list = AssetList.new()
	list.dock = self
	list.size_flags_vertical = Control.SIZE_EXPAND_FILL
	list.icon_mode = ItemList.ICON_MODE_TOP
	list.max_columns = 0
	list.same_column_width = true
	list.fixed_column_width = 96
	list.fixed_icon_size = Vector2i(80, 80)
	list.max_text_lines = 2
	list.item_activated.connect(func(index: int) -> void: add(index))
	list.item_selected.connect(func(index: int) -> void:
		show_selected()
		# Brought into the project as soon as it is picked: Godot imports it in the background,
		# so a drag right after finds it ready instead of being lost to the import.
		var asset: Dictionary = asset_at(index)
		if not asset.is_empty() and not here.is_empty() and plugin != null:
			plugin.prepare.call_deferred(asset))
	split.add_child(list)
	note = Label.new()
	note.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT, Control.PRESET_MODE_MINSIZE, 8)
	note.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	note.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	note.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	note.mouse_filter = Control.MOUSE_FILTER_IGNORE
	note.visible = false
	list.add_child(note)

	# The selected asset: what it is and what each way of adding it does.
	details = RichTextLabel.new()
	details.bbcode_enabled = true
	details.fit_content = true
	details.scroll_active = false
	details.selection_enabled = true
	details.custom_minimum_size = Vector2(160, 0)
	box.add_child(details)

	var buttons := VBoxContainer.new()
	buttons.add_theme_constant_override("separation", 2)
	box.add_child(buttons)
	add_button = action_button(buttons, "Add in front of the camera",
		func() -> void: with_selected(func(asset: Dictionary) -> void: plugin.add_in_view(asset)))
	own_button = action_button(buttons, "Add with its own logic",
		func() -> void: with_selected(func(asset: Dictionary) -> void: plugin.add_foreign(asset)))
	as_button = action_button(buttons, "Add on a class's logic...", show_classes)
	classes_menu = PopupMenu.new()
	classes_menu.id_pressed.connect(func(id: int) -> void:
		with_selected(func(asset: Dictionary) -> void: plugin.add_as(asset, id)))
	add_child(classes_menu)

	report = Label.new()
	report.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	report.custom_minimum_size = Vector2(160, 0)
	box.add_child(report)
	show_selected()


## A button of the actions under the list, full width, its text left.
func action_button(parent: Control, text: String, pressed: Callable) -> Button:
	var button := Button.new()
	button.text = text
	button.alignment = HORIZONTAL_ALIGNMENT_LEFT
	# A long label wraps instead of widening the dock.
	button.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	button.disabled = true
	button.pressed.connect(pressed)
	parent.add_child(button)
	return button


## Calls DO with the list's selected asset, if any.
func with_selected(action: Callable) -> void:
	var selected: PackedInt32Array = list.get_selected_items()

	if not selected.is_empty() and not asset_at(selected[0]).is_empty():
		action.call(asset_at(selected[0]))


## The editor's icons, once the dock is in the editor (none at _init).
func _notification(what: int) -> void:
	if what != NOTIFICATION_THEME_CHANGED or refresh_button == null:
		return

	refresh_button.icon = get_theme_icon("Reload", "EditorIcons")
	add_button.icon = get_theme_icon("Add", "EditorIcons")
	own_button.icon = get_theme_icon("Script", "EditorIcons")
	as_button.icon = get_theme_icon("Skeleton3D", "EditorIcons")
	placeholder = waiting_picture(get_theme_color("font_disabled_color", "Editor"))
	note.add_theme_color_override("font_color", get_theme_color("font_disabled_color", "Editor"))
	# Room for one whole row of assets, however small the dock: a cut-off row is hard to pick.
	var font: Font = list.get_theme_font("font")
	var line: float = font.get_height(list.get_theme_font_size("font_size")) + list.get_theme_constant("line_separation")
	list.custom_minimum_size.y = ceilf(list.fixed_icon_size.y + list.get_theme_constant("icon_margin")
		+ list.max_text_lines * line + list.get_theme_constant("v_separation")
		+ list.get_theme_stylebox("panel").get_minimum_size().y)


## Shows what the selected asset is and which ways of adding it work here, and why not the others.
func show_selected() -> void:
	var selected: PackedInt32Array = list.get_selected_items() if list != null else PackedInt32Array()
	var asset: Dictionary = asset_at(selected[0]) if not selected.is_empty() else {}

	if asset.is_empty():
		details.text = "[color=#%s]Pick an asset.[/color]" % LOOK_COLOUR.darkened(0.3).to_html(false)
		for button in [add_button, own_button, as_button]:
			button.disabled = true
			button.tooltip_text = "Pick an asset first."
		return

	var status: String = Library.status(asset, here)
	var refusal: String = Library.import_refusal(asset, here)
	var colour: Color = FULL_COLOUR if status == Library.FULL else (IMPORT_COLOUR if refusal.is_empty() else LOOK_COLOUR)
	var level_name: String = String(asset["level_name"])
	details.text = "[b][color=#%s]%s[/color][/b]  class %d\n%s %s, level %02d%s" % [colour.to_html(false),
		asset["name"], int(asset["class"]), asset["game"], asset["disc"], int(asset["level"]),
		"" if level_name.is_empty() else " (%s)" % level_name]

	add_button.disabled = here.is_empty()
	add_button.text = "Add in front of the camera" + (" (with its logic)" if status == Library.FULL else " (look only)")
	add_button.tooltip_text = ("Open a level scene first." if here.is_empty() else
		"The game's own %s of this level, with its logic." % asset["kind"].trim_suffix("s") if status == Library.FULL else
		"Static scenery: look only, no logic. Or drag it into the 3D view.")
	own_button.disabled = not refusal.is_empty()
	own_button.tooltip_text = (refusal if not refusal.is_empty() else
		"Brings its class and logic from its level, with the paths, zones and mobys it needs. "
		+ "Pack, then play. Costs about 0.5 ms a frame each.")
	as_button.disabled = not can_add_as(asset)
	as_button.tooltip_text = ("Pick a class of this level: it runs that class's logic and looks like this model."
		if can_add_as(asset) else "Only for a moby this level lacks, in a level with mobys.")


func _ready() -> void:
	rescan()


func _process(_delta: float) -> void:
	# Not while files are brought in: importing runs frames of its own, and a preview made inside
	# one of them can take the editor down.
	if importing:
		return

	for _i in range(PREVIEWS_PER_FRAME):
		if preview_queue.is_empty():
			return

		make_preview(preview_queue.pop_front())


## The edited scene's level changed.
func set_here(level: Dictionary) -> void:
	here = level
	refill()


## Says how the last action went: in the warning colour when it did not work.
func show_report(text: String, failed: bool = false) -> void:
	report.text = text

	if failed:
		report.add_theme_color_override("font_color", EditorInterface.get_editor_theme().get_color("warning_color", "Editor"))
	else:
		report.remove_theme_color_override("font_color")


# =============================================================================
# THE LEVELS
# =============================================================================

## Finds the levels again and lists them by game, disc and level.
func rescan() -> void:
	levels.clear()
	assets_by_level.clear()
	var folders: Dictionary = Library.game_folders(ProjectSettings.globalize_path("res://"), Library.extra_folders())

	for game in folders:
		for folder in folders[game]:
			levels.append_array(Library.levels_in(game, folder))

	tree.clear()
	var root: TreeItem = tree.create_item()
	var items: Dictionary = {}

	for level in levels:
		var game_key: String = String(level["game"])
		var disc_key: String = game_key + "/" + String(level["disc"])

		if not items.has(game_key):
			var game_item: TreeItem = tree.create_item(root)
			game_item.set_text(0, game_key)
			game_item.set_metadata(0, {"game": game_key})
			items[game_key] = game_item

		if not items.has(disc_key):
			var disc_item: TreeItem = tree.create_item(items[game_key])
			disc_item.set_text(0, String(level["disc"]))
			disc_item.set_metadata(0, {"game": game_key, "disc": level["disc"]})
			disc_item.collapsed = true
			items[disc_key] = disc_item

		var level_item: TreeItem = tree.create_item(items[disc_key])
		var name: String = String(level["name"])
		level_item.set_text(0, "Level %02d%s" % [int(level["level"]), "" if name.is_empty() else ": " + name])
		level_item.set_metadata(0, {"level": level})

		for kind in Library.KINDS:
			var kind_item: TreeItem = tree.create_item(level_item)
			kind_item.set_text(0, kind.capitalize())
			kind_item.set_metadata(0, {"level": level, "kind": kind})

		level_item.collapsed = true

	fill_planets()
	fit_tree()
	show_report("Levels found: %d, in games: %d." % [levels.size(), folders.size()] if not levels.is_empty()
		else "No levels found. Read a disc's levels from the launcher's Level editor tab first.")
	refill()


## The assets of LEVEL, read once.
func assets_of(level: Dictionary) -> Array:
	var key: String = String(level["dir"])

	if not assets_by_level.has(key):
		assets_by_level[key] = Library.assets_of(level)

	return assets_by_level[key]


# =============================================================================
# THE LIST
# =============================================================================

## Fills the list from the tree's selection, the search and the filter. With nothing selected, a
## search looks in every level (read as it goes); without a search, the edited level is shown.
func refill() -> void:
	if list == null:
		return

	list.clear()
	shown.clear()
	preview_queue.clear()
	queued.clear()
	show_selected()
	var selected: TreeItem = tree.get_selected()
	var scope: Dictionary = selected.get_metadata(0) if selected != null else {}
	var words: String = search.text.strip_edges().to_lower()
	var planet_name: String = chosen_planet()
	var candidates: Array = []

	for level in levels:
		if scope.has("level") and scope["level"]["dir"] != level["dir"]:
			continue

		if scope.has("game") and level["game"] != scope["game"]:
			continue

		if scope.has("disc") and level["disc"] != scope["disc"]:
			continue

		if not planet_name.is_empty() and String(level["name"]) != planet_name:
			continue

		# Nothing chosen, no search and a filter that this level's own can pass: those.
		if scope.is_empty() and words.is_empty() and planet_name.is_empty() and not is_here(level) \
				and filter.get_selected_id() in [SHOW_ALL, SHOW_HERE, SHOW_THIS_GAME]:
			continue

		for asset in assets_of(level):
			if scope.has("kind") and asset["kind"] != scope["kind"]:
				continue

			candidates.append(asset)

	var seen: Dictionary = {}

	for asset in candidates:
		if not matches(asset, words):
			continue

		var status: String = Library.status(asset, here)

		match filter.get_selected_id():
			SHOW_HERE:
				if status != Library.FULL:
					continue
			SHOW_IMPORTS:
				if not Library.import_refusal(asset, here).is_empty():
					continue
			SHOW_THIS_GAME:
				if here.is_empty() or asset["game"] != here["game"]:
					continue
			SHOW_OTHER_GAMES:
				if not here.is_empty() and asset["game"] == here["game"]:
					continue

		# A class shown once per game unless a level was chosen: the first level that has it.
		var key: String = "%s/%s/%d" % [asset["game"], asset["kind"], int(asset["class"])]

		if not scope.has("level") and seen.has(key):
			continue

		seen[key] = true
		var index: int = list.add_item(wrapped(String(asset["name"])))

		if status == Library.FULL:
			list.set_item_custom_fg_color(index, FULL_COLOUR)
		elif Library.import_refusal(asset, here).is_empty():
			list.set_item_custom_fg_color(index, IMPORT_COLOUR)

		list.set_item_tooltip(index, tooltip(asset, status))
		list.set_item_metadata(index, shown.size())
		shown.append(asset)

		var cached: Texture2D = cached_preview(asset)

		if cached != null:
			list.set_item_icon(index, cached)
		else:
			list.set_item_icon(index, placeholder)

			if not queued.has(asset["model"]):
				queued[asset["model"]] = true
				preview_queue.append(asset)

	# Says so instead of an empty grid, across the whole list (a grid cell would break its words).
	if shown.is_empty():
		var looked: bool = not words.is_empty() or filter.selected > 0 or not planet_name.is_empty() or selected != null
		note.text = ("No assets match." if looked else "No assets here." if not here.is_empty()
			else "Open a level scene, search, or pick a level above.")
	note.visible = shown.is_empty()


## The planet filter's choices: every level's name once, in the order of the levels, keeping the
## one chosen when it is still there.
func fill_planets() -> void:
	var was: String = chosen_planet()
	planet.clear()
	planet.add_item("Every planet")
	var names: PackedStringArray = []

	for level in levels:
		var name: String = String(level["name"])

		if not name.is_empty() and not names.has(name):
			names.append(name)

	for name in names:
		planet.add_item(name)

		if name == was:
			planet.select(planet.item_count - 1)

	planet.visible = not names.is_empty()


## The planet the filter shows, or "" for every one.
func chosen_planet() -> String:
	return planet.get_item_text(planet.selected) if planet != null and planet.selected > 0 else ""


## Whether ASSET is what WORDS look for: its class number, or each word in its name or its
## planet's, in any order ("turret gaspar", "gaspar turret" and "turret_gaspar" find turret_gaspar_1).
static func matches(asset: Dictionary, words: String) -> bool:
	if words.is_empty() or str(asset["class"]) == words:
		return true

	var name: String = String(asset["name"]).to_lower()
	var where: String = String(asset["level_name"]).to_lower()

	for word in words.replace("_", " ").split(" ", false):
		if not name.contains(word) and not where.contains(word):
			return false

	return true


func is_here(level: Dictionary) -> bool:
	return not here.is_empty() and level["game"] == here["game"] and level["disc"] == here["disc"] \
		and int(level["level"]) == int(here["level"])


func tooltip(asset: Dictionary, status: String) -> String:
	var where: String = "%s %s, level %02d%s" % [asset["game"], asset["disc"], int(asset["level"]),
		"" if String(asset["level_name"]).is_empty() else " (%s)" % asset["level_name"]]
	var does: String = ("In this level: placed as the game's own %s, with its logic." % asset["kind"].trim_suffix("s")
		if status == Library.FULL else
		"Placed as static scenery: look only, no logic."
		+ ("\nOr \"Add with its own logic\": brings its class from its level."
			if Library.import_refusal(asset, here).is_empty() else "")
		+ ("\nOr \"Add on a class's logic\": acts as a class of this level." if asset["kind"] == "mobys" else ""))
	return "%s, class %d\nFrom %s\n%s\nDrag it into the 3D view, or double-click to add it." % [
		asset["name"], int(asset["class"]), where, does]


## The asset of the list's item INDEX.
func asset_at(index: int) -> Dictionary:
	if index < 0 or index >= list.item_count:
		return {}

	var at: Variant = list.get_item_metadata(index)
	return shown[int(at)] if at != null and int(at) < shown.size() else {}


## Whether ASSET can go on a class's logic here: a moby this level does not have, with a skeleton.
func can_add_as(asset: Dictionary) -> bool:
	return not asset.is_empty() and asset["kind"] == "mobys" and not here.is_empty() \
		and Library.status(asset, here) != Library.FULL and not here["classes"].get("mobys", {}).is_empty()


## The level's moby classes, by name, to pick the one the selected asset behaves as.
func show_classes() -> void:
	classes_menu.clear()
	var entries: Array = []

	for number in here["classes"]["mobys"]:
		# The dock's own name, as the list shows it, else the class scene's.
		var label: String = Library.asset_name(String(here["game"]), "mobys", int(number))
		entries.append([label if not label.is_empty() else Library.class_label(here["classes"]["mobys"][number], number),
			int(number)])

	entries.sort_custom(func(a: Array, b: Array) -> bool: return a[0].naturalnocasecmp_to(b[0]) < 0)

	for entry in entries:
		classes_menu.add_item("%s (class %d)" % entry if entry[0] != "Class %d" % entry[1] else entry[0], entry[1])

	classes_menu.position = Vector2i(as_button.get_screen_position() + Vector2(0, as_button.size.y))
	classes_menu.max_size = Vector2i(400, 500)
	classes_menu.popup()


func add(index: int) -> void:
	var asset: Dictionary = asset_at(index)

	if not asset.is_empty():
		plugin.add_in_view(asset)


# =============================================================================
# PREVIEWS
# =============================================================================

## Shown until an asset's picture is made: a plain cube outline in COLOUR, drawn at the list's own
## size so it stays sharp (the editor's 16 px icons blur when scaled up).
static func waiting_picture(colour: Color, size: int = 80) -> Texture2D:
	var image := Image.create_empty(size, size, false, Image.FORMAT_RGBA8)
	var c: float = size * 0.5
	var r: float = size * 0.3
	# An upright cube seen from a corner: its outline and the three edges from the near corner.
	var top := Vector2(c, c - r)
	var corners: Array[Vector2] = [top, Vector2(c + r * 0.87, c - r * 0.5), Vector2(c + r * 0.87, c + r * 0.5),
		Vector2(c, c + r), Vector2(c - r * 0.87, c + r * 0.5), Vector2(c - r * 0.87, c - r * 0.5)]
	var near := Vector2(c, c)
	var edges: Array = []

	for i in range(6):
		edges.append([corners[i], corners[(i + 1) % 6]])

	for i in [1, 3, 5]:
		edges.append([near, corners[i]])

	var width: float = maxf(size / 40.0, 1.5)

	for y in range(size):
		for x in range(size):
			var p := Vector2(x + 0.5, y + 0.5)
			var distance: float = INF

			for edge in edges:
				distance = minf(distance, Geometry2D.get_closest_point_to_segment(p, edge[0], edge[1]).distance_to(p))

			var alpha: float = clampf(width - distance + 0.5, 0.0, 1.0)

			if alpha > 0.0:
				image.set_pixel(x, y, Color(colour, colour.a * alpha))

	return ImageTexture.create_from_image(image)


## A preview of ASSET's model: its first mesh, read straight from its file (it need not be in this
## project), drawn by the editor's mesh previewer.
func make_preview(asset: Dictionary) -> void:
	if cached_preview(asset) != null:
		return

	var model: String = String(asset["model"])

	# Its textures go in as data, so the editor does not try to import files outside the project.
	var data: PackedByteArray = Library.self_contained(model)
	var state := GLTFState.new()
	state.handle_binary_image_mode = GLTFState.HANDLE_BINARY_IMAGE_MODE_EMBED_AS_UNCOMPRESSED
	var document := GLTFDocument.new()

	if data.is_empty() or document.append_from_buffer(data, "", state) != OK:
		return

	var meshes: Array = state.get_meshes()

	if meshes.is_empty():
		return

	var mesh: Mesh = (meshes[0] as GLTFMesh).mesh.get_mesh()

	if mesh == null:
		return

	EditorInterface.get_resource_previewer().queue_edited_resource_preview(standing(mesh, Library.front_for(asset)),
		self, "handle_preview", asset)


## MESH turned from the game's axes (z up, FRONT on a side: Library.front_for()) to the
## previewer's (y up, front to the camera on z), so the picture shows the asset standing, from the
## front. Unskinned: the previewer draws the vertices as they are.
static func standing(mesh: Mesh, front: Vector3 = Vector3(1, 0, 0)) -> ArrayMesh:
	var turned := ArrayMesh.new()
	# The front goes to z (to the camera), the game's up to y and the side between them to x.
	var ahead: Vector3 = Vector3(front.x, front.y, 0).normalized() if not Vector3(front.x, front.y, 0).is_zero_approx() \
		else Vector3(1, 0, 0)
	var axes: Basis = Basis(Vector3(0, 0, 1).cross(ahead), Vector3(0, 0, 1), ahead).transposed()

	for surface in range(mesh.get_surface_count()):
		var arrays: Array = mesh.surface_get_arrays(surface)
		var vertices: PackedVector3Array = arrays[Mesh.ARRAY_VERTEX]

		for i in range(vertices.size()):
			vertices[i] = axes * vertices[i]

		arrays[Mesh.ARRAY_VERTEX] = vertices

		if arrays[Mesh.ARRAY_NORMAL] != null:
			var normals: PackedVector3Array = arrays[Mesh.ARRAY_NORMAL]

			for i in range(normals.size()):
				normals[i] = axes * normals[i]

			arrays[Mesh.ARRAY_NORMAL] = normals

		arrays[Mesh.ARRAY_TANGENT] = null
		arrays[Mesh.ARRAY_BONES] = null
		arrays[Mesh.ARRAY_WEIGHTS] = null
		turned.add_surface_from_arrays(mesh.surface_get_primitive_type(surface), arrays)
		turned.surface_set_material(turned.get_surface_count() - 1, mesh.surface_get_material(surface))

	return turned


func handle_preview(_path: String, preview: Texture2D, _thumbnail: Texture2D, asset: Variant) -> void:
	if preview == null or not asset is Dictionary:
		return

	var model: String = String(asset["model"])
	previews[model] = preview
	DirAccess.make_dir_recursive_absolute(PREVIEW_CACHE)
	preview.get_image().save_png(cache_path(asset))

	for index in range(list.item_count):
		var shown_asset: Dictionary = asset_at(index)

		if not shown_asset.is_empty() and shown_asset["model"] == model:
			list.set_item_icon(index, preview)


## The preview of ASSET's model made before, in memory or on disk, or null.
func cached_preview(asset: Dictionary) -> Texture2D:
	var model: String = String(asset["model"])

	if previews.has(model):
		return previews[model]

	var path: String = cache_path(asset)

	if not FileAccess.file_exists(path):
		return null

	var image := Image.load_from_file(path)

	if image == null or image.is_empty():
		return null

	previews[model] = ImageTexture.create_from_image(image)
	return previews[model]


## The cache file of ASSET's preview: named by its model's path, when that last changed and the
## front the names table gives, so a model extracted again or turned gets a new one.
func cache_path(asset: Dictionary) -> String:
	var model: String = String(asset["model"])
	var key: String = "standing:%s:%d" % [model, FileAccess.get_modified_time(model)]
	var front: String = Library.named_front(String(asset["game"]), String(asset["kind"]), int(asset["class"]))

	if not front.is_empty():
		key += ":" + front

	return PREVIEW_CACHE.path_join(key.md5_text() + ".png")


## NAME (small_turret) broken into lines of at most LINE characters after an underscore, as the
## list shows it: the list itself breaks inside words.
static func wrapped(name: String, line: int = 17) -> String:
	var lines: PackedStringArray = [""]
	var words: PackedStringArray = name.split("_")

	for i in range(words.size()):
		var word: String = words[i] + ("_" if i < words.size() - 1 else "")
		var last: String = lines[lines.size() - 1]

		if (last + word).length() <= line or last.is_empty():
			lines[lines.size() - 1] = last + word
		else:
			lines.append(word)

	return "\n".join(lines)


## Sizes the tree to its open rows (at most TREE_ROWS; it scrolls past them), so no empty band
## sits between it and the assets.
func fit_tree() -> void:
	var first: TreeItem = tree.get_root().get_first_child() if tree.get_root() != null else null
	var rows: int = 0
	var item: TreeItem = first

	while item != null:
		rows += 1
		item = item.get_next_visible()

	# A row's height as the tree draws it, once it has; else the font's line and the gap.
	var gap: float = tree.get_theme_constant("v_separation")
	var row: float = tree.get_item_area_rect(first).size.y if first != null else 0.0

	if row <= 0.0:
		row = tree.get_theme_font("font").get_height(tree.get_theme_font_size("font_size")) + 4.0

	var panel: StyleBox = tree.get_theme_stylebox("panel")
	var margins: float = panel.get_minimum_size().y if panel != null else 0.0
	tree.custom_minimum_size.y = ceilf(clampi(rows, 1, TREE_ROWS) * (row + gap) + margins + 4.0)
