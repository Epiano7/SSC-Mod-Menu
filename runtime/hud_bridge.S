// Preserve native integer/floating arguments and results across scoped HUD transforms.
.text
.globl ssc_hud_bridge
.def ssc_hud_bridge; .scl 2; .type 32; .endef
.seh_proc ssc_hud_bridge
ssc_hud_bridge:
 subq $0x228, %rsp
 .seh_stackalloc 0x228
 .seh_endprologue
 movq %rax, 0xc0(%rsp)
 movq %rcx, 0xc8(%rsp)
 movq %rdx, 0xd0(%rsp)
 movq %r8, 0xd8(%rsp)
 movq %r9, 0xe0(%rsp)
 movq %r10, 0xe8(%rsp)
 movq %r11, 0xf0(%rsp)
 movdqu %xmm0, 0x100(%rsp)
 movdqu %xmm1, 0x110(%rsp)
 movdqu %xmm2, 0x120(%rsp)
 movdqu %xmm3, 0x130(%rsp)
 movdqu %xmm4, 0x140(%rsp)
 movdqu %xmm5, 0x150(%rsp)
 pushfq
 popq %rax
 movq %rax, 0xf8(%rsp)
 movl %r10d, %ecx
 leaq 0x180(%rsp), %rdx
 call ssc_hud_before
 movq %rax, 0x168(%rsp)
 movl 0x180(%rsp), %ecx
 xorl %eax, %eax
1:
 cmpl %ecx, %eax
 jae 2f
 movq 0x250(%rsp,%rax), %rdx
 movq %rdx, 0x20(%rsp,%rax)
 addl $8, %eax
 jmp 1b
2:
 movq 0xf8(%rsp), %rax
 pushq %rax
 popfq
 movdqu 0x100(%rsp), %xmm0
 movdqu 0x110(%rsp), %xmm1
 movdqu 0x120(%rsp), %xmm2
 movdqu 0x130(%rsp), %xmm3
 movdqu 0x140(%rsp), %xmm4
 movdqu 0x150(%rsp), %xmm5
 movq 0xc0(%rsp), %rax
 movq 0xc8(%rsp), %rcx
 movq 0xd0(%rsp), %rdx
 movq 0xd8(%rsp), %r8
 movq 0xe0(%rsp), %r9
 movq 0xe8(%rsp), %r10
 movq 0xf0(%rsp), %r11
 call *0x168(%rsp)
 movq %rax, 0xc0(%rsp)
 movq %rcx, 0xc8(%rsp)
 movq %rdx, 0xd0(%rsp)
 movq %r8, 0xd8(%rsp)
 movq %r9, 0xe0(%rsp)
 movq %r10, 0xe8(%rsp)
 movq %r11, 0xf0(%rsp)
 movdqu %xmm0, 0x100(%rsp)
 movdqu %xmm1, 0x110(%rsp)
 movdqu %xmm2, 0x120(%rsp)
 movdqu %xmm3, 0x130(%rsp)
 movdqu %xmm4, 0x140(%rsp)
 movdqu %xmm5, 0x150(%rsp)
 pushfq
 popq %rax
 movq %rax, 0xf8(%rsp)
 leaq 0x180(%rsp), %rcx
 call ssc_hud_after
 movq 0xf8(%rsp), %rax
 pushq %rax
 popfq
 movdqu 0x100(%rsp), %xmm0
 movdqu 0x110(%rsp), %xmm1
 movdqu 0x120(%rsp), %xmm2
 movdqu 0x130(%rsp), %xmm3
 movdqu 0x140(%rsp), %xmm4
 movdqu 0x150(%rsp), %xmm5
 movq 0xc0(%rsp), %rax
 movq 0xc8(%rsp), %rcx
 movq 0xd0(%rsp), %rdx
 movq 0xd8(%rsp), %r8
 movq 0xe0(%rsp), %r9
 movq 0xe8(%rsp), %r10
 movq 0xf0(%rsp), %r11
 addq $0x228, %rsp
 ret
.seh_endproc
