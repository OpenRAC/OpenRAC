"""Windows dependency setup, without running an installer or needing Windows."""
import contextlib
import io
import os
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import build_port
import build_port_tools as build_tools
import build_game


class WindowsDependencies(unittest.TestCase):
    def setUp(self):
        self.stack = contextlib.ExitStack()
        self.addCleanup(self.stack.close)
        self.stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
        self.stack.enter_context(mock.patch.object(build_port, "WINDOWS", True))

    def test_installs_only_missing_tools_and_rechecks_until_ready(self):
        ready = ({"PATH": "new tools"}, Path("cmake"), ("clang", "clang++", "ninja"))
        missing = [build_port.MissingTool("no C++ tools", "Microsoft.VisualStudio.2022.BuildTools"),
                   build_port.MissingTool("no git", "Git.Git")]
        with mock.patch.object(build_port, "prepare_tools", side_effect=[*missing, ready]) as prepare, \
                mock.patch.object(build_port, "install_windows_tool") as install:
            self.assertEqual(build_port.prepare({}), ready)
        self.assertEqual(prepare.call_count, 3)
        self.assertEqual(install.call_args_list, [mock.call(tool) for tool in missing])

    def test_existing_tools_never_start_an_installer(self):
        with mock.patch.object(build_port, "prepare_tools", return_value="ready"), \
                mock.patch.object(build_port, "install_windows_tool") as install:
            self.assertEqual(build_port.prepare({}), "ready")
        install.assert_not_called()

    def test_opt_out_and_other_platforms_keep_the_missing_tool_error(self):
        missing = build_port.MissingTool("missing compiler", "LLVM.LLVM")
        for windows, enabled in ((True, False), (False, True)):
            with self.subTest(windows=windows, enabled=enabled), \
                    mock.patch.object(build_port, "WINDOWS", windows), \
                    mock.patch.object(build_port, "prepare_tools", side_effect=missing), \
                    mock.patch.object(build_port, "install_windows_tool") as install:
                with self.assertRaises(build_port.MissingTool):
                    build_port.prepare({}, install_deps=enabled)
                install.assert_not_called()

    def test_an_unusable_installation_is_not_retried_forever(self):
        with mock.patch.object(build_port, "prepare_tools", side_effect=build_port.MissingTool("missing", "Git.Git")), \
                mock.patch.object(build_port, "install_windows_tool") as install:
            with self.assertRaisesRegex(build_port.BuildError, "still unavailable"):
                build_port.prepare({})
        install.assert_called_once()

    def test_broken_environment_does_not_trigger_installation(self):
        with mock.patch.object(build_port, "prepare_tools", side_effect=build_port.BuildError("SDK setup failed")), \
                mock.patch.object(build_port, "install_windows_tool") as install:
            with self.assertRaisesRegex(build_port.BuildError, "SDK setup failed"):
                build_port.prepare({})
        install.assert_not_called()

    def test_failed_visual_studio_discovery_does_not_trigger_installation(self):
        for failure in (OSError("access denied"), mock.Mock(returncode=7)):
            with self.subTest(failure=failure), \
                    mock.patch.object(build_port, "windows_path", return_value=""), \
                    mock.patch.object(Path, "is_file", return_value=True), \
                    mock.patch.object(build_port.subprocess, "run", side_effect=[failure]), \
                    mock.patch.object(build_port, "install_windows_tool") as install:
                with self.assertRaisesRegex(build_port.BuildError, "Could not inspect Visual Studio"):
                    build_port.prepare({})
                install.assert_not_called()

    def test_installer_arguments_and_failures(self):
        for package in ("Git.Git", "LLVM.LLVM", "Ninja-build.Ninja", "Kitware.CMake",
                        "Microsoft.VisualStudio.2022.BuildTools"):
            with self.subTest(package=package), \
                    mock.patch.object(build_tools, "modify_build_tools", return_value=False), \
                    mock.patch.object(build_tools, "ensure_winget", return_value=Path("winget.exe")) as ensure, \
                    mock.patch.object(build_tools.subprocess, "run", return_value=mock.Mock(returncode=0)) as run:
                build_tools.install_windows_tool(build_tools.MissingTool("missing", package))
                ensure.assert_called_once_with()
                command = run.call_args.args[0]
                self.assertEqual(command[:8], ["winget.exe", "install", "--exact", "--id", package,
                                                "--source", "winget", "--silent"])
                self.assertIn("--disable-interactivity", command)
                self.assertNotIn("--allow-reboot", command)
                if package.endswith("BuildTools"):
                    override = command[command.index("--override") + 1]
                    self.assertIn("Microsoft.VisualStudio.Workload.VCTools", override)
                    self.assertIn("--includeRecommended", override)
                    self.assertIn("--norestart", override)
                run.return_value.returncode = 5
                with self.assertRaisesRegex(build_tools.BuildError, "exit code 5"):
                    build_tools.install_windows_tool(build_tools.MissingTool("missing", package))

    def test_failed_winget_setup_keeps_the_missing_tool_and_manual_recovery(self):
        with mock.patch.object(build_tools, "ensure_winget", side_effect=build_tools.BuildError("Install App Installer")), \
                mock.patch.object(build_tools.subprocess, "run") as run:
            with self.assertRaises(build_tools.BuildError) as error:
                build_tools.install_windows_tool(build_tools.MissingTool("Install LLVM", "LLVM.LLVM"))
        self.assertIn("App Installer", str(error.exception))
        self.assertIn("Install LLVM", str(error.exception))
        run.assert_not_called()

    def test_existing_build_tools_are_modified_without_winget(self):
        with mock.patch.object(build_tools, "modify_build_tools", return_value=True) as modify, \
                mock.patch.object(build_tools, "find_program") as find:
            build_tools.install_windows_tool(build_tools.MissingTool("no C++", "Microsoft.VisualStudio.2022.BuildTools"))
        modify.assert_called_once()
        find.assert_not_called()

    def test_modify_existing_instance_waits_for_elevation_and_preserves_paths(self):
        installation = r"C:\Custom tools\BuildTools $x & 'test'"
        for code in (0, 3010, 5):
            with self.subTest(code=code), \
                    mock.patch.object(Path, "is_file", return_value=True), \
                    mock.patch.object(build_tools.subprocess, "run", side_effect=[
                        mock.Mock(returncode=0, stdout=installation + "\n"), mock.Mock(returncode=code),
                    ]) as run:
                if code == 5:
                    with self.assertRaisesRegex(build_tools.BuildError, "installer exited with 5"):
                        build_tools.modify_build_tools()
                else:
                    self.assertTrue(build_tools.modify_build_tools())
                query, modify = run.call_args_list
                self.assertIn("Microsoft.VisualStudio.Product.BuildTools", query.args[0])
                self.assertIn("[17.0,18.0)", query.args[0])
                self.assertNotIn("-requires", query.args[0])
                script = modify.args[0][-1]
                self.assertNotIn(installation, script)
                self.assertIn("-Verb RunAs", script)
                self.assertIn("-Wait -PassThru", script)
                arguments = modify.kwargs["env"]["OPENRAC_VS_ARGUMENTS"]
                self.assertIn('"' + installation + '"', arguments)
                self.assertTrue(arguments.startswith("modify --installPath "))
                for required in ("Microsoft.VisualStudio.Workload.VCTools", "--includeRecommended", "--norestart"):
                    self.assertIn(required, arguments)
                self.assertNotIn("--wait", arguments)

    def test_no_existing_instance_leaves_installation_to_winget(self):
        with mock.patch.object(Path, "is_file", return_value=True), \
                mock.patch.object(build_tools.subprocess, "run", return_value=mock.Mock(returncode=0, stdout="")) as run:
            self.assertFalse(build_tools.modify_build_tools())
        self.assertEqual(run.call_count, 1)
        with mock.patch.object(Path, "is_file", return_value=False), \
                mock.patch.object(build_tools.subprocess, "run") as run:
            self.assertFalse(build_tools.modify_build_tools())
        run.assert_not_called()

    def test_failed_existing_instance_modification_does_not_fall_back_to_install(self):
        with mock.patch.object(build_tools, "modify_build_tools", side_effect=build_tools.BuildError("cancelled")), \
                mock.patch.object(build_tools, "find_program") as find:
            with self.assertRaisesRegex(build_tools.BuildError, "cancelled"):
                build_tools.install_windows_tool(build_tools.MissingTool("no C++", "Microsoft.VisualStudio.2022.BuildTools"))
        find.assert_not_called()

    def test_missing_installer_or_failed_detection_gives_recovery_error(self):
        with mock.patch.object(Path, "is_file", return_value=True), \
                mock.patch.object(build_tools.subprocess, "run", return_value=mock.Mock(returncode=7)):
            with self.assertRaisesRegex(build_tools.BuildError, "vswhere exited with 7"):
                build_tools.modify_build_tools()
        with mock.patch.object(Path, "is_file", side_effect=[True, False]), \
                mock.patch.object(build_tools.subprocess, "run", return_value=mock.Mock(returncode=0, stdout="C:/VS")):
            with self.assertRaisesRegex(build_tools.BuildError, "Repair the Visual Studio Installer"):
                build_tools.modify_build_tools()

    def test_windows_build_gets_new_paths_and_reports_git_before_configuring(self):
        with mock.patch.object(build_port, "windows_path", return_value="new git and ninja"), \
                mock.patch.object(build_port, "find_visual_studio", return_value=Path("VS")), \
                mock.patch.object(build_port, "msvc_environment", return_value={"PATH": "VS tools"}), \
                mock.patch.object(build_port, "find_program", return_value=None) as find:
            with self.assertRaises(build_port.MissingTool) as error:
                build_port.prepare_tools({})
        self.assertEqual(error.exception.package, "Git.Git")
        find.assert_called_once_with("git", os.pathsep.join(("VS tools", "new git and ninja")))

    def test_windows_path_reads_registry_and_keeps_current_tools_first(self):
        registry = mock.MagicMock()
        registry.QueryValueEx.side_effect = [("machine tools", 1), ("user tools", 1)]
        with mock.patch.dict("sys.modules", winreg=registry), \
                mock.patch.dict(os.environ, {"PATH": "current tools", "LOCALAPPDATA": "local"}):
            path = build_tools.windows_path().split(os.pathsep)
        self.assertEqual(path[:3], ["current tools", "machine tools", "user tools"])
        self.assertEqual(Path(path[-1]), Path("local/Microsoft/WinGet/Links"))

    def test_build_command_opt_out_reaches_dependency_setup(self):
        with mock.patch.object(build_port.background, "lower_priority"), \
                mock.patch.object(build_port, "read_cache", return_value={}), \
                mock.patch.object(build_port, "prepare", side_effect=build_port.BuildError("missing")) as prepare, \
                contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(build_port.main(["rac2-ntsc", "--no-install-deps"]), 1)
        prepare.assert_called_once_with({}, install_deps=False)

    def test_installed_launcher_build_forwards_opt_out(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            disc = root / "disc.iso"
            disc.touch()
            for opt_out in (False, True):
                with self.subTest(opt_out=opt_out), \
                        mock.patch.object(build_game, "carried_commit", return_value="abc"), \
                        mock.patch.object(build_game, "fetch_source", return_value=root / "src"), \
                        mock.patch.object(build_game, "copy_program", return_value=root / "game.exe"), \
                        mock.patch.object(build_game, "clean_up"), \
                        mock.patch.object(build_game.subprocess, "run") as run:
                    args = ["rac2-ntsc", "--disc", str(disc), "--out", str(root)]
                    self.assertEqual(build_game.main(args + (["--no-install-deps"] if opt_out else [])), 0)
                    self.assertEqual("--no-install-deps" in run.call_args.args[0], opt_out)


class WinGetSetup(unittest.TestCase):
    def setUp(self):
        self.stack = contextlib.ExitStack()
        self.addCleanup(self.stack.close)
        self.stack.enter_context(contextlib.redirect_stdout(io.StringIO()))

    def test_healthy_winget_needs_no_setup(self):
        with mock.patch.object(build_tools, "windows_path", return_value="fresh PATH"), \
                mock.patch.object(build_tools, "find_program", return_value=Path("winget.exe")) as find, \
                mock.patch.object(build_tools.subprocess, "run", return_value=mock.Mock(returncode=0, stdout="v1.29\n")) as run:
            self.assertEqual(build_tools.ensure_winget(), Path("winget.exe"))
        find.assert_called_once_with("winget", "fresh PATH")
        run.assert_called_once_with(["winget.exe", "--version"], capture_output=True, text=True, timeout=15)

    def test_broken_alias_or_hung_client_is_unavailable(self):
        failures = (OSError("bad alias"), build_tools.subprocess.TimeoutExpired("winget", 15),
                    mock.Mock(returncode=1, stdout=""), mock.Mock(returncode=0, stdout=""))
        for failure in failures:
            with self.subTest(failure=failure), \
                    mock.patch.object(build_tools, "windows_path", return_value=""), \
                    mock.patch.object(build_tools, "find_program", return_value=Path("winget.exe")), \
                    mock.patch.object(build_tools.subprocess, "run", side_effect=[failure]):
                self.assertIsNone(build_tools.working_winget())

    def test_missing_executable_is_not_started(self):
        with mock.patch.object(build_tools, "windows_path", return_value=""), \
                mock.patch.object(build_tools, "find_program", return_value=None), \
                mock.patch.object(build_tools.subprocess, "run") as run:
            self.assertIsNone(build_tools.working_winget())
        run.assert_not_called()

    def test_registration_can_fix_winget_without_installing_a_module(self):
        with mock.patch.object(build_tools, "working_winget", side_effect=[None, Path("registered/winget.exe")]), \
                mock.patch.object(build_tools.subprocess, "run", return_value=mock.Mock(returncode=0)) as run:
            self.assertEqual(build_tools.ensure_winget(), Path("registered/winget.exe"))
        self.assertEqual(run.call_count, 1)
        self.assertIn("Add-AppxPackage -RegisterByFamilyName", run.call_args.args[0][-1])

    def test_failed_registration_falls_back_to_current_user_repair(self):
        with mock.patch.object(build_tools, "working_winget", side_effect=[None, None, Path("repaired/winget.exe")]), \
                mock.patch.object(build_tools.subprocess, "run", side_effect=[
                    mock.Mock(returncode=1), mock.Mock(returncode=0),
                ]) as run:
            self.assertEqual(build_tools.ensure_winget(), Path("repaired/winget.exe"))
        self.assertEqual(run.call_count, 2)
        command = run.call_args.args[0]
        script = command[-1]
        self.assertIn("-NonInteractive", command)
        self.assertIn("Install-PackageProvider -Name NuGet", script)
        self.assertIn("Install-Module -Name Microsoft.WinGet.Client -Scope CurrentUser -Repository PSGallery", script)
        self.assertIn("Repair-WinGetPackageManager -Force -Latest", script)
        self.assertNotIn("Set-ExecutionPolicy", script)
        self.assertNotIn("Set-PSRepository", script)
        self.assertNotIn("-AllUsers", script)

    def test_failed_or_ineffective_repair_stops_after_one_attempt(self):
        for code in (0, 5):
            with self.subTest(code=code), \
                    mock.patch.object(build_tools, "working_winget", return_value=None) as probe, \
                    mock.patch.object(build_tools.subprocess, "run", return_value=mock.Mock(returncode=code)) as run:
                with self.assertRaises(build_tools.BuildError) as error:
                    build_tools.ensure_winget()
                self.assertEqual(run.call_count, 2)
                self.assertEqual(probe.call_count, 3)
                self.assertIn(f"exit code {code}", str(error.exception))
                self.assertIn("https://apps.microsoft.com/detail/9nblggh4nns1", str(error.exception))

    def test_cannot_launch_powershell_gives_manual_recovery(self):
        with mock.patch.object(build_tools, "working_winget", return_value=None), \
                mock.patch.object(build_tools.subprocess, "run", side_effect=OSError("access denied")) as run:
            with self.assertRaisesRegex(build_tools.BuildError, "Could not start WinGet setup: access denied"):
                build_tools.ensure_winget()
        self.assertEqual(run.call_count, 1)
