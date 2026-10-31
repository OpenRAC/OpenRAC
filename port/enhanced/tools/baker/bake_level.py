#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Bake one exported level: emission, region planning, transport and validation.

Python standard library only; ray tracing uses the native Embree baker.
Generated plans, logs and probe assets contain game data and must stay local.
"""
from __future__ import annotations

import argparse
from collections import deque
import contextlib
from dataclasses import asdict, dataclass
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace

sys.path.insert(1, str(next(p for p in Path(__file__).resolve().parents if (p / "tools/openrac_common").is_dir()) / "tools"))

import seed_regions as seeds
import automatic_interiors
import diagnostics
from openrac_common import files as file_ops

sys.path.insert(1, str(Path(__file__).resolve().parent.parent/'water'))
import prepare_water  # noqa: E402


@dataclass
class RegionSettings:
    enabled: bool = True
    interior_lighting: bool = True
    coarse_bounds: str = 'local_regions'
    coarse_padding: float = 128
    padding: float = 8
    vertical_padding: float = 4
    max_probes: int = 4096
    max_regions: int = 1024
    no_geometry_coverage: bool = False
    geometry_max_rise: float = 32
    geometry_padding: float = 3
    no_moby_seeds: bool = False
    adaptive_density: bool = False
    adaptive_relative_error: float = 0.1
    adaptive_absolute_error: float = 0.02

    @classmethod
    def load(cls, level_dir: Path):
        path = level_dir/'probe-bake.json'
        data = diagnostics.read_json(path) if path.exists() else {'version': 1}
        if not isinstance(data, dict) or data.get('version') != 1:
            raise ValueError('probe-bake.json requires version 1')
        values = data.get('regions', {})
        if not isinstance(values, dict):
            raise ValueError('regions requires an object')
        if values.keys() - cls.__dataclass_fields__.keys():
            raise ValueError('unknown region planning setting')
        settings = cls(**values)
        defaults = asdict(cls())
        for key, value in asdict(settings).items():
            if key == 'coarse_bounds':
                if value not in ('scene', 'local_regions'):
                    raise ValueError('coarse_bounds must be scene or local_regions')
            elif isinstance(defaults[key], bool):
                if not isinstance(value, bool):
                    raise ValueError(f'{key} requires a boolean')
            elif isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
                raise ValueError(f'{key} requires a finite number')
            elif value < 0:
                raise ValueError(f'{key} must not be negative')
        if not isinstance(settings.max_probes, int) or settings.max_probes < 64:
            raise ValueError('max_probes must be an integer of at least 64')
        if not isinstance(settings.max_regions, int) or not 1 <= settings.max_regions <= 1024:
            raise ValueError('max_regions must be between 1 and 1024')
        if settings.geometry_max_rise <= 0:
            raise ValueError('geometry_max_rise must be positive')
        if settings.adaptive_relative_error > 0.25 or settings.adaptive_absolute_error > 0.1:
            raise ValueError('adaptive error limits must not exceed 0.25 relative or 0.1 absolute')
        return settings

    def planner_options(self):
        return SimpleNamespace(**asdict(self), spacing=None, vertical_spacing=None,
                               moby_whitelist=None)


def write_json(path, value, *, replace=False):
    if path.exists() and not replace:
        raise FileExistsError(str(path))
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', dir=path.parent,
                                         prefix='.'+path.name, suffix='.part', delete=False) as out:
            temporary = Path(out.name)
            json.dump(value, out, indent=2, allow_nan=False)
            out.write('\n')
        file_ops.replace(temporary, path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def prepare_emission(level_dir, baker):
    """Resolve the same explicit material definitions used by normal level export."""
    manifest = diagnostics.read_json(level_dir/'manifest.json')
    if all(key in manifest for key in ('game', 'version', 'level')):
        profiles = seeds.ROOT/'port/enhanced/lighting/profiles'/f"{manifest['game']}-{manifest['version']}"
        diagnostics.capture([str(baker), '--prepare-level-emission', str(level_dir),
                             str(profiles/'emission-materials.json'),
                             str(profiles/f"level_{manifest['level']:02}"/'probe-emission.json')])


def inspection(text):
    try:
        fields = {line.split(' ', 1)[0]: line.split(' ', 1)[1]
                  for line in text.splitlines() if ' ' in line}
        grid = {'game_version': fields['game_version'], 'level': int(fields['level']),
                'origin': list(map(float, fields['probe_origin'].split())),
                'spacing': list(map(float, fields['probe_spacing'].split())),
                'dimensions': list(map(int, fields['probe_dimensions'].split()))}
        emission = {key: fields[key] for key in (
            'emissive_triangles', 'automatic_liquid_triangles', 'point_emitters',
            'bake_surfaces', 'emission', 'emission_vertex_colour_triangles') if key in fields}
        return grid, emission, fields['scene_hash']
    except (KeyError, ValueError) as error:
        raise diagnostics.NativeFailure(1, ['scene inspection'], text, protocol=True) from error


# The baker's exit status for a level with nothing to bake (level_scene.h, kNothingToBake).
NOTHING_TO_BAKE = 3


class NothingToBake(Exception):
    """The level has nothing for light to bounce off (the flight between planets: only a sky)."""


class NativeBatch:
    """Keep one native scene alive through region planning and verified grid baking."""

    def __init__(self, command, log_path, *, append=False, progress=None, process_factory=None):
        self.command = command
        self.log_path = log_path
        self.append = append
        # progress(fraction): the share of the level's probes baked, from the baker's "Probes" lines.
        self.progress = progress
        self.process_factory = process_factory
        self.process = None
        self.log = None
        self.inspection = ''
        self.tail = deque(maxlen=40)

    def __enter__(self):
        self.log = self.log_path.open('a' if self.append else 'x', encoding='utf-8', errors='replace')
        try:
            self.process = self.process_factory(self.command) if self.process_factory else subprocess.Popen(self.command, stdin=subprocess.PIPE,
                                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                            text=True, bufsize=1, encoding='utf-8', errors='replace')
            lines = []
            inspected_bytes = 0
            self.log.write('Command: '+subprocess.list2cmdline(list(map(str, self.command)))+'\n')
            for line in self.process.stdout:
                self.tail.append(line)
                self.log.write(line)
                self.log.flush()
                if line.startswith(('Bake stage: ', 'Emission audit: ')):
                    print(line.rstrip(), flush=True)
                if line.strip() == 'Probe batch ready':
                    self.inspection = ''.join(lines)
                    return self
                inspected_bytes += len(line)
                if inspected_bytes > 131072:
                    raise diagnostics.NativeFailure(1, self.command, 'Scene inspection exceeded 128 KiB without a ready marker', self.log_path, protocol=True)
                lines.append(line)
            self.process.wait()
            if self.process.returncode == NOTHING_TO_BAKE:
                raise NothingToBake(''.join(lines).strip().splitlines()[-1] if lines else 'nothing to bake')
            raise diagnostics.NativeFailure(self.process.returncode or 1, self.command, ''.join(self.tail),
                                            self.log_path, protocol=self.process.returncode == 0)
        except BaseException:
            self.close()
            raise

    def coverage(self, request):
        """Run the entire enclosure flood against the resident native collision acceleration."""
        self.process.stdin.write(json.dumps({'type': 'coverage', **request}, allow_nan=False)+'\n')
        self.process.stdin.flush()
        results = []
        for line in self.process.stdout:
            self.tail.append(line)
            self.log.write(line)
            if line.startswith('Coverage result '):
                results.append(json.loads(line[len('Coverage result '):]))
            elif line.strip() == 'Coverage complete':
                self.log.flush()
                if len(results) != len(request['seeds']):
                    raise ValueError('native coverage returned the wrong seed count')
                return results
        raise diagnostics.NativeFailure(self.process.wait() or 1, self.command,
                                        ''.join(self.tail), self.log_path, protocol=True)

    def planning_request(self, level, coarse, profile, gameplay, options):
        """Retain decoded legacy gameplay and supply only source paths and resolved settings."""
        path = level/'bake-inputs/gameplay.bin'
        if gameplay is None:
            path = None
        elif path.is_file():
            if path.read_bytes() != gameplay:
                raise ValueError('gameplay source changed before region planning')
        else:
            # Legacy exports retain the already-decoded disc source for this native stage.
            path = self.log_path.parent/'region-gameplay.bin'
            with tempfile.TemporaryDirectory(prefix='.region-gameplay-', dir=path.parent) as temporary:
                staged = Path(temporary)/path.name
                staged.write_bytes(gameplay)
                staged.replace(path)
        import moby_seeds
        manifest = diagnostics.read_json(level/'manifest.json')
        whitelist = options.moby_whitelist or moby_seeds.default_whitelist_path(manifest['game'])
        values = {key: value for key, value in vars(options).items() if key != 'moby_whitelist'}
        return dict(type='region_plan', coarse=coarse, profile=profile, options=values,
                    gameplay=str(path.resolve()) if path else '', whitelist=str(whitelist.resolve()))

    def plan(self, level, coarse, profile, gameplay, options):
        """Return a native planning report for inspection and reference comparisons."""
        request = self.planning_request(level, coarse, profile, gameplay, options)
        self.process.stdin.write(json.dumps(request, allow_nan=False)+'\n')
        self.process.stdin.flush()
        fragments = []
        size = 0
        for line in self.process.stdout:
            self.tail.append(line)
            self.log.write(line)
            if line.startswith('Region plan fragment '):
                fragment = json.loads(line[len('Region plan fragment '):])
                if not isinstance(fragment, str):
                    raise ValueError('native region plan fragment must be a string')
                size += len(fragment.encode('utf-8'))
                if size > 16*1024*1024:
                    raise ValueError('native region plan exceeds 16 MiB')
                fragments.append(fragment)
            elif line.strip() == 'Region plan complete':
                self.log.flush()
                plan = json.loads(''.join(fragments))
                if not isinstance(plan, dict) or not isinstance(plan.get('regions'), list):
                    raise ValueError('native region plan is incomplete')
                return plan
        raise diagnostics.NativeFailure(self.process.wait() or 1, self.command,
                                        ''.join(self.tail), self.log_path, protocol=True)

    def finish(self, plan):
        try:
            self.process.stdin.write(json.dumps(plan, allow_nan=False)+'\n')
            self.process.stdin.close()
        except (BrokenPipeError, OSError) as error:
            try:
                self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
            for line in self.process.stdout:
                self.tail.append(line)
                self.log.write(line)
            self.log.flush()
            raise diagnostics.NativeFailure(self.process.returncode or 1, self.command,
                                            ''.join(self.tail), self.log_path) from error
        completed = False
        for line in self.process.stdout:
            self.tail.append(line)
            self.log.write(line)
            self.log.flush()
            if line.startswith(('Baking ', 'Verified ', 'Kept ')):
                print(line.rstrip(), flush=True)
            elif line.startswith('Probes ') and self.progress is not None:
                done, _, total = line[7:].strip().partition('/')
                if done.isdigit() and total.isdigit() and int(total) > 0:
                    self.progress(int(done) / int(total))
            completed |= line.strip() == 'Probe batch complete'
        result = self.process.wait()
        if result or not completed:
            raise diagnostics.NativeFailure(result or 1, self.command, ''.join(self.tail),
                                            self.log_path, protocol=result == 0)

    def close(self):
        if self.process is not None:
            if self.process.poll() is None:
                self.process.terminate()
                try:
                    self.process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    self.process.kill()
                    self.process.wait()
            for stream in (self.process.stdin, self.process.stdout):
                if stream is not None:
                    stream.close()
        if self.log is not None:
            self.log.close()

    def __exit__(self, *_):
        self.close()


def bake_level(level_dir: Path, output_dir: Path, baker: Path, *, disc=None, options=(), resume=False,
               progress=None, process_factory=None):
    """RESUME: OUTPUT_DIR may hold what a stopped bake of this level wrote; the grids it verified
    for the same plan are kept (the baker reads each back and checks it), the rest is baked.
    PROGRESS(fraction) hears how far the level's probes are."""
    level_dir, output_dir, baker = level_dir.resolve(), output_dir.resolve(), baker.resolve()
    if output_dir.is_relative_to(level_dir):
        raise ValueError('bake outputs must be outside the source level directory')
    if output_dir.exists() and not resume:
        raise ValueError('choose a new output directory; previous bakes are preserved')
    manifest = diagnostics.stage('Read level manifest', diagnostics.read_json, level_dir/'manifest.json')
    diagnostics.stage('Prepare water', prepare_water.prepare, level_dir, manifest, disc, baker=baker)
    settings = diagnostics.stage('Read lighting settings', RegionSettings.load, level_dir)
    gameplay = diagnostics.stage('Read gameplay seeds', seeds.read_gameplay, level_dir, manifest, disc) \
        if settings.enabled or settings.interior_lighting else None
    common = [str(baker), '--level-dir', str(level_dir), *options]
    profile = diagnostics.stage('Read baker profile', lambda: diagnostics.parse_profile(diagnostics.capture(common+['--print-profile'])))
    output_dir.mkdir(parents=True, exist_ok=resume)

    # Resolve reviewed shared materials before the native scene snapshot, including older exports.
    diagnostics.stage('Prepare emission', prepare_emission, level_dir, baker)
    interior_options = []
    if settings.interior_lighting:
        interior_options = diagnostics.stage('Prepare interior input', automatic_interiors.native_options, level_dir, output_dir, manifest, gameplay)
    else:
        automatic_interiors.remove_generated(level_dir)

    command = common + ['--batch-output', str(output_dir), *interior_options] + (['--resume'] if resume else [])
    batch_options = dict(append=resume, progress=progress)
    if process_factory is not None:
        batch_options['process_factory'] = process_factory
    batch = NativeBatch(command, output_dir/'batch.log', **batch_options)
    try:
        batch.__enter__()
    except NothingToBake as nothing:
        # Done, with no bake: the game keeps the level's original lighting (no sparse.orpg).
        write_json(output_dir/'bake-result.json', replace=resume, value={
            'version': 1, 'verified': True, 'empty': True, 'note': str(nothing),
            'probe_format_version': profile['probe_format_version'], 'bake_version': profile['bake_version']})
        print(f'Nothing to bake: {nothing}', flush=True)
        return None
    # Started above (its start says whether there is anything to bake): closed whatever happens.
    with contextlib.closing(batch) as native:
        coarse, emission, source_hash = diagnostics.stage('Read native scene', inspection, native.inspection)
        request = native.planning_request(level_dir, coarse, profile, gameplay, settings.planner_options())
        request.pop('options')
        request.update(type='bake_level', region_settings=asdict(settings),
                       scene_hash=source_hash, emission=emission)
        diagnostics.stage('Plan, bake and register lighting', native.finish, request)

    sparse = output_dir/'sparse.orpg'
    print('Verified level bake:', sparse, flush=True)
    return sparse


