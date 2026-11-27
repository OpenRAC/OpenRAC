# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Run decomp-permuter's random search natively, for any game with a profile, with live progress.

Adapted from rac3-decomp's tools/permuter_engine.py (GPL-3.0): the same workers and collector, with
the compile and the score taken from the game's profile instead of rac3's tools. Each worker is a
process that imports decomp-permuter's `src` package (upstream.py), mutates base.c, compiles each
candidate with the profile's commands, and scores it with score.py against the function's retail
assembly. A candidate scoring 0 goes to the game's own checker; only when it says EXACT is the run
"matched", and a 0 it rejects counts as 1, so score 0 always means what the project calls a match.

The run writes OUT/output-<score>-<n>/ for every candidate that ties or beats the base (source.c,
function.c, artefacts.txt when accept.py finds permuter artefacts in it) and OUT/result.json:

    {"profile", "func", "status", "base_score", "best_score", "best": {"score", "path",
     "artefacts", "function"}, "iterations", "elapsed", "outputs": [...]}
"""
from __future__ import annotations

import hashlib
import json
import math
import multiprocessing as mp
import os
import queue
import signal
import shutil
import subprocess
import tempfile
import threading
import time
import traceback
from pathlib import Path

from . import accept, profile as profiles, score, structural, upstream

KEEP_PROB = 0.6   # decomp-permuter's DEFAULT_RAND_KEEP_PROB
REJECTED_ZERO = 1  # the score of a 0 the checker does not call EXACT


def prepare(prof: profiles.Profile, func: str, draft: str | None, permuter: Path,
            asm_passes: bool = True) -> dict:
    """Everything a worker needs, as plain data: base.c, the draft's declarations, the retail
    function, the source file and its flags. Raises FileNotFoundError, RuntimeError, ValueError."""
    modules = upstream.load(permuter)
    base, decls = profiles.base_source(prof, func, draft, modules.strip_other_fns.strip_other_fns)
    name = profiles.retail_name(prof, func)
    asm = profiles.find_asm(prof, name)
    retail = score.parse_asm(asm.read_text(errors="replace"), name, prof.word_order)
    source = profiles.find_source(prof, func)
    flags = ""
    if prof.flags:
        done = subprocess.run(prof.expand(prof.flags, {"func": func, "source": str(source)}), cwd=prof.checkout,
                              env=prof.environment(), capture_output=True, text=True)
        if done.returncode:
            raise RuntimeError(f"the flags command failed:\n{done.stderr[-2000:]}")
        flags = done.stdout.strip()
    weights = dict(prof.weights)
    allow_asm = asm_passes and prof.asm_passes and not prof.structural_passes
    if allow_asm and upstream.has_asm_passes(modules):
        weights.update(upstream.ASM_PASSES)
    else:
        for name in upstream.ASM_PASSES:
            weights.pop(name, None)
    return dict(profile=prof.name, func=func, base=base, decls=decls, retail=[retail.words, retail.masks, retail.calls],
                source=str(source), flags=flags, permuter=str(permuter), weights=weights, asm=str(asm),
                max_candidates=prof.max_candidates, candidate_ceiling=prof.candidate_ceiling,
                keep_work=prof.keep_work, structural_passes=list(prof.structural_passes),
                allow_asm_passes=allow_asm)


def run_start(prof: profiles.Profile) -> None:
    """The profile's start commands (a container to run the compiler in, say); each must be idempotent."""
    for step in prof.start:
        subprocess.run(prof.expand(step, {}), cwd=prof.checkout, env=prof.environment(), check=True,
                       capture_output=True)


def _lock(handle) -> None:
    """An exclusive lock on HANDLE for as long as it is open (none on Windows: one check at a time
    is then up to the caller)."""
    try:
        import fcntl
    except ImportError:
        return
    fcntl.flock(handle, fcntl.LOCK_EX)


