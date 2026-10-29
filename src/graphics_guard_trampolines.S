.syntax unified
.thumb
.fpu neon
.text
/* Unknown signature: preserve GPR arguments, stack arguments, VFP arguments and
 * original LR, then tail-call the exact previously resolved target. 88 bytes
 * preserves the caller's 8-byte stack alignment during the C notification. */
.type graphics_guard_trampoline,%function
.thumb_func
graphics_guard_trampoline:
 push {r0-r3,ip,lr}
 vpush {d0-d7}
 mov r0,ip
 bl graphics_guard_hit
 str r0,[sp,#80]
 vpop {d0-d7}
 pop {r0-r3,ip,lr}
 bx ip
.size graphics_guard_trampoline,.-graphics_guard_trampoline

.altmacro
.macro guard_entry number
.type graphics_guard_entry_\number,%function
.thumb_func
graphics_guard_entry_\number:
 movw ip,#\number
 b.w graphics_guard_trampoline
.endm
.set guard_number,0
.rept 256
 guard_entry %guard_number
 .set guard_number,guard_number+1
.endr
.section .rodata
.align 2
.global graphics_guard_entries
.type graphics_guard_entries,%object
graphics_guard_entries:
.macro guard_pointer number
 .word graphics_guard_entry_\number
.endm
.set guard_number,0
.rept 256
 guard_pointer %guard_number
 .set guard_number,guard_number+1
.endr
.size graphics_guard_entries,.-graphics_guard_entries
.section .note.GNU-stack,"",%progbits
