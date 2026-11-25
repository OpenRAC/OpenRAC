# SPDX-License-Identifier: GPL-3.0-or-later
"""Add neutral gameplay decision points to native copies, never to the decompilation.

The catalogue contains source/symbol facts. Only the requested expression is wrapped;
without a callback every query returns its original value. Generated bodies remain local.
"""
import hashlib
import json
import re
from pathlib import Path
import prep

DECL = "extern int openrac_mod_play_query(unsigned int, unsigned int, int, int, int);"

def query(kind, subject, index, original, alternate=0, boolean=False):
    original = "!!(" + original + ")" if boolean else original
    return f"openrac_mod_play_query({kind}u, (unsigned int)({subject}), {index}, ({original}), ({alternate}))"

def instrument(key, body, name, version):
    match = re.search(r"\b" + re.escape(name) + r"\s*\(([^)]*)\)\s*\{", body)
    if not match: raise ValueError("No query-point definition: " + name)
    first = match[1].split(",")[0].strip()
    param = re.search(r"(\w+)\s*(?:\[.*?\])?$", first)
    subject = param[1] if param and first != "void" else "0"
    count = 0
    if key.endswith("CONTROLLER_FUNC") or key == "BARLOW_SPAWN_CONTROLLER_FUNC":
        predicates = {
            "OOZLA_CONTROLLER_FUNC": "g_item_owned_flags[0x2e]",
            "MAKTAR_CONTROLLER_FUNC": "g_frame_l02_counter<3 && g_item_owned_flags[0x26]",
            "BARLOW_SPAWN_CONTROLLER_FUNC": "g_inventory_have_flags[0x1F] != 0 && g_game_flag_table[0x12] == 0 && g_game_tick_counter[0] < 5",
            "JOBA_CONTROLLER_FUNC": "g_focus_lock_flag",
            "TABORA_CONTROLLER_FUNC": "g_item_owned_flags[0x15]",
            "DAMOSEL_CONTROLLER_FUNC": "TRIGGER(0xB4) && !TRIGGER(0xB8)",
        }
        indices = dict(OOZLA_CONTROLLER_FUNC=46, MAKTAR_CONTROLLER_FUNC=38, BARLOW_SPAWN_CONTROLLER_FUNC=31,
                       JOBA_CONTROLLER_FUNC=-1, TABORA_CONTROLLER_FUNC=21, DAMOSEL_CONTROLLER_FUNC=-1)
        old = predicates[key]
        pattern = r"(if\s*\(\s*)" + re.escape(old)
        body, count = re.subn(pattern, lambda m: m[1] + query(2, subject, indices[key], old, 0, True), body, count=1)
    elif key == "SWAMP_MONSTER_GATE_FUNC":
        old = "g_hero_override_flag[0]"
        assert body.count(old) == 1
        body = body.replace(old, query(128, subject, 50, old, 0, True)); count = 1
    elif key == "RESET_MOBY_FUNCS":
        for var, base, kind in [("bolts2", "bolts", 4), ("xp2", "xp", 8)]:
            body, n = re.subn(var + r"\s*=\s*\*\(int\s*\*\)p;",
                             var + " = " + query(kind, "0", "i", "*(int *)p", base) + ";", body)
            count += n
        if count != 2: raise ValueError("Expected two revisit reward queries: " + str((key, count)))
    elif key == "SHORTCUT_MENU_FUNCS":
        at = body.index("{") + 1
        body = body[:at] + "\n    if (!" + query(64, "0", 0, "1", 0) + ") return 0;\n" + body[at:]
        count = 1
    elif key == "SPECIAL_MENU_FUNCS":
        bind = re.search(r"\btab_menu_set_tables(?:_pal|_us2)?\(", body)
        if bind is None: raise ValueError("No extras table binding")
        at = bind.start()
        lines = []
        for i, data in enumerate(["g_extras_unlock_entry_a", "g_extras_unlock_entry_b"]):
            labels = re.findall(r"menu->entries\[" + str(i) + r"\]\.text_id\s*=\s*(0x[0-9A-Fa-f]+)", body)
            if len(labels) != 2: raise ValueError("No verified extras text pair")
            text = labels[-1]
            prefix = f"g_extras_tab_table_c.entries[{i}]"
            lines.append(prefix + ".text_id = " + query(16, "&g_extras_tab_table_c", i, prefix + ".text_id", text) + ";")
            lines.append(prefix + ".data = (void *)" + query(32, "&g_extras_tab_table_c", i,
                         "(unsigned int)" + prefix + ".data", "(unsigned int)" + data) + ";")
        body = body[:at] + "\n        ".join(lines) + "\n        " + body[at:]; count = 4
    elif "ARENA" in key:
        needle = "g_game_flag_table["; at = 0; edits = []
        while (start := body.find(needle, at)) >= 0:
            end = start + len(needle); depth = 1
            while depth:
                if body[end] == "[": depth += 1
                if body[end] == "]": depth -= 1
                end += 1
            expr = body[start:end]
            if "arena_get_challenge_" in expr and ("MENU_FUNC" in key or "[i]" in expr):
                edits.append((start, end, query(1, subject, 0, expr, 1, True)))
            at = end
        for a, b, text in reversed(edits): body = body[:a] + text + body[b:]
        count = len(edits)
    elif key == "START_SPACESHIP_CHALLENGE_FUNCS":
        body, count = re.subn(r"if\s*\(\s*!\s*(g_feltzin_challenge_data|D_L05_001B1350_4\[[12]\])\s*\)",
            lambda m: "if (!" + query(1, subject, 1, m[1], 1, True) + ")", body)
    elif key == "SPACESHIP_MENU_FUNCS":
        # These records also hold reward-table indices (0..2). Only prerequisite
        # conditions are booleans; leave every numeric/indexing read untouched.
        record = r"(?:D_L05_001B1350_gp\[0\]|g_feltzin_challenge_data\[[12]\])"
        term = record + r"(?:\s*!=\s*0)?"
        condition = r"(\bif\s*\(\s*)(" + term + r"(?:\s*&&\s*" + term + r")*)(\s*\))"
        def prerequisite(match):
            nonlocal count
            def decision(term_match):
                nonlocal count
                count += 1
                return query(1, subject, 1, term_match[0], 1, True)
            return match[1] + re.sub(term, decision, match[2]) + match[3]
        body = re.sub(condition, prerequisite, body)
    else:
        body, count = re.subn(r"(g_race_challenge_table(?:\.entry)?\[[^\]]+\]\.flags\s*&\s*0x10000000)",
            lambda m: query(1, subject, 2, m[1], 1, True), body)
    if not count: raise ValueError("No verified gameplay query insertion: " + key + " " + name)
    first = body.index("{") + 1
    return body[:first] + "\n    " + DECL + "\n" + body[first:], count

