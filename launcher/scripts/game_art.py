#!/usr/bin/env python3
"""Each game's pictures in public/img, made from its SteamGridDB hero.

    python3 scripts/game_art.py [--cache DIR] [--only rac1,rac3]

For every game it writes three files (the site's names: rac1, gc, uya, deadlocked, size-matters,
sac):

- <name>-bg.webp: the hero, behind the game's band once its page is open;
- <name>-tile.webp: the same picture framed for a tile, the game's name written below it (Android's
  Games tab);
- <name>-thumb.webp: the small one for the game list.

The look is the launcher's, the same for every game: shadows pulled toward the page's warm coal in
the game's hue, the light under the bar (a glow from the top in the game's accent) and a soft bloom
on the bright parts; dark pictures are lifted a little without going grey. On top, a stage light in
the game's colour: its accent shines in from the top right and the foot and deep shadows sink into
the bar's dark ink, so each picture sits in its game's accent. The characters are not
retouched. The heroes carry no logo: the tile writes the game's name itself.

The downloads are kept in --cache (default ~/.cache/openrac-game-art), so a run after the first
needs no network. Needs Pillow, numpy and opencv-python-headless.
"""

import argparse
import json
import sys
import urllib.request
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter


HERE = Path(__file__).resolve().parent
IMG = HERE.parent / "public" / "img"
PALETTES = HERE.parent / "src" / "lib" / "palettes.json"
CDN = "https://cdn2.steamgriddb.com"

# game: (site name, hero, tile frame's left edge in the 1920 x 620 hero). The frame keeps the faces
# in sight. A hero under art/ is a picture made by hand and used as it is, without the grade below:
# Going Commando's is the characters from SteamGridDB's hero set on a scene of Oozla from the game
# (user decision).
GAMES = {
    "rac1": ("rac1", "hero/8bed36d1d9aac31ed938f56a5671a420.jpg", 230),
    "rac2": ("gc", "art/gc.jpg", 525),
    "rac3": ("uya", "hero/3bd318565e4adbe5f4b6abf2ffebf3a0.png", 400),
    "rac4": ("deadlocked", "hero/d22bb534b7964c0cafab6aab1995f000.jpg", 300),
    "rac5": ("size-matters", "hero/0102d6b30badc91266a51a24e8203c7d.png", 100),
    "rac6": ("sac", "hero/7e96c0d5321335207e7ae3aee2e3c0c2.png", 270),
}  # fmt: skip


TILE = (
    1280,
    569,
)  # 2.25 : 1, a tile on a tablet held sideways; GameTile anchors it top left
THUMB = (192, 132)  # the game list shows 64 x 44
Y = np.array([0.2126, 0.7152, 0.0722], np.float32)


def rgb(hex_colour: str) -> np.ndarray:
    return (
        np.array([int(hex_colour[i : i + 2], 16) for i in (1, 3, 5)], np.float32) / 255
    )


def colours(game: str) -> dict[str, np.ndarray]:
    """The game's colours: its accent (--glow), and the bar's ink, brand, amber and gold."""
    p = json.loads(PALETTES.read_text())
    c = dict(p["base"]) | p["game_colors"].get(game, {}) | p["accents"].get(game, {})
    return {
        k: rgb(c[f"--{k}"])
        for k in ("glow", "ink", "bar-ink", "brand", "amber", "gold")
    }


def screen(a: np.ndarray, b: np.ndarray) -> np.ndarray:
    return 1 - (1 - a) * (1 - b)


