.text
.global lab_shield_call
lab_shield_call:
    push %rbp
    push %rdi
    sub $584,%rsp
    movdqu %xmm7,384(%rsp)
    movdqu %xmm8,400(%rsp)
    movdqu %xmm9,416(%rsp)
    movdqu %xmm12,432(%rsp)
    movdqu %xmm14,448(%rsp)
    lea 128(%rsp),%rbp
    mov %rcx,%r11
    mov %rdx,%rdi
    lea 0x1390(%rdi),%rcx
    movss (%rcx),%xmm4
    movaps %xmm2,%xmm8
    xorps %xmm9,%xmm9
    mov $0x3f000000,%eax
    movd %eax,%xmm12
    mov $0x3f800000,%eax
    movd %eax,%xmm14
    call *%r11
    movdqu 384(%rsp),%xmm7
    movdqu 400(%rsp),%xmm8
    movdqu 416(%rsp),%xmm9
    movdqu 432(%rsp),%xmm12
    movdqu 448(%rsp),%xmm14
    add $584,%rsp
    pop %rdi
    pop %rbp
    ret
