#!/usr/bin/env python3
"""Each game's pictures in public/img, made from its SteamGridDB hero and logo.

    python3 scripts/game_art.py [--cache DIR] [--only rac1,rac3]

For every game it writes three files (the site's names: rac1, gc, uya, deadlocked, size-matters,
sac):

- <name>-bg.webp: the hero without a logo, behind the game's band once its page is open;
- <name>-tile.webp: the same picture framed for a tile, with the game's logo top left (Android's
  Games tab);
- <name>-thumb.webp: the small one for the game list.

The look is the launcher's, the same for every game: shadows pulled toward the page's warm coal in
the game's hue, the light under the bar (a glow from the top in the game's accent) and a soft bloom
on the bright parts; dark pictures are lifted a little without going grey. The characters are not
retouched. The logo is redrawn flat, as OpenRAC's own: its letters in the launcher's gold, the plate they
sit on in the darkest coal, its shapes kept as they are.

The downloads are kept in --cache (default ~/.cache/openrac-game-art), so a run after the first
needs no network. Needs Pillow, numpy, scipy and opencv-python-headless.
"""

import argparse
import json
import sys
import urllib.request
from pathlib import Path

import cv2
import numpy as np
from PIL import Image, ImageFilter
from scipy import ndimage as ndi

HERE = Path(__file__).resolve().parent
IMG = HERE.parent / "public" / "img"
PALETTES = HERE.parent / "src" / "lib" / "palettes.json"
CDN = "https://cdn2.steamgriddb.com"

# game: (site name, hero, logo, tile frame's left edge in the 1920 x 620 hero, logo height in the
# tile). The frame is chosen so the logo sits on background and the faces stay in sight.
GAMES = {
    "rac1": ("rac1", "hero/8bed36d1d9aac31ed938f56a5671a420.jpg", "logo/8b2a9c176d358811a479f771a5874c1b.png", 230, 0.42),
    "rac2": ("gc", "hero/95e1533eb1b20a97777749fb94fdb944.jpg", "logo/2e9777b99786a3ef6e5d786e2bc2e16f.png", 330, 0.46),
    "rac3": ("uya", "hero/3bd318565e4adbe5f4b6abf2ffebf3a0.png", "logo/bd73768ac25cd57aae7b69c63c8adf99.png", 400, 0.44),
    "rac4": ("deadlocked", "hero/d22bb534b7964c0cafab6aab1995f000.jpg", "logo/481263854facf41a2f8a64a21956d87b.png", 300, 0.42),
    "rac5": ("size-matters", "hero/0102d6b30badc91266a51a24e8203c7d.png", "logo/876e8108f87eb61877c6263228b67256.png", 100, 0.44),
    "rac6": ("sac", "hero/7e96c0d5321335207e7ae3aee2e3c0c2.png", "logo/09779bb7930c8a0a44360e12b538ae3c.png", 270, 0.34),
}  # fmt: skip

# How each logo's letters are told from its plate (flat_logo): tc, the colour a letter has at
# least; tl, the lightness that makes any part a letter; lmin, the lightness a coloured letter has
# at least; open, the clean-up's radius; min, the smallest letter as a share of the logo.
LOGO_TUNE = {
    "rac3": {"tc": 0.25},
    "rac4": {"tc": 0.11, "lmin": 0.10},
    "rac5": {"tc": 0.25, "min": 0.0006, "open": 1},
    "rac6": {"tc": 0.30, "tl": 0.62},
}
GOLD = np.array([0xF5, 0xC0, 0x40], np.float32) / 255  # --amber
PLATE = np.array([0x14, 0x12, 0x0E], np.float32) / 255  # --ink-deep

TILE = (1280, 569)  # 2.25 : 1, a tile on a tablet held sideways; GameTile anchors it top left
THUMB = (192, 132)  # the game list shows 64 x 44
Y = np.array([0.2126, 0.7152, 0.0722], np.float32)


def rgb(hex_colour: str) -> np.ndarray:
    return np.array([int(hex_colour[i : i + 2], 16) for i in (1, 3, 5)], np.float32) / 255


def colours(game: str) -> dict[str, np.ndarray]:
    """The game's colours: its accent (--glow), and the bar's ink, brand, amber and gold."""
    p = json.loads(PALETTES.read_text())
    c = dict(p["base"]) | p["game_colors"].get(game, {}) | p["accents"].get(game, {})
    return {k: rgb(c[f"--{k}"]) for k in ("glow", "ink", "bar-ink", "brand", "amber", "gold")}


def screen(a: np.ndarray, b: np.ndarray) -> np.ndarray:
    return 1 - (1 - a) * (1 - b)


