#!/usr/bin/env python3
"""The disc prints in public/img/discs, from the LaunchBox Games Database.

    python3 scripts/disc_prints.py

For each game it writes <game>-ntsc.webp (the North American disc) and <game>-pal.webp (the
European one): the scan cut to the disc, at most 768 px square. The scans are the ones Cartouche
uses (github.com/Hakimba/cartouche, app/data/art/ps2.tsv). Needs Pillow, NumPy and curl.

Secret Agent Clank's PAL disc is not a scan: its only scan with a print is 226 px, so it is a
photo of the disc, straightened, white-balanced on its white and black and given the hub of Up
Your Arsenal's PAL disc. The script leaves that file as it is (KEEP).
"""

import io
import subprocess
from pathlib import Path

import numpy as np
from PIL import Image

LAUNCHBOX = "https://images.launchbox-app.com/"
OUT = Path(__file__).resolve().parent.parent / "public" / "img" / "discs"
SIZE = 768

# The scan of each game's disc, by region. Going Commando uses the Australian PAL disc: the European
# scan is a Platinum disc.
PRINTS = {
    "rac1": {"ntsc": "03c0c663-f1f1-4fbb-adb1-91a44d755eff", "pal": "c1b455c3-82e0-4b55-8e3e-56cc508c6f78"},
    "rac2": {"ntsc": "2b79e872-135c-47a4-a5fa-191483b7a1cf", "pal": "5d66507e-ae5e-406e-b980-3d6c21709fb9"},
    "rac3": {"ntsc": "0a3fd622-89ee-44cb-97ff-4167dc56ef76", "pal": "46d42ac4-b987-43b1-b185-b3ecfc802989"},
    "rac4": {"ntsc": "d07c9d1f-0966-46ae-9b51-2c6404e32d82", "pal": "0e7e6169-ad74-41df-b37a-2a9730acc340"},
    "rac5": {"ntsc": "b21c6132-f24a-4f4b-afbe-8ca153aff245", "pal": "0095ef61-7cca-432a-963b-4ada2e7bc04e"},
    "rac6": {"ntsc": "e596a445-b75c-4b48-acd6-3097eafff420", "pal": "728530ca-db61-4653-ad5b-3a850638c96f"},
}

# Scans without the clear hub round the hole (Secret Agent Clank's NTSC one is cut at its print, black
# to the hole): it is taken from another disc of the same region, cut the same way.
HUB_FROM = {("rac6", "ntsc"): ("rac4", "ntsc")}
HUB = 0.2

# Made otherwise (the module's notes): the script does not write these.
KEEP = {("rac6", "pal")}


def fetch(url: str) -> bytes:
    # Fetched with curl: the image server refuses Python's urllib.
    return subprocess.run(["curl", "-sSfL", url], check=True, capture_output=True).stdout


def with_hub(im: Image.Image, donor: Image.Image) -> Image.Image:
    """`im` with `donor`'s middle, to HUB of the radius, blended over a pixel or two."""
    a = np.asarray(im.convert("RGBA"), np.float32)
    b = np.asarray(donor.convert("RGBA").resize(im.size, Image.LANCZOS), np.float32)
    s = a.shape[0]
    c = (s - 1) / 2
    y, x = np.mgrid[:s, :s]
    r = np.hypot(x - c, y - c) / c
    k = np.clip((HUB - r) * c / 2, 0, 1)[..., None]
    return Image.fromarray((a * (1 - k) + b * k + 0.5).astype(np.uint8), "RGBA")


def disc(data: bytes) -> Image.Image:
    """The scan centred on its disc, square, at most SIZE wide. Some scans cut the disc's edge off
    at a side: the circle is fitted to the edge where it is whole, and the cut part stays empty."""
    im = Image.open(io.BytesIO(data)).convert("RGBA")
    alpha = im.getchannel("A").load()
    w, h = im.size
    # The disc's outline: per row and column, its first and last solid pixel, away from the image edge.
    edge = []
    for y in range(0, h, 2):
        xs = [x for x in range(w) if alpha[x, y] > 128]
        if xs:
            edge += [(x, y) for x in (xs[0], xs[-1]) if 1 < x < w - 2]
    for x in range(0, w, 2):
        ys = [y for y in range(h) if alpha[x, y] > 128]
        if ys:
            edge += [(x, y) for y in (ys[0], ys[-1]) if 1 < y < h - 2]
    cx, cy, r = fit_circle(edge)
    side = round(2 * r)
    square = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    square.paste(im, (round(r - cx), round(r - cy)))
    size = min(SIZE, side)
    return square.resize((size, size), Image.LANCZOS)


def fit_circle(points: list[tuple[int, int]]) -> tuple[float, float, float]:
    """The circle nearest the points (least squares, Kasa's method): centre and radius."""
    n = len(points)
    sx = sum(x for x, _ in points)
    sy = sum(y for _, y in points)
    sxx = sum(x * x for x, _ in points)
    syy = sum(y * y for _, y in points)
    sxy = sum(x * y for x, y in points)
    sz = [x * x + y * y for x, y in points]
    szx = sum(z * x for z, (x, _) in zip(sz, points))
    szy = sum(z * y for z, (_, y) in zip(sz, points))
    szs = sum(sz)
    # Solve [sxx sxy sx; sxy syy sy; sx sy n] [a b c] = [szx szy szs], with x² + y² = a x + b y + c.
    m = [[sxx, sxy, sx, szx], [sxy, syy, sy, szy], [sx, sy, n, szs]]
    for col in range(3):
        pivot = max(range(col, 3), key=lambda row: abs(m[row][col]))
        m[col], m[pivot] = m[pivot], m[col]
        for row in range(3):
            if row != col:
                f = m[row][col] / m[col][col]
                m[row] = [a - f * b for a, b in zip(m[row], m[col])]
    a, b, c = (m[k][3] / m[k][k] for k in range(3))
    cx, cy = a / 2, b / 2
    return cx, cy, (c + cx * cx + cy * cy) ** 0.5


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for game, regions in PRINTS.items():
        for region, image_id in regions.items():
            if (game, region) in KEEP:
                continue
            data = fetch(f"{LAUNCHBOX}{image_id}.png")
            path = OUT / f"{game}-{region}.webp"
            im = disc(data)
            if (game, region) in HUB_FROM:
                im = with_hub(im, Image.open(OUT / "{}-{}.webp".format(*HUB_FROM[game, region])))
            im.save(path, "WEBP", quality=85, method=6)
            print(path.name, path.stat().st_size // 1024, "KB")


if __name__ == "__main__":
    main()