def main():
    diagnostics.configure_streams()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--level-dir', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--disc', type=Path, help='fallback for exports made before gameplay retention')
    parser.add_argument('--baker', type=Path, default=seeds.ROOT/'port/build/release'/
                        ('openrac-enhanced-baker.exe' if os.name == 'nt' else 'openrac-enhanced-baker'))
    parser.add_argument('--rays', type=int, help='quality override for every grid and verification')
    parser.add_argument('--bounces', type=int, help='bounce override for every grid and verification')
    parser.add_argument('--threads', type=int, help='CPU bake threads; 0 uses all, 1 is serial')
    args = parser.parse_args()
    options = []
    for key in ('rays', 'bounces', 'threads'):
        value = getattr(args, key)
        if value is not None:
            options.extend(('--' + key, str(value)))
    try:
        if args.output_dir.resolve().is_relative_to(args.level_dir.resolve()):
            raise ValueError('bake outputs must be outside the source level directory')
        if args.output_dir.exists():
            raise ValueError('choose a new output directory; previous bakes are preserved')
        with diagnostics.transcript(args.output_dir) as log:
            try:
                bake_level(args.level_dir, args.output_dir, args.baker, disc=args.disc, options=options, resume=True)
            except Exception as error:
                diagnostics.failed(error, level=args.level_dir.name, log=log)
                return 1
    except Exception as error:
        diagnostics.failed(error, level=args.level_dir.name)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
