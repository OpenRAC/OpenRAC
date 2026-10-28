#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Create an interior-source sidecar from a collision inference report.

Usage: prepare_interior_lights.py --inference REPORT --level-dir EXPORTED_LEVEL
The sidecar contains derived game data and must remain in ignored local storage.
Only unambiguous, low-sky components supply samples. The native baker performs
ceiling, enclosure and clearance checks before creating finite sources.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path


def source_plan(report, manifest):
    """Preserve agreeing transition labels and reject unresolved or sky-exposed samples."""
    game = manifest.get('game', '') + '-' + manifest.get('version', '')
    if report.get('format') != 1 or report.get('game') != game or report.get('level') != manifest.get('level'):
        raise ValueError('inference report does not match the exported level')
    regions = {region['id']: region for region in report['regions']}
    transitions = {transition['id']: transition for transition in report['transitions']}
    labels = {}
    for seed in report['seeds']:
        transition = transitions[seed['transition']]
        side = seed['side']
        value = (transition['light'][side], transition['ambient'][side])
        if seed['label'] in labels and labels[seed['label']] != value:
            raise ValueError('conflicting inference label')
        labels[seed['label']] = value
    samples = []
    for cell in report['cells']:
        region = regions[cell[3]]
        if len(region['labels']) != 1 or cell[4] > .2:
            continue
        light, ambient = labels[region['labels'][0]]
        samples.append(dict(position=cell[:3], region=cell[3], light_set=light, ambient=ambient))
    return dict(version=1, level=manifest['level'], source_sha256=report['source_sha256'],
                settings=dict(emitter_boost=2, emission_limit=4, point_limit=64,
                              ceiling_limit=12, ceiling_strength=1, half_extent=1.5,
                              separation=6, max_height=32, max_lights=1024, max_per_region=128),
                samples=samples)


def main():
    """Write a new sidecar without replacing an existing experiment."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inference', type=Path, required=True)
    parser.add_argument('--level-dir', type=Path, required=True)
    args = parser.parse_args()
    report = json.loads(args.inference.read_text())
    manifest = json.loads((args.level_dir / 'manifest.json').read_text())
    plan = source_plan(report, manifest)
    with (args.level_dir / 'probe-interiors.json').open('x') as output:
        json.dump(plan, output, separators=(',', ':'), allow_nan=False)
        output.write('\n')
    print(f"Prepared {len(plan['samples'])} labelled interior samples")


if __name__ == '__main__':
    main()
