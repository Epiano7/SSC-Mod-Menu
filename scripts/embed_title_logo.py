"""Embed the title PNG; run after replacing runtime/assets/larpshot-logo.png."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]/'runtime/assets'
data=(root/'larpshot-logo.png').read_bytes()
assert data.startswith(b'\x89PNG\r\n\x1a\n')
(root/'larpshot_logo_data.h').write_text('// Generated from larpshot-logo.png; retain the PNG as the editable source.\n#pragma once\ninline const unsigned char larpshot_png[]={\n'+''.join(','.join(str(x) for x in data[i:i+32])+',\n' for i in range(0,len(data),32))+'};\n')
