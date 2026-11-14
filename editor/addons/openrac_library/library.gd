@tool
extends RefCounted
## The Assets dock's library: the mobys, ties and shrubs of every level project the extractor
## wrote, for any game and disc (dock.gd shows them, plugin.gd places them). Nothing here is
## copied until an asset is used: then prepare() brings its model into this project.
##
## Where the projects are: a checkout keeps them in assets/levels/<game>/<disc>/level_NN, an
## installed launcher in <install>/active/<game>/data/levels/<disc>. From this project's folder
## the library finds the other discs of its game and the other games' folders beside it; the
## editor's command line (after --) may add folders with --openrac-library DIR, and so may the
## project setting openrac/library/folders.
##
## What an asset does when placed (status()):
## - FULL: this level has its class, so it is the game's own moby, tie or shrub (the class scene
##   or model of this level is placed, and pack writes a record);
## - LOOK: anything else, placed as static scenery under Game/StaticAssets (editor/static_assets.py
##   bakes it into the level's terrain at pack, with collision when rc1_static_collision is on).

const KINDS: PackedStringArray = ["mobys", "ties", "shrubs"]

const FULL: String = "full"
const LOOK: String = "look"

## Folders a disc's projects are never in: the extractor's backups and the tutorial's copy.
const SKIPPED_SUFFIXES: PackedStringArray = ["-backups", "-tutorial"]

## Where a used asset's files go in this project.
const LIBRARY_DIR: String = "res://library"

## Where skins are copied (prepare_skin()): a folder Godot does not import (.gdignore), so a
## skinned model never goes through the importer; the plugin draws it from the file.
const SKINS_DIR: String = "res://library/skins"

## The project setting that lists more folders of projects.
const FOLDERS_SETTING: String = "openrac/library/folders"

## Readable names of each game's assets, by kind and class: names/<game>.tsv beside this script
## ("kind<TAB>class<TAB>name"), read once.
const NAMES_DIR: String = "res://addons/openrac_library/names"
static var names_by_game: Dictionary = {}


# =============================================================================
# WHERE THINGS ARE
# =============================================================================

## This project's game and disc, from where it sits: {"game", "disc", "games"}, where "games" is
## the folder that holds the game folders (assets/levels, or <install>/active).
static func place_of(project_dir: String) -> Dictionary:
	var project: String = project_dir.simplify_path().trim_suffix("/")
	# A project of one level sits in its disc's folder; a project of every level is the disc's.
	var disc_dir: String = project.get_base_dir() if project.get_file().begins_with("level_") else project
	var game_dir: String = disc_dir.get_base_dir()
	var installed: bool = game_dir.get_file() == "levels" and game_dir.get_base_dir().get_file() == "data"
	var game: String = game_dir.get_base_dir().get_base_dir().get_file() if installed else game_dir.get_file()
	return {"game": game, "disc": disc_dir.get_file(), "installed": installed,
		"games": game_dir.get_base_dir().get_base_dir().get_base_dir() if installed else game_dir.get_base_dir()}


## The game folders to look in: {game: [folder of its discs, ...]}.
static func game_folders(project_dir: String, extra: PackedStringArray = PackedStringArray()) -> Dictionary:
	var here: Dictionary = place_of(project_dir)
	var found: Dictionary = {}
	var games: String = String(here["games"])

	for game in DirAccess.get_directories_at(games):
		var folder: String = games.path_join(game).path_join("data/levels") if here["installed"] else games.path_join(game)
		if DirAccess.dir_exists_absolute(folder):
			add_folder(found, game, folder)

	for folder in extra:
		var absolute: String = ProjectSettings.globalize_path(folder).simplify_path()
		if DirAccess.dir_exists_absolute(absolute):
			add_folder(found, place_of(absolute.path_join("x/level_00"))["game"], absolute)

	return found


static func add_folder(found: Dictionary, game: String, folder: String) -> void:
	var list: Array = found.get(game, [])
	if not list.has(folder):
		list.append(folder)
	found[game] = list


## Folders the command line and the project setting add.
static func extra_folders() -> PackedStringArray:
	var extra := PackedStringArray()
	var args: PackedStringArray = OS.get_cmdline_user_args()

	for at in range(args.size() - 1):
		if args[at] == "--openrac-library":
			extra.append(args[at + 1])

	if ProjectSettings.has_setting(FOLDERS_SETTING):
		extra.append_array(PackedStringArray(ProjectSettings.get_setting(FOLDERS_SETTING)))

	return extra