class Candidate:
    """Compiles one candidate the way the profile says and asks its checker about a 0."""

    def __init__(self, cfg: dict, tmp: Path):
        self.cfg, self.tmp = cfg, tmp
        self.prof = profiles.load(cfg["profile"])
        self.env = self.prof.environment()
        self.retail = score.Function(*cfg["retail"])
        self.source = Path(cfg["source"])
        self.n = 0
        self.last_error = ""
        self.last_function = ""
        self.last_output = ""

    def values(self, function: str) -> dict:
        func = self.cfg["func"]
        (self.tmp / "decls.c").write_text(self.cfg["decls"])
        (self.tmp / "snippet.c").write_text(self.cfg["decls"] + function + "\n")
        text = self.source.read_text(errors="replace")
        span = profiles.definition(text, func) or profiles.stub(text, func)
        spliced = self.tmp / "spliced" / self.source.name
        spliced.parent.mkdir(exist_ok=True)
        whole = text[:span[0]] + self.cfg["decls"] + function + text[span[1]:]
        if self.prof.prelude and self.prof.prelude.strip() not in whole:
            whole = self.prof.prelude + whole
        spliced.write_text(whole)
        self.n += 1
        return dict(func=func, snippet=str(self.tmp / "snippet.c"), decls=str(self.tmp / "decls.c"),
                    out=str(self.tmp / f"out{self.n % 4}.o"), tmp=str(self.tmp), spliced=str(spliced),
                    source=str(self.source), srcdir=str(self.source.parent), flags=self.cfg["flags"])

    def compile(self, source: str, show_errors: bool = False) -> str | None:
        """decomp-permuter's Compiler interface: the object for candidate SOURCE, or None."""
        try:
            function = profiles.function_text(source, self.cfg["func"])
        except ValueError as e:
            self.last_error = str(e)
            return None
        values = self.values(function)
        out = Path(values["out"])
        out.unlink(missing_ok=True)
        output = []
        for step in self.prof.compile:
            done = subprocess.run(self.prof.expand(step, values), cwd=self.prof.checkout, env=self.env,
                                  capture_output=True, text=True, errors="replace")
            output.append(done.stdout + done.stderr)
            if done.returncode and not self.prof.compile_may_fail:
                break
        self.last_output = "".join(output)
        if not out.is_file():
            self.last_error = self.last_output[-2000:]
            return None
        self.last_function, self.last_values = function, values
        return str(out)

    def exact(self) -> bool:
        """The game's own checker on the last candidate compiled."""
        pattern = self.prof.exact_re(self.cfg["func"])
        if not self.prof.check:
            return bool(pattern.search(self.last_output))
        lock = Path(tempfile.gettempdir()) / f"openrac-permuter-check-{self.prof.name.replace('/', '-')}.lock"
        with open(lock, "w") as held:
            _lock(held)
            if self.prof.check_in_place:
                return self.exact_in_place(pattern)
            done = subprocess.run(self.prof.expand(self.prof.check, self.last_values), cwd=self.prof.checkout,
                                  env=self.env, capture_output=True, text=True, errors="replace")
        return bool(pattern.search(done.stdout + done.stderr))

    def exact_in_place(self, pattern) -> bool:
        """For a checker that only reads the project's own source: the candidate goes into the
        source file for the length of the check, and the file's bytes come back after it (unless
        someone changed the file meanwhile, which is then left alone and reported)."""
        original = self.source.read_bytes()
        spliced = Path(self.last_values["spliced"]).read_bytes()
        self.source.write_bytes(spliced)
        try:
            done = subprocess.run(self.prof.expand(self.prof.check, self.last_values), cwd=self.prof.checkout,
                                  env=self.env, capture_output=True, text=True, errors="replace")
        finally:
            if self.source.read_bytes() == spliced:
                self.source.write_bytes(original)
            else:
                print(f"warning: {self.source} changed during the check; not restored", flush=True)
        return bool(pattern.search(done.stdout + done.stderr))


class Scorer:
    """decomp-permuter's Scorer interface over score.py."""
    PENALTY_INF = 10 ** 9

    def __init__(self, cand: Candidate):
        self.cand = cand

    def score(self, obj: str | None) -> tuple[int, str]:
        if not obj:
            return self.PENALTY_INF, ""
        try:
            ours = score.from_object(Path(obj), self.cand.cfg["func"])
        except (KeyError, ValueError, OSError):
            return self.PENALTY_INF, ""
        n = score.diffs(ours, self.cand.retail)
        if n == 0 and not self.cand.exact():
            n = REJECTED_ZERO
        return n, hashlib.sha256(repr(ours.words).encode()).hexdigest()


