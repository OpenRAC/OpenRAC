import subprocess
import unittest
from pathlib import Path
from unittest.mock import patch

import build_game
import build_tools as tools


class Prerequisites(unittest.TestCase):
    def setUp(self):
        for name, value in [
            ("find_visual_studio", Path("C:/VS")),
            ("msvc_environment", {"WINDOWSSDKDIR": "C:/SDK", "PATH": "tools"}),
            ("find_clang", (Path("clang"), Path("clang++"))),
            ("find_cmake", Path("cmake")),
            ("find_program", Path("ninja")),
        ]:
            mock = patch.object(tools.build_port, name, return_value=value)
            setattr(self, name, mock.start())
            self.addCleanup(mock.stop)
        platform = patch.object(tools.sys, "platform", "win32")
        platform.start()
        self.addCleanup(platform.stop)

    def test_ready_computer_installs_nothing(self):
        with patch.object(tools, "install") as install:
            tools.ensure(True)
        install.assert_not_called()
        self.assertEqual(tools.missing(), [])

    def test_only_clang_is_missing(self):
        self.find_clang.side_effect = tools.build_port.BuildError("no clang")
        self.assertEqual(tools.missing(), [tools.LLVM])
        text = tools.description(tools.missing())
        self.assertIn("LLVM (Clang compiler)", text)
        self.assertIn("Installed apps > LLVM > Uninstall", text)
        self.assertNotIn("Build Tools 2022 > More", text)

    def test_no_visual_studio_and_no_compiler(self):
        self.find_visual_studio.side_effect = tools.build_port.BuildError("no VS")
        self.find_clang.side_effect = tools.build_port.BuildError("no clang")
        self.assertEqual(tools.missing(), [tools.VS, tools.LLVM])

    def test_sdk_or_ninja_missing_requires_cpp_components(self):
        self.msvc_environment.return_value = {"PATH": "tools"}
        self.find_program.return_value = None
        self.assertEqual(tools.missing(), [tools.VS])

    def test_other_platforms_do_not_probe_windows(self):
        with patch.object(tools.sys, "platform", "linux"):
            self.assertEqual(tools.missing(), [])
        self.find_visual_studio.assert_not_called()

    def test_missing_tools_never_install_without_consent(self):
        self.find_clang.side_effect = tools.build_port.BuildError("no clang")
        with patch.object(tools, "install") as install:
            with self.assertRaisesRegex(tools.build_port.BuildError, "no source was downloaded"):
                tools.ensure()
        install.assert_not_called()

    def test_install_only_requested_package_and_recheck(self):
        with patch.object(tools.shutil, "which", return_value="winget"), \
                patch.object(tools.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)) as run:
            tools.install([tools.LLVM])
        self.assertEqual(run.call_count, 1)
        self.assertIn(tools.LLVM, run.call_args.args[0])
        self.assertNotIn(tools.VS, run.call_args.args[0])
        self.find_visual_studio.assert_called_once()

    def test_failed_or_cancelled_install_does_not_continue(self):
        with patch.object(tools.shutil, "which", return_value="winget"), \
                patch.object(tools.subprocess, "run", return_value=subprocess.CompletedProcess([], 1602)) as run:
            with self.assertRaisesRegex(tools.build_port.BuildError, "1602"):
                tools.install([tools.VS, tools.LLVM])
        self.assertEqual(run.call_count, 1)

    def test_cpp_package_requests_only_explicit_components_without_rebooting(self):
        with patch.object(tools.shutil, "which", return_value="winget"), \
                patch.object(tools.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)) as run:
            tools.install([tools.VS])
        command = run.call_args.args[0]
        self.assertIn(tools.VS, command)
        override = command[command.index("--override") + 1]
        args = override.split()
        components = [args[i + 1] for i, arg in enumerate(args) if arg == "--add"]
        self.assertEqual(components, [
            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "Microsoft.VisualStudio.Component.Windows11SDK.26100",
            "Microsoft.VisualStudio.Component.VC.CMake.Project",
        ])
        self.assertNotIn("Workload", override)
        self.assertNotIn("--includeRecommended", override)
        self.assertNotIn("--includeOptional", override)
        self.assertIn("--norestart", override)
        self.assertIn("--disable-interactivity", command)

    def test_cpp_description_names_components_and_their_required_dependencies(self):
        text = tools.description([tools.VS])
        for name in ("MSVC v143", "Windows SDK 10.0.26100", "CMake", "Ninja", "required Microsoft dependencies"):
            self.assertIn(name, text)
        self.assertIn("Modify to remove individual components", text)

    def test_successful_installer_is_not_enough_if_tools_are_still_missing(self):
        self.find_clang.side_effect = tools.build_port.BuildError("no clang")
        with patch.object(tools.shutil, "which", return_value="winget"), \
                patch.object(tools.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)):
            with self.assertRaisesRegex(tools.build_port.BuildError, "not ready yet"):
                tools.install([tools.LLVM])

    def test_no_package_manager_gives_manual_installation_guidance(self):
        with patch.object(tools.shutil, "which", return_value=None):
            with self.assertRaisesRegex(tools.build_port.BuildError, "Windows App Installer"):
                tools.install([tools.LLVM])

    def test_build_checks_tools_before_fetching_source(self):
        with patch.object(Path, "is_file", return_value=True), \
                patch.object(tools, "ensure", side_effect=tools.build_port.BuildError("missing tools")), \
                patch.object(build_game, "fetch_source") as fetch:
            with self.assertRaisesRegex(SystemExit, "missing tools"):
                build_game.main(["rac2-ntsc", "--disc", "disc.iso", "--out", "unused"])
        fetch.assert_not_called()


if __name__ == "__main__":
    unittest.main()