## The levels of a game folder: [{"game", "disc", "level", "name", "dir", "project"}], by disc then
## level. A disc folder holds one project per level (level_NN/project.godot) or one of every level.
static func levels_in(game: String, folder: String) -> Array:
	var levels: Array = []

	for disc in DirAccess.get_directories_at(folder):
		if disc.begins_with(".") or Array(SKIPPED_SUFFIXES).any(func(suffix: String) -> bool: return disc.ends_with(suffix)):
			continue

		var disc_dir: String = folder.path_join(disc)
		var projects: PackedStringArray = []

		if FileAccess.file_exists(disc_dir.path_join("project.godot")):
			projects.append(disc_dir)
		else:
			for sub in DirAccess.get_directories_at(disc_dir):
				if FileAccess.file_exists(disc_dir.path_join(sub).path_join("project.godot")):
					projects.append(disc_dir.path_join(sub))

		for project in projects:
			var title: String = project_title(project)

			for sub in DirAccess.get_directories_at(project.path_join("levels")):
				var dir: String = project.path_join("levels").path_join(sub)

				if not sub.begins_with("level_") or not FileAccess.file_exists(dir.path_join("level.json")):
					continue

				levels.append({"game": game, "disc": disc, "level": sub.trim_prefix("level_").to_int(),
					"name": level_name(title, projects.size() == 1 and not project.get_file().begins_with("level_")),
					"dir": dir, "project": project})

	levels.sort_custom(func(a: Dictionary, b: Dictionary) -> bool:
		return [a["disc"], a["level"]] < [b["disc"], b["level"]])
	return levels


## project.godot's config/name ("Ratchet & Clank: Rilgar (NTSC-U)").
static func project_title(project: String) -> String:
	var text: String = FileAccess.get_file_as_string(project.path_join("project.godot"))
	var at: int = text.find("config/name=\"")

	if at < 0:
		return ""

	var start: int = at + "config/name=\"".length()
	return text.substr(start, text.find("\"", start) - start)


## The level's name from its project's title, "Rilgar"; empty for a project of every level.
static func level_name(title: String, every_level: bool) -> String:
	if every_level or not title.contains(": "):
		return ""

	# The part after the last ": ", as later games' titles name the game in two parts.
	var name: String = title.substr(title.rfind(": ") + 2)
	return name.left(name.rfind(" (")) if name.contains(" (") else name


# =============================================================================
# ASSETS
# =============================================================================

## A level's assets: [{"kind", "class", "name", "model", "scene", "scale", "game", "disc", "level",
## "level_name", "dir"}], mobys first, by class. "model" is the class's .glb (absolute), "scene" its
## class scene (mobys only), "scale" the class scale its scene gives the model.
static func assets_of(level: Dictionary) -> Array:
	var assets: Array = []

	for kind in KINDS:
		var folder: String = String(level["dir"]).path_join(kind)

		if not DirAccess.dir_exists_absolute(folder):
			continue

		var prefix: String = kind.trim_suffix("s") + "_"

		for file in DirAccess.get_files_at(folder):
			if not file.begins_with(prefix) or file.get_extension() != "glb" or not file.get_basename().trim_prefix(prefix).is_valid_int():
				continue

			var number: int = file.get_basename().trim_prefix(prefix).to_int()
			var asset: Dictionary = {"kind": kind, "class": number, "name": "%s %d" % [kind.trim_suffix("s"), number],
				"model": folder.path_join(file), "scene": "", "scale": 1.0, "game": level["game"],
				"disc": level["disc"], "level": level["level"], "level_name": level["name"], "dir": level["dir"]}

			if kind == "mobys":
				var scene: String = folder.path_join(file.get_basename() + ".tscn")

				if FileAccess.file_exists(scene):
					asset["scene"] = scene
					read_class_scene(asset, FileAccess.get_file_as_string(scene))

			var known: String = asset_name(String(level["game"]), kind, number)

			if not known.is_empty():
				asset["name"] = known

			assets.append(asset)

	assets.sort_custom(func(a: Dictionary, b: Dictionary) -> bool:
		return [KINDS.find(a["kind"]), a["class"]] < [KINDS.find(b["kind"]), b["class"]])
	return assets


