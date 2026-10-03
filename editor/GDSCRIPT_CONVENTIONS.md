# GDScript conventions

Rules for the `.gd` files in this repository (the scripts the level extractor
puts in the Godot project, `editor/rc1/*.gd`). New code follows them;
touch an old file and bring it in line. Indentation is tabs.

## File structure

Top to bottom:

```gdscript
class_name LevelView
extends Node3D
## What the script is for, in a sentence or two.
## More lines are consecutive `##` lines.

# =============================================================================
# VARIABLES
# =============================================================================

## What the member holds.
var speed: float = 30.0
```

`class_name` comes first when the script has one, then `extends`, then the
class doc comment. Scripts that nothing refers to by name need no
`class_name`.

### Sections

Group members under a header, in this order:

1. `NODE REFERENCES`: `@onready var` nodes and any other `@onready` value
2. `VARIABLES`: `var` and `static var`
3. `CONSTANTS`: `const` and `enum`
4. `SIGNALS`: `signal`
5. `METHODS`: functions

Inner classes go last under `INNER CLASSES`. A section a file does not need is
left out and the rest keep their order. A file under about 30 lines can skip
the headers, but its members keep the order.

### Method order

Inside `METHODS`, lifecycle callbacks sit at the two ends and custom functions
between them:

1. `_init`, `_enter_tree`, `_ready` (and `_initialize` for a `SceneTree`
   script), in that order, each only when the script has it
2. custom functions
3. every other `_` engine override (`_input`, `_unhandled_input`,
   `_notification`, `_exit_tree`, `_physics_process`, ...)
4. `_process`, always last

### Sub-groups

Inside a section, `# --- Name ---` captions group related members. Members
without a caption come first in their section. Once the custom functions carry
captions, the engine callbacks at the end go under `# --- Engine Callbacks ---`.

```gdscript
# --- Camera ---

## The fly camera, created in _ready.
var camera: Camera3D
```

## Comments

### Doc comments (`##`)

`##` says what a member or function is. Put it above the declaration, never in
a body. Every `var`, `const`, `signal` and `func` gets one; a one-line
summary is enough. Several lines are consecutive `##` lines. `@param` and
`@return` are optional, for parameters the name does not explain.

```gdscript
## The size of the environment's sky panorama, or zero without one.
func sky_size(root: Node) -> Vector2i:
```

### Inline comments (`#`)

`#` explains why, not what. Use consecutive `#` lines, not `'''` strings.

```gdscript
# Good: the reason.
# Triangle lists without an index buffer use three vertices per triangle.

# Bad: the code again.
# Divide the vertex count by three.
```

Comment every branch (`if`, `elif`, `match` arm, guard clause, deciding
ternary): each is a rule someone chose and the reason is not on the line.
Plain sequential statements need none, and a branch whose reason is obvious from
a name a line or two above can stay bare.

## Naming

- `snake_case` for variables and functions; `UPPER_SNAKE_CASE` for constants;
  `PascalCase` for classes and enums.
- No abbreviations unless everyone knows them (`id`, `pos`).
- Booleans read naturally: `is_captured`, `has_sky`, `can_move`.
- Signals are past tense and carry no prefix: `has_loaded`, `has_updated`.
- Do not start custom functions with `_`; Godot reserves it for engine
  callbacks. Signal handlers are named `handle_<what>()`
  (`handle_button_pressed()`), helpers get plain names (`terrain_bounds()`).
- Name node references for their purpose, not their type: `owner_label`, not
  `label1`.

## Types

- Annotate every function parameter and return type.
- Annotate every `@onready var`.
- Annotate other `var`s when the type is not obvious from the value. `:=` is
  fine when it is (`var box := mesh.get_aabb()`). Spell the type when the value
  is a dynamic lookup (a `Dictionary` read, `JSON.parse_string`) or a
  constant that needs a float or int pinned.
- Cast `instantiate()` results: `scene.instantiate() as Thing`.

## Spacing

- One blank line between declarations that are distinct, none between
  declarations that belong together (a doc comment and its member stay
  together).
- In a function, a blank line between logical blocks: setup, conditionals,
  loops, the return. No blank lines between tightly coupled one-liners.
- One blank line between functions, never two.
- No trailing whitespace, and none on blank lines.
- Wrap long expressions inside brackets, with the continuation indented.

## Other

- Do not repeat a value already in scope; pass objects rather than their parts
  (`place(position)`, not `place(position.x, position.y)`).
- Do not guard a call whose function already handles the fallback.
- Prefer `signal.connect(callable)` in code; pass extra arguments with `.bind()`.
- Prefer scenes over building visual nodes in code, `preload()` scene
  references at the top, and add a node to the tree before using its
  `@onready` children.
- Named constants for tuning numbers (speeds, limits, sensitivities).
