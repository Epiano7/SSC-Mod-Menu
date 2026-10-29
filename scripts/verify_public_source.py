"""Verify the reviewed public inventory and optional owner-only build input."""
from pathlib import Path
import argparse,hashlib,json,re

def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def inventory(root):
 return {p.relative_to(root).as_posix():p for p in root.rglob('*') if p.is_file() and not any(x in ('.git','build','__pycache__') for x in p.relative_to(root).parts)}
def verify(root,private=None):
 manifest=json.loads((root/'source-export.json').read_text(encoding='utf-8'))
 component=manifest['private_component']
 if hashlib.sha256(json.dumps(component['files'],sort_keys=True,separators=(',',':')).encode()).hexdigest()!=component['sha256']:raise ValueError('Private component manifest digest mismatch')
 files=inventory(root);expected=manifest['public_files']
 if set(files)!=set(expected)|{'source-export.json'}:raise ValueError('Unreviewed/missing public files: '+str(sorted(set(files)^(set(expected)|{'source-export.json'}))))
 patterns=[rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----',rb'gh[pousr]_[A-Za-z0-9]{30,}',rb'github_pat_[A-Za-z0-9_]{60,}',rb'AKIA[0-9A-Z]{16}',rb'xox[baprs]-[A-Za-z0-9-]{20,}']
 for name,sha in expected.items():
  if digest(files[name])!=sha:raise ValueError('Public file changed since review: '+name)
  if any(re.search(p,files[name].read_bytes()) for p in patterns):raise ValueError('Potential secret requires review: '+name)
 if private is not None:
  wanted=manifest['private_component']['files'];actual=inventory(private)
  if set(actual)!=set(wanted):raise ValueError('Private component inventory mismatch')
  for name,sha in wanted.items():
   if digest(actual[name])!=sha:raise ValueError('Private component digest mismatch: '+name)
 return {'public_files':len(expected),'private_files':len(manifest['private_component']['files']) if private else 0,'verified':True}

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);p.add_argument('--private-source',type=Path);a=p.parse_args()
 try:print(json.dumps(verify(a.root.resolve(),a.private_source.resolve() if a.private_source else None)))
 except (ValueError,OSError,KeyError) as e:raise SystemExit(str(e))
