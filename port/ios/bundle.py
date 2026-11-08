# SPDX-License-Identifier: GPL-3.0-or-later
"""Describe the engine actually linked into an iOS app, using the shared catalogue."""
import argparse
import json
from pathlib import Path


def configuration(root, target, store_url="", store_artwork=False):
    supported = json.loads((root / "port/ios/engines.json").read_text())
    if target not in supported:
        raise ValueError(f"No iOS importer is registered for {target}")
    engine = dict(supported[target])
    game, version = engine["key"].split("/")
    manifest = json.loads((root / f"games/{game}/game.json").read_text())
    build = manifest["versions"][version]
    engine.update(game=game, version=version, serial=build["serial"], target=target,
                  discSize=build["disc"]["size"], bootFile=build["boot"]["file"],
                  bootSize=build["boot"]["size"], bootSha1=build["boot"]["sha1"], levels=f"port/{version}")
    if store_url and not store_url.startswith("https://apps.apple.com/"):
        raise ValueError("The optional update page must be an App Store HTTPS URL")
    package = json.loads((root / "launcher/package.json").read_text())
    return {"version": package["version"], "storeUrl": store_url, "storeArtwork": store_artwork, "engines": [engine]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--target", required=True)
    parser.add_argument("--store-url", default="")
    parser.add_argument("--store-artwork", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    rendered = json.dumps(configuration(args.root, args.target, args.store_url, args.store_artwork), indent=2) + "\n"
    if not args.output.exists() or args.output.read_text() != rendered:
        args.output.write_text(rendered)


if __name__ == "__main__":
    main()
