"""Each game's logo redrawn flat, no gradients, for its tile (game_art.py).

The logo keeps its own shapes, one game at a time: game_logos.json says, for each game, which of
its parts are letters (gold), which are plate (a dark bronze) and which are outline (ink), as
expressions over the logo's colour (L, A, B, ch from Lab, 0..1) and place (x, y, 0..1). Each part
is then cleaned (holes filled, specks dropped, edges smoothed and straightened), so the result is
three flat colours. Sizes in the file are at 720 px high.

Layer keys: m (the mask), c (its colour), close/open/smooth/poly (clean-up), hole/speck (shares of
the logo), ring (its ink outline), cx (keep parts whose centre is in this x range), gear (redraw as
a clean gear). Logo keys: ink (more outline), inkmin, dots (rivets), rim (bronze line inside a
letter's outline), gmin/pmin/imin/popen (chip clean-up), frame, inkzone (plate there turns ink),
base (an older logo's Ratchet & Clank mark, sharper, moved in place with the homography H, down to
y = below).
"""

import json
from pathlib import Path

import cv2
import numpy as np
from PIL import Image

PARAMS = json.loads((Path(__file__).resolve().parent / "game_logos.json").read_text())
GOLD = np.array([0xF5, 0xC0, 0x40], np.float32) / 255  # --amber
INK = np.array([0x14, 0x12, 0x0E], np.float32) / 255  # --ink-deep
PALETTE = {
    "ink": INK,
    "plate": INK * 0.55 + GOLD * 0.25,
    "mid": INK * 0.3 + GOLD * 0.5,
    "gold": GOLD,
}
KINDS = list(PALETTE)


def disk(r: int) -> np.ndarray:
    return cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (2 * r + 1,) * 2)


def parts(m: np.ndarray, conn: int = 8) -> tuple[np.ndarray, np.ndarray]:
    _, label, stats, _ = cv2.connectedComponentsWithStats(
        m.astype(np.uint8), connectivity=conn
    )
    return label, stats[:, cv2.CC_STAT_AREA]


def fill_holes(m: np.ndarray, limit: float, inside: np.ndarray) -> np.ndarray:
    label, size = parts(~m, 4)
    border = np.zeros(len(size), bool)
    border[
        np.unique(np.concatenate([label[0], label[-1], label[:, 0], label[:, -1]]))
    ] = True
    hole = (size < limit) & ~border
    return m | (hole[label] & inside)


def drop(m: np.ndarray, limit: float) -> np.ndarray:
    label, size = parts(m)
    keep = size >= limit
    keep[0] = False
    return keep[label] & m


def straighten(m: np.ndarray, eps: float) -> np.ndarray:
    """The mask's outlines as polygons: wobbly edges become straight ones."""
    contours, hier = cv2.findContours(
        m.astype(np.uint8), cv2.RETR_CCOMP, cv2.CHAIN_APPROX_NONE
    )
    out = np.zeros(m.shape, np.uint8)
    if hier is None:
        return out.astype(bool)
    for hole in (False, True):
        for c, h in zip(contours, hier[0]):
            if (h[3] >= 0) == hole:
                cv2.fillPoly(out, [cv2.approxPolyDP(c, eps, True)], 0 if hole else 1)
    return out.astype(bool)


def gear(m: np.ndarray) -> np.ndarray:
    """A clean gear from a rough or partly hidden one: its radius per angle, the teeth found by
    their period."""
    ys, xs = np.nonzero(m)
    cy, cx = ys.mean(), xs.mean()
    ang, rad = np.arctan2(ys - cy, xs - cx), np.hypot(ys - cy, xs - cx)
    bins = 720
    idx = ((ang + np.pi) / (2 * np.pi) * bins).astype(int) % bins
    prof = np.zeros(bins)
    np.maximum.at(prof, idx, rad)
    ok = prof > np.quantile(prof[prof > 0], 0.1) * 0.8
    lo, hi = np.quantile(prof[ok], 0.2), np.quantile(prof[ok], 0.92)
    lo = max(lo, 0.74 * hi)
    t = (np.arange(bins) + 0.5) / bins * 2 * np.pi - np.pi
    sig = np.where(ok, prof - (lo + hi) / 2, 0)
    teeth, phase = max(
        ((n, (sig * np.exp(-1j * n * t)).sum()) for n in range(7, 11)),
        key=lambda v: abs(v[1]),
    )
    r = np.where(np.cos(teeth * t - np.angle(phase)) > 0, hi, lo)
    pts = np.stack([cx + r * np.cos(t), cy + r * np.sin(t)], 1).astype(np.int32)
    out = np.zeros(m.shape, np.uint8)
    cv2.fillPoly(out, [pts], 1)
    return out.astype(bool)


