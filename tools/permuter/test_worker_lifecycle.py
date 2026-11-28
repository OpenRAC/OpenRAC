# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Search termination contracts, including a hung worker's compiler child."""
import os
from pathlib import Path
import random
import signal
import subprocess
import sys
import tempfile
import time
from types import SimpleNamespace
import unittest
from unittest import mock

from permuter import engine, runner


def unresponsive_worker(cfg, wid, outq, stop, seed):
    """Deliberately ignore cooperative cancellation, like a blocked native tool."""
    os.setsid()
    outq.put(('group', wid, os.getpid()))
    child = subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(120)'])
    Path(cfg['child_pid_file']).write_text(str(child.pid))
    outq.put(('base', wid, 5, 'int f(void) { return 1; }'))
    while True:
        time.sleep(.05)


class ConstructorStopTests(unittest.TestCase):
    def test_exact_constructor_is_exported_as_a_queue_win(self):
        cfg = {'profile': 'synthetic', 'func': 'f', 'flags': '', 'weights': {}}
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            job = engine.Job(cfg, folder / 'result', jobs=1)
            job.out_dir.mkdir()
            messages = iter([('base_start', 0),
                             ('base', 0, 0, 'int f(void) { return 7; }'),
                             ('done', 0)])
            job._q = SimpleNamespace(get=lambda timeout: next(messages))
            job._collect()
            path = job.out_dir / 'result.json'
            result = __import__('json').loads(path.read_text())
            self.assertEqual(result['iterations'], 0)
            self.assertEqual(result['candidates_including_base'], 1)
            self.assertEqual(result['completed_candidates_including_base'], 1)
            self.assertEqual(runner.record(folder, cfg, path, 1), 'WIN')
            self.assertIn('return 7;', (folder / 'wins/synthetic/f.c').read_text())

    def test_invalid_limits_are_rejected_before_workers_exist(self):
        for seconds in (-1, float('nan'), float('inf')):
            with self.subTest(seconds=seconds), self.assertRaises(ValueError):
                engine.Job({}, Path('unused'), seconds=seconds)
        for maximum in (-1, True, 2.5, None):
            with self.subTest(maximum=maximum), self.assertRaises(ValueError):
                engine.Job({'max_candidates': maximum}, Path('unused'))

    def test_started_constructor_is_counted_when_interrupted(self):
        cfg = {'profile': 'synthetic', 'func': 'f', 'flags': '', 'weights': {}}
        with tempfile.TemporaryDirectory() as directory:
            job = engine.Job(cfg, Path(directory), jobs=1)
            messages = iter([('base_start', 0), ('fatal', 0, 'compile interrupted'), ('done', 0)])
            job._q = SimpleNamespace(get=lambda timeout: next(messages))
            job._collect()
            result = __import__('json').loads((Path(directory)/'result.json').read_text())
            self.assertEqual(result['candidates_including_base'], 1)
            self.assertEqual(result['completed_candidates_including_base'], 0)
            self.assertEqual(result['status'], 'error')

    def test_exact_or_constructor_only_does_not_request_a_seed(self):
        for value, maximum, wid in ((0, 3, 0), (5, 1, 0), (0, 3, 1)):
            with self.subTest(value=value, worker=wid), tempfile.TemporaryDirectory() as directory:
                compiled, messages = [], []

                class Compiler:
                    last_error = ''

                    def __init__(self, *args):
                        pass

                    def compile(self, source):
                        compiled.append(source)
                        return 'object.o'

                class Randomizer:
                    def randomize(self, *args):
                        raise AssertionError('no transformation should be attempted')

                class Permuter:
                    def __init__(self, *args, **kwargs):
                        args[2].compile('base')
                        self.base_score, self.base_source = value, args[5]

                    def seed_iterator(self):
                        raise AssertionError('the constructor is already the complete search')

                modules = SimpleNamespace(
                    helpers=SimpleNamespace(get_default_randomization_weights=lambda _: {}),
                    randomizer=SimpleNamespace(Randomizer=Randomizer),
                    permuter=SimpleNamespace(Permuter=Permuter))
                cfg = {'permuter': 'unused', 'func': 'f', 'base': 'int f(void){return 1;}',
                       'weights': {}, 'max_candidates': maximum, 'keep_work': True}
                original = Randomizer.randomize
                with mock.patch.object(engine.upstream, 'load', return_value=modules), \
                        mock.patch.object(engine.upstream, 'cache_dir', return_value=Path(directory)), \
                        mock.patch.object(engine, 'Candidate', Compiler):
                    engine._worker(cfg, wid, SimpleNamespace(put=messages.append),
                                   SimpleNamespace(is_set=lambda: False), 1)
                self.assertEqual(compiled, ['base'])
                self.assertEqual(messages[-1], ('done', wid))
                if value == 0:
                    self.assertTrue(next(m[3] for m in messages if m[0] == 'base'))
                self.assertFalse(any(m[0] == 'fatal' for m in messages), messages)
                self.assertIs(Randomizer.randomize, original)

    def test_unavailable_structural_pass_finishes_with_only_a_base(self):
        self.run_retry_case(structural=True)

    def test_cooperative_stop_escapes_upstream_retry_loop(self):
        self.run_retry_case(structural=False)

    def run_retry_case(self, structural):
        compiled, messages, checks = [], [], [0]

        class Failure(Exception):
            pass

        class Compiler:
            last_error = ''

            def __init__(self, *args):
                pass

            def compile(self, text):
                compiled.append(text)
                return 'object.o'

        def unavailable(*args):
            raise Failure()

        class Randomizer:
            def __init__(self):
                self.methods = [(unavailable, 1)]
                self.random = random.Random(1)

            def randomize(self, *args):
                # Bound the test double, not the contract under test. The old
                # wrapper gets here 100 times and fails instead of hanging CI.
                for _ in range(100):
                    try:
                        self.methods[0][0]()
                    except Failure:
                        pass
                raise AssertionError('cooperative stop was never observed')

        class Permuter:
            def __init__(self, *args, **kwargs):
                args[2].compile('base')
                self.base_score, self.base_source = 5, args[5]
                self.randomizer = Randomizer()

            def seed_iterator(self):
                return iter([0])

            def try_eval_candidate(self, seed):
                self.randomizer.randomize(object(), 'f')
                raise AssertionError('no applicable plan should compile')

        def stopped():
            checks[0] += 1
            return not structural and checks[0] > 20

        modules = SimpleNamespace(
            helpers=SimpleNamespace(get_default_randomization_weights=lambda _: {}),
            randomizer=SimpleNamespace(Randomizer=Randomizer, ca=object(),
                ast_util=SimpleNamespace(extract_fn=lambda *a: (object(),),
                                         compute_node_indices=lambda *a: None),
                get_randomization_region=lambda *a: None),
            permuter=SimpleNamespace(Permuter=Permuter))
        with tempfile.TemporaryDirectory() as directory:
            cfg = {'permuter': 'unused', 'func': 'f', 'base': 'int f(void){return 1;}',
                   'weights': {}, 'max_candidates': 3, 'keep_work': True,
                   'structural_passes': ['reverse_cond'] if structural else []}
            with mock.patch.object(engine.upstream, 'load', return_value=modules), \
                    mock.patch.object(engine.upstream, 'cache_dir', return_value=Path(directory)), \
                    mock.patch.object(engine.structural, 'plans', return_value=[]), \
                    mock.patch.object(engine, 'Candidate', Compiler):
                engine._worker(cfg, 0, SimpleNamespace(put=messages.append),
                               SimpleNamespace(is_set=stopped), 1)
        self.assertEqual(compiled, ['base'])
        self.assertEqual(messages[-1], ('done', 0))
        self.assertTrue(any(m[0] == 'stopped' for m in messages), messages)
        self.assertFalse(any(m[0] == 'fatal' for m in messages), messages)
        if structural:
            self.assertTrue(any(m[0] == 'applicable' for m in messages), messages)