def grade(img: Image.Image, c: dict[str, np.ndarray]) -> Image.Image:
    """The launcher's look on a picture (see the module's text)."""
    a = np.asarray(img.convert("RGB"), np.float32) / 255
    h, w = a.shape[:2]
    acc, coal = c["glow"], c["ink"]
    # A dark picture is lifted toward a 0.30 median (gamma at most 0.85), its colour kept.
    med = float(np.median(a @ Y))
    g = 1.0 if med >= 0.30 else max(0.85, np.log(med + (0.30 - med) * 0.5) / np.log(max(med, 1e-3)))
    lum = (a @ Y)[..., None]
    a = a * (lum**g / np.maximum(lum, 1e-4))
    # Vibrance: the duller a pixel, the more it gains.
    sat = (a.max(2, keepdims=True) - a.min(2, keepdims=True)) / np.maximum(a.max(2, keepdims=True), 1e-4)
    lum = (a @ Y)[..., None]
    a = np.clip(lum + (a - lum) * (1 + 0.20 * (1 - sat) ** 2), 0, 1)
    # Shadows take the page's coal, lit in the game's hue: one ink under every game.
    shade = coal * 0.7 + acc * 0.105
    lum = (a @ Y)[..., None]
    k = 0.34 * np.clip(1 - lum / 0.6, 0, 1) ** 1.4
    a = a * (1 - k) + (a * 0.55 + shade * 0.45) * k
    # The light under the bar (app.css body::after): from the top middle, in the accent.
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    top = np.clip(1 - np.sqrt(((xx / w - 0.5) / 0.65) ** 2 + (yy / h / 1.1) ** 2), 0, 1) ** 1.5
    a = screen(a, acc * top[..., None] * 0.26)
    # Bloom: the bright parts give off a soft light in the accent.
    hi = np.clip(((a @ Y) - 0.62) / 0.38, 0, 1)
    hi = Image.fromarray((hi * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(w * 0.012))
    a = screen(a, (acc * 0.6 + 0.4) * (np.asarray(hi, np.float32) / 255)[..., None] * 0.30)
    # The edges sink a little into the page.
    edge = np.sqrt(((xx / w - 0.5) / 0.62) ** 2 + ((yy / h - 0.45) / 0.85) ** 2)
    v = (0.30 * np.clip((edge - 0.55) / 0.6, 0, 1))[..., None]
    a = a * (1 - v) + coal * v
    return Image.fromarray((np.clip(a, 0, 1) * 255 + 0.5).astype(np.uint8))


def flat_logo(logo: Image.Image, tune: dict, height: int = 440) -> Image.Image:
    """The logo redrawn flat, as OpenRAC's own: its letters in the launcher's gold (--amber), the
    plate they sit on in the page's darkest coal, nothing else. The shapes are the logo's own: the
    letters are told from the plate by their colour (copper, red, orange, gold) or, for the silver
    ones, as the bright, grey parts the plate's dark outlines close in. TUNE adjusts that per logo."""
    tc, tl, lmin = tune.get("tc", 0.22), tune.get("tl", 0.75), tune.get("lmin", 0.0)
    radius, min_part = tune.get("open", 2), tune.get("min", 0.002)
    logo = logo.convert("RGBA").crop(logo.getbbox())
    im = logo.resize((round(logo.width * height / logo.height), height), Image.LANCZOS)
    alpha = np.asarray(im.getchannel("A"), np.float32) / 255
    inside = alpha > 0.5
    smooth = np.ascontiguousarray(np.asarray(im.convert("RGB")))
    for _ in range(2):
        smooth = cv2.bilateralFilter(smooth, 9, 50, 9)
    lab = cv2.cvtColor(smooth, cv2.COLOR_RGB2LAB).astype(np.float32)
    lum, chroma = lab[..., 0] / 255, np.hypot(lab[..., 1] - 128, lab[..., 2] - 128) / 100
    letters = (((chroma > tc) & (lum > lmin)) | (lum > tl)) & inside
    # Silver letters: the parts between the outlines (the darkest pixels and the sharpest edges)
    # that are bright and grey.
    # (A lighter smoothing here, so the outlines stay closed.)
    lab1 = cv2.cvtColor(cv2.bilateralFilter(np.ascontiguousarray(np.asarray(im.convert("RGB"))), 9, 40, 7), cv2.COLOR_RGB2LAB).astype(np.float32)
    lum1, chroma1 = lab1[..., 0] / 255, np.hypot(lab1[..., 1] - 128, lab1[..., 2] - 128) / 128
    grad = np.sqrt((cv2.Sobel(lab1, cv2.CV_32F, 1, 0) ** 2 + cv2.Sobel(lab1, cv2.CV_32F, 0, 1) ** 2).sum(-1))
    bounds = (lum1 < np.quantile(lum1[inside], 0.18)) | (grad > np.quantile(grad[inside], 0.88)) | ~inside
    parts, count = ndi.label(~bounds)
    total = inside.sum()
    for i in range(1, count + 1):
        part = parts == i
        size = part.sum()
        if 0.0004 * total <= size < 0.05 * total and lum1[part].mean() > 0.6 and chroma1[part].mean() < 0.1:
            letters |= part
    k = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (2 * radius + 1, 2 * radius + 1))
    letters = cv2.morphologyEx(letters.astype(np.uint8), cv2.MORPH_CLOSE, k)
    letters = cv2.morphologyEx(letters, cv2.MORPH_OPEN, k)
    parts, count = ndi.label(letters)
    sizes = ndi.sum(letters, parts, range(1, count + 1))
    letters = np.isin(parts, 1 + np.where(sizes >= min_part * total)[0])
    face = cv2.GaussianBlur((cv2.GaussianBlur(letters.astype(np.float32), (0, 0), height / 220) > 0.5).astype(np.float32), (0, 0), 0.8)
    out = np.zeros(face.shape + (4,), np.float32)
    out[..., :3] = GOLD * face[..., None] + PLATE * (1 - face[..., None])
    out[..., 3] = np.clip(alpha * (0.92 * (1 - face) + face), 0, 1)
    return Image.fromarray((out * 255).clip(0, 255).astype(np.uint8), "RGBA")


