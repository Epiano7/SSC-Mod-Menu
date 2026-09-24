.text
.globl combat_test_call
.def combat_test_call; .scl 2; .type 32; .endef
.seh_proc combat_test_call
combat_test_call:
pushq %rbx
.seh_pushreg %rbx
pushq %rsi
.seh_pushreg %rsi
pushq %rdi
.seh_pushreg %rdi
subq $0x20, %rsp
.seh_stackalloc 0x20
.seh_endprologue
movq %r8, %rsi
movq %r9, %rbx
movq 0x60(%rsp), %rdi
call ssc_combat_hit_bridge
addq $0x20, %rsp
popq %rdi
popq %rsi
popq %rbx
ret
.seh_endproc