## The name and the model's scale a moby class scene gives (godot.py writes them).
static func read_class_scene(asset: Dictionary, text: String) -> void:
	var name_at: int = text.find("_class_name = \"")

	if name_at >= 0:
		var start: int = name_at + "_class_name = \"".length()
		var name: String = text.substr(start, text.find("\"", start) - start)

		if not name.is_empty():
			asset["name"] = name

	# The Model node's transform: its first entry is the class scale (no turn).
	var model_at: int = text.find("[node name=\"Model\"")
	var transform_at: int = text.find("transform = Transform3D(", model_at) if model_at >= 0 else -1

	var next_node: int = text.find("[node", model_at + 1) if model_at >= 0 else -1

	# Its own transform: before the next node, or anywhere after it when it is the last.
	if transform_at >= 0 and (next_node < 0 or next_node > transform_at):
		var start: int = transform_at + "transform = Transform3D(".length()
		var scale: float = text.substr(start, text.find(",", start) - start).to_float()

		if scale > 0.0:
			asset["scale"] = scale


## What placing ASSET does in the level HERE (here_of()): FULL or LOOK.
static func status(asset: Dictionary, here: Dictionary) -> String:
	if here.is_empty() or asset["game"] != here["game"]:
		return LOOK

	var classes: Dictionary = here["classes"].get(asset["kind"], {})
	return FULL if classes.has(int(asset["class"])) else LOOK


## Why ASSET cannot come into the level HERE with its own class and logic (editor/foreign_classes.py),
## or "" when it can.
static func import_refusal(asset: Dictionary, here: Dictionary) -> String:
	if asset.is_empty() or here.is_empty():
		return "Open a level scene first."

	if asset["kind"] != "mobys":
		return "Only mobys have logic of their own."

	if status(asset, here) == FULL:
		return "This level has the class already."

	if asset["game"] != "rac1" or here["game"] != "rac1":
		return "Only for Ratchet & Clank 1 levels, for now."

	if asset["disc"] != here["disc"]:
		return "Only between levels of the same disc."

	if int(asset["level"]) < 0 or int(asset["level"]) >= 19 or int(here["level"]) < 0 or int(here["level"]) >= 19:
		return "Only between the game's own levels."

	return ""


## The level the edited scene is: {"game", "disc", "level", "dir" (res://), "classes": {kind:
## {class: res path}}}, or empty for a scene that is not a level.
static func here_of(scene_path: String) -> Dictionary:
	var dir: String = scene_path.get_base_dir()

	if not dir.get_file().begins_with("level_") or not FileAccess.file_exists(dir.path_join("level.json")):
		return {}

	var place: Dictionary = place_of(ProjectSettings.globalize_path("res://"))
	var classes: Dictionary = {}

	for kind in KINDS:
		var prefix: String = kind.trim_suffix("s") + "_"
		var by_class: Dictionary = {}

		for file in DirAccess.get_files_at(dir.path_join(kind)):
			var number: String = file.get_basename().trim_prefix(prefix)

			if file.begins_with(prefix) and number.is_valid_int() and file.get_extension() == ("tscn" if kind == "mobys" else "glb"):
				by_class[number.to_int()] = dir.path_join(kind).path_join(file)

		classes[kind] = by_class

	return {"game": place["game"], "disc": place["disc"], "level": dir.get_file().trim_prefix("level_").to_int(),
		"dir": dir, "classes": classes}


# =============================================================================
# BRINGING AN ASSET IN
# =============================================================================

## The res:// file to place for ASSET in the level HERE: this level's own class scene or model for
## a FULL asset; else a static copy of its model under res://library (made once, imported).
## Empty, with the reason in `error`, when it cannot be brought in.
static func prepare(asset: Dictionary, here: Dictionary, error: Array) -> String:
	if status(asset, here) == FULL:
		return String(here["classes"][asset["kind"]][int(asset["class"])])

	return copy_in(asset, String(asset["model"]).get_file(), true, error)


## A moby's model as it is, skin and animations kept, to draw on another class's logic (a skin:
## plugin.gd add_as()); made once under SKINS_DIR, not imported. Empty, with the reason in
## `error`, when it cannot be brought in.
static func prepare_skin(asset: Dictionary, error: Array) -> String:
	if not FileAccess.file_exists(SKINS_DIR.path_join(".gdignore")):
		DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(SKINS_DIR))
		FileAccess.open(SKINS_DIR.path_join(".gdignore"), FileAccess.WRITE).close()

	return copy_in(asset, String(asset["model"]).get_file(), false, error, SKINS_DIR)


