"""Per-launch Discord IPC bridge for Proton. Local sockets only; no tokens or network."""
import json
import os
from pathlib import Path
import re
import socket
import stat
import struct
import time


def candidates():
    roots = [os.environ.get('XDG_RUNTIME_DIR'), os.environ.get('TMPDIR'), os.environ.get('TMP'), os.environ.get('TEMP'), '/tmp']
    for root in dict.fromkeys(r for r in roots if r):
        for base in (Path(root), Path(root) / 'app/com.discordapp.Discord'):
            for number in range(10):
                yield base / f'discord-ipc-{number}'


def validated(data):
    if not isinstance(data, dict) or type(data.get('enabled')) is not bool:
        raise ValueError('Invalid presence request')
    if not data['enabled']:
        return None
    app = data.get('client_id', '')
    if not isinstance(app, str) or not re.fullmatch(r'[1-9][0-9]{16,19}', app) or int(app) > 2**64 - 1:
        raise ValueError('Invalid application ID')
    raw = data.get('activity')
    if not isinstance(raw, dict):
        raise ValueError('Invalid activity')
    activity = {'instance': False, 'assets': {'large_image': 'ssc_mods', 'large_text': 'Skillshot City - SSC Mod Menu'}}
    for key in ('details', 'state'):
        value = raw.get(key)
        if not isinstance(value, str) or len(value.encode('utf-8')) > 128 or any(ord(c) < 32 for c in value):
            raise ValueError('Invalid activity text')
        activity[key] = value
    if 'timestamps' in raw:
        start = raw['timestamps'].get('start') if isinstance(raw['timestamps'], dict) else None
        if type(start) is not int or not 0 < start < 2**53:
            raise ValueError('Invalid activity time')
        activity['timestamps'] = {'start': start}
    return app, activity


class Bridge:
    def __init__(self, folder, log=lambda message: None, connector=None, clock=time.monotonic):
        self.folder = Path(folder)
        self.log, self.clock = log, clock
        self.connector = connector or self.connect_socket
        self.sock = None
        self.buffer = bytearray()
        self.ready = False
        self.app = None
        self.retry = self.deadline = self.last_send = 0
        self.last_activity = None
        self.pending = None
        self.nonce = 0
        self.status = None
        self.last_revision = None
        self.fresh_at = None

    def set_status(self, value):
        if self.status == value:
            return
        tmp = self.folder / 'status.tmp'
        try:
            tmp.write_text(value, encoding='utf-8')
            tmp.replace(self.folder / 'status.txt')
        except OSError:
            return  # Diagnostics must never stop the game launch coordinator.
        self.status = value
        try:
            self.log('Discord bridge: ' + value)
        except OSError:
            pass

    @staticmethod
    def connect_socket():
        for path in candidates():
            try:
                info = path.stat()
                if not stat.S_ISSOCK(info.st_mode) or info.st_uid != os.getuid():
                    continue
                s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                try:
                    s.settimeout(0.2)
                    s.connect(str(path))
                    return s
                except OSError:
                    s.close()
            except OSError:
                continue
        raise OSError('Discord socket unavailable')

    def send(self, opcode, body):
        raw = body if isinstance(body, bytes) else json.dumps(body, separators=(',', ':')).encode()
        if len(raw) > 65536:
            raise ValueError('Frame too large')
        self.sock.sendall(struct.pack('<II', opcode, len(raw)) + raw)

    def disconnect(self):
        if self.sock:
            self.sock.close()
        self.sock = None
        self.ready = False
        self.buffer.clear()
        self.pending = None
        self.last_activity = None
        self.last_send = self.deadline = 0
        self.retry = self.clock() + 5

    def clear(self):
        if self.sock and self.ready:
            try:
                self.send(1, {'cmd': 'SET_ACTIVITY', 'args': {'pid': os.getpid(), 'activity': None}, 'nonce': 'clear'})
            except (OSError, ValueError):
                pass
        self.disconnect()

    def close(self):
        self.clear()
        self.set_status('Stopped')

    def receive(self):
        try:
            chunk = self.sock.recv(65544 - len(self.buffer))
        except (BlockingIOError, socket.timeout):
            chunk = None
        if chunk == b'':
            raise OSError('Discord closed connection')
        if chunk:
            self.buffer.extend(chunk)
        for _ in range(32):
            if len(self.buffer) < 8:
                break
            op, length = struct.unpack_from('<II', self.buffer)
            if length > 65536 or op > 4:
                raise ValueError('Invalid Discord frame')
            if len(self.buffer) < 8 + length:
                break
            body = bytes(self.buffer[8:8 + length])
            del self.buffer[:8 + length]
            if op == 3:
                self.send(4, body)
                continue
            if op == 4:
                continue
            if op != 1:
                raise OSError('Discord closed connection')
            message = json.loads(body)
            if not isinstance(message, dict) or message.get('evt') == 'ERROR':
                raise ValueError('Discord rejected activity')
            if not self.ready and message.get('evt') == 'READY':
                self.ready = True
                self.deadline = 0
            if self.pending and message.get('cmd') == 'SET_ACTIVITY' and message.get('nonce') == self.pending:
                self.pending = None
                self.deadline = 0
                self.set_status('Activity accepted by Discord')

    def tick(self):
        now = self.clock()
        path = self.folder / 'activity.json'
        try:
            if path.is_symlink() or path.stat().st_size > 8192:
                raise ValueError('Invalid presence file')
            data = json.loads(path.read_text(encoding='utf-8'))
            request = validated(data)
            revision = data.get('revision')
            if type(revision) is not int or revision < 0:
                raise ValueError('Invalid revision')
            if revision != self.last_revision:
                self.last_revision, self.fresh_at = revision, now
            if self.fresh_at is None or now - self.fresh_at > 10:
                request = None
        except FileNotFoundError:
            request = None
        except (OSError, ValueError, RecursionError):
            self.clear()
            self.set_status('Invalid presence request')
            return
        if request is None:
            self.clear()
            self.set_status('Disabled or waiting for game status')
            return
        app, activity = request
        if app != self.app:
            self.clear()
            self.app = app
            self.retry = 0
        try:
            if not self.sock and now >= self.retry:
                self.sock = self.connector()
                self.sock.settimeout(0.05)
                self.send(0, {'v': 1, 'client_id': app})
                self.deadline = now + 5
                self.set_status('Connecting to Discord')
            if self.sock:
                self.receive()
                if self.deadline and now > self.deadline:
                    raise OSError('Discord response timeout')
                if self.ready and not self.pending and activity != self.last_activity and (self.last_activity is None or now - self.last_send >= 15):
                    self.nonce += 1
                    self.pending = str(self.nonce)
                    self.send(1, {'cmd': 'SET_ACTIVITY', 'args': {'pid': os.getpid(), 'activity': activity}, 'nonce': self.pending})
                    self.deadline = now + 5
                    self.last_send, self.last_activity = now, activity
        except (OSError, ValueError, RecursionError):
            self.disconnect()
            self.set_status('Discord desktop unavailable or connection lost; retrying')
