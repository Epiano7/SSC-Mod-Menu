"""Steam integration and local diagnostics for the native Linux installer."""
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import subprocess
import sys
import traceback
import tempfile
import zipfile

import ssc_installer as core


def state_dir():
    return Path(os.environ.get('XDG_STATE_HOME', str(Path.home() / '.local/state'))) / 'ssc-mod-menu'


def log(message):
    folder = state_dir() / 'logs'
    folder.mkdir(parents=True, exist_ok=True)
    path = folder / 'installer.log'
    if path.exists() and path.stat().st_size > 2 * 1024 * 1024:
        os.replace(path, folder / 'installer.previous.log')
    with path.open('a', encoding='utf-8') as stream:
        stream.write(datetime.datetime.now().isoformat(timespec='seconds') + ' ' + str(message) + '\n')


def require_steam_closed():
    core.require_closed()
    for proc in Path('/proc').glob('[0-9]*'):
        try:
            if (proc / 'comm').read_text().strip().lower() in ('steam', 'steamwebhelper'):
                raise RuntimeError('Exit Steam using Steam → Exit, then click Install / Update again. This prevents Steam from overwriting its launch setting')
        except (FileNotFoundError, ProcessLookupError, PermissionError):
            continue


def tokens(text):
    pattern = re.compile(r'\s+|//[^\n]*|"(?:\\.|[^"\\])*"|[{}]')
    result = []
    pos = 0
    while pos < len(text):
        match = pattern.match(text, pos)
        if not match:
            raise ValueError('Steam settings use an unsupported format; no settings were changed')
        raw = match[0]
        if not raw.isspace() and not raw.startswith('//'):
            value = re.sub(r'\\([\\"])', r'\1', raw[1:-1]) if raw.startswith('"') else raw
            result.append((value, match.start(), match.end()))
        pos = match.end()
    return result


def parse(text):
    ts = tokens(text)
    def group(i, closing=False):
        entries = {}
        while i < len(ts) and ts[i][0] != '}':
            key = ts[i][0].lower()
            if key in entries or i + 1 >= len(ts):
                raise ValueError('Ambiguous Steam settings; no settings were changed')
            val = ts[i + 1]
            if val[0] == '{':
                children, stop = group(i + 2, True)
                entries[key] = dict(children=children, end=ts[stop][1])
                i = stop + 1
            else:
                if val[0] == '}':
                    raise ValueError('Invalid Steam settings')
                entries[key] = dict(value=val[0], start=val[1], end=val[2])
                i += 2
        if closing and (i >= len(ts) or ts[i][0] != '}'):
            raise ValueError('Incomplete Steam settings')
        return entries, i
    entries, stop = group(0)
    if stop != len(ts):
        raise ValueError('Unexpected Steam settings delimiter')
    return entries


def quote(value):
    return '"' + value.replace('\\', '\\\\').replace('"', '\\"') + '"'


APP_PATH = ['UserLocalConfigStore', 'Software', 'Valve', 'Steam', 'apps', core.APP_ID, 'LaunchOptions']


def value_at(text, path):
    entries = parse(text)
    for index, key in enumerate(path):
        node = entries.get(key.lower())
        if node is None:
            return None
        if index == len(path) - 1:
            return node.get('value')
        entries = node.get('children', {})


def set_value(text, path, value):
    entries = parse(text)
    end = len(text)
    for index, key in enumerate(path):
        node = entries.get(key.lower())
        if node is None:
            addition = quote(path[-1]) + '\t' + quote(value) + '\n'
            for parent in reversed(path[index:-1]):
                addition = quote(parent) + '\n{\n' + addition + '}\n'
            return text[:end] + '\n' + addition + text[end:]
        if index == len(path) - 1:
            if 'value' not in node:
                raise ValueError('Invalid Steam launch setting')
            return text[:node['start']] + quote(value) + text[node['end']:]
        if 'children' not in node:
            raise ValueError('Invalid Steam settings group')
        entries, end = node['children'], node['end']


