.text
.globl test_score_call
.seh_proc test_score_call
test_score_call:
    pushq %rdi
    .seh_pushreg %rdi
    subq $0x70,%rsp
    .seh_stackalloc 0x70
    .seh_endprologue
    movq 0xf0(%rsp),%rdi
    movq 0xa0(%rsp),%rax
    movq %rax,0x20(%rsp)
    movq 0xa8(%rsp),%rax
    movq %rax,0x28(%rsp)
    movq 0xb0(%rsp),%rax
    movq %rax,0x30(%rsp)
    movq 0xb8(%rsp),%rax
    movq %rax,0x38(%rsp)
    movq 0xc0(%rsp),%rax
    movq %rax,0x40(%rsp)
    movq 0xc8(%rsp),%rax
    movq %rax,0x48(%rsp)
    movq 0xd0(%rsp),%rax
    movq %rax,0x50(%rsp)
    movq 0xd8(%rsp),%rax
    movq %rax,0x58(%rsp)
    movq 0xe0(%rsp),%rax
    movq %rax,0x60(%rsp)
    movq 0xe8(%rsp),%rax
    movq %rax,0x68(%rsp)
    call ssc_score_bridge
    addq $0x70,%rsp
    popq %rdi
    ret
.seh_endproc
