// Exact-build scoreboard name call sites keep the identity-bearing row in RDI.
// Forward all 14 native arguments and pass that row as argument 15. Windows x64
// unwind information covers the extra frame; nonvolatile registers are untouched.
.text
.globl ssc_score_bridge
.def ssc_score_bridge; .scl 2; .type 32; .endef
.seh_proc ssc_score_bridge
ssc_score_bridge:
    subq $0x88, %rsp
    .seh_stackalloc 0x88
    .seh_endprologue
    movq 0xb0(%rsp), %rax
    movq %rax, 0x20(%rsp)
    movq 0xb8(%rsp), %rax
    movq %rax, 0x28(%rsp)
    movq 0xc0(%rsp), %rax
    movq %rax, 0x30(%rsp)
    movq 0xc8(%rsp), %rax
    movq %rax, 0x38(%rsp)
    movq 0xd0(%rsp), %rax
    movq %rax, 0x40(%rsp)
    movq 0xd8(%rsp), %rax
    movq %rax, 0x48(%rsp)
    movq 0xe0(%rsp), %rax
    movq %rax, 0x50(%rsp)
    movq 0xe8(%rsp), %rax
    movq %rax, 0x58(%rsp)
    movq 0xf0(%rsp), %rax
    movq %rax, 0x60(%rsp)
    movq 0xf8(%rsp), %rax
    movq %rax, 0x68(%rsp)
    movq %rdi, 0x70(%rsp)
    call ssc_score_draw
    addq $0x88, %rsp
    ret
.seh_endproc
