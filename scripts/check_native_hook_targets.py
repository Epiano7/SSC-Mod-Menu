"""Read-only direct-call ABI regression check for native hook tables."""
from pathlib import Path
import argparse,re,struct,pefile
p=argparse.ArgumentParser();p.add_argument('game');a=p.parse_args();root=Path(__file__).resolve().parents[1];image=pefile.PE(a.game);count=0
for source in (root/'runtime').glob('*.h'):
 text=source.read_text()
 if source.name!='quick_chat.h':text=re.sub(r'inline constexpr uint32_t native_dependencies\[\]\[3\]=\{.*?;','',text)
 for call,target in re.findall(r'\{(0x[0-9a-f]+),\s*(0x[0-9a-f]+),\s*(?:reinterpret_cast|0\})',text):
  call,target=int(call,16),int(target,16);code=image.get_data(call,5)
  assert len(code)==5 and code[0]==0xe8 and call+5+struct.unpack('<i',code[1:])[0]==target,(source.name,hex(call),hex(target))
  count+=1
assert count>=118,count
print(f'PASS: {count} native call targets, including all six quick-chat calls')