## Copies ASSET's model into <ROOT>/<game>/<disc>/level_NN/<kind>/NAME, still when STILL_MODEL,
## with its textures by the same relative paths and a .source.json, and imports them (none under
## SKINS_DIR).
static func copy_in(asset: Dictionary, name: String, still_model: bool, error: Array,
		root: String = LIBRARY_DIR) -> String:
	var level_dir: String = root.path_join(String(asset["game"])).path_join(String(asset["disc"])) \
		.path_join("level_%02d" % int(asset["level"]))
	var target: String = level_dir.path_join(String(asset["kind"])).path_join(name)

	if FileAccess.file_exists(target):
		return target

	var source: PackedByteArray = FileAccess.get_file_as_bytes(String(asset["model"]))
	var document: Dictionary = read_glb(source)

	if document.is_empty():
		error.append("%s is not a glTF model the library can read." % asset["model"])
		return ""

	var json: Dictionary = document["json"]

	if still_model:
		still(json)

	var copied: PackedStringArray = []

	# Its textures, by the same relative paths ("../textures/x.png"), so the copy finds them.
	for image in json.get("images", []):
		var uri: String = String(image.get("uri", ""))

		if uri.is_empty() or uri.begins_with("data:"):
			continue

		var from: String = String(asset["model"]).get_base_dir().path_join(uri).simplify_path()
		var to: String = target.get_base_dir().path_join(uri).simplify_path()

		if not to.begins_with(level_dir + "/"):
			error.append("%s names a texture outside its level (%s)." % [asset["model"], uri])
			return ""

		if not FileAccess.file_exists(to):
			if not FileAccess.file_exists(from):
				error.append("%s's texture %s is missing." % [asset["model"], uri])
				return ""

			DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(to.get_base_dir()))
			var data: PackedByteArray = FileAccess.get_file_as_bytes(from)
			var file := FileAccess.open(to, FileAccess.WRITE)
			file.store_buffer(data)
			file.close()

		copied.append(to)

	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(target.get_base_dir()))
	var out := FileAccess.open(target, FileAccess.WRITE)

	if out == null:
		error.append("Cannot write %s." % target)
		return ""

	out.store_buffer(write_glb(json, document["bin"]))
	out.close()

	# Where it came from, for the pack and for whoever shares the level.
	var note := FileAccess.open(target.trim_suffix(".glb") + ".source.json", FileAccess.WRITE)
	note.store_string(JSON.stringify({"format": 1, "game": asset["game"], "disc": asset["disc"],
		"level": asset["level"], "kind": asset["kind"], "class": asset["class"], "name": asset["name"]}, "  ") + "\n")
	note.close()

	copied.append(target)

	if root != SKINS_DIR:
		import_files(copied)

	return target


## The model at PATH (a skin under SKINS_DIR) as a scene, read from its file, not imported; null
## when it cannot be read.
static func scene_of(path: String) -> Node3D:
	var state := GLTFState.new()
	state.handle_binary_image_mode = GLTFState.HANDLE_BINARY_IMAGE_MODE_EMBED_AS_UNCOMPRESSED
	var document := GLTFDocument.new()
	var data: PackedByteArray = self_contained(path)

	if data.is_empty() or document.append_from_buffer(data, "", state) != OK:
		return null

	return document.generate_scene(state) as Node3D


## The .glb at PATH with its textures inside it as data, so that reading it never asks the editor
## to import a file (outside the project, or under SKINS_DIR); empty when it cannot be read.
static func self_contained(path: String) -> PackedByteArray:
	var glb: Dictionary = read_glb(FileAccess.get_file_as_bytes(path))

	if glb.is_empty():
		return PackedByteArray()

	for image in glb["json"].get("images", []):
		var uri: String = String(image.get("uri", ""))

		if not uri.is_empty() and not uri.begins_with("data:"):
			var bytes: PackedByteArray = FileAccess.get_file_as_bytes(path.get_base_dir().path_join(uri).simplify_path())
			image["uri"] = "data:image/png;base64," + Marshalls.raw_to_base64(bytes)

	return write_glb(glb["json"], glb["bin"])


## The animations' names of the model at PATH, in order (a skin's: "seq_00"...).
static func animation_names(path: String) -> PackedStringArray:
	var names := PackedStringArray()
	var document: Dictionary = read_glb(FileAccess.get_file_as_bytes(path))

	for animation in document.get("json", {}).get("animations", []):
		names.append(String(animation.get("name", "animation %d" % names.size())))

	return names


## The name a class scene gives its class (rc1_class_name), or "Class N".
static func class_label(scene_path: String, number: int) -> String:
	var asset: Dictionary = {"name": "Class %d" % number}
	read_class_scene(asset, FileAccess.get_file_as_string(scene_path))
	return String(asset["name"])