def grade(img: Image.Image, c: dict[str, np.ndarray]) -> Image.Image:
    """The launcher's look on a picture (see the module's text)."""
    a = np.asarray(img.convert("RGB"), np.float32) / 255
    h, w = a.shape[:2]
    acc, coal = c["glow"], c["ink"]
    # A dark picture is lifted toward a 0.30 median (gamma at most 0.85), its colour kept.
    med = float(np.median(a @ Y))
    g = (
        1.0
        if med >= 0.30
        else max(0.85, np.log(med + (0.30 - med) * 0.5) / np.log(max(med, 1e-3)))
    )
    lum = (a @ Y)[..., None]
    a = a * (lum**g / np.maximum(lum, 1e-4))
    # Vibrance: the duller a pixel, the more it gains.
    sat = (a.max(2, keepdims=True) - a.min(2, keepdims=True)) / np.maximum(
        a.max(2, keepdims=True), 1e-4
    )
    lum = (a @ Y)[..., None]
    a = np.clip(lum + (a - lum) * (1 + 0.20 * (1 - sat) ** 2), 0, 1)
    # Shadows take the page's coal, lit in the game's hue: one ink under every game.
    shade = coal * 0.7 + acc * 0.105
    lum = (a @ Y)[..., None]
    k = 0.34 * np.clip(1 - lum / 0.6, 0, 1) ** 1.4
    a = a * (1 - k) + (a * 0.55 + shade * 0.45) * k
    # The light under the bar (app.css body::after): from the top middle, in the accent.
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    top = (
        np.clip(1 - np.sqrt(((xx / w - 0.5) / 0.65) ** 2 + (yy / h / 1.1) ** 2), 0, 1)
        ** 1.5
    )
    a = screen(a, acc * top[..., None] * 0.26)
    # Bloom: the bright parts give off a soft light in the accent.
    hi = np.clip(((a @ Y) - 0.62) / 0.38, 0, 1)
    hi = Image.fromarray((hi * 255).astype(np.uint8)).filter(
        ImageFilter.GaussianBlur(w * 0.012)
    )
    a = screen(
        a, (acc * 0.6 + 0.4) * (np.asarray(hi, np.float32) / 255)[..., None] * 0.30
    )
    # The edges sink a little into the page.
    edge = np.sqrt(((xx / w - 0.5) / 0.62) ** 2 + ((yy / h - 0.45) / 0.85) ** 2)
    v = (0.30 * np.clip((edge - 0.55) / 0.6, 0, 1))[..., None]
    a = a * (1 - v) + coal * v
    # Stage light in the game's colour: its accent shines in from the top right, the foot fades
    # into the bar's dark ink and the deep shadows take that ink too.
    deep = c["bar-ink"]
    light = (
        np.clip(
            1 - np.sqrt(((xx / w - 0.85) / 0.7) ** 2 + ((yy / h + 0.1) / 0.9) ** 2),
            0,
            1,
        )
        ** 1.3
    )
    a = screen(a * 0.85, acc * light[..., None] * 0.75)
    foot = (np.clip((yy / h - 0.45) / 0.55, 0, 1) ** 1.5)[..., None] * 0.7
    a = a * (1 - foot) + deep * foot
    dark = 0.35 * np.clip(1 - (a @ Y) / 0.4, 0, 1)[..., None]
    a = a * (1 - dark) + deep * dark
    return Image.fromarray((np.clip(a, 0, 1) * 255 + 0.5).astype(np.uint8))


def tile(picture: Image.Image, left: int) -> Image.Image:
    """The tile: a 2.25 : 1 frame of the picture from LEFT."""
    w = round(picture.height * TILE[0] / TILE[1])
    left = min(left, picture.width - w)
    return picture.crop((left, 0, left + w, picture.height)).resize(TILE, Image.LANCZOS)


def thumb(picture: Image.Image) -> Image.Image:
    s = max(THUMB[0] / picture.width, THUMB[1] / picture.height)
    small = picture.resize(
        (round(picture.width * s), round(picture.height * s)), Image.LANCZOS
    )
    x = (small.width - THUMB[0]) // 2
    return small.crop((x, 0, x + THUMB[0], THUMB[1]))


def fetch(cache: Path, path: str) -> Path:
    out = cache / path
    if not out.is_file():
        out.parent.mkdir(parents=True, exist_ok=True)
        req = urllib.request.Request(
            f"{CDN}/{path}", headers={"User-Agent": "openrac-game-art"}
        )
        with urllib.request.urlopen(req, timeout=60) as r:
            out.write_bytes(r.read())
    return out


def main(argv: list[str]) -> int:
    p = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    p.add_argument(
        "--cache", type=Path, default=Path.home() / ".cache" / "openrac-game-art"
    )
    p.add_argument(
        "--out", type=Path, default=IMG, help="where to write (default public/img)"
    )
    p.add_argument("--only", help="comma-separated game ids (default all)")
    a = p.parse_args(argv)
    games = a.only.split(",") if a.only else list(GAMES)
    for game in games:
        name, hero, left = GAMES[game]
        c = colours(game)
        if hero.startswith("art/"):
            picture = Image.open(HERE / hero).convert("RGB")
        else:
            picture = grade(Image.open(fetch(a.cache, hero)), c)
        picture.save(a.out / f"{name}-bg.webp", quality=88, method=6)
        tile(picture, left).save(a.out / f"{name}-tile.webp", quality=88, method=6)
        thumb(picture).save(a.out / f"{name}-thumb.webp", quality=80, method=6)
        print(f"{game}: {name}-bg, -tile, -thumb")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