class _SearchStopped(BaseException):
    """Leave an upstream retry loop that catches ordinary Exception itself."""


def _worker(cfg: dict, wid: int, outq, stop, seed: int) -> None:
    cand, tmp, original, modules = None, None, None, None
    try:
        # Each POSIX worker owns its compiler/checker descendants. Terminating
        # this group never signals the user's shell or another search.
        if os.name == "posix" and mp.current_process().name != "MainProcess":
            os.setsid()
            outq.put(("group", wid, os.getpid()))
        modules = upstream.load(Path(cfg["permuter"]))
        import random
        random.seed(seed)
        weights = dict(modules.helpers.get_default_randomization_weights("gcc"))
        weights.update({k: v for k, v in cfg["weights"].items() if k in weights})
        if cfg.get("allow_asm_passes") is False or cfg.get("structural_passes"):
            for name in upstream.ASM_PASSES:
                if name in weights:
                    weights[name] = 0
        last_pass = [None]
        original = modules.randomizer.Randomizer.randomize
        enabled = cfg.get("structural_passes", [])
        if set(enabled) - set(structural.PASSES):
            raise ValueError("unknown structural pass")
        used_sites = set()
        current_slot = [0]

        def check_stop():
            if stop.is_set():
                raise _SearchStopped("stopped")
            if cfg.get("worker_deadline") and time.monotonic() >= cfg["worker_deadline"]:
                raise _SearchStopped("time limit")

        def randomize(self, ast, fn_name):
            check_stop()
            if enabled:
                ca = modules.randomizer.ca
                fn = modules.randomizer.ast_util.extract_fn(ast, fn_name)[0]
                indices = modules.randomizer.ast_util.compute_node_indices(fn)
                region = modules.randomizer.get_randomization_region(fn, indices, self.random)
                choices = structural.plans(ast, fn_name, ca, enabled, region)
                counts = {name: sum(p.name == name for p in choices) for name in enabled}
                outq.put(("applicable", wid, counts))
                choices = [(i, p) for i, p in enumerate(choices)
                           if (p.name, i) not in used_sites]
                if not choices:
                    raise _SearchStopped("no remaining applicable structural transformation")
                index, plan = self.random.choice(choices)
                used_sites.add((plan.name, index))
                plan.apply()
                last_pass[0] = plan.name
                outq.put(("candidate_start", wid, current_slot[0]))
                return
            methods, chosen = self.methods, []

            def wrap(method):
                def apply(*args, **kwargs):
                    check_stop()
                    chosen.append(method.__name__)
                    return method(*args, **kwargs)
                apply.__name__ = method.__name__
                return apply
            self.methods = [(wrap(method), weight) for method, weight in methods]
            try:
                original(self, ast, fn_name)
            finally:
                self.methods = methods
            last_pass[0] = chosen[-1] if chosen else None
            outq.put(("candidate_start", wid, current_slot[0]))
        modules.randomizer.Randomizer.randomize = randomize

        work = upstream.cache_dir() / "work"
        work.mkdir(parents=True, exist_ok=True)
        tmp = Path(tempfile.mkdtemp(prefix=f"w{wid}-", dir=work))
        cand = Candidate(cfg, tmp)
        scorer = Scorer(cand)
        sp = modules.permuter
        outq.put(("base_start", wid))
        p = sp.Permuter(str(tmp), cfg["func"], cand, scorer, "base.c", cfg["base"],
                        randomization_weights=weights, force_seed=None, force_rng_seed=None,
                        keep_prob=0.0 if enabled else KEEP_PROB,
                        need_profiler=False, need_all_sources=False,
                        show_errors=False, best_only=False, better_only=False,
                        score_threshold=None, debug_mode=False, speed=100)
        if p.base_score >= Scorer.PENALTY_INF:
            raise RuntimeError("the base does not compile:\n" + cand.last_error)
        outq.put(("base", wid, p.base_score, p.base_source if wid == 0 else None))
        # Stop before even asking for a seed: the constructor already compiled
        # the base, and an applicable randomizer need not exist for that source.
        if p.base_score == 0 or cfg.get("max_candidates") == 1:
            return
        batch, flushed = [], time.time()
        try:
            for candidate_no, source_seed in enumerate(p.seed_iterator(), start=2):
                if cfg.get("max_candidates", 0) and candidate_no > cfg["max_candidates"]:
                    break
                check_stop()
                last_pass[0] = None
                current_slot[0] = candidate_no
                res = p.try_eval_candidate(source_seed)
                if isinstance(res, sp.EvalError):
                    batch.append((last_pass[0], None))
                    if res.exc_str:
                        outq.put(("error", wid, res.exc_str[-1500:]))
                else:
                    batch.append((last_pass[0], res.score if res.score < Scorer.PENALTY_INF else None))
                    if p.should_output(res):
                        p.record_result(res)
                        outq.put(("found", wid, res.score, res.source, last_pass[0]))
                if time.time() - flushed > 0.5:
                    outq.put(("tick", wid, batch))
                    batch, flushed = [], time.time()
        finally:
            outq.put(("tick", wid, batch))
    except _SearchStopped as ended:
        outq.put(("stopped", wid, str(ended)))
    except Exception:
        tb = traceback.format_exc()
        if cand is not None and cand.last_error:
            tb += "\ncompiler output:\n" + cand.last_error
        outq.put(("fatal", wid, tb))
    finally:
        if original is not None:
            modules.randomizer.Randomizer.randomize = original
        if tmp is not None and not cfg.get("keep_work", False):
            shutil.rmtree(tmp, ignore_errors=True)
        outq.put(("done", wid))