@unittest.skipUnless(sys.platform.startswith('linux'), 'POSIX process-group ownership test')
class HardStopTests(unittest.TestCase):
    def run_stop_case(self, seconds):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            child_pid_file = folder / 'compiler.pid'
            cfg = {'profile': 'synthetic', 'func': 'f', 'flags': '', 'weights': {},
                   'child_pid_file': str(child_pid_file)}
            job = engine.Job(cfg, folder / 'result', jobs=1, seconds=seconds)
            start = time.monotonic()
            with mock.patch.object(engine, '_worker', unresponsive_worker):
                job.start()
            try:
                deadline = time.monotonic() + 5
                while not child_pid_file.exists() and time.monotonic() < deadline:
                    time.sleep(.02)
                self.assertTrue(child_pid_file.exists(), 'test worker did not start')
                if not seconds:
                    job.stop('manual stop')
                job.thread.join(8)
                self.assertFalse(job.thread.is_alive(), 'collector failed to finish')
                self.assertLess(time.monotonic() - start, 9)
                self.assertTrue(all(not p.is_alive() for p in job._procs))
                child = int(child_pid_file.read_text())
                stat = Path(f'/proc/{child}/stat')
                if stat.exists():
                    # An exited child may briefly await the host init's reaper.
                    self.assertEqual(stat.read_text().split()[2], 'Z')
                self.assertTrue((folder / 'result/result.json').is_file())
                self.assertEqual(job.snapshot()['status'], 'time limit' if seconds else 'manual stop')
            finally:
                job.stop()
                for process in job._procs:
                    if process.pid in job._groups:
                        try:
                            os.killpg(process.pid, signal.SIGKILL)
                        except ProcessLookupError:
                            pass
                    elif process.is_alive():
                        process.kill()
                    process.join(2)
                job.thread.join(3)

    def test_deadline_reaps_hung_worker_and_compiler(self):
        self.run_stop_case(0.4)

    def test_manual_stop_reaps_hung_worker_and_compiler(self):
        self.run_stop_case(0)


if __name__ == '__main__':
    unittest.main()
