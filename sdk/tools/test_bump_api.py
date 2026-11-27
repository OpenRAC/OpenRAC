# SPDX-License-Identifier: MIT
# Copyright (c) 2026 the OpenRAC contributors
"""bump_api.py: the version worked out from the last release, in every place that holds it."""
from __future__ import annotations

import contextlib
import io
import os
import subprocess
import tempfile
import unittest
import unittest.mock
from pathlib import Path

import bump_api

HEADER = """#define OPENRAC_MOD_API_MAJOR 1
/* The interface's minor version. 1.1 added script mods, 1.2 the keyboard and mouse, and 1.3 the
 * pointer (openrac_input.pointer_*).
 */
#define OPENRAC_MOD_API_MINOR 3
#define OPENRAC_FLAG_A 0x1u
typedef struct openrac_thing {
    uint32_t size;
} openrac_thing;
typedef struct openrac_api {
    uint32_t size;
    /* Old. Since 1.3. */
    int (*old)(openrac_mod* mod);
} openrac_api;
"""
CPP = """struct Api {
    // Since 1.3: whether the port has old().
    bool has_old() const {
        return m_api->api_minor >= 3
               && m_api->size >= offsetof(openrac_api, old) + sizeof(void*);
    }
};
"""
LOCKS = {
    "openrac_api.lock": ["0\tsize\t0\tuint32_t", "1\told\t8\tint (*)(openrac_mod *)"],
    "types.lock": ["openrac_thing\t(size)\t4\t-", "openrac_thing\tsize\t0\tuint32_t"],
    "macros.lock": ["OPENRAC_FLAG_A\t0x1u", "OPENRAC_MOD_API_MINOR\t3"],
}
MEMBERS = "# member\tarea\tsince\tstatus\nsize\tcore\t1.0\tstable\nold\tcore\t1.3\tstable\n"


# A contributor's own git settings (signing, hooks) must not reach the test repositories.
GIT_CONFIG = ("-c", "user.name=t", "-c", "user.email=t@t", "-c", "commit.gpgsign=false", "-c", "tag.gpgsign=false",
              "-c", "core.hooksPath=/dev/null")


def lock_text(rows: list[str]) -> str:
    return "# Generated.\n" + "".join(r + "\n" for r in rows)


