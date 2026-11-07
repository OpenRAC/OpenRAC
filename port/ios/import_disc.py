# SPDX-License-Identifier: GPL-3.0-or-later
"""Dispatch local preparation to a signed importer; retail images never supply code."""
import json
from importers import rac1

IMPORTERS = {"rac1": rac1.prepare}


def prepare(data_folder, progress, engine_json):
    engine = json.loads(engine_json)
    importer = IMPORTERS.get(engine["importer"])
    if importer is None:
        raise ValueError("This engine has no bundled disc importer")
    importer(data_folder, progress, engine)