def build(source: Path, catalogue: Path, version: str, view=None):
    data = json.loads(catalogue.read_text(encoding="utf-8"))
    if data.get("format") != 1: raise ValueError("Unknown gameplay query catalogue")
    out = {}
    for key, versions in data["points"].items():
        row = versions[version]
        text = (source / row["source"]).read_text(encoding="utf-8")
        span = prep._definition(text, row["definition"])
        if span is None: raise ValueError("Missing query source definition: " + key)
        definition = text[span[0]:span[1]]
        if hashlib.sha256(definition.encode("utf-8")).hexdigest() != row["definition_sha256"]:
            raise ValueError("Gameplay query source changed; audit the definition before updating its evidence: " + key)
        body, count = instrument(key, text[span[0]:span[1]], row["definition"], version)
        if count != versions["expected_queries"]:
            raise ValueError("Gameplay query count changed: " + key + " " + version)
        if view is not None:
            body = re.sub(r"\b" + re.escape(row["definition"]) + r"(?=\s*\()", row["symbol"], body, count=1)
            body = view.text(body)
        if not prep.defines(body, row["symbol"]):
            body = re.sub(r"\b" + re.escape(row["definition"]) + r"(?=\s*\()", row["symbol"], body, count=1)
        out[row["symbol"]] = "/* Native gameplay query points; GPL-3.0-or-later. */\n" + body
    return out