class Repo(unittest.TestCase):
    """A small tree with the version's places, its locks, and the release sdk-v1.3.0 as a tag."""

    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.env = unittest.mock.patch.dict(os.environ, {"OPENRAC_SDK_OFFLINE": "1"})
        self.env.start()
        self.addCleanup(self.env.stop)
        self.addCleanup(self.tmp.cleanup)
        files = {
            bump_api.HEADER: HEADER,
            bump_api.KEYS: "#define OPENRAC_KEY_COUNT_X 1\n",
            bump_api.CPP: CPP,
            bump_api.MEMBERS: MEMBERS,
            Path("sdk/docs/guide.md"): "A guide.\n",
            Path("sdk/tools/other.py"): "# not in the archive\n",
        }
        for name, rows in LOCKS.items():
            files[Path("sdk/api") / name] = lock_text(rows)
        for path, text in files.items():
            self.put(path, text)
        self.git("init", "-q", "-b", "develop")
        self.commit("the release")
        self.git("tag", "sdk-v1.3.0")
        self.git("checkout", "-q", "-b", "feature/glow")

    def git(self, *args: str) -> str:
        return subprocess.run(["git", "-C", str(self.root), *GIT_CONFIG, *args],
                              check=True, capture_output=True, text=True).stdout

    def commit(self, message: str) -> None:
        self.git("add", "-A")
        self.git("commit", "-qm", message, "--allow-empty")

    def put(self, path: Path | str, text: str) -> None:
        (self.root / path).parent.mkdir(parents=True, exist_ok=True)
        (self.root / path).write_text(text)

    def text(self, path: Path | str) -> str:
        return (self.root / path).read_text()

    def run_tool(self, *args: str) -> tuple[int, str, str]:
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            try:
                code = bump_api.main(["--root", str(self.root), "--no-regen", *args])
            except SystemExit as e:
                code, _ = 2, err.write(str(e))
        return code, out.getvalue(), err.getvalue()

    def add_member(self, name: str, since: str, places: int | None = None) -> None:
        """What new_api.py and a hand-written wrapper add for one member."""
        rows = LOCKS["openrac_api.lock"]
        api = lock_text(rows + [f"{len(rows) + i}\t{n}\t{8 * (len(rows) + i)}\tint (*)(openrac_mod *)"
                                for i, n in enumerate(self.added + [name])])
        self.added.append(name)
        self.put("sdk/api/openrac_api.lock", api)
        self.put(bump_api.MEMBERS, self.text(bump_api.MEMBERS) + f"{name}\tcore\t{since}\texperimental\n")
        header = self.text(bump_api.HEADER)
        self.put(bump_api.HEADER, header.replace(
            "} openrac_api;", f"    /* Does {name}. Since {since}. */\n    int (*{name})(openrac_mod* mod);\n"
                              "} openrac_api;"))
        minor = since.split(".")[1]
        self.put(bump_api.CPP, self.text(bump_api.CPP).replace("};\n", (
            f"    // Since {since}: whether the port has {name}().\n"
            f"    bool has_{name}() const {{\n        return m_api->api_minor >= {minor}\n"
            f"               && m_api->size >= offsetof(openrac_api, {name}) + sizeof(void*);\n    }}\n}};\n")))
        stub = self.root / bump_api.PORT_STUBS / "core.cpp"
        stub.parent.mkdir(parents=True, exist_ok=True)
        stub.write_text((stub.read_text() if stub.exists() else "")
                        + f"// openrac_api.{name}. Since {since}.\nint api_{name}(openrac_mod*) {{ return 0; }}\n")
        if places is not None:
            bump_api.set_places(self.root, places)

    added: list[str]

    def run(self, result=None):
        self.added = []
        return super().run(result)