class Job:
    """One run: JOBS worker processes and a collector thread. All state is plain JSON-able data
    in self.state, read through snapshot()."""

    def __init__(self, cfg: dict, out_dir: Path, jobs: int = 2, seconds: float = 0):
        self.cfg, self.out_dir = cfg, Path(out_dir)
        self.jobs, self.seconds = max(1, int(jobs)), float(seconds or 0)
        maximum, ceiling = cfg.get("max_candidates", 0), cfg.get("candidate_ceiling", 0)
        if not math.isfinite(self.seconds) or self.seconds < 0:
            raise ValueError("seconds must be finite and non-negative")
        for value in (maximum, ceiling):
            if not isinstance(value, int) or isinstance(value, bool) or value < 0:
                raise ValueError("candidate limits must be non-negative integers")
        if ceiling and (not maximum or maximum > ceiling):
            raise ValueError(f"candidate limit exceeds profile ceiling {ceiling}")
        if cfg.get("max_candidates", 0) and self.jobs != 1:
            raise ValueError("a total candidate cap requires one worker")
        if set(cfg.get("structural_passes", [])) - set(structural.PASSES):
            raise ValueError("unknown structural pass")
        self.lock = threading.Lock()
        self.state = dict(profile=cfg["profile"], func=cfg["func"], flags=cfg["flags"], jobs=self.jobs,
                          seconds=self.seconds, status="starting", message="", started=time.time(),
                          elapsed=0.0, iterations=0, base_candidates=0,
                          completed_base_candidates=0, attempted_candidates=0,
                          errors=0, compile_errors=0, base_score=None,
                          best_score=None, rate=0.0, timeline=[], events=[], event_seq=0, passes={},
                          outputs=[], asm_passes=any(k in cfg["weights"] for k in upstream.ASM_PASSES),
                          out_dir=str(self.out_dir), worker_errors=[],
                          applicable_passes={}, stop_reasons={})
        self.base_source = None
        self._seen = set()
        self._ctx = mp.get_context("spawn")
        self._stop = self._ctx.Event()
        self._q = self._ctx.Queue()
        self._procs = []
        self._groups = set()
        self._base_started = set()
        self._base_completed = set()
        self._candidate_started = set()
        self._rate = []

    def start(self) -> "Job":
        self.out_dir.mkdir(parents=True, exist_ok=True)
        worker_cfg = dict(self.cfg)
        if self.seconds:
            worker_cfg["worker_deadline"] = time.monotonic() + self.seconds
        for w in range(self.jobs):
            pr = self._ctx.Process(target=_worker, args=(worker_cfg, w, self._q, self._stop,
                                                         int(time.time() * 1e6) + w), daemon=True)
            pr.start()
            self._procs.append(pr)
        self.thread = threading.Thread(target=self._collect, daemon=True)
        self.thread.start()
        return self

    def stop(self, reason: str = "stopped") -> None:
        with self.lock:
            if self.state["status"] in ("running", "starting"):
                self.state["status"] = reason
        self._stop.set()

    def _event(self, kind: str, **kw) -> None:
        st = self.state
        st["event_seq"] += 1
        st["events"].append(dict(seq=st["event_seq"], t=round(time.time() - st["started"], 2), kind=kind, **kw))
        del st["events"][:-600]

    def _found(self, wid: int, value: int, src: str, pas: str | None) -> None:
        st = self.state
        digest = hashlib.sha256(src.encode()).hexdigest()
        if digest in self._seen:
            return
        self._seen.add(digest)
        improved = st["best_score"] is None or value < st["best_score"]
        if improved:
            st["best_score"] = value
            st["timeline"].append([st["elapsed"], value])
            if pas:
                st["passes"].setdefault(pas, [0, 0, 0])[1] += 1
        n = len(st["outputs"]) + 1
        d = self.out_dir / f"output-{value}-{n}"
        d.mkdir(parents=True, exist_ok=True)
        (d / "source.c").write_text(src)
        try:
            function = profiles.function_text(src, self.cfg["func"])
        except ValueError:
            function = ""
        (d / "function.c").write_text(function + "\n")
        artefacts = accept.artefacts(self.base_source, src, self.cfg["func"]) if self.base_source else []
        if artefacts:
            (d / "artefacts.txt").write_text("".join(a + "\n" for a in artefacts))
        st["outputs"].append(dict(n=n, score=value, t=st["elapsed"], pass_=pas, worker=wid, path=str(d),
                                  improved=improved, artefacts=artefacts))
        self._event("found", worker=wid, pass_=pas, score=value, n=n, improved=improved, artefacts=artefacts)
        if value == 0 and not artefacts:
            st["status"] = "matched"
            self._stop.set()

    def _collect(self) -> None:
        done = 0
        stopping_since = None
        while done < self.jobs:
            try:
                msg = self._q.get(timeout=0.5)
            except queue.Empty:
                msg = None
                if not any(pr.is_alive() for pr in self._procs):   # died before they could say so
                    with self.lock:
                        if self.state["status"] in ("running", "starting"):
                            self.state["status"] = "error"
                            self.state["message"] = self.state["message"] or "the workers exited"
                    break
            now = time.time()
            with self.lock:
                st = self.state
                st["elapsed"] = round(now - st["started"], 1)
                if self.seconds and st["elapsed"] >= self.seconds and st["status"] in ("running", "starting"):
                    st["status"] = "time limit"
                    self._stop.set()
                kind = msg[0] if msg else None
                if kind == "group":
                    if any(pr.pid == msg[2] for pr in self._procs):
                        self._groups.add(msg[2])
                elif kind == "base_start":
                    if msg[1] not in self._base_started:
                        self._base_started.add(msg[1])
                        st["base_candidates"] += 1
                        st["attempted_candidates"] += 1
                elif kind == "candidate_start":
                    key = msg[1], msg[2]
                    if key not in self._candidate_started:
                        self._candidate_started.add(key)
                        st["attempted_candidates"] += 1
                elif kind == "base":
                    _, wid, value, src = msg
                    if wid not in self._base_started:  # older worker/message fixtures
                        self._base_started.add(wid)
                        st["base_candidates"] += 1
                        st["attempted_candidates"] += 1
                    if wid not in self._base_completed:
                        self._base_completed.add(wid)
                        st["completed_base_candidates"] += 1
                    if src:
                        self.base_source = src
                    if st["base_score"] is None:
                        st["base_score"] = st["best_score"] = value
                        st["timeline"].append([st["elapsed"], value])
                        self._event("base", score=value)
                        if value == 0:
                            st["status"] = "matched"
                            self._stop.set()
                    if st["status"] == "starting":
                        st["status"] = "running"
                elif kind == "tick":
                    for pas, value in msg[2]:
                        st["iterations"] += 1
                        counts = st["passes"].setdefault(pas or "(none)", [0, 0, 0])
                        counts[0] += 1
                        if value is None:
                            st["compile_errors"] += 1
                            counts[2] += 1
                    self._rate.append((now, len(msg[2])))
                elif kind == "found":
                    self._found(msg[1], msg[2], msg[3], msg[4])
                elif kind == "error":
                    st["errors"] += 1
                    if len(st["worker_errors"]) < 5:
                        st["worker_errors"].append(msg[2])
                elif kind == "fatal":
                    st["worker_errors"].append(msg[2])
                    self._event("fatal", worker=msg[1], text=msg[2][-1500:])
                    if st["status"] != "matched":
                        st["status"] = "error"
                        st["message"] = msg[2].strip().split("\n")[-1]
                        self._stop.set()
                elif kind == "applicable":
                    st["applicable_passes"][str(msg[1])] = msg[2]
                elif kind == "stopped":
                    st["stop_reasons"][str(msg[1])] = msg[2]
                    if st["status"] in ("starting", "running"):
                        st["message"] = msg[2]
                elif kind == "done":
                    done += 1
                self._rate = [(t, k) for t, k in self._rate if now - t < 10]
                st["rate"] = round(sum(k for _, k in self._rate) / max(1.0, min(10.0, now - st["started"])), 2)
                if self._stop.is_set():
                    stopping_since = stopping_since or time.monotonic()
                    # Cooperative checks cannot interrupt a hung compiler or a
                    # native extension. Give it a brief chance, then own cleanup.
                    if time.monotonic() - stopping_since >= 1.0:
                        break
        for pr in self._procs:
            if self._stop.is_set():
                self._finish_worker(pr)
            else:
                pr.join(timeout=3)
                self._finish_worker(pr)
        with self.lock:
            if self.state["status"] in ("running", "starting"):
                self.state["status"] = "finished"
            self.state["elapsed"] = round(time.time() - self.state["started"], 1)
            self.write_result()

    def _finish_worker(self, process) -> None:
        """Reap only this Job's workers and their dedicated process groups."""
        if process not in self._procs:
            raise ValueError("process does not belong to this job")
        if os.name == "posix" and process.pid in self._groups:
            try:
                os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            process.join(timeout=2)
            try:
                # A compiler grandchild can outlive an exited worker.
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
        elif process.is_alive():
            if os.name == "nt":
                subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                               capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
            else:
                process.terminate()
            process.join(timeout=2)
            if process.is_alive():
                process.kill()
        process.join(timeout=2)

    def write_result(self) -> None:
        st = self.state
        clean = [o for o in st["outputs"] if not o["artefacts"]]
        ranked = sorted(clean or st["outputs"], key=lambda o: (o["score"], o["n"]))
        best = None
        if ranked:
            o = ranked[0]
            best = dict(score=o["score"], path=o["path"], artefacts=o["artefacts"],
                        function=(Path(o["path"]) / "function.c").read_text())
        result = {k: st[k] for k in ("profile", "func", "status", "message", "base_score", "best_score",
                                     "iterations", "elapsed", "flags", "jobs")}
        result["best"] = best
        result["max_candidates"] = self.cfg.get("max_candidates", 0)
        result["candidates_including_base"] = max(st["attempted_candidates"],
                                                 st["iterations"] + st["base_candidates"])
        result["completed_candidates_including_base"] = st["iterations"] + st["completed_base_candidates"]
        result["keep_work"] = self.cfg.get("keep_work", False)
        result["structural_passes"] = list(self.cfg.get("structural_passes", []))
        result["applicable_passes"] = st["applicable_passes"]
        result["stop_reasons"] = st["stop_reasons"]
        result["outputs"] = [{k: o[k] for k in ("n", "score", "path", "artefacts")} for o in st["outputs"]]
        (self.out_dir / "result.json").write_text(json.dumps(result, indent=1) + "\n")

    def snapshot(self, since: int = 0) -> dict:
        with self.lock:
            st = dict(self.state)
            st["events"] = [e for e in self.state["events"] if e["seq"] > since]
            st["outputs"] = sorted(self.state["outputs"], key=lambda o: (o["score"], o["n"]))[:40]
            return st
