# SPDX-License-Identifier: GPL-3.0-or-later
"""Overlay an emissive inspection report on a PNG of the same game view."""

import argparse
import base64
from html import escape
import json
from pathlib import Path
import struct


def label_capture(report, capture):
    if capture[:16] != b"\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR":
        raise ValueError("Expected a PNG game capture")
    width, height = struct.unpack_from(">II", capture, 16)
    candidates = report["candidates"]
    total_height = max(height, 44 + 22 * len(candidates))
    png = base64.b64encode(capture).decode("ascii")
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width + 490}" height="{total_height}">',
        f'<rect width="100%" height="100%" fill="#181c24"/>',
        f'<image width="{width}" height="{height}" href="data:image/png;base64,{png}"/>',
        '<g font-family="monospace" font-size="13" fill="white">',
        f'<text x="{width + 12}" y="22">{escape(report["task_id"])} · nearest aim first</text>',
    ]
    for row, candidate in enumerate(candidates):
        label = candidate["label"]
        colour = ("#73e4ff", "#f4d36c", "#f9a9ee", "#91efab")[row % 4]
        source = candidate["source"] or {}
        model = source.get("model", "unknown model")
        texture = source.get("texture_path") or "untextured"
        slot = source.get("material_slot")
        legend = escape(f"{label}. {model} [{slot}]  {texture}")
        parts.append(f'<text x="{width + 12}" y="{44 + row * 22}" fill="{colour}">{legend}</text>')
        for index, (x, y) in enumerate(candidate["samples"]):
            px, py = width * (1 + x) / 2, height * (1 - y) / 2
            radius = 11 if index == 0 else 3
            parts.append(
                f'<circle cx="{px:.1f}" cy="{py:.1f}" r="{radius}" fill="#18202c" '
                f'stroke="{colour}"><title>{legend}</title></circle>'
            )
            if index == 0:
                parts.append(
                    f'<text x="{px:.1f}" y="{py + 4:.1f}" text-anchor="middle" '
                    f'fill="{colour}">{label}</text>'
                )
    return "\n".join([*parts, "</g></svg>"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path, help="JSON from --inspect-emissive-task")
    parser.add_argument("capture", type=Path, help="PNG of the game picture at the reported pose")
    parser.add_argument("output", type=Path, help="standalone labelled SVG")
    args = parser.parse_args()
    report = json.loads(args.report.read_text(encoding="utf-8-sig"))
    args.output.write_text(label_capture(report, args.capture.read_bytes()), encoding="utf-8")


if __name__ == "__main__":
    main()