## Has the editor import new files now, the textures before the model that names them.
static func import_files(files: PackedStringArray) -> void:
	if not Engine.is_editor_hint():
		return

	var filesystem: EditorFileSystem = EditorInterface.get_resource_filesystem()

	# A folder the editor has not seen yet: it finds the files and imports them in a scan of its own
	# (plugin.gd places the asset once that is done); update_file() cannot reach them.
	for file in files:
		if filesystem.get_filesystem_path(file.get_base_dir()) == null:
			filesystem.scan()
			return

	for file in files:
		filesystem.update_file(file)

	filesystem.reimport_files(files)


## The model as static scenery (editor/static_assets.py validate()): no skin, no animation and no
## bone weights, its mesh as it is stored (the class's bind pose).
static func still(json: Dictionary) -> void:
	json.erase("skins")
	json.erase("animations")

	for node in json.get("nodes", []):
		node.erase("skin")
		node.erase("weights")

	for mesh in json.get("meshes", []):
		for primitive in mesh.get("primitives", []):
			var attributes: Dictionary = primitive.get("attributes", {})

			for key in attributes.keys():
				if String(key).begins_with("JOINTS_") or String(key).begins_with("WEIGHTS_"):
					attributes.erase(key)


## A .glb's JSON and binary chunk: {"json", "bin"}, or empty when it is not one.
static func read_glb(data: PackedByteArray) -> Dictionary:
	if data.size() < 20 or data.slice(0, 4).get_string_from_ascii() != "glTF":
		return {}

	var length: int = data.decode_u32(12)

	if data.decode_u32(16) != 0x4E4F534A or 20 + length > data.size():
		return {}

	var json: Variant = JSON.parse_string(data.slice(20, 20 + length).get_string_from_utf8())

	if not json is Dictionary:
		return {}

	var bin := PackedByteArray()
	var at: int = 20 + length

	if at + 8 <= data.size() and data.decode_u32(at + 4) == 0x004E4942:
		bin = data.slice(at + 8, at + 8 + data.decode_u32(at))

	return {"json": json, "bin": bin}


## VALUE with every whole float made an int: Godot's JSON reads every number as a float, and
## glTF's indices and counts must be written as integers (a float that is whole may be too).
static func whole_numbers(value: Variant) -> Variant:
	if value is float and is_finite(value) and value == floorf(value) and absf(value) < 9.0e15:
		return int(value)

	if value is Array:
		return value.map(func(item: Variant) -> Variant: return whole_numbers(item))

	if value is Dictionary:
		var out: Dictionary = {}

		for key in value:
			out[key] = whole_numbers(value[key])

		return out

	return value


## A .glb of JSON and a binary chunk, each padded to 4 bytes as the format asks.
static func write_glb(json: Dictionary, bin: PackedByteArray) -> PackedByteArray:
	var text: PackedByteArray = JSON.stringify(whole_numbers(json), "", false, true).to_utf8_buffer()

	while text.size() % 4 != 0:
		text.append(0x20)

	var body := bin.duplicate()

	while body.size() % 4 != 0:
		body.append(0)

	var out := PackedByteArray()
	out.resize(12)
	out.encode_u32(0, 0x46546C67)
	out.encode_u32(4, 2)
	out.encode_u32(8, 12 + 8 + text.size() + (8 + body.size() if body.size() > 0 else 0))
	var head := PackedByteArray()
	head.resize(8)
	head.encode_u32(0, text.size())
	head.encode_u32(4, 0x4E4F534A)
	out.append_array(head)
	out.append_array(text)

	if body.size() > 0:
		head.encode_u32(0, body.size())
		head.encode_u32(4, 0x004E4942)
		out.append_array(head)
		out.append_array(body)

	return out


## The readable name of class NUMBER of KIND ("mobys", "ties", "shrubs") in GAME, or "".
static func asset_name(game: String, kind: String, number: int) -> String:
	if not names_by_game.has(game):
		var table: Dictionary = {}
		var path: String = NAMES_DIR.path_join(game + ".tsv")
		var text: String = FileAccess.get_file_as_string(path) if FileAccess.file_exists(path) else ""

		for line in text.split("\n"):
			var parts: PackedStringArray = line.split("\t")

			if parts.size() >= 3 and not line.begins_with("#") and parts[1].is_valid_int():
				table["%s/%d" % [parts[0], parts[1].to_int()]] = parts[2]

		names_by_game[game] = table

	return String(names_by_game[game].get("%s/%d" % [kind, number], ""))
