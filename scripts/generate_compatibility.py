"""Generate conservative relocatable native checks from a locally validated executable.

Requires pefile and capstone. Never embeds the executable or game assets. The
manifest contains short locators and hashes of complete dependency functions.
Changed function bodies fail closed; this is not a universal ABI guarantee.
"""
from pathlib import Path
import argparse, bisect, collections, re
import pefile, capstone

parser=argparse.ArgumentParser()
parser.add_argument('game');parser.add_argument('--root',default=str(Path(__file__).resolve().parents[1]))
args=parser.parse_args();root=Path(args.root);pe=pefile.PE(args.game)
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);md.detail=True
sec=next(s for s in pe.sections if s.Name.startswith(b'.text'));code=sec.get_data();start=sec.VirtualAddress
funcs=[e.struct for e in pe.DIRECTORY_ENTRY_EXCEPTION];begins=[e.BeginAddress for e in funcs]
def function(a):
 e=funcs[bisect.bisect_right(begins,a)-1]
 assert e.BeginAddress<=a<e.EndAddress,hex(a)
 return e
dependencies=collections.defaultdict(int)
for name,group in [('cosmetic_adapter.h',1),('hud_editor.h',2),('hud_geometry.h',2),('presence_source.h',4)]:
 text=(root/'runtime'/name).read_text()
 for a in re.findall(r'(?:image_base|game_base|base)\+(?:ssc_compat::resolve\()?(0x[0-9a-f]+)',text):dependencies[int(a,16)]|=group
 for a,b in re.findall(r'\{(0x[0-9a-f]+),\s*(0x[0-9a-f]+),',text):
  if int(a,16)>4096:dependencies[int(a,16)]|=group;dependencies[int(b,16)]|=group
assert dependencies,'Generate before wrapping native addresses in resolve()'
refs=collections.defaultdict(list);md.detail=False;md.skipdata=True
for a,size,mn,op in md.disasm_lite(code,start):
 match=re.search(r'\[rip ([+-]) (0x[0-9a-f]+)\]',op)
 if match:
  target=a+size+(1 if match[1]=='+' else -1)*int(match[2],16)
  if target in dependencies:
   try:e=function(a)
   except AssertionError:continue
   if e.EndAddress-e.BeginAddress>=256:refs[target].append((e.EndAddress-e.BeginAddress,a))
md.detail=True;md.skipdata=False
def unique_function(a):
 e=function(a);raw=pe.get_data(e.BeginAddress,e.EndAddress-e.BeginAddress)
 # Skip duplicated small compiler helpers when selecting data references.
 return len(raw)>512 or code.count(raw)==1
function_groups=collections.defaultdict(int);bindings=[]
for key,group in sorted(dependencies.items()):
 if start<=key<start+len(code):
  e=function(key);bindings.append((key,e.BeginAddress,key-e.BeginAddress,0,0,group,True))
 else:
  assert refs[key],hex(key)
  # Two independent instructions must agree on each data address when available.
  choices=[x for x in sorted(refs[key]) if unique_function(x[1])];selected=[choices[0]]
  second=next((x for x in choices if function(x[1]).BeginAddress!=function(choices[0][1]).BeginAddress),None)
  if second:selected.append(second)
  for _,a in selected:
   e=function(a);ins=next(md.disasm(pe.get_data(a,15),a))
   bindings.append((key,e.BeginAddress,a-e.BeginAddress,ins.disp_offset,ins.size,group,False))
   function_groups[e.BeginAddress]|=group
 function_groups[e.BeginAddress]|=group
def fnv(data):
 h=14695981039346656037
 for b in data:h=((h^b)*1099511628211)&0xffffffffffffffff
 return h
rows=[]
for a,group in sorted(function_groups.items()):
 e=function(a);raw=pe.get_data(a,e.EndAddress-a);mask=bytearray([1]*len(raw));md.skipdata=True;ins=list(md.disasm(raw,a));md.skipdata=False
 assert ins and ins[-1].address+ins[-1].size==e.EndAddress,(hex(a),'incomplete function')
 for x in ins:
  if not x.id:continue
  if any(o.type==capstone.x86.X86_OP_MEM and o.mem.base==capstone.x86.X86_REG_RIP for o in x.operands):mask[x.address-a+x.disp_offset:x.address-a+x.disp_offset+4]=bytes(4)
  if x.group(capstone.CS_GRP_BRANCH_RELATIVE) and not a<=x.operands[0].imm<e.EndAddress:mask[x.address-a+x.imm_offset:x.address-a+x.imm_offset+x.imm_size]=bytes(x.imm_size)
 for block in getattr(pe,'DIRECTORY_ENTRY_BASERELOC',[]):
  for reloc in block.entries:
   if a<=reloc.rva<e.EndAddress and reloc.type==10:mask[reloc.rva-a:reloc.rva-a+8]=bytes(8)
 # Use a unique short window, retaining the full-function fingerprint separately.
 locator=None
 for length in [64,96,128,192,256,512]:
  for offset in [0,len(raw)//3,len(raw)//2]:
   n=min(length,len(raw)-offset)
   if n<16:continue
   pat=b''.join(re.escape(bytes([v])) if m else b'.' for v,m in zip(raw[offset:offset+n],mask[offset:offset+n]))
   hits=list(re.finditer(pat,code,re.DOTALL))
   if len(hits)==1:locator=(offset,' '.join(f'{v:02x}' if m else '??' for v,m in zip(raw[offset:offset+n],mask[offset:offset+n])));break
  if locator:break
 assert locator,(hex(a),len(raw),group)
 zeros=[i for i,v in enumerate(mask) if not v]
 rows.append('{0x%x,%d,%d,%d,"%s",0x%xULL,{%s}}'%(a,group,len(raw),locator[0],locator[1],fnv(bytes(v if m else 0 for v,m in zip(raw,mask))),','.join(map(str,zeros))))
out='// Generated compatibility fingerprints; regenerate only after native ABI validation.\n'
out+='inline const FunctionSpec function_specs[]={\n'+',\n'.join(rows)+'\n};\n'
out+='inline const Binding binding_specs[]={\n'+',\n'.join('{0x%x,0x%x,%d,%d,%d,%d,%s}'%(*b[:-1],'true' if b[-1] else 'false') for b in bindings)+'\n};\n'
(root/'runtime/compatibility_data.h').write_text(out)
print('Generated',len(rows),'function fingerprints,',len(bindings),'bindings,',len(dependencies),'native addresses')
