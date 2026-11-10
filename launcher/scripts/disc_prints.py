#!/usr/bin/env python3
"""The disc prints in public/img/discs, from the LaunchBox Games Database.

    python3 scripts/disc_prints.py

For each game it writes <game>-ntsc.webp (the North American disc) and <game>-pal.webp (the
European one): the scan cut to the disc, at most 768 px square. The scans are the ones Cartouche
uses (github.com/Hakimba/cartouche, app/data/art/ps2.tsv). Needs Pillow and curl.
"""

import io
import subprocess
from pathlib import Path

from PIL import Image

LAUNCHBOX = "https://images.launchbox-app.com/"
OUT = Path(__file__).resolve().parent.parent / "public" / "img" / "discs"
SIZE = 768

# The scan of each game's disc, by region.
PRINTS = {
    "rac1": {"ntsc": "03c0c663-f1f1-4fbb-adb1-91a44d755eff", "pal": "c1b455c3-82e0-4b55-8e3e-56cc508c6f78"},
    "rac2": {"ntsc": "2b79e872-135c-47a4-a5fa-191483b7a1cf", "pal": "8cf4d363-4040-44b8-8412-dd2058a80c30"},
    "rac3": {"ntsc": "0a3fd622-89ee-44cb-97ff-4167dc56ef76", "pal": "46d42ac4-b987-43b1-b185-b3ecfc802989"},
    "rac4": {"ntsc": "d07c9d1f-0966-46ae-9b51-2c6404e32d82", "pal": "0e7e6169-ad74-41df-b37a-2a9730acc340"},
    "rac5": {"ntsc": "b21c6132-f24a-4f4b-afbe-8ca153aff245", "pal": "0095ef61-7cca-432a-963b-4ada2e7bc04e"},
    "rac6": {"ntsc": "e596a445-b75c-4b48-acd6-3097eafff420", "pal": "0be486f6-8286-44a6-8d05-3694a86a7ba7"},
}


def disc(data: bytes) -> Image.Image:
    """The scan cut to the disc (its transparent corners), square, at most SIZE wide."""
    im = Image.open(io.BytesIO(data)).convert("RGBA")
    im = im.crop(im.getchannel("A").getbbox() or (0, 0, *im.size))
    side = max(im.size)
    square = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    square.paste(im, ((side - im.width) // 2, (side - im.height) // 2))
    size = min(SIZE, side)
    return square.resize((size, size), Image.LANCZOS)


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for game, regions in PRINTS.items():
        for region, image_id in regions.items():
            # Fetched with curl: the server refuses Python's urllib.
            data = subprocess.run(
                ["curl", "-sSfL", f"{LAUNCHBOX}{image_id}.png"], check=True, capture_output=True
            ).stdout
            path = OUT / f"{game}-{region}.webp"
            disc(data).save(path, "WEBP", quality=85, method=6)
            print(path.name, path.stat().st_size // 1024, "KB")


if __name__ == "__main__":
    main()