def steam_config(home=None):
    home = Path(home) if home else Path.home()
    roots = [home / '.local/share/Steam', home / '.steam/steam', home / '.steam/root']
    seen = set()
    for root in roots:
        root = root.resolve()
        if root in seen:
            continue
        seen.add(root)
        login = root / 'config/loginusers.vdf'
        if not login.is_file():
            continue
        users = parse(login.read_text(encoding='utf-8-sig')).get('users', {}).get('children', {})
        recent = [uid for uid, user in users.items() if user.get('children', {}).get('mostrecent', {}).get('value') == '1']
        # Some Steam installations omit MostRecent entirely, including the
        # single-account AutoLogin format reported by our Arch tester.
        if not recent:
            if len(users) == 1:
                recent = list(users)
            else:
                recent = [uid for uid, user in users.items() if user.get('children', {}).get('autologin', {}).get('value') == '1']
        if len(recent) != 1:
            raise ValueError('Steam account selection is ambiguous. Open Steam with the account you play on, enable automatic sign-in for that account, then exit Steam and retry')
        if not recent[0].isdigit():
            raise ValueError('Invalid Steam account identifier')
        account = int(recent[0]) - 76561197960265728
        if not 0 <= account <= 4294967295:
            raise ValueError('Invalid Steam account identifier')
        config = root / 'userdata' / str(account) / 'config/localconfig.vdf'
        if config.is_symlink() or not config.is_file():
            raise ValueError('Steam account settings were not found. Launch the game once through Steam, then exit Steam and retry')
        return config
    raise ValueError('Regular Steam account settings were not found. Open Steam once and sign in first')


def integration_dir(game):
    key = hashlib.sha256(str(Path(game).resolve()).encode()).hexdigest()[:20]
    return Path(os.environ.get('XDG_DATA_HOME', str(Path.home() / '.local/share'))) / 'ssc-mod-menu' / key


def checked_record(game):
    path = integration_dir(game) / 'integration.json'
    if not path.exists():
        return None
    data = core.read_json(path)
    if data.get('game') != str(Path(game).resolve()) or not isinstance(data.get('original'), str) or not isinstance(data.get('configured'), str):
        raise ValueError('Invalid Steam integration record')
    return data


def helper_command(folder):
    return shlex.join([str(folder / 'python/bin/python3'), str(folder / 'ssc_installer.py'), '--launch'])


def launch_options(original, command):
    if original.count('%command%') > 1:
        raise ValueError('Multiple Steam command placeholders; no settings were changed')
    if '"%command%"' in original or "'%command%'" in original:
        raise ValueError('Quoted Steam command placeholder is unsupported; remove the quotes around %command% in Steam first')
    # Insert at the executable position, preserving environment variables and wrappers.
    return original.replace('%command%', command + ' %command%') if '%command%' in original else command + ' %command%' + (' ' + original if original else '')


def install_desktop(folder, game, repair=False):
    log('Selected game path: ' + repr(str(game)))
    base = integration_dir(game)
    if base.is_symlink():
        raise ValueError('Linked installer storage is unsupported')
    base.mkdir(parents=True, exist_ok=True)
    with core.installation_lock(base):
        return _install_desktop(folder, game, repair)