class OpenMinorTest(Repo):
    def test_nothing_new_is_the_release(self) -> None:
        self.assertEqual(self.run_tool("--check")[0], 0)
        self.assertEqual(self.run_tool("--release-version")[1], "\n")  # nothing to release

    def test_a_change_outside_the_archive_needs_no_release(self) -> None:
        self.put("sdk/tools/other.py", "# changed\n")
        self.commit("tool")
        self.assertEqual(self.run_tool("--release-version")[1], "\n")

    def test_a_change_in_the_archive_is_a_patch(self) -> None:
        self.put("sdk/docs/guide.md", "A better guide.\n")
        self.commit("docs")
        code, out, _ = self.run_tool("--release-version")
        self.assertEqual((code, out), (0, "1.3.1\n"))
        self.git("tag", "sdk-v1.3.1")
        self.assertEqual(self.run_tool("--release-version")[1], "\n")
        self.put("sdk/docs/guide.md", "Again.\n")
        self.assertEqual(self.run_tool("--release-version")[1], "1.3.2\n")

    def test_a_new_member_opens_the_next_minor(self) -> None:
        self.add_member("glow", "1.4")
        code, _, err = self.run_tool("--check")
        self.assertEqual(code, 1)
        self.assertIn("openrac_mod.h says 1.3, the interface is 1.4", err)
        # The tool sets the header, then only the note is left.
        code, out, err = self.run_tool()
        self.assertEqual(code, 1)
        self.assertIn("has no note", err)
        self.assertIn("#define OPENRAC_MOD_API_MINOR 4\n", self.text(bump_api.HEADER))
        code, out, err = self.run_tool("minor", "--note", "a moby's glow (glow)")
        self.assertEqual(code, 0, err)
        self.assertEqual(self.text("sdk/changes/1.4/glow.md"), "A moby's glow (glow).\n")
        self.assertEqual(self.run_tool("--check")[0], 0)
        # A dev release is a patch of the last stable minor; the stable one closes 1.4.
        self.assertEqual(self.run_tool("--release-version")[1], "1.3.1\n")
        self.assertEqual(self.run_tool("--release-version", "--stable")[1], "1.4.0\n")

    def test_a_note_must_name_each_new_member(self) -> None:
        self.add_member("glow", "1.4", places=4)
        self.add_member("dim", "1.4")
        self.put("sdk/changes/1.4/glow.md", "A moby's glow (glow).\n")
        code, _, err = self.run_tool("--check")
        self.assertEqual(code, 1)
        self.assertIn("no note names dim", err)
        self.run_tool("minor", "--note", "and dim (dim)", "--name", "glow")
        self.assertEqual(self.text("sdk/changes/1.4/glow.md"), "A moby's glow (glow).\n\nAnd dim (dim).\n")
        self.assertEqual(self.run_tool("--check")[0], 0)

    def test_two_branches_pick_the_same_minor(self) -> None:
        self.add_member("glow", "1.4")
        self.run_tool("minor", "--note", "glow (glow)")
        one = bump_api.header_api(self.root)
        self.commit("glow")
        self.git("checkout", "-q", "-b", "feature/dim", "sdk-v1.3.0")
        self.added = []
        self.add_member("dim", "1.4")
        self.run_tool("minor", "--note", "dim (dim)")
        two = bump_api.header_api(self.root)
        self.assertEqual(one, two)  # the same edits: git merges them without a conflict

    def test_a_branch_from_before_a_release_moves_to_the_next_minor(self) -> None:
        # glow is released in 1.4.0; dim, written for 1.4 on a branch made before that, must become
        # 1.5 once the branch takes the release in (the merge keeps both members, glow first).
        self.add_member("glow", "1.4", places=4)
        self.put("sdk/changes/1.4/glow.md", "Glow (glow).\n")
        self.commit("glow")
        self.git("tag", "sdk-v1.4.0")
        self.add_member("dim", "1.4")
        self.put("sdk/changes/1.4/dim.md", "Dim (dim).\n")
        self.commit("dim, merged with develop")
        code, _, err = self.run_tool("--check")
        self.assertEqual(code, 1)
        self.assertIn("dim is since 1.4, but it is new in 1.5", err)
        self.assertIn("dim's comment says Since 1.4, want Since 1.5", err)
        self.assertIn("api_minor >= 4", err)
        self.assertIn("sdk/changes/1.4/dim.md: 1.4 is released", err)
        self.assertIn("api_dim says Since 1.4, want 1.5", err)
        code, out, err = self.run_tool()
        self.assertEqual(code, 0, err)
        self.assertIn("look at: sdk/include/openrac/mod.hpp", out)  # its "// Since 1.4:" comment
        self.assertTrue(self.text(bump_api.MEMBERS).endswith("glow\tcore\t1.4\texperimental\n"
                                                             "dim\tcore\t1.5\texperimental\n"))
        header = self.text(bump_api.HEADER)
        self.assertIn("/* Does glow. Since 1.4. */", header)
        self.assertIn("/* Does dim. Since 1.5. */", header)
        cpp = self.text(bump_api.CPP)
        self.assertIn("api_minor >= 4\n               && m_api->size >= offsetof(openrac_api, glow)", cpp)
        self.assertIn("api_minor >= 5\n               && m_api->size >= offsetof(openrac_api, dim)", cpp)
        self.assertEqual(self.text("sdk/changes/1.5/dim.md"), "Dim (dim).\n")
        stubs = self.text(bump_api.PORT_STUBS / "core.cpp")
        self.assertIn("// openrac_api.glow. Since 1.4.", stubs)
        self.assertIn("// openrac_api.dim. Since 1.5.", stubs)
        self.assertFalse((self.root / "sdk/changes/1.4/dim.md").exists())
        self.assertTrue((self.root / "sdk/changes/1.4/glow.md").exists())
        self.assertEqual(self.run_tool("--check")[0], 0)
        self.assertEqual(self.run_tool("--release-version")[1], "1.4.1\n")
        self.assertEqual(self.run_tool("--release-version", "--stable")[1], "1.5.0\n")

    def test_hook_points_are_not_a_minor(self) -> None:
        # games/*.h rows: a hook point's signature macro and type.
        self.put("sdk/api/macros.lock", lock_text(LOCKS["macros.lock"] + ['OPENRAC_RAC1_SIG_Jump\t"void(u32)"']))
        self.put("sdk/api/types.lock", lock_text(LOCKS["types.lock"] + ["openrac_rac1_Jump\t(typedef)\t-\tvoid (*)(uint32_t)"]))
        self.put("sdk/include/openrac/games/rac1.h", '#define OPENRAC_RAC1_SIG_Jump "void(u32)"\n')
        self.commit("hook")
        self.assertEqual(self.run_tool("--check")[0], 0)
        self.assertEqual(self.run_tool("--release-version")[1], "1.3.1\n")

    def test_a_new_core_macro_or_field_is_a_minor(self) -> None:
        for lock, row, text in (("macros.lock", "OPENRAC_FLAG_B\t0x2u", "#define OPENRAC_FLAG_B 0x2u\n"),
                                ("types.lock", "openrac_thing\tflags\t4\tuint32_t", None)):
            with self.subTest(lock=lock):
                self.git("checkout", "-q", "--", ".")
                self.git("clean", "-qfd")
                self.put(f"sdk/api/{lock}", lock_text(LOCKS[lock] + [row]))
                if text:
                    self.put(bump_api.HEADER, self.text(bump_api.HEADER) + text)
                code, _, err = self.run_tool("--check")
                self.assertEqual(code, 1)
                self.assertIn("the interface is 1.4", err)
                self.run_tool("minor", "--note", "a flag", "--name", "flag")
                self.assertEqual(self.run_tool("--check")[0], 0)

    def test_a_moving_or_fixed_value_is_not_a_minor(self) -> None:
        self.put("sdk/api/macros.lock", lock_text(["OPENRAC_FLAG_A\t0x1u", "OPENRAC_MOD_API_MINOR\t4"]))
        self.assertEqual(self.run_tool("--check")[0], 0)

    def test_a_note_alone_opens_the_minor(self) -> None:
        # A change of what the table means, with no new row.
        self.put("sdk/changes/1.4/meaning.md", "Paint draws effects too.\n")
        code, _, err = self.run_tool("--check")
        self.assertIn("the interface is 1.4", err)
        self.assertEqual(self.run_tool()[0], 0)
        self.assertEqual(self.run_tool("--release-version", "--stable")[1], "1.4.0\n")

    def test_dev_releases_are_patches_until_the_next_stable(self) -> None:
        self.add_member("glow", "1.4", places=4)
        self.put("sdk/changes/1.4/glow.md", "Glow (glow).\n")
        self.commit("glow")
        self.assertEqual(self.run_tool("--release-version")[1], "1.3.1\n")
        self.git("tag", "sdk-v1.3.1")
        # --versions: a dev release's interface is its header's (1.4 already), a stable one's its tag's.
        table = self.run_tool("--versions")[1]
        self.assertIn("| 1.3.1 | 1.4 |", table)
        self.assertIn("| 1.3.0 | 1.3 |", table)
        # A dev release does not close the minor: the next addition joins 1.4 too.
        self.add_member("dim", "1.4")
        self.put("sdk/changes/1.4/dim.md", "Dim (dim).\n")
        self.assertEqual(self.run_tool("--check")[0], 0)
        self.assertEqual(self.run_tool("--release-version")[1], "1.3.2\n")
        self.commit("dim")
        self.git("tag", "sdk-v1.4.0")  # the stable launcher's release
        self.assertEqual(self.run_tool("--release-version", "--stable")[1], "\n")  # nothing new
        self.assertEqual(self.run_tool("--release-version")[1], "\n")
        self.put("sdk/docs/guide.md", "After the stable.\n")
        self.assertEqual(self.run_tool("--release-version")[1], "1.4.1\n")
        self.assertEqual(self.run_tool("--release-version", "--stable")[1], "\n")

    def test_a_major_starts_at_0(self) -> None:
        for tag in self.git("tag").split():
            self.git("tag", "-d", tag)
        self.put(bump_api.HEADER, self.text(bump_api.HEADER).replace("API_MINOR 3", "API_MINOR 0"))
        self.put(bump_api.MEMBERS, MEMBERS.replace("1.3", "1.0"))
        self.put(bump_api.CPP, CPP.replace("m_api->api_minor >= 3\n               && ", ""))
        self.assertEqual(self.run_tool("--check")[:2], (0, "SDK interface 1.0 in every place (no sdk-v1.* release yet)\n"))
        self.assertEqual(self.run_tool("--release-version")[1], "1.0.0\n")
        self.assertEqual(self.run_tool("--release-version", "--stable")[1], "1.0.0\n")
        self.put(bump_api.HEADER, self.text(bump_api.HEADER).replace("API_MINOR 0", "API_MINOR 3"))
        self.assertIn("says 1.3, the interface is 1.0", self.run_tool("--check")[2])

    def test_old_minors_notes_before_the_first_release_with_them(self) -> None:
        # sdk-v1.3.0 has no sdk/changes/: the old minors' notes moved out of the header are not a
        # branch's, so they open nothing and move nowhere.
        self.put("sdk/changes/1.2/notes.md", "The keyboard (keys).\n")
        self.put("sdk/changes/1.3/notes.md", "The pointer.\n")
        self.assertEqual(self.run_tool("--check")[0], 0)
        self.assertEqual(self.run_tool()[0], 0)
        self.assertTrue((self.root / "sdk/changes/1.3/notes.md").exists())
        self.commit("notes")
        self.git("tag", "sdk-v1.3.1")
        # Once a release has them, a new note in a released minor's folder is a branch's: moved.
        self.put("sdk/changes/1.3/late.md", "Late.\n")
        self.assertIn("1.3 is released (sdk-v1.3.0)", self.run_tool("--check")[2])
        self.assertEqual(self.run_tool()[0], 0)
        self.assertTrue((self.root / "sdk/changes/1.4/late.md").exists())
        self.assertTrue((self.root / "sdk/changes/1.3/notes.md").exists())

    def test_rows_the_release_has_must_be_here(self) -> None:
        self.put("sdk/api/types.lock", lock_text(LOCKS["types.lock"][:1]))
        code, _, err = self.run_tool("--check")
        self.assertEqual(code, 1)
        self.assertIn("merge develop into this branch first", err)
        self.assertNotEqual(self.run_tool()[0], 0)  # nothing written
        self.assertEqual(bump_api.header_api(self.root), (1, 3))

    def test_no_note_folder_past_the_open_minor(self) -> None:
        self.put("sdk/changes/1.5/x.md", "X.\n")
        code, _, err = self.run_tool("--check")
        self.assertEqual(code, 1)
        self.assertIn("the open minor is 1.4 at most", err)

    def test_a_bad_note_name(self) -> None:
        self.add_member("glow", "1.4")
        self.assertEqual(self.run_tool("minor", "--note", "x (glow)", "--name", "Bad Name")[0], 2)
        self.put("sdk/changes/1.4/Bad Name.md", "glow\n")
        self.run_tool()
        self.assertIn("a note's name is lower case", self.run_tool("--check")[2])

    def test_the_note_takes_the_branch_name(self) -> None:
        self.add_member("glow", "1.4")
        self.assertEqual(self.run_tool("minor", "--note", "glow (glow)")[0], 0)
        self.assertTrue((self.root / "sdk/changes/1.4/glow.md").exists())  # feature/glow
        self.git("checkout", "-q", "-B", "develop")
        self.assertEqual(self.run_tool("minor", "--note", "x")[0], 2)  # no branch name to take

    def test_patch_and_note_use(self) -> None:
        self.assertEqual(self.run_tool("patch")[0], 0)
        self.assertEqual(bump_api.header_api(self.root), (1, 3))
        self.assertEqual(self.run_tool("patch", "--note", "x")[0], 2)
        self.assertEqual(self.run_tool("minor")[0], 2)
        self.assertEqual(self.run_tool("--note", "x")[0], 2)

    def test_the_highest_tag_is_the_release(self) -> None:
        self.git("tag", "sdk-v1.3.10")
        self.git("tag", "sdk-v1.3.9")
        self.git("tag", "sdk-v1.3.x")
        self.put("sdk/docs/guide.md", "New.\n")
        self.assertEqual(self.run_tool("--release-version")[1], "1.3.11\n")

    def test_a_dev_release_without_a_stable_one(self) -> None:
        self.git("tag", "-d", "sdk-v1.3.0")
        self.git("tag", "sdk-v1.3.4")
        self.assertNotEqual(self.run_tool("--release-version")[0], 0)

    def test_old_local_tags_are_refused_when_offline(self) -> None:
        # The canonical repository cannot be reached and this clone's tags stop at 1.1.0 while the
        # tree is at 1.3: the tool refuses to renumber anything from them.
        self.git("tag", "-d", "sdk-v1.3.0")
        self.git("tag", "sdk-v1.1.0")
        env = {"OPENRAC_SDK_OFFLINE": "0", "OPENRAC_SDK_REPO": str(self.root / "nowhere")}
        with unittest.mock.patch.dict(os.environ, env), \
                unittest.mock.patch.object(bump_api, "_remote", {}), \
                unittest.mock.patch.object(bump_api, "_fell_back", False):
            code, _, err = self.run_tool("--check")
            self.assertNotEqual(code, 0)
            self.assertIn("the local tags end at sdk-v1.1.0, older than this tree", err)
            self.assertEqual(err.count("cannot list the tags"), 1)  # asked once
            self.assertNotEqual(self.run_tool()[0], 0)
            self.assertIn("API_MINOR 3", self.text(bump_api.HEADER))  # nothing written

    def test_the_header_disagrees(self) -> None:
        self.put(bump_api.HEADER, self.text(bump_api.HEADER).replace("API_MINOR 3", "API_MINOR 2"))
        self.assertEqual(bump_api.problems(self.root, 3), [f"{bump_api.HEADER} says 1.2, the interface is 1.3"])
        self.assertEqual(self.run_tool("--check")[0], 1)
        self.assertEqual(self.run_tool()[0], 0)
        self.assertEqual(bump_api.problems(self.root, 3), [])


class RealRepoTest(unittest.TestCase):

    def test_archived_is_what_package_sdk_takes(self) -> None:
        root = bump_api.package_sdk.ROOT
        for name, path in bump_api.package_sdk.files().items():
            self.assertTrue(bump_api.archived(path.relative_to(root).as_posix()), name)
        for path in ("sdk/tools/bump_api.py", "sdk/api/members.tsv", "sdk/changes/1.43/x.md",
                     "sdk/bindings/rac1.json", "sdk/docs/build/x.md", "docs/port/MOD_API.md", "port/x.cpp"):
            self.assertFalse(bump_api.archived(path), path)
        self.assertTrue(bump_api.archived("sdk/bindings/rac9.ids.lock"))


if __name__ == "__main__":
    unittest.main()
