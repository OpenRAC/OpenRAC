# SPDX-License-Identifier: GPL-3.0-or-later
"""Actionable bake failures and the UTF-8 transcript shared by CLI and launcher."""
from __future__ import annotations

import contextlib
from datetime import datetime, timezone
import errno
import json
from pathlib import Path
import platform
import subprocess
import sys
import threading
import traceback

PREFIX = 'openrac-bake-error: '
CATALOG = {
    'busy': ('Lighting preparation is already running', 'Another lighting task is using this cache.',
             'Wait for that task to finish or stop it in Activity, then retry.'),
    'input': ('Lighting data is missing or out of date', 'The exported level data cannot be read by this baker.',
              'Read the levels from your disc again, then retry Prepare enhanced lighting.'),
    'settings': ('Lighting settings could not be read', 'A lighting configuration or data file is invalid.',
                 'Restore the affected lighting configuration or re-export the levels, then retry.'),
    'tool': ('A lighting tool is missing', 'The enhanced baker was not found.',
             'Update or reinstall the launcher. For a source build, rebuild openrac-enhanced-baker.'),
    'dependency': ('The lighting tool could not start', 'A library required by the lighting tool is missing.',
                   'Reinstall the launcher for your operating system and check whether antivirus quarantined a file.'),
    'cpu': ('The lighting tool cannot run on this processor', 'The operating system reported an unsupported CPU instruction.',
            'Send this log with your processor model and launcher version so we can check the packaged build.'),
    'memory': ('Not enough memory to prepare lighting', 'The baker ran out of memory.',
               'Close the game and other large apps, then retry. For command-line baking, use --jobs 1.'),
    'space': ('Not enough disk space to save lighting', 'The drive containing the lighting cache is full.',
              'Free space on that drive, then retry. Keep already completed lighting bakes.'),
    'permission': ('Lighting files could not be accessed', 'A file could not be opened, written or replaced.',
                   'Close the game and any app using these files. Check the folder is writable and retry.'),
    'changed': ('The level changed during preparation', 'The level data changed while lighting was being calculated.',
                'Let level export finish and close tools editing the level, then retry.'),
    'validation': ('The lighting result failed verification', 'The generated lighting did not pass its consistency checks.',
                   'Retry preparation. If it fails again, send this log; do not use the unverified bake.'),
    'regions': ('The level needs too many lighting regions', 'The region plan exceeds the supported limit.',
                'Restore the standard region settings or send this log so the level plan can be corrected.'),
    'protocol': ('The lighting tool returned incomplete data', 'The tool stopped without returning the expected result.',
                 'Update or reinstall the launcher so its tools match, then retry and send the log if it repeats.'),
    'crash': ('The lighting tool crashed', 'The operating system stopped the native lighting tool unexpectedly.',
              'Retry with other large apps closed. If it repeats, send this log and your system details.'),
    'native': ('The lighting tool could not finish', 'The native baker reported an error; the log contains its reason.',
               'Check the detailed error in the log. Retry after addressing it, or send the log for help.'),
    'publish': ('Prepared lighting could not be installed', 'The verified bake could not replace the cached lighting.',
                'Close the game, check free space and folder permissions, then retry. Existing verified lighting is retained when possible.'),
    'unexpected': ('Lighting preparation stopped unexpectedly', 'An unexpected error interrupted preparation.',
                   'Retry once. If it happens again, send this log with your launcher version.'),
}


def configure_streams():
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, 'reconfigure'):
            stream.reconfigure(encoding='utf-8', errors='backslashreplace')


class NativeFailure(subprocess.CalledProcessError):
    def __init__(self, result, command, output='', log=None, *, protocol=False):
        super().__init__(result, command, output=output)
        self.log_path = log
        self.bake_code = 'protocol' if protocol else None

    def __str__(self):
        detail = self.output.strip() if self.output else 'No diagnostic output was returned.'
        return f'{super().__str__()}\nNative output:\n{detail}'


def capture(command):
    """Retain stderr for short queries such as --print-profile, including start-up failures."""
    try:
        return subprocess.check_output(command, text=True, stderr=subprocess.STDOUT,
                                       encoding='utf-8', errors='replace')
    except subprocess.CalledProcessError as error:
        raise NativeFailure(error.returncode, command, error.output or '') from error


def read_json(path):
    try:
        return json.loads(path.read_text(encoding='utf-8'))
    except (ValueError, UnicodeError) as error:
        raise ValueError(f'{path}: {error}') from error


def parse_profile(text):
    try:
        profile = json.loads(text)
        if not isinstance(profile, dict) or any(
                not isinstance(profile.get(key), int) or isinstance(profile[key], bool) or profile[key] < 1
                for key in ('probe_format_version', 'bake_version')):
            raise ValueError('profile requires positive probe_format_version and bake_version')
        return profile
    except (ValueError, TypeError) as error:
        raise NativeFailure(1, ['--print-profile'], f'Invalid native profile: {error}\n{text[-12000:]}', protocol=True) from error


def stage(name, operation, *args, **kwargs):
    print(f'Bake stage: {name}', flush=True)
    try:
        return operation(*args, **kwargs)
    except Exception as error:
        if not getattr(error, 'bake_stage', None):
            error.bake_stage = name
        raise


