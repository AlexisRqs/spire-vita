.syntax unified
.thumb
.fpu neon
.text
.global text_original
.type text_original,%function
.thumb_func
text_original:
 push {r4-r10,lr}
 vpush {d8-d12}
 ldr ip, =text_original_resume
 ldr ip, [ip]
 bx ip
.section .note.GNU-stack,"",%progbits
