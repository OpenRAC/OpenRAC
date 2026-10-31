# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Python preparation callbacks for one native, memory-admitted multi-level bake session."""
from concurrent.futures import ThreadPoolExecutor
import json
import queue
import subprocess
import threading

import diagnostics


class _Channel:
    """One level's diagnostics and exit status, published only by the session reader."""
    def __init__(self):
        self.lines = queue.Queue()
        self.exited = threading.Event()
        self.returncode = None

    def finish(self, code):
        if not self.exited.is_set():
            self.returncode = code
            self.lines.put(None)
            self.exited.set()


class _Output:
    def __init__(self, channel):
        self.channel = channel
        self.closed = False

    def __iter__(self):
        return self

    def __next__(self):
        if self.closed:
            raise StopIteration
        line = self.channel.lines.get()
        if line is None:
            self.closed = True
            raise StopIteration
        return line

    def close(self):
        self.closed = True


class _Plan:
    def __init__(self, session, index):
        self.session, self.index = session, index
        self.closed = False

    def write(self, text):
        if self.closed:
            raise ValueError('write to closed region-plan pipe')
        kind = 'coverage' if json.loads(text).get('type') in ('coverage', 'region_plan') else 'plan'
        self.session.send({'type': kind, 'id': self.index, 'text': text})
        return len(text)

    def flush(self):
        pass  # send() already flushes the shared process pipe.

    def close(self):
        self.closed = True


class _Process:
    """The NativeBatch pipe/wait interface for a job sharing the session's native process."""
    def __init__(self, session, index, arguments):
        self.session, self.index = session, index
        self.channel = session.channels[index]
        self.stdin, self.stdout = _Plan(session, index), _Output(self.channel)
        session.send({'type': 'command', 'id': index, 'arguments': list(map(str, arguments))})

    @property
    def returncode(self):
        return self.channel.returncode

    def poll(self):
        return self.returncode

    def wait(self, timeout=None):
        if not self.channel.exited.wait(timeout):
            raise subprocess.TimeoutExpired('native level pipeline', timeout)
        return self.returncode

    def terminate(self):
        self.session.complete(self.index)

    kill = terminate


class NativeCoordinator:
    """Own one native scheduler; Python threads perform only admitted preparation/publication."""
    def __init__(self, baker, output, *, threads, jobs, memory):
        self.command = [str(baker), '--coordinate-levels']
        self.log_path = output/'coordinator.log'
        self.settings = dict(version=1, threads=threads, jobs=jobs, memory=memory)
        self.channels = []
        self.events = queue.Queue()
        self.lock = threading.Lock()
        self.closed_jobs = set()
        self.process = self.reader = self.log = None
        self.failure = None

    def __enter__(self):
        self.log = self.log_path.open('w', encoding='utf-8', errors='replace')
        try:
            self.process = subprocess.Popen(self.command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                            stderr=self.log, text=True, bufsize=1, encoding='utf-8',
                                            errors='replace')
        except BaseException:
            self.log.close()
            raise
        return self

    def send(self, message):
        with self.lock:
            self.process.stdin.write(json.dumps(message, allow_nan=False)+'\n')
            self.process.stdin.flush()

    def complete(self, index):
        with self.lock:
            if index in self.closed_jobs:
                return
            self.closed_jobs.add(index)
            self.process.stdin.write(json.dumps({'type': 'complete', 'id': index})+'\n')
            self.process.stdin.flush()

    def process_factory(self, index):
        """Return a NativeBatch-compatible factory bound to the admitted client ID."""
        return lambda command: _Process(self, index, command[1:])

    def _read(self):
        try:
            for line in self.process.stdout:
                event = json.loads(line)
                kind = event.get('event')
                if kind in ('line', 'exit', 'released'):
                    index = event.get('id')
                    if type(index) is not int or not 0 <= index < len(self.channels):
                        raise ValueError('native coordinator returned an unknown level ID')
                    channel = self.channels[index]
                    if kind == 'line':
                        channel.lines.put(event['text'])
                    elif kind == 'exit':
                        channel.finish(event['status'])
                    else:
                        channel.finish(1)  # A cancelled/preparation-only job has no native exit event.
                elif kind == 'fatal':
                    raise ValueError(event.get('text', 'native coordinator failed'))
                elif kind in ('admitted', 'done'):
                    self.events.put(event)
                else:
                    raise ValueError('unrecognised native coordinator event')
        except Exception as error:
            self.failure = error
        finally:
            for channel in self.channels:
                channel.finish(1)
            self.events.put({'event': 'eof'})

    def run(self, levels, callback):
        """Submit all metadata; only native admission events start Python per-level callbacks."""
        self.channels = [_Channel() for _ in levels]
        self.reader = threading.Thread(target=self._read, name='Bake coordinator reader', daemon=True)
        self.reader.start()
        self.send({**self.settings, 'levels': levels})

        def admitted(index):
            try:
                callback(index)
            finally:
                self.complete(index)

        futures = []
        with ThreadPoolExecutor(max_workers=self.settings['jobs']) as callbacks:
            while True:
                event = self.events.get()
                if event['event'] == 'admitted':
                    futures.append(callbacks.submit(admitted, event['id']))
                elif event['event'] == 'done':
                    break
                else:
                    raise diagnostics.NativeFailure(self.process.poll() or 1, self.command,
                        str(self.failure or 'native coordinator closed before completing levels'), self.log_path)
        for future in futures:
            future.result()

    def __exit__(self, error_type, *_):
        if self.process is not None:
            with self.lock:
                self.process.stdin.close()
            try:
                self.process.wait(timeout=30)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
            self.process.stdout.close()
        if self.reader is not None:
            self.reader.join(timeout=5)
        if self.log is not None:
            self.log.close()
        if error_type is None and self.process is not None and self.process.returncode:
            raise diagnostics.NativeFailure(self.process.returncode, self.command,
                                            'Native coordinator failed; see coordinator.log', self.log_path)
