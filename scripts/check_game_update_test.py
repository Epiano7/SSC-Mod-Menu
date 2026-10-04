"""Synthetic monitor contract tests; no Steam account or game required."""
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import check_game_update as target

ACF = b'''"AppState" { "appid" "308600" "StateFlags" "4" "buildid" "123"
"installdir" "SkillshotCity" "BytesToDownload" "10" "BytesDownloaded" "10"
"BytesToStage" "20" "BytesStaged" "20" }'''


class MonitorTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.manifest = root / 'appmanifest_308600.acf'
        self.manifest.write_bytes(ACF)
        self.game = root / 'common/SkillshotCity/SkillshotCity.exe'
        self.game.parent.mkdir(parents=True)
        self.game.write_bytes(b'synthetic executable fixture')
        self.probe = root / 'probe.exe'
        self.probe.write_bytes(b'probe fixture')

    def check(self):
        return target.check(self.game, self.manifest, self.probe)

    def response(self, code=0):
        return subprocess.CompletedProcess([], code, json.dumps(dict(schema=1,
            required_groups=255, available_groups=255 if code == 0 else 254,
            status='compatible' if code == 0 else 'needs_review')), '')

    def test_compatible_is_not_publication_approval(self):
        with patch.object(target.subprocess, 'run', return_value=self.response()):
            report, code = self.check()
        self.assertEqual(code, 0)
        self.assertFalse(report['publish_allowed'])
        self.assertEqual(report['game_sha256'], target.digest(self.game))

    def test_incompatible(self):
        with patch.object(target.subprocess, 'run', return_value=self.response(2)):
            report, code = self.check()
        self.assertEqual((code, report['status']), (2, 'needs_review'))

    def test_incomplete_wrong_app_duplicate_and_traversal(self):
        for old, new in [(b'"4"', b'"6"'), (b'"308600"', b'"480"'),
                         (b'"BytesDownloaded" "10"', b'"BytesDownloaded" "9"'),
                         (b'"BytesStaged" "20"', b'"BytesStaged" "19"'),
                         (b'"123"', b'"0"'), (b'"SkillshotCity"', b'"../escape"'),
                         (b'"appid" "308600"', b'"appid" "308600" "appid" "308600"')]:
            with self.subTest(new=new), self.assertRaises(ValueError):
                target.steam_state(ACF.replace(old, new))

    def test_wrong_library(self):
        self.manifest.write_bytes(ACF.replace(b'SkillshotCity', b'Other'))
        with self.assertRaises(ValueError):
            self.check()

    def test_game_or_manifest_changes_during_scan(self):
        for change in ('game', 'manifest'):
            with self.subTest(change=change):
                def mutate(*args, **kwargs):
                    path = self.game if change == 'game' else self.manifest
                    path.write_bytes(path.read_bytes() + b' ')
                    return self.response()
                with patch.object(target.subprocess, 'run', side_effect=mutate), self.assertRaises(ValueError):
                    self.check()

    def test_timeout(self):
        with patch.object(target.subprocess, 'run', side_effect=subprocess.TimeoutExpired('probe', 90)):
            with self.assertRaises(subprocess.TimeoutExpired):
                self.check()

    def test_crash_malformed_and_inconsistent_report(self):
        for reply in (subprocess.CompletedProcess([], 3, '', ''),
                      subprocess.CompletedProcess([], 0, '<html>', ''),
                      subprocess.CompletedProcess([], 0, '[]', ''),
                      subprocess.CompletedProcess([], 0, self.response(2).stdout, '')):
            with self.subTest(reply=reply), patch.object(target.subprocess, 'run', return_value=reply):
                with self.assertRaises(ValueError):
                    self.check()


if __name__ == '__main__':
    unittest.main()
