"""Check that every literal runtime resolver lookup has a manifest binding.

A new manifest can pass its own probe while an unscanned consumer still asks
for a retired key. Run this alongside the native compatibility probe.
"""
from pathlib import Path
import re


def audit(root):
    runtime = Path(root) / 'runtime'
    manifest = (runtime / 'compatibility_data.h').read_text()
    bindings = manifest.split('inline const Binding binding_specs[]=', 1)[1]
    keys = {int(k, 16) for k in re.findall(r'^\{(0x[0-9a-f]+),', bindings, re.M)}
    errors = []
    for source in sorted(runtime.iterdir()):
        if source.suffix not in ('.h', '.cpp'):
            continue
        for number, line in enumerate(source.read_text().splitlines(), 1):
            for key in re.findall(r'ssc_compat::resolve\((0x[0-9a-f]+)\)', line):
                if int(key, 16) not in keys:
                    errors.append(f'{source.name}:{number}: unbound runtime key {key}')
    return errors


if __name__ == '__main__':
    errors = audit(Path(__file__).resolve().parents[1])
    if errors:
        raise SystemExit('\n'.join(errors))
    print('PASS: all literal runtime resolver lookups have manifest bindings')
