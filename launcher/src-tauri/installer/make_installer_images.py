"""Build the Windows installers' pictures from the launcher's icon.

The look: the app's icon tile, with a soft shadow, on the same white as
Windows' installer pages. Only pictures: the installers'
pages, paths and steps are Tauri's own and stay as they are.

    python3 make_installer_images.py    # needs Pillow

Writes, next to this script:
    nsis-sidebar.bmp   164x314  NSIS welcome and finish pages
    nsis-header.bmp    150x57   NSIS page header (right side)
    wix-dialog.bmp     493x312  MSI welcome and finish dialogs (left 164 px is the picture)
    wix-banner.bmp     493x58   MSI page banner (WiX draws black text on its left)
"""

import pathlib

from PIL import Image, ImageFilter

HERE = pathlib.Path(__file__).resolve().parent
ICON = HERE.parent / "icons" / "icon.png"

LIGHT = (0xFF, 0xFF, 0xFF)


def tile(size: int) -> Image.Image:
    """The icon at SIZE pixels with a soft shadow, on a transparent canvas a little larger."""
    pad = max(4, size // 6)
    canvas = Image.new("RGBA", (size + 2 * pad, size + 2 * pad), (0, 0, 0, 0))
    icon = Image.open(ICON).convert("RGBA").resize((size, size), Image.LANCZOS)
    shadow = Image.new("RGBA", canvas.size, (0, 0, 0, 0))
    alpha = icon.getchannel("A").point(lambda a: a * 150 // 255)
    shadow.paste((0, 0, 0, 255), (pad, pad + max(1, size // 24)), alpha)
    shadow = shadow.filter(ImageFilter.GaussianBlur(max(1, size // 14)))
    canvas.alpha_composite(shadow)
    canvas.alpha_composite(icon, (pad, pad))
    return canvas


def place(background: Image.Image, size: int, centre: tuple[int, int]) -> None:
    t = tile(size)
    background.alpha_composite(t, (centre[0] - t.width // 2, centre[1] - t.height // 2))


def save(image: Image.Image, name: str) -> None:
    # 24-bit BMP: what NSIS and WiX both read.
    image.convert("RGB").save(HERE / name, "BMP")
    print(f"wrote {name} {image.width}x{image.height}")


def main() -> None:
    sidebar = Image.new("RGBA", (164, 314), LIGHT + (255,))
    place(sidebar, 112, (82, 150))
    save(sidebar, "nsis-sidebar.bmp")

    header = Image.new("RGBA", (150, 57), LIGHT + (255,))
    place(header, 42, (150 - 32, 28))
    save(header, "nsis-header.bmp")

    dialog = Image.new("RGBA", (493, 312), LIGHT + (255,))
    place(dialog, 112, (82, 150))
    save(dialog, "wix-dialog.bmp")

    banner = Image.new("RGBA", (493, 58), LIGHT + (255,))
    place(banner, 42, (493 - 32, 29))
    save(banner, "wix-banner.bmp")


if __name__ == "__main__":
    main()
