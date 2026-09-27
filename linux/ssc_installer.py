#!/usr/bin/env python3
"""Native Linux installer for the Windows mod used by Steam/Proton (stdlib only)."""
import argparse
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile

APP_ID = '308600'
MANIFEST = 'SSCMods/linux-manifest.json'
OWNED = ('opengl32.dll', 'SSCMods/runtime.dll', 'SSCMods/AUDIO-NOTICES.txt')
LAUNCH_OPTIONS = 'WINEDLLOVERRIDES="opengl32=n,b" %command%'


def digest(path):
    with open(path, 'rb') as stream:
        h = hashlib.sha256()
        for block in iter(lambda: stream.read(1048576), b''):
            h.update(block)
        return h.hexdigest()


def read_json(path):
    if path.stat().st_size > 65536:
        raise ValueError('Metadata is too large')
    return json.loads(path.read_text(encoding='utf-8'))


def destination(root, relative):
    if relative not in (*OWNED, MANIFEST):
        raise ValueError('Unrecognized installation destination')
    target = root / relative
    # Steam library ancestors may be symlinks. Nothing beneath the resolved game
    # directory may redirect writes, including dangling mod links.
    current = root
    for component in Path(relative).parts:
        current /= component
        if current.is_symlink():
            raise ValueError(f'Remove the linked mod destination first: {current}')
    if target.exists() and not target.is_file():
        raise ValueError(f'Expected a file: {target}')
    return target


def game_root(path):
    if not str(path).strip():
        raise ValueError('No game folder selected. Use Browse to choose the Skillshot City installation folder')
    root = Path(path).expanduser().resolve(strict=True)
    if not root.is_dir():
        raise ValueError(f'The selected path is a file, not a folder: {root}. Select the folder containing SkillshotCity.exe')
    game = root / 'SkillshotCity.exe'
    if game.is_symlink():
        raise ValueError(f'SkillshotCity.exe is a symbolic link in {root}. Choose the actual Steam game installation folder')
    if not game.is_file():
        from game_build import detect
        if any(item['kind'] == 'linux-x86_64' for item in detect(root)):
            raise ValueError('Native Linux game detected. This package currently contains the Proton mod only; native Linux support is being ported. To use this package now, select Proton in Steam Properties → Compatibility and let Steam download the Windows game')
        executables = sorted(p.name for p in root.iterdir() if p.suffix.lower() == '.exe')[:20]
        found = ', '.join(executables) or 'none'
        raise ValueError(f'SkillshotCity.exe was not found in {root}. Executable filenames found: {found}. Check the folder and filename capitalization')
    return root


def require_closed():
    # Conservative across all Steam libraries and Wine prefixes.
    for proc in Path('/proc').glob('[0-9]*'):
        try:
            comm = (proc / 'comm').read_text().strip().lower()
            args = (proc / 'cmdline').read_bytes().split(b'\0')
            if comm.startswith('skillshotcity') or any(
                arg.replace(b'\\', b'/').rsplit(b'/', 1)[-1].lower() == b'skillshotcity.exe'
                for arg in args[:2]
            ):
                raise RuntimeError('Close Skillshot City before changing its mod files')
        except (FileNotFoundError, ProcessLookupError, PermissionError):
            continue


def package(folder):
    folder = Path(folder).resolve(strict=True)
    data = read_json(folder / 'package.json')
    if (data.get('schema') != 1 or data.get('product') != 'SSCMods'
            or not re.fullmatch(r'\d+\.\d+\.\d+', data.get('version', ''))
            or set(data.get('files', {})) != set(OWNED)
            or not re.fullmatch('[a-f0-9]{64}', data.get('game_sha256', ''))
            or not isinstance(data.get('game_size'), int) or data['game_size'] < 1):
        raise ValueError('Invalid release metadata')
    for relative, expected in data['files'].items():
        source = destination(folder / 'payload', relative)
        if not source.is_file() or digest(source) != expected:
            raise ValueError(f'Package checksum failed: {relative}')
    return data


def installed(root):
    path = destination(root, MANIFEST)
    if not path.exists():
        return None
    data = read_json(path)
    if (data.get('schema') != 1 or data.get('product') != 'SSCMods'
            or set(data.get('files', {})) != set(OWNED)
            or any(not isinstance(h, str) or not re.fullmatch('[a-f0-9]{64}', h)
                   for h in data['files'].values())):
        raise ValueError('Invalid ownership manifest; no files changed')
    return data


