.syntax unified
.thumb
.text
.global map_dot_original
.type map_dot_original,%function
.thumb_func
map_dot_original:
 push {r4-r8,lr}
 sub sp, #56
 mov r5,r0
 ldr ip, =map_original_resume
 ldr ip, [ip]
 bx ip
.section .note.GNU-stack,"",%progbits