def load(logo: Image.Image, height: int) -> Image.Image:
    logo = logo.convert("RGBA").crop(logo.getbbox())
    return logo.resize(
        (round(logo.width * height / logo.height), height), Image.LANCZOS
    )


def classes(im: Image.Image, p: dict) -> tuple[np.ndarray, np.ndarray]:
    """Each pixel's kind (ink, plate, gold) and the logo's shape."""
    H, W = im.height, im.width
    a = np.asarray(im.getchannel("A")) > 127
    a = drop(
        a, 0.003 * a.sum()
    )  # the marks (TM, R) are their own little shapes: left out
    total = a.sum()
    raw = np.ascontiguousarray(np.asarray(im.convert("RGB")))
    lab = cv2.cvtColor(cv2.bilateralFilter(raw, 9, 25, 9), cv2.COLOR_RGB2LAB).astype(
        np.float32
    )
    L, A, B = lab[..., 0] / 255, (lab[..., 1] - 128) / 100, (lab[..., 2] - 128) / 100
    yy, xx = np.mgrid[0:H, 0:W]
    env = {
        "L": L,
        "A": A,
        "B": B,
        "ch": np.hypot(A, B),
        "np": np,
        "y": yy / H,
        "x": xx / W,
    }

    def u(f: float) -> int:
        return max(1, round(f * H / 720))

    def regions(base: np.ndarray, cond: str) -> np.ndarray:
        n, label = cv2.connectedComponents((base & a).astype(np.uint8), connectivity=4)
        count = np.maximum(np.bincount(label.ravel(), None, n), 1)
        mean = {
            k: np.bincount(label.ravel(), v.ravel(), n) / count
            for k, v in (("L", L), ("A", A), ("B", B))
        }
        ok = eval(cond, dict(np=np, n=count / total, **mean))
        ok[0] = False
        return ok[label]

    env["regions"] = regions
    env["inner"] = cv2.erode(a.astype(np.uint8), disk(u(p.get("frame", 14)))).astype(
        bool
    )
    kind = np.full(a.shape, "plate", dtype=object)
    taken, ink = ~a, np.zeros_like(a)
    for ly in p["layers"]:
        m = eval(ly["m"], env) & a
        m = (
            cv2.morphologyEx(
                m.astype(np.uint8), cv2.MORPH_CLOSE, disk(u(ly.get("close", 3)))
            ).astype(bool)
            & a
        )
        m = fill_holes(m, ly.get("hole", 0.002) * total, a)
        m = drop(m, ly.get("speck", 0.0003) * total)
        if "cx" in ly:
            n, label, _, centre = cv2.connectedComponentsWithStats(
                m.astype(np.uint8), connectivity=8
            )
            x = centre[:, 0] / W
            keep = (x >= ly["cx"][0]) & (x < ly["cx"][1])
            keep[0] = False
            m = keep[label] & m
        m = cv2.morphologyEx(
            m.astype(np.uint8), cv2.MORPH_OPEN, disk(u(ly.get("open", 1)))
        ).astype(bool)
        if ly.get("gear"):
            m = gear(m)
        m = (
            cv2.GaussianBlur(m.astype(np.float32), (0, 0), u(ly.get("smooth", 2.5)))
            > 0.5
        ) & a
        if ly.get("poly", 1.6):
            m = straighten(m, u(1) * ly.get("poly", 1.6)) & a
        m &= ~taken
        if ly.get("ring", 6):
            ink |= (
                cv2.dilate(m.astype(np.uint8), disk(u(ly.get("ring", 6)))).astype(bool)
                & ~m
                & a
                & ~taken
            )
        kind[m] = ly["c"]
        taken |= m
    ink |= a & ~cv2.erode(a.astype(np.uint8), disk(u(6))).astype(bool)
    if p.get("ink"):
        extra = eval(p["ink"], env) & a
        extra = cv2.morphologyEx(
            extra.astype(np.uint8), cv2.MORPH_OPEN, disk(u(2))
        ).astype(bool)
        extra = drop(extra, p.get("inkmin", 0.003) * total)
        extra = cv2.GaussianBlur(extra.astype(np.float32), (0, 0), u(3)) > 0.5
        ink |= straighten(extra, u(1) * 2.5) & a
    rivets = np.zeros(a.shape, np.uint8)
    if p.get("dots"):
        lum = cv2.cvtColor(raw, cv2.COLOR_RGB2LAB)[..., 0].astype(np.float32) / 255
        spots = (cv2.morphologyEx(lum, cv2.MORPH_BLACKHAT, disk(u(12))) > p["dots"]) & a
        n, _, st, centre = cv2.connectedComponentsWithStats(
            spots.astype(np.uint8), connectivity=8
        )
        for i in range(1, n):
            w, h = st[i, cv2.CC_STAT_WIDTH], st[i, cv2.CC_STAT_HEIGHT]
            if (
                u(1) ** 2 * 35 <= st[i, cv2.CC_STAT_AREA] <= u(1) ** 2 * 450
                and 0.6 < w / max(h, 1) < 1.6
                and max(w, h) < u(26)
            ):
                cv2.circle(rivets, (int(centre[i, 0]), int(centre[i, 1])), u(5), 1, -1)
    ink = (cv2.GaussianBlur(ink.astype(np.float32), (0, 0), u(2)) > 0.5) & a
    kind[ink & ~taken] = "ink"

    # Chips: thin bronze slivers and specks in the ink go, small ink specks in the bronze go.
    gold = drop(kind == "gold", p.get("gmin", 0.0015) * total)
    plate = (kind == "plate") | ((kind == "gold") & ~gold)
    plate = cv2.morphologyEx(
        plate.astype(np.uint8), cv2.MORPH_OPEN, disk(u(p.get("popen", 3)))
    ).astype(bool)
    plate = drop(plate, p.get("pmin", 0.002) * total)
    rest = a & ~gold & ~plate
    label, size = parts(rest)
    small = size < p.get("imin", 0.0015) * total
    small[0] = False
    plate |= small[label] & rest
    plate = (cv2.GaussianBlur(plate.astype(np.float32), (0, 0), u(2)) > 0.5) & a & ~gold
    kind = np.where(gold, "gold", np.where(plate, "plate", "ink")).astype(object)
    if p.get(
        "rim"
    ):  # the letters' metal rim: a thin bronze line inside their ink outline
        rim = cv2.dilate(gold.astype(np.uint8), disk(u(p["rim"]))).astype(bool) & ~gold
        near = cv2.dilate(gold.astype(np.uint8), disk(u(1))).astype(bool) & ~gold
        kind[rim & ~near & (kind == "ink")] = "plate"
    kind[~a] = "plate"
    if p.get("dots"):  # rivets only on the plate, away from the letters' outlines
        clear = cv2.erode(
            ((kind == "plate") & ~taken).astype(np.uint8), disk(u(2))
        ).astype(bool)
        n, label = cv2.connectedComponents(rivets)
        for i in range(1, n):
            m = label == i
            if clear[m].mean() > 0.5:
                kind[m & (kind == "plate")] = "ink"
    return kind, a