def atomic_write(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temp = tempfile.mkstemp(prefix='.ssc-', dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as stream:
            stream.write(content)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temp, path)
    finally:
        if os.path.exists(temp):
            os.unlink(temp)


@contextmanager
def installation_lock(root):
    lock = root / '.ssc-mod-menu-install-lock'
    try:
        lock.mkdir()
    except FileExistsError as error:
        raise RuntimeError('Another installation is active or was interrupted. Check the backup before removing .ssc-mod-menu-install-lock and retrying') from error
    try:
        yield
    finally:
        lock.rmdir()


def install(folder, path, repair=False):
    with installation_lock(game_root(path)):
        return _install(folder, path, repair)


def _install(folder, path, repair=False):
    folder = Path(folder).resolve(strict=True)
    release = package(folder)
    root = game_root(path)
    require_closed()
    game = root / 'SkillshotCity.exe'
    if game.stat().st_size != release['game_size'] or digest(game) != release['game_sha256']:
        raise ValueError('This game update needs a newer mod package; nothing was changed')
    previous = installed(root)
    targets = {name: destination(root, name) for name in (*OWNED, MANIFEST)}
    if not previous and any((root / name).exists() or (root / name).is_symlink()
                            for name in ('opengl32.dll', 'SSCMods')):
        raise ValueError('An existing mod was found without Linux ownership metadata; it will not be overwritten')
    if previous and not repair:
        for name, expected in previous['files'].items():
            if not targets[name].is_file() or digest(targets[name]) != expected:
                raise ValueError('An installed file changed or is missing; use Repair to restore it')
    # Preserve originals for recovery, including altered files when repairing.
    backup = None
    if previous:
        backup_root = Path(os.environ.get('XDG_STATE_HOME', str(Path.home() / '.local/state'))) / 'ssc-mod-menu/backups'
        backup_root.mkdir(parents=True, exist_ok=True)
        backup = Path(tempfile.mkdtemp(prefix='update-', dir=backup_root))
        for name, target in targets.items():
            if target.exists():
                saved = backup / name
                saved.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(target, saved)
    originals = {name: target.read_bytes() if target.exists() else None for name, target in targets.items()}
    created_mod = not (root / 'SSCMods').exists()
    try:
        for name in OWNED:
            require_closed()
            atomic_write(destination(root, name), (folder / 'payload' / name).read_bytes())
        manifest = dict(release)
        manifest['platform'] = 'linux-proton'
        atomic_write(destination(root, MANIFEST), (json.dumps(manifest, indent=2) + '\n').encode())
    except Exception:
        for name, content in originals.items():
            target = destination(root, name)
            if content is None:
                target.unlink(missing_ok=True)
            else:
                atomic_write(target, content)
        if created_mod:
            (root / 'SSCMods').rmdir()
        raise
    return f'Installed v{release["version"]}' + (f'\nBackup: {backup}' if backup else '')


def uninstall(path):
    with installation_lock(game_root(path)):
        return _uninstall(path)


def _uninstall(path):
    root = game_root(path)
    require_closed()
    data = installed(root)
    if not data:
        raise ValueError('No installation owned by this Linux installer was found')
    targets = {name: destination(root, name) for name in (*OWNED, MANIFEST)}
    for name, expected in data['files'].items():
        if targets[name].exists() and digest(targets[name]) != expected:
            raise ValueError(f'{name} has changed; nothing was removed')
    for name in OWNED:
        targets[name].unlink(missing_ok=True)
    targets[MANIFEST].unlink()
    if not any((root / 'SSCMods').iterdir()):
        (root / 'SSCMods').rmdir()
    return 'Mod removed; settings, recordings, and custom sounds were preserved'


def discover(home=None):
    home = Path(home) if home else Path.home()
    libraries = {home / '.local/share/Steam', home / '.steam/steam',
                 home / '.steam/root', home / '.var/app/com.valvesoftware.Steam/data/Steam'}
    for library in list(libraries):
        vdf = library / 'steamapps/libraryfolders.vdf'
        if vdf.is_file():
            libraries.update(Path(p.replace('\\\\', '\\')) for p in re.findall(r'"path"\s+"([^"]+)"', vdf.read_text(errors='replace')))
    found = set()
    for library in libraries:
        manifest = library / f'steamapps/appmanifest_{APP_ID}.acf'
        if not manifest.is_file():
            continue
        data = manifest.read_text(errors='replace')
        match = re.search(r'"installdir"\s+"([^"/\\]+)"', data)
        if not match or match[1] in ('.', '..') or not re.search(r'"appid"\s+"308600"', data):
            continue
        try:
            found.add(game_root(library / 'steamapps/common' / match[1]))
        except (OSError, ValueError):
            pass
    return sorted(found)


def gui(folder):
    from desktop_ui import gui as show
    show(folder)


def main():
    if len(sys.argv) == 4 and sys.argv[1] == '--native-update':
        from linux_update import update
        return update(sys.argv[2], sys.argv[3])
    if len(sys.argv) > 1 and sys.argv[1] == "--launch":
        import linux_desktop
        try:
            return linux_desktop.launch(sys.argv[2:])
        except Exception:
            import traceback
            linux_desktop.log(traceback.format_exc())
            raise
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', help='Steam game directory')
    parser.add_argument('--action', choices=('install', 'repair', 'uninstall', 'inspect'))
    args = parser.parse_args()
    folder = Path(__file__).resolve().parent
    try:
        if args.action:
            if not args.game:
                parser.error('--game is required with --action')
            if args.action == 'inspect':
                root = game_root(args.game)
                print(json.dumps({'game': str(root), 'installed': installed(root), 'launch_options': LAUNCH_OPTIONS}, indent=2))
            else:
                print(uninstall(args.game) if args.action == 'uninstall' else install(folder, args.game, args.action == 'repair'))
                print('Steam launch options: ' + LAUNCH_OPTIONS)
        else:
            gui(folder)
        return 0
    except Exception as error:
        import traceback
        import linux_desktop
        linux_desktop.log(traceback.format_exc())
        print(str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
