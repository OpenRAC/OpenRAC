#!/usr/bin/env python3
"""The disc prints in public/img/discs, from the LaunchBox Games Database.

    python3 scripts/disc_prints.py

For each game it writes <game>-ntsc.webp (the North American disc) and <game>-pal.webp (the
European one): the scan cut to the disc, at most 768 px square. The scans are the ones Cartouche
uses (github.com/Hakimba/cartouche, app/data/art/ps2.tsv). Needs Pillow, NumPy and curl.

A scan too small for the disc's size on Android (Secret Agent Clank's PAL one: 226 px) is first
made four times bigger by Real-ESRGAN's general model, run here in NumPy, half its denoising one
(as its denoise strength 0.5), with a fifth of a plain resize and a light grain mixed back in so
it looks like the other scans, not painted.
"""

import hashlib
import io
import pickle
import subprocess
import zipfile
from pathlib import Path

import numpy as np
from PIL import Image

LAUNCHBOX = "https://images.launchbox-app.com/"
OUT = Path(__file__).resolve().parent.parent / "public" / "img" / "discs"
SIZE = 768

# The scan of each game's disc, by region. Going Commando and Secret Agent Clank use the Australian
# PAL disc: the European scans are a Platinum disc and a plain silver one, with no picture.
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

# The scans made bigger first (the module's notes), and Real-ESRGAN's weights (BSD-3-Clause).
UPSCALE = {("rac6", "pal")}
REAL_ESRGAN = "https://github.com/xinntao/Real-ESRGAN/releases/download/v0.2.5.0/"
WEIGHTS = {
    "realesr-general-x4v3.pth": "8dc7edb9ac80ccdc30c3a5dca6616509367f05fbc184ad95b731f05bece96292",
    "realesr-general-wdn-x4v3.pth": "1641f8c4464b9f097c9fdda5589273713f67cf59f3d909e0bd688f0cee269dca",
}


def fetch(url: str) -> bytes:
    # Fetched with curl: the image server refuses Python's urllib.
    return subprocess.run(["curl", "-sSfL", url], check=True, capture_output=True).stdout


def weights(name: str) -> dict[str, np.ndarray]:
    """A .pth's tensors, read without PyTorch."""
    data = fetch(REAL_ESRGAN + name)
    if hashlib.sha256(data).hexdigest() != WEIGHTS[name]:
        raise SystemExit(f"{name}: not the expected file")
    z = zipfile.ZipFile(io.BytesIO(data))
    prefix = z.namelist()[0].split("/")[0]

    def rebuild(key, offset, size, *_):
        flat = np.frombuffer(z.read(f"{prefix}/data/{key}"), dtype="<f4")
        return flat[offset : offset + int(np.prod(size))].reshape(size)

    class Unpickler(pickle.Unpickler):
        def find_class(self, module, name):
            if name == "_rebuild_tensor_v2":
                return rebuild
            if module == "collections" and name == "OrderedDict":
                import collections

                return collections.OrderedDict
            if name == "FloatStorage":
                return "float"
            raise pickle.UnpicklingError(f"{module}.{name} is not expected in weights")

        def persistent_load(self, pid):
            return pid[2]

    state = Unpickler(z.open(f"{prefix}/data.pkl")).load()
    return state.get("params_ema") or state.get("params", state)


def upscale(data: bytes) -> bytes:
    """The scan four times bigger (Real-ESRGAN's compact network: 3x3 convolutions and PReLUs, then
    a pixel shuffle over the plain enlargement)."""
    a, b = weights("realesr-general-x4v3.pth"), weights("realesr-general-wdn-x4v3.pth")
    w = {k: 0.5 * a[k] + 0.5 * b[k] for k in a}
    im = Image.open(io.BytesIO(data)).convert("RGBA")
    rgb = np.asarray(im, np.float32)[..., :3].transpose(2, 0, 1) / 255
    x = rgb
    for i in sorted({int(k.split(".")[1]) for k in w}):
        k = w[f"body.{i}.weight"]
        if k.ndim == 4:
            c, h, wd = x.shape
            p = np.pad(x, ((0, 0), (1, 1), (1, 1)), mode="edge")
            cols = np.stack([p[:, r : r + h, q : q + wd] for r in range(3) for q in range(3)], 1)
            x = (k.reshape(k.shape[0], -1) @ cols.reshape(c * 9, h * wd)).reshape(k.shape[0], h, wd)
            x += w[f"body.{i}.bias"][:, None, None]
        else:
            x = np.where(x > 0, x, x * k[:, None, None])
    _, h, wd = x.shape
    x = x.reshape(3, 4, 4, h, wd).transpose(0, 3, 1, 4, 2).reshape(3, h * 4, wd * 4)
    x += rgb.repeat(4, 1).repeat(4, 2)
    big = (wd * 4, h * 4)
    plain = np.asarray(im.convert("RGB").resize(big, Image.LANCZOS), np.float32)
    mixed = 0.8 * x.transpose(1, 2, 0) * 255 + 0.2 * plain
    mixed += np.random.default_rng(6).normal(0, 2.2, mixed.shape)
    out = Image.fromarray(np.clip(mixed + 0.5, 0, 255).astype(np.uint8)).convert("RGBA")
    out.putalpha(im.getchannel("A").resize(big, Image.LANCZOS))
    buf = io.BytesIO()
    out.save(buf, "PNG")
    return buf.getvalue()


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
            data = fetch(f"{LAUNCHBOX}{image_id}.png")
            if (game, region) in UPSCALE:
                data = upscale(data)
            path = OUT / f"{game}-{region}.webp"
            im = disc(data)
            if (game, region) in HUB_FROM:
                im = with_hub(im, Image.open(OUT / "{}-{}.webp".format(*HUB_FROM[game, region])))
            im.save(path, "WEBP", quality=85, method=6)
            print(path.name, path.stat().st_size // 1024, "KB")


if __name__ == "__main__":
    main()
