"""Read-only game platform detection; does not treat ELF as a Windows payload."""
from pathlib import Path
import struct


def executable_kind(path):
    with Path(path).open('rb') as stream:
        header = stream.read(64)
        if header[:4] == b'\x7fELF':
            if len(header) < 20 or header[4:6] != b'\x02\x01':
                return 'unsupported-elf'
            machine = struct.unpack_from('<H', header, 18)[0]
            return 'linux-x86_64' if machine == 62 else 'unsupported-elf'
        if len(header) == 64 and header[:2] == b'MZ':
            offset = struct.unpack_from('<I', header, 60)[0]
            if not 64 <= offset <= 1048576:
                return 'unknown'
            stream.seek(offset)
            pe = stream.read(6)
            return 'windows-x86_64' if pe == b'PE\0\0\x64\x86' else 'unsupported-pe'
    return 'unknown'


def detect(folder):
    root = Path(folder).expanduser().resolve(strict=True)
    candidates = []
    if root.is_dir():
        for name in ('SkillshotCity.exe', 'SkillshotCity'):
            path = root / name
            if path.is_file() and not path.is_symlink():
                candidates.append({'filename': name, 'kind': executable_kind(path)})
    return candidates
