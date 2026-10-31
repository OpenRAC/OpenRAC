# SPDX-License-Identifier: GPL-3.0-or-later
"""Select native in-process interior preparation and preserve exported gameplay for legacy levels."""
from pathlib import Path
import tempfile

import diagnostics


def remove_generated(level_dir):
    """An opt-out removes only this tool's derived sources, never explicit sidecars."""
    path = level_dir/'probe-interiors.json'
    if path.exists() and diagnostics.read_json(path).get('generator') == 'openrac-interiors-v1':
        path.unlink()
        print('Removed automatically generated interior sources', flush=True)


def native_options(level_dir, output_dir, manifest, gameplay):
    """Pass retained gameplay to the persistent native worker, without an inference subprocess."""
    if gameplay is None or not 0 <= manifest.get('level', -1) <= 18:
        return []
    path = level_dir/'bake-inputs/gameplay.bin'
    if path.is_file():
        if path.read_bytes() != gameplay:
            raise ValueError('gameplay source changed before interior preparation')
    else:
        # Older exports can read gameplay from their owned disc; retain that snapshot for this batch.
        path = output_dir/'interior-gameplay.bin'
        with tempfile.TemporaryDirectory(prefix='.interior-gameplay-', dir=output_dir) as temporary:
            staged = Path(temporary)/path.name
            staged.write_bytes(gameplay)
            staged.replace(path)
    return ['--prepare-interiors', str(path)]
