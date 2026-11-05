#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Draft and audit interior lighting from native bake reports.

Usage: lighting_review.py draft BAKE_DIR OUTPUT --region ID [--region ID ...]
       lighting_review.py compare BASELINE_DIR CANDIDATE_DIR [--authoring FILE]

BAKE_DIR is a single baked level directory. Keep outputs with local bake data.
"""
import argparse
from collections import defaultdict
import hashlib
import json
import math
from pathlib import Path


SKY_LIMIT = 0.2  # Match the native interior source planner.


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def read_inference(directory):
    report = read_json(directory / "interior-inference.json")
    if report["format"] != 1:
        raise ValueError("unsupported interior inference report")
    return report


def room_suggestions(report, include_labelled=False, near=None):
    """Find interior seed candidates and nearby original transition references."""
    rooms = defaultdict(list)
    for *position, room, sky in report["cells"]:
        if sky <= SKY_LIMIT:
            rooms[room].append(position)

    transitions = {transition["id"]: transition for transition in report["transitions"]}
    endpoints = [
        seed for seed in report["seeds"]
        if seed["cell"] >= 0 and report["cells"][seed["cell"]][4] <= SKY_LIMIT
    ]
    suggestions = []
    for room in report["regions"]:
        points = rooms[room["id"]]
        if not points or (not include_labelled and len(room["labels"]) == 1):
            continue
        centroid = [sum(axis) / len(points) for axis in zip(*points)]
        position = min(points, key=lambda point: (math.dist(point, centroid), point)).copy()
        position[2] += report["height"] * 0.5
        # Prefer nearby endpoints from the room's existing conflicting moods when available.
        references = [seed for seed in endpoints if seed["label"] in room["labels"]] or endpoints
        nearby = []
        for seed in sorted(references, key=lambda item: math.dist(item["target"], position))[:3]:
            transition = transitions[seed["transition"]]
            side = seed["side"]
            nearby.append({
                "distance": round(math.dist(seed["target"], position), 2),
                "transition": seed["transition"],
                "side": side,
                "light": transition["light"][side],
                "ambient": transition["ambient"][side],
            })
        suggestions.append({
            "region": room["id"], "low_sky_cells": len(points), "labels": room["labels"],
            "position": position, "references": nearby,
        })
    if near is not None:
        def distance(room):
            nearest = min(math.dist(point, near) for point in rooms[room["region"]])
            return nearest, room["region"]

        return sorted(suggestions, key=distance)
    return sorted(suggestions, key=lambda room: (-room["low_sky_cells"], room["region"]))


def cell_samples(report, regions):
    """Index cells by world position because barriers can renumber room IDs."""
    labels = {room["id"]: tuple(room["labels"]) for room in report["regions"]}
    bounds = [region["bounds"] for region in regions]
    samples = {}
    authored = report.get("authored_cell_labels", {})
    for index, (x, y, z, room, sky) in enumerate(report["cells"]):
        override = authored[index] if authored else None
        effective = labels[room] if override is None else tuple(override)
        body = z + report["height"] * 0.5
        dense = any(a <= x <= d and b <= y <= e and c <= body <= f
                    for a, b, c, d, e, f in bounds)
        samples[x, y, z] = (effective, sky <= SKY_LIMIT, dense)
    return samples


def load_run(directory):
    report = read_inference(directory)
    plan = read_json(directory / "seed-plan.json")
    lights = read_json(directory / "interior-lights-report.json")
    if (plan["game_version"], plan["level"]) != (report["game"], report["level"]):
        raise ValueError(f"seed plan and inference report disagree in {directory}")

    return {
        "identity": (report["game"], report["level"], report["source_sha256"]),
        "generation": report.get("generation_sha256"),
        "samples": cell_samples(report, plan["regions"]),
        "regions": len(plan["regions"]),
        "dense_bytes": plan["dense_resident_bytes"],
        "emitters": plan["emission"],
        "lights": lights["lights"],
    }


def nearest_lights(lights, position):
    return [
        {"receiver": light["receiver"], "radiance": light["radiance"],
         "distance": round(math.dist(light["receiver"], position), 2)}
        for light in sorted(lights, key=lambda item: math.dist(item["receiver"], position))[:3]
    ]


def compare_runs(before, after, authoring=None):
    if before["identity"] != after["identity"]:
        raise ValueError("compare requires the same game, level and collision/gameplay inputs")
    old, new = before["samples"], after["samples"]
    if old.keys() != new.keys():
        raise ValueError("compare requires matching sampled floor positions")

    covered = [point for point in new if new[point][2] and not old[point][2]]
    lost = [point for point in new if old[point][2] and not new[point][2]]
    result = {
        "identity": after["identity"],
        "before_generation": before["generation"],
        "after_generation": after["generation"],
        "matched_cells": len(new),
        "low_sky_cells_with_one_label": [
            sum(low and len(labels) == 1 for labels, low, _ in run.values())
            for run in (old, new)
        ],
        "dense_bounds": {
            "regions": [before["regions"], after["regions"]],
            "resident_bytes": [before["dense_bytes"], after["dense_bytes"]],
            "covered": [sum(sample[2] for sample in run.values()) for run in (old, new)],
            "newly_covered": sorted(covered),
            "lost_coverage": sorted(lost),
            "remaining_low_sky_gaps": sorted(point for point, (_, low, dense) in new.items()
                                              if low and not dense),
        },
        "emission": [before["emitters"], after["emitters"]],
        "ceiling_lights": [len(before["lights"]), len(after["lights"])],
    }
    if authoring:
        data = read_json(authoring)
        result["authoring_sha256"] = hashlib.sha256(authoring.read_bytes()).hexdigest()
        result["sources"] = [
            {"source": source, "before": nearest_lights(before["lights"], source["position"]),
             "after": nearest_lights(after["lights"], source["position"])}
            for source in data["sources"]
        ]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    suggest = commands.add_parser("suggest", help="print candidate seeds and nearby original moods")
    suggest.add_argument("bake", type=Path)
    suggest.add_argument("--near", nargs=3, type=float, metavar=("X", "Y", "Z"),
                         help="include labelled rooms and sort by distance to a picked point")
    draft = commands.add_parser("draft", help="write a source sidecar for explicitly chosen rooms")
    draft.add_argument("bake", type=Path)
    draft.add_argument("output", type=Path)
    draft.add_argument("--region", type=int, action="append", required=True)
    compare = commands.add_parser("compare", help="compare verified level bake reports")
    compare.add_argument("baseline", type=Path)
    compare.add_argument("candidate", type=Path)
    compare.add_argument("--authoring", type=Path)
    args = parser.parse_args()

    try:
        if args.command == "compare":
            result = compare_runs(load_run(args.baseline), load_run(args.candidate), args.authoring)
        else:
            near = getattr(args, "near", None)
            if near is not None and not all(map(math.isfinite, near)):
                raise ValueError("--near requires finite coordinates")
            suggestions = room_suggestions(
                read_inference(args.bake),
                include_labelled=near is not None or args.command == "draft",
                near=near,
            )
            result = suggestions
            if args.command == "draft":
                by_id = {room["region"]: room for room in suggestions}
                sources = []
                for region in sorted(set(args.region)):
                    room = by_id.get(region)
                    if not room or not room["references"]:
                        raise ValueError(f"region {region} has no suggested seed and original mood")
                    reference = room["references"][0]
                    sources.append({key: reference[key] for key in ("light", "ambient")}
                                   | {"position": room["position"]})
                result = {"version": 1, "barriers": [], "sources": sources}
                with args.output.open("x", encoding="utf-8", newline="\n") as stream:
                    json.dump(result, stream, indent=2)
                    stream.write("\n")
        print(json.dumps(result, indent=2, allow_nan=False))
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(1, f"lighting review: {error}\n")


if __name__ == "__main__":
    main()
