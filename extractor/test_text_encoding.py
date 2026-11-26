"""The extractor runs on Windows too, where Python reads and writes text in the system's code page
(cp1252 on most) unless told otherwise: every text read and write here names UTF-8, and writes \\n
line ends, so a file is the same bytes on every system.

    python3 -m unittest test_text_encoding
"""

import ast
from pathlib import Path
import unittest

HERE = Path(__file__).resolve().parent


class TextEncoding(unittest.TestCase):
    def test_every_text_read_and_write_names_utf8(self):
        missing = []
        for path in sorted(p for p in HERE.glob("*.py") if not p.name.startswith("test_")):
            for node in ast.walk(ast.parse(path.read_text(encoding="utf-8"))):
                if not isinstance(node, ast.Call):
                    continue
                func, keywords = node.func, {k.arg for k in node.keywords}
                if isinstance(func, ast.Attribute) and func.attr in ("read_text", "write_text"):
                    if "encoding" not in keywords:
                        missing.append(f"{path.name}:{node.lineno} {func.attr} without encoding")
                    if func.attr == "write_text" and "newline" not in keywords:
                        missing.append(f"{path.name}:{node.lineno} write_text without newline")
                elif isinstance(func, ast.Name) and func.id == "open":
                    mode = node.args[1] if len(node.args) > 1 else next(
                        (k.value for k in node.keywords if k.arg == "mode"), ast.Constant("r"))
                    if isinstance(mode, ast.Constant) and "b" not in mode.value and "encoding" not in keywords:
                        missing.append(f"{path.name}:{node.lineno} open() in text mode without encoding")
        self.assertEqual(missing, [], "\n".join(missing))


if __name__ == "__main__":
    unittest.main()
