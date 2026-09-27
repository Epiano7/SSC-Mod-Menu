"""Native, verified release download and game restart coordinator."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tarfile
import tempfile
import time
import traceback
import urllib.request

import ssc_installer as core
import linux_desktop as desktop

REPO = 'https://github.com/Epiano7/SSC-Mod-Menu'


def choose_asset(data, version):
    if not re.fullmatch(r'\d{1,5}\.\d{1,5}\.\d{1,5}', version):
        raise ValueError('Invalid update version')
    if data.get('tag_name') != 'v' + version or data.get('draft') or data.get('prerelease'):
        raise ValueError('Requested stable release is unavailable')
    assets = [a for a in data.get('assets', []) if a.get('name') == 'SSC-Mod-Menu-Linux.tar.gz']
    if len(assets) != 1:
        raise ValueError('The release does not contain exactly one Linux installer')
    asset = assets[0]
    expected = REPO + '/releases/download/v' + version + '/SSC-Mod-Menu-Linux.tar.gz'
    if (asset.get('browser_download_url') != expected or asset.get('state') != 'uploaded'
            or not isinstance(asset.get('size'), int) or not 1024 <= asset['size'] <= 134217728
            or not re.fullmatch(r'sha256:[0-9a-fA-F]{64}', asset.get('digest', ''))):
        raise ValueError('The Linux release failed metadata validation')
    return asset


def download(version, destination):
    url = 'https://api.github.com/repos/Epiano7/SSC-Mod-Menu/releases/tags/v' + version
    request = urllib.request.Request(url, headers={'User-Agent': 'SSC-Mod-Menu-Linux', 'Accept': 'application/vnd.github+json'})
    with urllib.request.urlopen(request, timeout=20) as response:
        body = response.read(1048577)
    if len(body) > 1048576:
        raise ValueError('Release metadata is too large')
    asset = choose_asset(json.loads(body), version)
    digest = hashlib.sha256()
    size = 0
    deadline = time.monotonic() + 300
    with urllib.request.urlopen(asset['browser_download_url'], timeout=20) as response, Path(destination).open('wb') as output:
        if not response.url.startswith(('https://github.com/', 'https://release-assets.githubusercontent.com/', 'https://objects.githubusercontent.com/')):
            raise ValueError('Unexpected release download host')
        while True:
            block = response.read(1048576)
            if not block:
                break
            size += len(block)
            if size > asset['size'] or time.monotonic() > deadline:
                raise ValueError('Release download exceeded its limits')
            digest.update(block)
            output.write(block)
    if size != asset['size'] or digest.hexdigest() != asset['digest'][7:].lower():
        raise ValueError('Downloaded installer checksum mismatch')


def extract(archive, destination):
    with tarfile.open(archive) as source:
        members = source.getmembers()
        if len(members) > 30000 or sum(m.size for m in members) > 1500000000:
            raise ValueError('Installer archive exceeds its limits')
        for member in members:
            if (not member.name.startswith('SSC-Mod-Menu-Linux/') or '\\' in member.name
                    or '..' in Path(member.name).parts or not (member.isfile() or member.isdir() or member.issym() or member.islnk())):
                raise ValueError('Invalid installer archive entry')
        source.extractall(destination, filter='data')
    return Path(destination) / 'SSC-Mod-Menu-Linux'


def update(game, version):
    game = core.game_root(game)
    status = game / 'SSCMods/linux-update-status.txt'
    def write(message):
        core.atomic_write(status, (message + '\n').encode())
    try:
        if not re.fullmatch(r'\d{1,5}\.\d{1,5}\.\d{1,5}', version):
            raise ValueError('Invalid update version')
        lock_parent = desktop.integration_dir(game) / 'update-lock-parent'
        lock_parent.mkdir(parents=True, exist_ok=True)
        with core.installation_lock(lock_parent):
            return _update(game, version, write)
    except Exception as error:
        desktop.log(traceback.format_exc())
        write('error ' + str(error).replace('\n', ' ')[:1000])
        try:
            core.require_closed()
            import tkinter as tk
            from tkinter import messagebox
            window = tk.Tk(); window.withdraw()
            messagebox.showerror('SSC Mod Menu update', str(error) + '\nOpen the installer and use Export Logs for diagnostics', parent=window)
            window.destroy()
        except Exception:
            pass
        return 1


def _update(game, version, write):
    write('downloading Downloading the Linux update...')
    with tempfile.TemporaryDirectory(prefix='ssc-update-') as directory:
        archive = Path(directory) / 'update.tar.gz'
        download(version, archive)
        package = extract(archive, Path(directory) / 'unpacked')
        metadata = core.package(package)
        if metadata['version'] != version:
            raise ValueError('Installer version does not match the release')
        if core.digest(game / 'SkillshotCity.exe') != metadata['game_sha256']:
            raise ValueError('This release does not match your current game build')
        write('ready Update verified; closing the game to install...')
        deadline = time.monotonic() + 120
        while True:
            try:
                core.require_closed()
                break
            except RuntimeError:
                if time.monotonic() >= deadline:
                    raise RuntimeError('The game did not close; no files were changed')
                time.sleep(0.5)
        desktop.install_desktop(package, game)
        write('complete Update installed; restarting through Steam')
        subprocess.Popen(['xdg-open', 'steam://rungameid/' + core.APP_ID], start_new_session=True)
        desktop.log('Native update completed: ' + version)
    return 0
