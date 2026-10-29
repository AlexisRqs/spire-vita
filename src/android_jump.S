.syntax unified
.arch armv7-a
.fpu vfpv3-d16
.arm
.text
/* Public buffer: 64 ARM32 words, at least word aligned. Private bridge layout:
 * 0 magic; 4 save-mask flag; 8 mask[8]; 16 d8-d15[64]; 80 FPSCR;
 * 84 r4-r11[32]; 116 SP; 120 LR. Reserved bytes 124..255 untouched.
 * No C wrapper may surround the saved continuation. */
.global android_setjmp
.type android_setjmp,%function
android_setjmp:
 mov r1,#1
 b android_sigsetjmp
.size android_setjmp,.-android_setjmp
.global android__setjmp
.type android__setjmp,%function
android__setjmp:
 mov r1,#0
 b android_sigsetjmp
.size android__setjmp,.-android__setjmp
.global android_sigsetjmp
.type android_sigsetjmp,%function
android_sigsetjmp:
 movw r2,#0x4d50
 movt r2,#0x564a
 str r2,[r0]
 cmp r1,#0
 movne r1,#1
 str r1,[r0,#4]
 add r2,r0,#84
 stmia r2,{r4-r11,sp,lr}
 add r2,r0,#16
 vstmia r2,{d8-d15}
 vmrs r2,fpscr
 str r2,[r0,#80]
 cmp r1,#0
 beq 1f
 push {r0,lr}
 add r0,r0,#8
 bl bionic_jump_mask_get
 pop {r0,lr}
1:
 mov r0,#0
 bx lr
.size android_sigsetjmp,.-android_sigsetjmp
.global android_longjmp
.global android_siglongjmp
.type android_longjmp,%function
.type android_siglongjmp,%function
android_longjmp:
android_siglongjmp:
 movw r2,#0x4d50
 movt r2,#0x564a
 ldr r3,[r0]
 cmp r2,r3
 bne bionic_jump_bad_env
 ldr r2,[r0,#4]
 cmp r2,#0
 beq 2f
 push {r0-r2,lr}
 add r0,r0,#8
 bl bionic_jump_mask_set
 pop {r0-r2,lr}
2:
 add r2,r0,#16
 vldmia r2,{d8-d15}
 ldr r2,[r0,#80]
 vmsr fpscr,r2
 add r2,r0,#84
 ldmia r2,{r4-r11,sp,lr}
 cmp r1,#0
 moveq r1,#1
 mov r0,r1
 bx lr
.size android_longjmp,.-android_longjmp
.size android_siglongjmp,.-android_siglongjmp
.section .note.GNU-stack,"",%progbits
