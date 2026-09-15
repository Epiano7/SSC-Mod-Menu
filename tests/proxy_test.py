"""Run on Windows x64 in a fresh Python process (requires pefile).

Usage: python tests/proxy_test.py RUNTIME_BUILD EMPTY_TEST_DIRECTORY
"""
import ctypes
import os
from pathlib import Path
import shutil
import sys
import pefile

build, target = map(lambda s: Path(s).resolve(), sys.argv[1:])
target.mkdir(parents=True, exist_ok=False)
(target / 'SSCMods').mkdir()
for name, dest in [('opengl32.dll', target), ('runtime.dll', target / 'SSCMods')]:
    shutil.copyfile(build / name, dest / name)
def exports(path):
    with pefile.PE(str(path)) as pe:
        return {e.name: e.ordinal for e in pe.DIRECTORY_ENTRY_EXPORT.symbols}
assert exports(target / 'opengl32.dll') == exports(Path(os.environ['WINDIR']) / 'System32/opengl32.dll')
os.environ['SSCMODS_STATE_DIR'] = str(target / 'state')
proxy = ctypes.WinDLL(str(target / 'opengl32.dll'))
proxy.wglGetCurrentContext.restype = ctypes.c_void_p
assert proxy.wglGetCurrentContext() is None
assert proxy.wglGetCurrentDC() == 0
assert 'Unsupported executable; runtime inactive' in (target / 'state/runtime.log').read_text()
print('PASS: exact export names/ordinals, native forwarding, unknown host remains inactive')