def with_logo(picture: Image.Image, logo: Image.Image, left: int, share: float, tune: dict) -> Image.Image:
    """The tile: a 2.25 : 1 frame of the picture from LEFT, the logo top left on it."""
    w = round(picture.height * TILE[0] / TILE[1])
    left = min(left, picture.width - w)
    tile = picture.crop((left, 0, left + w, picture.height)).resize(TILE, Image.LANCZOS).convert("RGBA")
    logo = flat_logo(logo, tune)
    lh = round(TILE[1] * share)
    lw = round(logo.width * lh / logo.height)
    if lw > TILE[0] * 0.36:
        lw = round(TILE[0] * 0.36)
        lh = round(logo.height * lw / logo.width)
    logo = logo.resize((lw, lh), Image.LANCZOS)
    pad = round(TILE[1] * 0.07)
    at = (round(pad * 1.15), pad)

    def blurred(colour: tuple, strength: float, radius: float, dy: int = 0) -> Image.Image:
        layer = Image.new("RGBA", TILE, (0, 0, 0, 0))
        fill = Image.new("RGBA", logo.size, colour + (255,))
        fill.putalpha(logo.getchannel("A").point(lambda v: int(v * strength)))
        layer.paste(fill, (at[0], at[1] + dy), fill)
        return layer.filter(ImageFilter.GaussianBlur(radius))

    tile = Image.alpha_composite(tile, blurred((10, 8, 6), 0.55, lh * 0.06, round(lh * 0.03)))  # reads on any picture
    tile.alpha_composite(logo, at)
    return tile.convert("RGB")


def thumb(picture: Image.Image) -> Image.Image:
    s = max(THUMB[0] / picture.width, THUMB[1] / picture.height)
    small = picture.resize((round(picture.width * s), round(picture.height * s)), Image.LANCZOS)
    x = (small.width - THUMB[0]) // 2
    return small.crop((x, 0, x + THUMB[0], THUMB[1]))


def fetch(cache: Path, path: str) -> Path:
    out = cache / path
    if not out.is_file():
        out.parent.mkdir(parents=True, exist_ok=True)
        req = urllib.request.Request(f"{CDN}/{path}", headers={"User-Agent": "openrac-game-art"})
        with urllib.request.urlopen(req, timeout=60) as r:
            out.write_bytes(r.read())
    return out


def main(argv: list[str]) -> int:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--cache", type=Path, default=Path.home() / ".cache" / "openrac-game-art")
    p.add_argument("--out", type=Path, default=IMG, help="where to write (default public/img)")
    p.add_argument("--only", help="comma-separated game ids (default all)")
    a = p.parse_args(argv)
    games = a.only.split(",") if a.only else list(GAMES)
    for game in games:
        name, hero, logo, left, share = GAMES[game]
        tune = LOGO_TUNE.get(game, {})
        c = colours(game)
        picture = grade(Image.open(fetch(a.cache, hero)), c)
        picture.save(a.out / f"{name}-bg.webp", quality=88, method=6)
        with_logo(picture, Image.open(fetch(a.cache, logo)), left, share, tune).save(
            a.out / f"{name}-tile.webp", quality=88, method=6
        )
        thumb(picture).save(a.out / f"{name}-thumb.webp", quality=80, method=6)
        print(f"{game}: {name}-bg, -tile, -thumb")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
