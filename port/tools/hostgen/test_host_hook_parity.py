# SPDX-License-Identifier: GPL-3.0-or-later
"""Shared renderer notifications must be wired into both game versions; no disc or compiler needed."""
import json
from pathlib import Path
import re
import tempfile
import unittest

GAMES = Path(__file__).resolve().parents[2] / "game"
# Deliberately shared contracts, not every host function: regional implementations can differ.
RENDER_HOOKS = {
    "openrac_game_moby_colour": "hand/overlays_shared_mobyfunc_0024FD50.c",
}


def missing_hooks(games):
    errors = []
    for version in ("pal", "ntsc"):
        game = games / f"rac1-{version}"
        registered = json.loads((game / "hostgen.json").read_text())["host_functions"]
        for hook, source in RENDER_HOOKS.items():
            if hook not in registered:
                errors.append(f"{version}: {hook} is not registered in hostgen.json")
            # A declaration alone is insufficient: require the statement that invokes the hook.
            if not re.search(r"^\s*" + re.escape(hook) + r"\s*\(",
                             (game / source).read_text(), re.MULTILINE):
                errors.append(f"{version}: {source} does not call {hook}")
    return errors


class RenderingHostHookParity(unittest.TestCase):
    def test_shared_hooks_are_registered_and_called_in_both_versions(self):
        self.assertEqual(missing_hooks(GAMES), [])

    def test_missing_registration_or_call_is_detected_in_either_version(self):
        for version in ("pal", "ntsc"):
            for omission in ("registration", "call"):
                with self.subTest(version=version, omission=omission), tempfile.TemporaryDirectory() as tmp:
                    games = Path(tmp)
                    for region in ("pal", "ntsc"):
                        game = games / f"rac1-{region}"
                        (game / "hand").mkdir(parents=True)
                        hooks = list(RENDER_HOOKS)
                        if region == version and omission == "registration":
                            hooks = []
                        (game / "hostgen.json").write_text(json.dumps({"host_functions": hooks}))
                        for hook, source in RENDER_HOOKS.items():
                            text = f"extern void {hook}(unsigned, unsigned);\n"
                            if region != version or omission != "call":
                                text += f"void colour(void) {{\n    {hook}(1, 2);\n}}\n"
                            (game / source).write_text(text)
                    errors = missing_hooks(games)
                    self.assertEqual(len(errors), 1, errors)
                    self.assertTrue(errors[0].startswith(f"{version}:"), errors)


if __name__ == "__main__":
    unittest.main()
