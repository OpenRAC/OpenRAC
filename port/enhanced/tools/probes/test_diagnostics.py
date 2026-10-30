# SPDX-License-Identifier: GPL-3.0-or-later
import contextlib
import errno
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import bake_all
import bake_level
import diagnostics


class BakeDiagnostics(unittest.TestCase):
    def test_classifies_known_failures_without_claiming_unknown_crashes_are_memory(self):
        cases = [(MemoryError(), 'memory'), (OSError(errno.ENOSPC, 'full'), 'space'),
                 (PermissionError('locked'), 'permission'),
                 (ValueError('manifest format 3 (this baker reads 4)'), 'input'),
                 (ValueError('planned dense regions exceed runtime limit'), 'regions'),
                 (ValueError('enhanced baker not found: missing.exe'), 'tool'),
                 (diagnostics.NativeFailure(1, ['baker'], 'verification failed'), 'validation'),
                 (diagnostics.NativeFailure(1, ['baker'], 'Verified region-402.orpg\nprobe bake: environment '
                                            'placement incomplete: 3 uncovered samples; 177 captures, '
                                            'safety limit 512'), 'native'),
                 (diagnostics.NativeFailure(0xc000001d, ['baker']), 'cpu'),
                 (diagnostics.NativeFailure(0xc0000135, ['baker']), 'dependency'),
                 (diagnostics.NativeFailure(0xc0000005, ['baker']), 'crash'),
                 (RuntimeError('unexpected condition'), 'unexpected')]
        for error, code in cases:
            with self.subTest(code=code):
                result = diagnostics.describe(error)
                self.assertEqual(result['code'], code)
                self.assertTrue(result['explanation'] and result['suggestion'])

    def test_real_native_failure_retains_stderr_and_exit_status(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = Path(tmp)/'batch.log'
            command = [sys.executable, '-c', 'import sys; print("failed scene verification", file=sys.stderr); sys.exit(7)']
            with self.assertRaises(diagnostics.NativeFailure) as caught:
                with bake_level.NativeBatch(command, log):
                    self.fail('must not become ready')
            self.assertEqual(caught.exception.returncode, 7)
            self.assertIn('failed scene verification', str(caught.exception))
            self.assertIn('failed scene verification', log.read_text())

    def test_native_success_without_ready_is_a_protocol_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(diagnostics.NativeFailure) as caught:
                with bake_level.NativeBatch([sys.executable, '-c', 'print("incomplete")'], Path(tmp)/'batch.log'):
                    self.fail('must not become ready')
            self.assertEqual(diagnostics.describe(caught.exception)['code'], 'protocol')

    def test_missing_completion_marker_and_malformed_inspection_are_explicit(self):
        with tempfile.TemporaryDirectory() as tmp:
            command = [sys.executable, '-c', 'import sys; print("Probe batch ready", flush=True); sys.stdin.readline()']
            with bake_level.NativeBatch(command, Path(tmp)/'batch.log') as native:
                with self.assertRaises(diagnostics.NativeFailure) as caught:
                    native.finish({'version': 1})
                self.assertEqual(diagnostics.describe(caught.exception)['code'], 'protocol')
        with self.assertRaises(diagnostics.NativeFailure):
            bake_level.inspection('scene_hash 42')

    def test_bad_native_profile_retains_the_response(self):
        for text in ('not JSON', '[]', '{"probe_format_version":4}'):
            with self.subTest(text=text), self.assertRaises(diagnostics.NativeFailure) as caught:
                diagnostics.parse_profile(text)
            self.assertIn(text, str(caught.exception))
            self.assertEqual(diagnostics.describe(caught.exception)['code'], 'protocol')

    def test_failed_metadata_write_preserves_the_previous_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            file = Path(tmp)/'bake-result.json'
            file.write_text('{"verified":true}')
            with self.assertRaises(ValueError):
                bake_level.write_json(file, {'bad': float('nan')}, replace=True)
            self.assertEqual(json.loads(file.read_text()), {'verified': True})
            self.assertEqual(list(Path(tmp).glob('*.part')), [])

    def test_transcript_keeps_traceback_and_structured_record_with_suggestion(self):
        with tempfile.TemporaryDirectory() as tmp, contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            root = Path(tmp)
            with diagnostics.transcript(root) as log:
                try:
                    diagnostics.stage('Read scene', lambda: (_ for _ in ()).throw(ValueError('manifest format 3')))
                except ValueError as error:
                    diagnostics.failed(error, level='level_01', log=log)
            text = log.read_text(encoding='utf-8')
            self.assertIn('Traceback', text)
            self.assertIn('Suggested fix:', text)
            record = next(line for line in text.splitlines() if line.startswith(diagnostics.PREFIX))
            data = json.loads(record[len(diagnostics.PREFIX):])
            self.assertEqual(data['stage'], 'Read scene')
            self.assertEqual(data['log'], str(log.resolve()))

    def test_same_cache_cannot_be_baked_twice_and_lock_releases(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            with diagnostics.cache_lock(root):
                with self.assertRaises(OSError) as caught:
                    with diagnostics.cache_lock(root):
                        self.fail('second writer acquired lock')
                self.assertEqual(diagnostics.describe(caught.exception)['code'], 'busy')
            with diagnostics.cache_lock(root):
                pass

    def test_log_failure_does_not_hide_console_error(self):
        class Full(io.StringIO):
            def write(self, text):
                raise OSError(errno.ENOSPC, 'disk full')
        stream = io.StringIO()
        state = {'error': None}
        import threading
        tee = diagnostics.Tee(stream, Full(), threading.RLock(), state)
        tee.write('first message')
        tee.write('still reportable')
        self.assertIsInstance(state['error'], OSError)
        self.assertIn('still reportable', stream.getvalue())

    def test_publish_failure_restores_the_previous_verified_bake(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            final, work = root/'level_01', root/'.level_01.partial'
            final.mkdir(); work.mkdir()
            (final/'verified.txt').write_text('old verified bake')
            (work/'batch.log').write_text('new log')
            rename = bake_all.file_ops.rename
            def fail_install(source, destination):
                if source == work:
                    raise PermissionError('replacement blocked')
                return rename(source, destination)
            with patch.object(bake_all.file_ops, 'rename', side_effect=fail_install):
                with self.assertRaises(PermissionError):
                    bake_all.publish(work, final)
            self.assertEqual((final/'verified.txt').read_text(), 'old verified bake')
            self.assertTrue((work/'batch.log').exists())

    def test_corrupt_or_unverified_cache_is_not_kept(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            stamp = {'version': 3}
            (root/bake_all.STAMP).write_text(json.dumps(stamp))
            (root/'sparse.orpg').write_bytes(b'nonempty')
            for result in ([], {'verified': False}, {'verified': True, 'regions': ['missing.orpg']}):
                (root/'bake-result.json').write_text(json.dumps(result))
                self.assertFalse(bake_all.up_to_date(root, stamp))


if __name__ == '__main__':
    unittest.main()
