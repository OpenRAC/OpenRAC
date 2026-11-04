# Reviewing a PR

1. `bash tools/check_all.sh` passes, and no report number drops.
2. Names follow [STYLE.md](STYLE.md). New names are in `config/names_local.tsv`.
3. Comments use names, not IDs. No `Moby_564`, no `class 0x3EF`.
4. Numbers with a known meaning use an enum or a macro.
5. New raw offsets use a struct field or `STRUCT_OFFSET`.
6. Types used by two files are in `include/`.
7. Hacks carry a `FAKEMATCH` comment with a reason, and are in `config/fakematch.tsv`.
8. Provenance and licence lines are unchanged.
9. No generated file is edited by hand. No retail bytes or disc data, even as text.
10. Mass mechanical commits are listed in `.git-blame-ignore-revs`.