def describe(error, *, level=None, log=None):
    detail = str(error)
    text = detail.lower()
    code = getattr(error, 'bake_code', None)
    status = (getattr(error, 'returncode', 0) or 0) & 0xffffffff
    if isinstance(error, MemoryError) or status == 0xc0000017 or 'bad_alloc' in text or 'out of memory' in text:
        code = 'memory'
    elif (isinstance(error, OSError) and error.errno == errno.ENOSPC) or 'no space left on device' in text or 'disk full' in text:
        code = 'space'
    elif code is None and (isinstance(error, PermissionError) or 'permission denied' in text or 'access is denied' in text):
        code = 'permission'
    elif status == 0xc0000135 or 'error while loading shared libraries' in text or 'library not loaded' in text:
        code = 'dependency'
    elif status == 0xc000001d or status == ((-4) & 0xffffffff):
        code = 'cpu'
    elif status == 0xc0000005 or (getattr(error, 'returncode', 0) or 0) < 0:
        code = 'crash'
    elif code is None:
        if 'inference tool not found' in text or 'baker not found' in text:
            code = 'tool'
        elif 'changed' in text and ('source' in text or 'level' in text or 'scene' in text):
            code = 'changed'
        elif ('manifest format' in text or 'no exported level directories' in text
              or 'missing gameplay bake input' in text
              or ('manifest' in text and ('missing' in text or 'not found' in text))
              or getattr(error, 'bake_stage', None) == 'Read level manifest'):
            code = 'input'
        elif 'environment placement incomplete' in text:
            # Its output names region files and a safety limit, which is not a region plan.
            code = 'native'
        elif 'region' in text and ('exceed' in text or 'limit' in text):
            code = 'regions'
        elif 'verif' in text or 'checksum' in text:
            code = 'validation'
        elif isinstance(error, FileNotFoundError):
            code = 'tool' if getattr(error, 'filename', '') and 'openrac-' in str(error.filename) else 'input'
        elif isinstance(error, (ValueError, KeyError, TypeError)):
            code = 'settings'
        elif isinstance(error, subprocess.CalledProcessError):
            code = 'native'
        else:
            code = 'unexpected'
    title, explanation, suggestion = CATALOG.get(code, CATALOG['unexpected'])
    return dict(version=1, code=code, title=title, explanation=explanation, suggestion=suggestion,
                detail=detail[-12000:], stage=getattr(error, 'bake_stage', None), level=level,
                log=str(log.resolve()) if log else None)


def emit(error, *, level=None, log=None):
    diagnostic = describe(error, level=level, log=log)
    print(f"Lighting error [{diagnostic['code']}] {level or ''}: {diagnostic['title']}", file=sys.stderr, flush=True)
    print(f"{diagnostic['detail']}\nSuggested fix: {diagnostic['suggestion']}", file=sys.stderr, flush=True)
    if getattr(error, 'log_path', None):
        print(f'Native log: {error.log_path}', file=sys.stderr, flush=True)
    print(PREFIX + json.dumps(diagnostic, ensure_ascii=True), file=sys.stderr, flush=True)
    return diagnostic


@contextlib.contextmanager
def cache_lock(directory):
    # OS locks release on process exit, including crashes; never delete another process's lock.
    with (directory/'probe-bake.lock').open('a+b') as handle:
        handle.seek(0, 2)
        if handle.tell() == 0:
            handle.write(b'0')
            handle.flush()
        handle.seek(0)
        try:
            if sys.platform == 'win32':
                import msvcrt
                msvcrt.locking(handle.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError as error:
            error.bake_code = 'busy'
            raise
        try:
            yield
        finally:
            handle.seek(0)
            if sys.platform == 'win32':
                msvcrt.locking(handle.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(handle, fcntl.LOCK_UN)


class Tee:
    def __init__(self, stream, log, lock, state):
        self.stream, self.log, self.lock, self.state = stream, log, lock, state

    def write(self, text):
        with self.lock:
            if self.state['error'] is None:
                try:
                    self.log.write(text)
                    self.log.flush()
                except OSError as error:
                    self.state['error'] = error
            return self.stream.write(text)

    def flush(self):
        with self.lock:
            if self.state['error'] is None:
                try:
                    self.log.flush()
                except OSError as error:
                    self.state['error'] = error
            self.stream.flush()


@contextlib.contextmanager
def transcript(directory):
    directory.mkdir(parents=True, exist_ok=True)
    path = directory/'probe-bake.log'
    with cache_lock(directory), path.open('a', encoding='utf-8', errors='replace') as log:
        lock = threading.RLock()
        state = {'error': None}
        with contextlib.redirect_stdout(Tee(sys.stdout, log, lock, state)), contextlib.redirect_stderr(Tee(sys.stderr, log, lock, state)):
            print(f'\nProbe bake started {datetime.now(timezone.utc).isoformat()} | {platform.platform()} | Python {platform.python_version()}', flush=True)
            print(f'Bake log: {path.resolve()}', flush=True)
            yield path
        if state['error'] is not None:
            raise state['error']


def failed(error, *, level=None, log=None):
    # Unexpected Python exceptions retain the traceback, not just the last exception's message.
    traceback.print_exception(type(error), error, error.__traceback__)
    return emit(error, level=level, log=log)
