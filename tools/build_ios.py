#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Build the native iPhone/iPad application; keep discs and build output private."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--moltenvk", type=Path, required=True,
                        help="MoltenVK.framework from the iOS ARM64 XCFramework slice")
    parser.add_argument("--disc", type=Path, required=True, help="Your Ratchet & Clank NTSC-U ISO")
    parser.add_argument("--python-xcframework", type=Path, required=True, help="Python 3.14 iOS XCFramework")
    parser.add_argument("--launcher-library", type=Path, help="Prebuilt openrac-launcher-ios static library")
    parser.add_argument("--skip-launcher-build", action="store_true", help="Use the existing shared frontend and adapter")
    parser.add_argument("--game", default="rac1-ntsc", help="Registered iOS engine target")
    parser.add_argument("--store-url", default="", help="Optional App Store HTTPS page")
    parser.add_argument("--build-number", default="1")
    parser.add_argument("--store-artwork", action="store_true", help="Use original OpenRAC art and omit desktop game pictures")
    parser.add_argument("--team", default="")
    parser.add_argument("--bundle-id", default="dev.openrac.launcher.ios")
    parser.add_argument("--jobs", type=int, default=6)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    if not shutil.which("cmake") or not shutil.which("ninja"):
        parser.error("Put CMake and Ninja on PATH before building.")
    if not (args.moltenvk / "MoltenVK").is_file() or not args.disc.is_file():
        parser.error("The iOS framework and your disc image must exist.")
    if not args.skip_launcher_build:
        subprocess.run(["yarn", "build"], cwd=root / "launcher", check=True)
        subprocess.run(["cargo", "build", "--manifest-path", str(root / "launcher/ios/Cargo.toml"),
                        "--release", "--target", "aarch64-apple-ios", "--locked"], check=True)
    target_dir = Path(os.environ.get("CARGO_TARGET_DIR", root / "launcher/target")).resolve()
    library = args.launcher_library or target_dir / "aarch64-apple-ios/release/libopenrac_launcher_ios.a"
    if not library.is_file():
        parser.error("Build openrac-launcher-ios for aarch64-apple-ios or provide --launcher-library.")
    subprocess.run([
        "cmake", "-S", str(root / "port"), "-B", str(args.build_dir.resolve()), "-G", "Ninja",
        "-DCMAKE_SYSTEM_NAME=iOS", "-DCMAKE_OSX_SYSROOT=iphoneos", "-DCMAKE_OSX_ARCHITECTURES=arm64",
        "-DCMAKE_OSX_DEPLOYMENT_TARGET=17.0", "-DCMAKE_BUILD_TYPE=Release",
        "-DOPENRAC_STATIC_SDL=ON", "-DOPENRAC_ENHANCED_RENDERING=OFF",
        f"-DPython3_EXECUTABLE={sys.executable}",
        f"-DOPENRAC_IOS_MOLTENVK_FRAMEWORK={args.moltenvk.resolve()}",
        f"-DOPENRAC_IOS_PYTHON_XCFRAMEWORK={args.python_xcframework.resolve()}",
        f"-DOPENRAC_IOS_LAUNCHER_LIBRARY={library.resolve()}",
        f"-DOPENRAC_{args.game.upper().replace(chr(45), chr(95))}_RETAIL_IMAGE={args.disc.resolve()}",
        f"-DOPENRAC_IOS_GAME={args.game}", f"-DOPENRAC_IOS_STORE_URL={args.store_url}",
        f"-DOPENRAC_IOS_STORE_ARTWORK={'ON' if args.store_artwork else 'OFF'}",
        f"-DOPENRAC_IOS_BUILD_NUMBER={args.build_number}", f"-DOPENRAC_IOS_TEAM={args.team}", f"-DOPENRAC_IOS_BUNDLE_ID={args.bundle_id}",
    ], check=True)
    subprocess.run(["cmake", "--build", str(args.build_dir.resolve()), "--target", f"openrac-{args.game}",
                    "--parallel", str(args.jobs)], check=True)
    print(f"Built {args.build_dir.resolve() / 'OpenRAC.app'} (sign before installing).")


if __name__ == "__main__":
    main()
