# CLAUDE.md

- After a small change, build only the targets it touches and run only the tests that cover it (`cmake --build --preset release --target <target>`, `ctest --preset release -R <name>`), not a full build or the full suite.
