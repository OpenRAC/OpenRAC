# Changelog notes

One file per change of the mod interface: `1.<minor>/<name>.md`, one short
paragraph that says what it adds and names the new members, for example
`A moby's glow (moby_glow).` `sdk/tools/bump_api.py minor --note "..."`
writes it. `sdk/tools/gen_reference.py` puts a minor's notes together in
`sdk/docs/reference/changelog.md`.
