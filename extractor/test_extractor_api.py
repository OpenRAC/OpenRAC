"""extractor_api.json: what OpenRAC Editor's tools may import from this folder. Every module it
lists is here, and no module here imports one that is not (the level editor's modules left with
the editor: a leftover import would only fail on a player's computer)."""

import ast
import json
from pathlib import Path
import sys
import unittest

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
API = json.loads((HERE / "extractor_api.json").read_text())


def imported(path: Path) -> set[str]:
    names = set()
    for node in ast.walk(ast.parse(path.read_text())):
        if isinstance(node, ast.Import):
            names.update(alias.name.split(".")[0] for alias in node.names)
        elif isinstance(node, ast.ImportFrom) and node.module and node.level == 0:
            names.add(node.module.split(".")[0])
    return names


class ExtractorApi(unittest.TestCase):
    def test_every_listed_module_is_here(self):
        self.assertIsInstance(API["version"], int)
        self.assertIsInstance(API["packed_format"], int)
        missing = [name for name in API["modules"] if not (HERE / f"{name}.py").is_file()]
        self.assertEqual(missing, [])

    def test_nothing_here_imports_the_editor(self):
        local = {p.stem for p in HERE.glob("*.py")} | {"enhanced"}
        shared = {p.name for p in (ROOT / "tools").iterdir() if p.is_dir()} | {p.stem for p in (ROOT / "tools").glob("*.py")}
        known = local | shared | set(sys.stdlib_module_names)
        strays = {}
        for path in [*HERE.glob("*.py"), *HERE.glob("enhanced/*.py")]:
            unknown = sorted(name for name in imported(path) if name not in known)
            if unknown:
                strays[path.relative_to(HERE).as_posix()] = unknown
        self.assertEqual(strays, {})


if __name__ == "__main__":
    unittest.main()
