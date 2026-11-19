"""Run the compiled Windows fallback-updater fixture without downloads or installation.
Usage: python scripts/test_fallback_update.py PATH_TO_FIXTURE_EXE
Build scripts/fallback_update_test.cpp with C++17, WinHTTP and Shell32.
"""
from pathlib import Path
import subprocess,sys,tempfile

def main():
    if len(sys.argv)!=2:
        raise SystemExit('Usage: test_fallback_update.py PATH_TO_FIXTURE_EXE')
    executable=Path(sys.argv[1]).resolve(strict=True)
    for version in ('0.1.41','0.2.0','1.23.456','99.99.99'):
        with tempfile.TemporaryDirectory(prefix='ssc-fallback-fixture-') as fixture:
            result=subprocess.run([str(executable),fixture,version],capture_output=True,text=True,timeout=30,shell=False)
            if result.returncode:
                raise RuntimeError(result.stdout+result.stderr)
            print(version+': '+result.stdout.strip())

if __name__=='__main__':
    main()
