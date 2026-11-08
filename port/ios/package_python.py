# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Package signed extraction code; never copy a checkout's game data into the app."""
from pathlib import Path
import json
import plistlib
import shutil


def install(app, framework, root):
    frameworks = app / "Frameworks"
    shutil.copytree(framework / "ios-arm64/Python.framework", frameworks / "Python.framework", dirs_exist_ok=True)
    destination = app / "python/lib"
    ignored = shutil.ignore_patterns("__pycache__", "*.pyc", "*.so", "*.dylib", "test", "tests", "idlelib", "tkinter", "ensurepip", "site-packages")
    for source in (framework / "lib", framework / "ios-arm64/lib-arm64"):
        shutil.copytree(source, destination, dirs_exist_ok=True, ignore=ignored)
    native = framework / "ios-arm64/lib-arm64/python3.14/lib-dynload"
    modules = destination / "python3.14/lib-dynload"
    modules.mkdir(parents=True, exist_ok=True)
    template = plistlib.loads((framework / "build/iOS-dylib-Info-template.plist").read_bytes())
    for source in native.glob("*.so"):
        name = source.name.split(".")[0]
        if name.startswith("_test"):
            continue
        folder = frameworks / (name + ".framework")
        folder.mkdir(exist_ok=True)
        shutil.copy2(source, folder / name)
        metadata = dict(template, CFBundleExecutable=name, CFBundlePackageType="FMWK",
                        CFBundleIdentifier="org.python.ios." + name.replace("_", "-"))
        (folder / "Info.plist").write_bytes(plistlib.dumps(metadata))
        marker = modules / (source.stem + ".fwork")
        marker.write_text(f"Frameworks/{name}.framework/{name}\n")
        (folder / (name + ".origin")).write_text(str(marker.relative_to(app)) + "\n")
    tools = app / "app_tools"
    # An explicit source-only allowlist keeps every private disc and generated export out.
    for relative in ("editor", "tools/openrac_common", "port/ios/importers"):
        source = root / relative
        for file in source.rglob("*"):
            if file.is_file() and file.suffix in (".py", ".json", ".tsv") and "__pycache__" not in file.parts and not file.name.startswith("test_"):
                target = tools / file.relative_to(root)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(file, target)
    dispatcher = tools / "port/ios/import_disc.py"
    dispatcher.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(root / "port/ios/import_disc.py", dispatcher)
    for manifest in (root / "games").glob("*/game.json"):
        target = tools / manifest.relative_to(root)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(manifest, target)
    for relative in ("launcher/actions.json", "progress/summary.json"):
        file = root / relative
        if file.is_file():
            target = tools / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(file, target)
    for file in (root / "port/game").glob("*/differences.json"):
        target = tools / file.relative_to(root)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(file, target)
    # Keep the shared launcher's artwork, including the game-page backgrounds used on desktop.
    store_artwork = json.loads((app.parent / "ios-bundle.json").read_text()).get("storeArtwork", False)
    install_launcher(app, root, store_artwork)
    shutil.copy2(app.parent / "ios-bundle.json", app / "ios-bundle.json")
    (tools / "STANDALONE").write_text("OpenRAC's bundled disc preparation tools. No game data.\n")
    forbidden = {".iso", ".chd", ".elf", ".glb", ".wad", ".vag"}
    for file in app.rglob("*"):
        if file.is_file() and (file.suffix.lower() in forbidden or file.name == "SCUS_971.99"):
            raise RuntimeError(f"Game data must never be packaged: {file.relative_to(app)}")


def install_launcher(app, root, store_artwork=False):
    dist = root / "launcher/dist"
    def ignored_art(directory, names):
        hidden = {name for name in names if name.startswith("mock-") and name.endswith(".js")}
        if store_artwork and Path(directory) == dist / "img":
            hidden.update(name for name in names if name != "store")
        return hidden
    shutil.copytree(dist, app / "launcher-dist", dirs_exist_ok=True, ignore=ignored_art)
    if store_artwork:
        pictures = app / "launcher-dist/img"
        if any(file.is_file() and "store" not in file.relative_to(pictures).parts for file in pictures.rglob("*")):
            raise RuntimeError("Store artwork requires a fresh launcher-dist: prior desktop images remain in the build bundle.")
