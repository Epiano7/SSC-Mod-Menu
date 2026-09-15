"""Generate ABI-preserving x64 forwarding stubs from the local Windows GL exports.
Requires pefile; no Windows DLL is copied or packaged.
"""
import sys
from pathlib import Path
import pefile

source, output = Path(sys.argv[1]), Path(sys.argv[2])
output.mkdir(parents=True, exist_ok=True)
exports = sorted((s for s in pefile.PE(str(source)).DIRECTORY_ENTRY_EXPORT.symbols if s.name), key=lambda s:s.ordinal)
names = [s.name.decode('ascii') for s in exports]
(output/'proxy_names.h').write_text('static const char* names[] = {\n'+''.join('"'+n+'",\n' for n in names)+'};\n')
definition = ['LIBRARY opengl32', 'EXPORTS']
assembly = ['.text']
for i,s in enumerate(exports):
    definition.append(f'    {s.name.decode()}=forward_{i} @{s.ordinal}')
    assembly += [f'.globl forward_{i}',f'.seh_proc forward_{i}',f'forward_{i}:',
        'subq $136, %rsp', '.seh_stackalloc 136', '.seh_endprologue', 'movq %rcx, 32(%rsp)', 'movq %rdx, 40(%rsp)',
        'movq %r8, 48(%rsp)', 'movq %r9, 56(%rsp)',
        'movdqu %xmm0, 64(%rsp)', 'movdqu %xmm1, 80(%rsp)',
        'movdqu %xmm2, 96(%rsp)', 'movdqu %xmm3, 112(%rsp)',
        f'movl ${i}, %ecx','call proxy_resolve','movq %rax, %r11',
        'movq 32(%rsp), %rcx','movq 40(%rsp), %rdx','movq 48(%rsp), %r8','movq 56(%rsp), %r9',
        'movdqu 64(%rsp), %xmm0','movdqu 80(%rsp), %xmm1','movdqu 96(%rsp), %xmm2','movdqu 112(%rsp), %xmm3',
        'addq $136, %rsp','jmp *%r11','.seh_endproc']
(output/'proxy.def').write_text('\n'.join(definition)+'\n')
(output/'proxy_stubs.S').write_text('\n'.join(assembly)+'\n')
print(f'Generated {len(exports)} forwarding stubs')
