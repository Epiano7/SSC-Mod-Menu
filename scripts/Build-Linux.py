"""Package the shared validated Windows payload with the native Linux installer."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import re
import tarfile


def build(runtime, output, version=None, python_archive=None, launcher=None):
    root = Path(__file__).resolve().parents[1]
    setup = (root / 'installer/Setup.cs').read_text()
    updater = (root / 'installer/Updater.cs').read_text()
    version = version or re.search(r'CurrentVersion="([^"]+)"', updater)[1]
    if not re.fullmatch(r'\d+\.\d+\.\d+', version):
        raise ValueError('Expected a three-part version')
    game_hash = re.search(r'GameHash="([A-F0-9]{64})"', setup)[1].lower()
    game_size = int(re.search(r'GameSize=(\d+)', setup)[1])
    approved = dict(re.findall(r"'(opengl32.dll|runtime.dll)'='([A-F0-9]{64})'", (root / 'scripts/Build-Installer.ps1').read_text()))
    files = {}
    for name in ('opengl32.dll', 'runtime.dll'):
        content = (Path(runtime) / name).read_bytes()
        if hashlib.sha256(content).hexdigest().upper() != approved[name]:
            raise ValueError(f'Unapproved release payload: {name}')
        files[name if name == 'opengl32.dll' else 'SSCMods/' + name] = content
    files['SSCMods/AUDIO-NOTICES.txt'] = (root / 'runtime/third_party/AUDIO-NOTICES.txt').read_bytes()
    metadata = dict(schema=1, product='SSCMods', version=version, experimental=True, native_desktop=True,
                    game_sha256=game_hash, game_size=game_size,
                    files={name: hashlib.sha256(content).hexdigest() for name, content in files.items()})
    entries = {'payload/' + name: content for name, content in files.items()}
    entries['package.json'] = (json.dumps(metadata, indent=2) + '\n').encode()
    for name in ('install.sh', 'ssc_installer.py', 'game_build.py', 'linux_desktop.py', 'linux_update.py', 'desktop_ui.py', 'README.md'):
        entries[name] = (root / 'linux' / name).read_bytes().replace(b'\r\n', b'\n')
    for notice in (root / 'linux/licenses').iterdir():
        if notice.is_file():
            entries['licenses/' + notice.name] = notice.read_bytes()
    if not python_archive or not launcher:
        raise ValueError('Native desktop packages require the bundled Linux Python archive and native launcher')
    pinned_hash = 'c20e1ff8600a0241849588b36948942eaccdc80da34df69674f3784a687197de'
    if hashlib.sha256(Path(python_archive).read_bytes()).hexdigest() != pinned_hash:
        raise ValueError('Bundled Python does not match the reviewed 20260924 CPython 3.13.15 release')
    entries['SSC-Mod-Menu-Setup'] = Path(launcher).read_bytes()
    if entries['SSC-Mod-Menu-Setup'][:6] != b'\x7fELF\x02\x01':
        raise ValueError('Expected a native Linux x86-64 launcher')
    entries['BUNDLED-RUNTIME.txt'] = ('CPython 3.13.15 / python-build-standalone 20260924\n'
        'https://github.com/astral-sh/python-build-standalone/releases/tag/20260924\n'
        'Original archive SHA256: ' + pinned_hash + '\nLicense: python/lib/python3.13/LICENSE.txt\n').encode()
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tarfile.open(output, 'w:gz') as archive:
        for name, content in sorted(entries.items()):
            info = tarfile.TarInfo('SSC-Mod-Menu-Linux/' + name)
            info.size = len(content)
            info.mode = 0o755 if name in ('install.sh', 'ssc_installer.py', 'SSC-Mod-Menu-Setup') else 0o644
            info.uid = info.gid = 0
            info.uname = info.gname = ''
            info.mtime = 0
            archive.addfile(info, io.BytesIO(content))
        with tarfile.open(python_archive) as python_tar:
            for item in python_tar:
                if not item.name.startswith('python/') or '..' in Path(item.name).parts or item.isdev() or item.isfifo():
                    raise ValueError('Unsafe bundled runtime entry')
                data = python_tar.extractfile(item) if item.isfile() else None
                item.name = 'SSC-Mod-Menu-Linux/' + item.name
                item.uid = item.gid = 0
                item.uname = item.gname = ''
                item.mtime = 0
                archive.addfile(item, data)
    print(f'{output}: {hashlib.sha256(output.read_bytes()).hexdigest()}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--version', help='Preview version override; normally use the shared Windows version')
    parser.add_argument('--python-archive', required=True)
    parser.add_argument('--launcher', required=True)
    args = parser.parse_args()
    build(args.runtime, args.output, args.version, args.python_archive, args.launcher)
