#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Compile Apple's icon catalogue and carry OpenRAC's notices into the iOS app."""
from pathlib import Path
import json
import os
import plistlib
import re
import shutil
import subprocess
import sys
from package_python import install

app, build = (Path(arg) for arg in sys.argv[1:3])
here = Path(__file__).resolve().parent
root = here.parents[1]
catalogue = build / "ios-assets/Assets.xcassets"
subprocess.run(["xcrun", "--sdk", "macosx", "swift", str(here / "make_assets.swift"),
                str(root / "launcher/src-tauri/icons/icon.png"), str(catalogue)], check=True)
partial = build / "ios-assets/icons.plist"
subprocess.run(["xcrun", "actool", "--compile", str(app), "--platform", "iphoneos",
                "--minimum-deployment-target", "17.0", "--target-device", "iphone", "--target-device", "ipad",
                "--app-icon", "AppIcon", "--output-partial-info-plist", str(partial), str(catalogue)], check=True)
info = app / "Info.plist"
metadata = plistlib.loads(info.read_bytes())
metadata.update(plistlib.loads(partial.read_bytes()))
sdk = subprocess.check_output(["xcrun", "--sdk", "iphoneos", "--show-sdk-version"], text=True).strip()
sdk_build = subprocess.check_output(["xcrun", "--sdk", "iphoneos", "--show-sdk-build-version"], text=True).strip()
xcode = subprocess.check_output(["xcodebuild", "-version"], text=True).splitlines()
version = xcode[0].split()[-1].split(".")
metadata.update({
    "CFBundleSupportedPlatforms": ["iPhoneOS"], "MinimumOSVersion": "17.0",
    "DTPlatformName": "iphoneos", "DTPlatformVersion": sdk, "DTSDKName": "iphoneos" + sdk,
    "DTSDKBuild": sdk_build, "DTPlatformBuild": sdk_build,
    "DTXcode": f"{int(version[0]):02d}{int(version[1]):01d}0", "DTXcodeBuild": xcode[1].split()[-1],
    "DTCompiler": "com.apple.compilers.llvm.clang.1_0",
})
info.write_bytes(plistlib.dumps(metadata))
shutil.copy2(here / "PrivacyInfo.xcprivacy", app / "PrivacyInfo.xcprivacy")
for name in ("COPYING", "LICENSE.md", "CREDITS.md", "THIRD_PARTY_NOTICES.md"):
    shutil.copy2(root / name, app / name)
notices = app / "Notices"
notice_files = {"launcher/THIRD_PARTY_NOTICES.md", "port/game/common/fonts/OFL-Inter.txt",
                "port/game/common/fonts/OFL-Orbitron.txt"}
for engine in json.loads((build / "ios-bundle.json").read_text())["engines"]:
    notice_files.add(f"games/{engine['game']}/THIRD_PARTY_NOTICES.md")
    notice_files.add(f"games/{engine['game']}/{engine['version']}/THIRD_PARTY_NOTICES.md")
for relative in sorted(notice_files):
    target = notices / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(root / relative, target)
for folder in list((build / "_deps").glob("*-src")) + list((root / "launcher/node_modules/@fontsource").glob("*")) + list((root / "launcher/node_modules/@fontsource-variable").glob("*")):
    for file in folder.iterdir():
        if file.is_file() and file.name.upper().split(".")[0] in ("LICENSE", "COPYING", "NOTICE"):
            target = notices / folder.name / file.name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(file, target)
for folder in Path(sys.argv[4]).parents[:5]:
    license = next((folder / name for name in ("LICENSE", "LICENSE.txt") if (folder / name).is_file()), None)
    if license:
        shutil.copy2(license, notices / "MoltenVK-LICENSE.txt")
        break
else:
    raise RuntimeError("Keep MoltenVK's LICENSE beside its official distribution when packaging.")
cargo_home = Path(os.environ.get("CARGO_HOME", Path.home() / ".cargo"))
cargo = shutil.which("cargo") or str(cargo_home / "bin/cargo")
tree = subprocess.check_output([cargo, "tree", "--manifest-path", str(root / "launcher/ios/Cargo.toml"),
    "-p", "openrac-launcher-ios", "--target", "aarch64-apple-ios",
    "--edges", "normal,build", "--prefix", "none", "--format", "{p}", "--offline"], text=True)
packages = set(re.findall(r"^([A-Za-z0-9_-]+) v([^\s]+)", tree, re.MULTILINE))
for name, version in sorted(packages):
    if name in ("openrac-launcher-core", "openrac-launcher-ios"):
        continue
    folders = list((cargo_home / "registry/src").glob(f"*/{name}-{version}"))
    if name == "zip" and not folders:
        folders = [path for path in (cargo_home / "git/checkouts").glob("zip2-*/*") if (path / "Cargo.toml").is_file()]
    if not folders:
        raise RuntimeError(f"Build the Rust core first so its license is available: {name} {version}")
    for file in folders[0].iterdir():
        if file.is_file() and file.name.upper().startswith(("LICENSE", "COPYING", "NOTICE")):
            target = notices / "rust" / f"{name}-{version}" / file.name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(file, target)
(notices / "rust/DEPENDENCIES.txt").write_text("\n".join(f"{name} {version}" for name, version in sorted(packages)) + "\n")
install(app, Path(sys.argv[3]), root)
