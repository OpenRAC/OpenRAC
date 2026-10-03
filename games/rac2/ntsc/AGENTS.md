# Repository language and commit messages

Write all repository documentation, code comments, user-facing messages,
catalogue descriptions and commit messages in English. Preserve measured game
identifiers, symbol names, program identities and pinned reference hashes.

Inside OpenRAC, use the repository's Conventional Commits subject,
`type(rac2): summary` ([CONTRIBUTING.md](../../../CONTRIBUTING.md#commits)),
followed by a substantive body.
Describe the concrete change and its technical reason, the measured scope and
before/after progress when relevant, and the validation actually performed.
State any remaining limitation needed to interpret the result. A subject alone
is insufficient for a substantive matching or tooling change.

Keep source, catalogue, object and integration proofs coherent. Regenerate
affected reviews and full loaded-byte gates after changing hashed inputs,
including translations of comments or catalogue descriptions. Publish only
authored source, structural identifiers and proof metadata; keep game bytes,
proprietary tools and private runtime artifacts outside the repository.