def with_base(
    im: Image.Image, kind: np.ndarray, a: np.ndarray, base: dict, logos: dict
) -> np.ndarray:
    """The Ratchet & Clank mark the later logos share, drawn from the sharper first logo and moved
    into place, so it looks the same on every tile."""
    H = im.height
    src = load(logos[base["logo"]], H)
    bkind, ba = classes(src, PARAMS[base["logo"]])
    s = np.diag([H / 720, H / 720, 1.0])
    M = s @ np.array(base["H"], np.float64) @ np.linalg.inv(s)
    idx = np.full(ba.shape, 255, np.uint8)
    for i, k in enumerate(KINDS):
        idx[(bkind == k) & ba] = i
    moved = cv2.warpPerspective(
        idx, M, (im.width, H), flags=cv2.INTER_NEAREST, borderValue=255
    )
    use = (moved != 255) & (np.mgrid[0:H, 0 : im.width][0] / H < base["below"]) & a
    out = kind.copy()
    for i, k in enumerate(KINDS):
        out[use & (moved == i)] = k
    return out


def flat_logo(game: str, logos: dict, height: int = 440) -> Image.Image:
    """GAME's logo (from LOGOS, game id -> original logo) flat in three colours, HEIGHT px high."""
    p = PARAMS[game]
    im = load(logos[game], height * 2)
    kind, a = classes(im, p)
    if p.get("base"):
        kind = with_base(im, kind, a, p["base"], logos)
    if p.get("inkzone"):
        yy, xx = np.mgrid[0 : im.height, 0 : im.width]
        kind[
            eval(p["inkzone"], {"y": yy / im.height, "x": xx / im.width})
            & (kind == "plate")
        ] = "ink"
    col = np.zeros(a.shape + (3,), np.float32)
    for k, v in PALETTE.items():
        col[kind == k] = v
    alpha = (
        np.asarray(im.getchannel("A"), np.float32)
        / 255
        * cv2.dilate(a.astype(np.uint8), disk(1))
    )
    out = Image.fromarray(
        (np.dstack([col, alpha]) * 255).clip(0, 255).astype(np.uint8), "RGBA"
    )
    return out.resize((out.width // 2, height), Image.LANCZOS)