def _install_desktop(folder, game, repair=False):
    folder, game = Path(folder).resolve(), core.game_root(game)
    core.require_closed()
    release = core.package(folder)
    if (game / 'SkillshotCity.exe').stat().st_size != release['game_size'] or core.digest(game / 'SkillshotCity.exe') != release['game_sha256']:
        raise ValueError('This game update needs a newer mod package; nothing was changed')
    config = steam_config()
    old_bytes = config.read_bytes()
    old_text = old_bytes.decode('utf-8')
    original = value_at(old_text, APP_PATH) or ''
    record = checked_record(game)
    reusable = bool(record and record.get('format') == 2)
    if not reusable:
        require_steam_closed()
    if record:
        if record['config'] != str(config):
            raise ValueError('This install belongs to another Steam account. Switch to that account before updating')
        if original == record['configured']:
            original = record['original']
        elif record['command'] + ' %command%' in original:
            original = original.replace(record['command'] + ' %command%', '%command%', 1)
        else:
            raise ValueError('Steam launch settings changed. Uninstall the managed setup before reconfiguring it')
    base = integration_dir(game)
    if base.is_symlink():
        raise ValueError('Linked installer storage is unsupported')
    base.mkdir(parents=True, exist_ok=True)
    # Keep a persistent helper independent of the downloaded folder.
    helper = Path(core.tempfile.mkdtemp(prefix='helper-', dir=base))
    shutil.copytree(folder, helper, dirs_exist_ok=True, symlinks=True, ignore=shutil.ignore_patterns('__pycache__'))
    if not (helper / 'python/bin/python3').exists():
        raise ValueError('Bundled Linux Python is missing; extract the full installer archive')
    dispatcher = base / 'dispatch.py'
    if reusable:
        command = record['command']
    else:
        core.atomic_write(dispatcher, b'import json,os,sys\nfrom pathlib import Path\nb=Path(__file__).resolve().parent\np=Path(json.loads((b/"current.json").read_text())["helper"]).resolve()\nif p.parent!=b or not p.name.startswith("helper-"): raise RuntimeError("Invalid installed helper")\nos.execv(str(p/"python/bin/python3"),[str(p/"python/bin/python3"),str(p/"ssc_installer.py"),*sys.argv[1:]])\n')
        command = shlex.join([str(helper / 'python/bin/python3'), str(dispatcher), '--launch'])
    configured = launch_options(original, command)
    new_bytes = set_value(old_text, APP_PATH, configured).encode('utf-8')
    targets = {name: core.destination(game, name) for name in (*core.OWNED, core.MANIFEST)}
    saved = {name: target.read_bytes() if target.is_file() else None for name, target in targets.items()}
    had_mod_folder = (game / 'SSCMods').exists()
    config_written = False
    pointer = base / 'current.json'
    old_pointer = pointer.read_bytes() if pointer.exists() else None
    log('Install started: ' + str(game))
    backup = state_dir() / 'backups' / ('steam-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f') + '.vdf')
    core.atomic_write(backup, old_bytes)
    try:
        result = core.install(folder, game, repair)
        if not reusable:
            require_steam_closed()
        if config.read_bytes() != old_bytes:
            raise RuntimeError('Steam settings changed during installation; retry with Steam closed')
        if new_bytes != old_bytes:
            require_steam_closed()
            core.atomic_write(config, new_bytes)
            config_written = True
        core.atomic_write(pointer, json.dumps({'helper': str(helper)}).encode())
        data = dict(format=2, game=str(game), config=str(config), original=original, configured=configured, command=command)
        core.atomic_write(base / 'integration.json', json.dumps(data, indent=2).encode())
    except Exception:
        if old_pointer is None:
            pointer.unlink(missing_ok=True)
        else:
            core.atomic_write(pointer, old_pointer)
        if config_written and config.read_bytes() == new_bytes:
            core.atomic_write(config, old_bytes)
        for name, content in saved.items():
            target = core.destination(game, name)
            if content is None:
                target.unlink(missing_ok=True)
            else:
                core.atomic_write(target, content)
        if not had_mod_folder and (game / 'SSCMods').is_dir() and not any((game / 'SSCMods').iterdir()):
            (game / 'SSCMods').rmdir()
        log(traceback.format_exc())
        raise
    log(result + '; Steam launch integration configured')
    return 'Installed successfully — open Steam and play'


def uninstall_desktop(game):
    base = integration_dir(game)
    base.mkdir(parents=True, exist_ok=True)
    if base.is_symlink():
        raise ValueError('Linked installer storage is unsupported')
    with core.installation_lock(base):
        return _uninstall_desktop(game)


def _uninstall_desktop(game):
    require_steam_closed()
    game = core.game_root(game)
    record = checked_record(game)
    if not record:
        return core.uninstall(game)
    config = steam_config()
    if str(config) != record['config']:
        raise ValueError('Switch to the Steam account used for this installation before uninstalling')
    before = config.read_bytes()
    text = before.decode('utf-8')
    current = value_at(text, APP_PATH) or ''
    if current == record['configured']:
        restored = record['original']
    else:
        marker = record['command'] + ' %command%'
        restored = current.replace(marker, '%command%', 1) if marker in current else current
    after = set_value(text, APP_PATH, restored).encode()
    core.atomic_write(config, after)
    try:
        result = core.uninstall(game)
    except Exception:
        if config.read_bytes() == after:
            core.atomic_write(config, before)
        raise
    (integration_dir(game) / 'integration.json').unlink()
    for name in ('linux-update-status.txt', 'linux-update-request.json', 'linux-update-request.json.tmp'):
        path = game / 'SSCMods' / name
        if path.is_file() and not path.is_symlink():
            path.unlink()
    if (game / 'SSCMods').is_dir() and not any((game / 'SSCMods').iterdir()):
        (game / 'SSCMods').rmdir()
    log(result)
    return result


