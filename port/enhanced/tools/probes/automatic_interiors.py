# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare default interior sources before the native baker fingerprints a level."""
import json
import hashlib
from pathlib import Path
import subprocess
import tempfile

import diagnostics

from prepare_interiors import prepare_export
from prepare_interior_lights import source_plan


def inference_tool(baker):
    """Find the companion program, including Android's executable-library naming."""
    name = 'libopenrac-interior-infer.so' if baker.name.startswith('lib') else 'openrac-interior-infer'+baker.suffix
    return baker.with_name(name)


def remove_generated(level_dir):
    """An opt-out removes only this tool's derived sources, never explicit sidecars."""
    path = level_dir/'probe-interiors.json'
    if path.exists() and diagnostics.read_json(path).get('generator') == 'openrac-interiors-v1':
        path.unlink()
        print('Removed automatically generated interior sources', flush=True)


def prepare(level_dir, output_dir, baker, manifest, gameplay):
    """Preserve explicit sidecars; refresh owned derived sources when their inputs change."""
    sidecar = level_dir/'probe-interiors.json'
    previous = diagnostics.read_json(sidecar) if sidecar.exists() else None
    if previous is not None and previous.get('generator') != 'openrac-interiors-v1':
        print('Using explicit interior sources:', sidecar, flush=True)
        return
    if gameplay is None or not 0 <= manifest.get('level', -1) <= 18:
        return
    inputs = prepare_export(level_dir, manifest, gameplay)
    executable = inference_tool(baker)
    if not executable.is_file():
        raise ValueError(f'interior inference tool not found: {executable}; rebuild the probe baker')
    identity = hashlib.sha256(inputs['source_sha256'].encode()+executable.read_bytes())
    for name in ('automatic_interiors.py', 'prepare_interiors.py', 'prepare_interior_lights.py'):
        identity.update(Path(__file__).with_name(name).read_bytes())
    if previous is not None and previous.get('generation_sha256') == identity.hexdigest():
        print('Interior sources are current:', sidecar, flush=True)
        return
    input_path, report_path = output_dir/'interior-input.json', output_dir/'interior-inference.json'
    input_path.write_text(json.dumps(inputs, separators=(',', ':'), allow_nan=False)+'\n')
    log_path = output_dir/'interior-inference.log'
    try:
        with log_path.open('w', encoding='utf-8') as log:
            subprocess.run([str(executable), str(input_path), str(report_path)],
                           stdout=log, stderr=subprocess.STDOUT, check=True)
    except subprocess.CalledProcessError as error:
        tail = log_path.read_text(encoding='utf-8', errors='replace')[-12000:]
        raise diagnostics.NativeFailure(error.returncode, error.cmd, tail, log_path) from error
    plan = source_plan(diagnostics.read_json(report_path), manifest)
    plan['generator'] = 'openrac-interiors-v1'
    plan['generation_sha256'] = identity.hexdigest()
    # Replace only this tool's derived output, after successful inference and validation.
    with tempfile.TemporaryDirectory(prefix='.probe-interiors-', dir=level_dir) as staged:
        temporary = Path(staged)/sidecar.name
        temporary.write_text(json.dumps(plan, separators=(',', ':'), allow_nan=False)+'\n')
        temporary.replace(sidecar)
    print(f"Prepared {len(plan['samples'])} interior lighting samples", flush=True)
