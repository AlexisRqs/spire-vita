.syntax unified
.thumb
.text
.align 2
.global buffer_pointer_original
.type buffer_pointer_original,%function
.thumb_func
buffer_pointer_original:
 push {r4,r5,r7,lr}
 add.w r4,r0,#20
 mov r5,r0
 ldr ip, =buffer_pointer_resume
 ldr ip, [ip]
 bx ip
.section .note.GNU-stack,"",%progbits