def launch(args):
    if not args:
        raise ValueError('Steam did not supply a game command')
    folder = state_dir() / 'logs'
    folder.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    overrides = []
    for clause in env.get('WINEDLLOVERRIDES', '').split(';'):
        if not clause:
            continue
        names, sep, mode = clause.partition('=')
        if not sep:
            raise ValueError('Unrecognized WINEDLLOVERRIDES setting')
        names = ','.join(n for n in names.split(',') if n.strip().lower().removesuffix('.dll') != 'opengl32')
        if names:
            overrides.append(names + '=' + mode)
    env['WINEDLLOVERRIDES'] = ';'.join(overrides + ['opengl32=n,b'])
    env['PROTON_LOG'] = '1'
    env['PROTON_LOG_DIR'] = str(folder)
    prefix = env.get('STEAM_COMPAT_DATA_PATH', '')
    core.atomic_write(state_dir() / 'last-launch.json', json.dumps({'prefix': prefix, 'command': args[0]}).encode())
    proton_log = folder / ('steam-' + core.APP_ID + '.log')
    if proton_log.exists():
        os.replace(proton_log, folder / 'proton.previous.log')
    log('Launching through Steam; Proton diagnostics enabled')
    # Keep a native coordinator outside Proton for in-game update handoff.
    base = Path(__file__).resolve().parent.parent
    record_path = base / 'integration.json'
    record = core.read_json(record_path) if record_path.is_file() else None
    request = Path(record['game']) / 'SSCMods/linux-update-request.json' if record else None
    if request:
        request.unlink(missing_ok=True)
    # A private per-launch mailbox bridges Wine to the native Discord socket.
    # It carries presentation data only, and is removed when the game exits.
    from discord_bridge import Bridge
    relay = tempfile.TemporaryDirectory(prefix='.discord-', dir=Path(record['game']) / 'SSCMods') if record else None
    bridge = Bridge(relay.name, log=log) if relay else None
    if relay:
        env['SSC_DISCORD_RELAY'] = 'Z:' + relay.name.replace('/', '\\')
    try:
        process = subprocess.Popen(args, env=env)
        while process.poll() is None:
            if bridge:
                bridge.tick()
            if request and request.is_file():
                try:
                    data = core.read_json(request)
                    request.unlink()
                    version = data.get('version', '')
                    if not re.fullmatch(r'\d{1,5}\.\d{1,5}\.\d{1,5}', version):
                        raise ValueError('Invalid requested update')
                    subprocess.Popen([sys.executable, str(Path(__file__).resolve().with_name('ssc_installer.py')),
                                      '--native-update', record['game'], version], start_new_session=True)
                except Exception:
                    log(traceback.format_exc())
            import time
            time.sleep(0.5)
        return process.returncode
    finally:
        if bridge:
            bridge.close()
        if relay:
            relay.cleanup()



def export_logs(game, destination):
    files = []
    folder = state_dir() / 'logs'
    if folder.exists():
        files.extend(('installer/' + p.name, p) for p in folder.iterdir() if p.is_file() and p.suffix == '.log')
    details = dict(platform=platform.platform(), machine=platform.machine(), python=sys.version.split()[0])
    details['selected_game_path'] = str(game).replace(str(Path.home()), '<HOME>')
    if game:
        try:
            root = core.game_root(game)
            details['game_sha256'] = core.digest(root / 'SkillshotCity.exe')
            details['mod'] = core.installed(root)
            # Library-local prefix used by ordinary Steam installations.
            prefixes = {root.parent.parent / 'compatdata' / core.APP_ID / 'pfx/drive_c/users'}
            last_launch = state_dir() / 'last-launch.json'
            if last_launch.is_file():
                data = core.read_json(last_launch)
                if data.get('prefix'):
                    prefixes.add(Path(data['prefix']) / 'pfx/drive_c/users')
            for prefix in prefixes:
                if not prefix.is_dir():
                    continue
                for user in prefix.iterdir():
                    state = user / 'AppData/Local/SkillshotCityMod'
                    for name in ('runtime.log', 'runtime.previous.log', 'update-status.txt'):
                        p = state / name
                        if p.is_file():
                            files.append(('game/' + user.name + '/' + name, p))
        except (OSError, ValueError, KeyError) as error:
            details["game_log_collection_error"] = str(error)
    with zipfile.ZipFile(destination, 'w', zipfile.ZIP_DEFLATED) as archive:
        archive.writestr('system.json', json.dumps(details, indent=2))
        archive.writestr('READ-ME.txt', 'Logs are local and may contain usernames, paths, hardware and game information. Review before sharing. Settings, recordings, Steam account files and custom audio are not included.\n')
        for name, path in files:
            with path.open('rb') as stream:
                stream.seek(max(0, path.stat().st_size - 4 * 1024 * 1024))
                text = stream.read().decode('utf-8', errors='replace')
            text = text.replace(str(Path.home()), '<HOME>')
            archive.writestr(name, text)
    log('Diagnostic export completed')
    return 'Logs exported — review the ZIP before sharing'
