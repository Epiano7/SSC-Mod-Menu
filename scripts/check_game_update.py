"""Offline Steam snapshot -> runtime compatibility report. No game launch or patching."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def steam_state(raw):
    text = raw.decode('utf-8-sig')
    def value(key):
        found = re.findall(r'"' + re.escape(key) + r'"\s*"([^"\r\n]*)"', text, re.I)
        if len(found) != 1:
            raise ValueError('missing or ambiguous Steam field: ' + key)
        return found[0]
    if value('appid') != '308600' or value('StateFlags') != '4':
        raise ValueError('Steam app is not fully installed/idle')
    build = value('buildid')
    if not build.isdigit() or int(build) <= 0:
        raise ValueError('invalid Steam build ID')
    for pending, finished in (('BytesToDownload', 'BytesDownloaded'), ('BytesToStage', 'BytesStaged')):
        todo, done = value(pending), value(finished)
        if not todo.isdigit() or not done.isdigit() or int(todo) != int(done):
            raise ValueError('Steam download is incomplete')
    directory = value('installdir')
    if not directory or directory in ('.', '..') or any(x in directory for x in '/\\:'):
        raise ValueError('invalid Steam install directory')
    return build, directory


def check(game, manifest, probe):
    before = manifest.read_bytes()
    build, directory = steam_state(before)
    expected = manifest.parent / 'common' / directory / 'SkillshotCity.exe'
    if game.resolve() != expected.resolve():
        raise ValueError('game does not belong to the supplied Steam manifest')
    original = digest(game)
    with tempfile.TemporaryDirectory(prefix='ssc-compat-') as temporary:
        snapshot = Path(temporary) / 'SkillshotCity.exe'
        shutil.copyfile(game, snapshot)
        if digest(snapshot) != original:
            raise ValueError('game changed while copying; retry after Steam finishes')
        run = subprocess.run([str(probe.resolve()), str(snapshot)], capture_output=True,
                             text=True, timeout=90, check=False)
        if run.returncode not in (0, 2):
            raise ValueError('probe rejected the executable or failed')
        data = json.loads(run.stdout)
        status = 'compatible' if run.returncode == 0 else 'needs_review'
        if (not isinstance(data, dict) or data.get('schema') != 1 or data.get('status') != status
                or type(data.get('required_groups')) is not int
                or type(data.get('available_groups')) is not int
                or not 0 < data['required_groups'] <= 255
                or not 0 <= data['available_groups'] <= data['required_groups']
                or (data['required_groups'] == data['available_groups']) != (run.returncode == 0)):
            raise ValueError('invalid probe report')
    if manifest.read_bytes() != before or digest(game) != original:
        raise ValueError('Steam changed during inspection; retry when idle')
    return dict(schema=1, app_id=308600, build_id=build, game_sha256=original,
                status=status, required_groups=data['required_groups'],
                available_groups=data['available_groups'], publish_allowed=False,
                probe_sha256=digest(probe), details=run.stderr.splitlines()), run.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', required=True, type=Path)
    parser.add_argument('--steam-manifest', required=True, type=Path)
    parser.add_argument('--probe', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    try:
        report, code = check(args.game, args.steam_manifest, args.probe)
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        report, code = dict(schema=1, status='error', publish_allowed=False,
                            error=str(error)), 3
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(dir=args.output.parent, prefix='.compat-', suffix='.tmp')
    try:
        with os.fdopen(fd, 'w', encoding='utf-8') as stream:
            json.dump(report, stream, indent=2)
            stream.write('\n')
        os.replace(temporary, args.output)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    print(report['status'])
    return code


if __name__ == '__main__':
    raise SystemExit(main())
