"""Check that every literal runtime resolver lookup has a manifest binding.

A new manifest can pass its own probe while an unscanned consumer still asks
for a retired key. Run this alongside the native compatibility probe.
"""
from pathlib import Path
import re


def audit(root, private=None):
    runtime = Path(root) / 'runtime'
    manifest = (runtime / 'compatibility_data.h').read_text()
    bindings = manifest.split('inline const Binding binding_specs[]=', 1)[1]
    keys = {int(k, 16) for k in re.findall(r'^\{(0x[0-9a-f]+),', bindings, re.M)}
    errors = []
    for source in sorted(list(runtime.iterdir())+(list((Path(private)/"runtime").iterdir()) if private else [])):
        if source.suffix not in ('.h', '.cpp'):
            continue
        for number, line in enumerate(source.read_text(encoding="utf-8").splitlines(), 1):
            for key in re.findall(r'ssc_compat::resolve\((0x[0-9a-f]+)\)', line):
                if int(key, 16) not in keys:
                    errors.append(f'{source.name}:{number}: unbound runtime key {key}')
    return errors


if __name__ == '__main__':
    import argparse
    from verify_public_source import verify
    parser=argparse.ArgumentParser();parser.add_argument('--private-source',type=Path);args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    if not args.private_source:raise SystemExit('Full runtime audit requires --private-source; the compatibility probe does not')
    verify(root,args.private_source)
    errors = audit(root,args.private_source)
    if errors:
        raise SystemExit('\n'.join(errors))
    print('PASS: all literal runtime resolver lookups have manifest bindings')
