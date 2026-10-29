.syntax unified
.thumb
.fpu neon
.text
.global gameplay_original_0
.type gameplay_original_0,%function
.thumb_func
gameplay_original_0:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x4,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #0]
 bx ip
.global gameplay_original_1
.type gameplay_original_1,%function
.thumb_func
gameplay_original_1:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x2,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #4]
 bx ip
.global gameplay_original_2
.type gameplay_original_2,%function
.thumb_func
gameplay_original_2:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x6,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #8]
 bx ip
.global gameplay_original_3
.type gameplay_original_3,%function
.thumb_func
gameplay_original_3:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x8,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #12]
 bx ip
.global gameplay_original_4
.type gameplay_original_4,%function
.thumb_func
gameplay_original_4:
 .byte 0x2d,0xe9,0xf0,0x41,0x2d,0xed,0x2,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #16]
 bx ip
.global gameplay_original_5
.type gameplay_original_5,%function
.thumb_func
gameplay_original_5:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x6,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #20]
 bx ip

.global gameplay_original_6
.type gameplay_original_6,%function
.thumb_func
gameplay_original_6:
 .byte 0x2d,0xe9,0xf0,0x41,0x82,0xb0,0x80,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #24]
 bx ip
.global gameplay_original_7
.type gameplay_original_7,%function
.thumb_func
gameplay_original_7:
 .byte 0x2d,0xe9,0xf0,0x4f,0x8f,0xb0,0x82,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #28]
 bx ip
.global gameplay_original_8
.type gameplay_original_8,%function
.thumb_func
gameplay_original_8:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x2,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #32]
 bx ip
.global gameplay_original_9
.type gameplay_original_9,%function
.thumb_func
gameplay_original_9:
 .byte 0x2d,0xe9,0xf0,0x47,0x88,0xb0,0x80,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #36]
 bx ip

.global gameplay_original_10
.type gameplay_original_10,%function
.thumb_func
gameplay_original_10:
 .byte 0x2d,0xe9,0xf0,0x4f,0x85,0xb0,0x5,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #40]
 bx ip
.global gameplay_original_11
.type gameplay_original_11,%function
.thumb_func
gameplay_original_11:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0xc,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #44]
 bx ip
.global gameplay_original_12
.type gameplay_original_12,%function
.thumb_func
gameplay_original_12:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x2,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #48]
 bx ip
.global gameplay_original_13
.type gameplay_original_13,%function
.thumb_func
gameplay_original_13:
 .byte 0x2d,0xe9,0xf0,0x41,0x8a,0xb0,0x5,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #52]
 bx ip
.global gameplay_original_14
.type gameplay_original_14,%function
.thumb_func
gameplay_original_14:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x8,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #56]
 bx ip

.global gameplay_original_15
.type gameplay_original_15,%function
.thumb_func
gameplay_original_15:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x6,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #60]
 bx ip
.global gameplay_original_16
.type gameplay_original_16,%function
.thumb_func
gameplay_original_16:
 .byte 0x2d,0xe9,0xf0,0x4f,0x8d,0xb0,0x80,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #64]
 bx ip
.global gameplay_original_17
.type gameplay_original_17,%function
.thumb_func
gameplay_original_17:
 .byte 0x2d,0xe9,0xf0,0x43,0x8d,0xb0,0x80,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #68]
 bx ip
.global gameplay_original_18
.type gameplay_original_18,%function
.thumb_func
gameplay_original_18:
 .byte 0x2d,0xe9,0xf0,0x4f,0x89,0xb0,0x5,0x90,0x89,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #72]
 bx ip
.global gameplay_original_19
.type gameplay_original_19,%function
.thumb_func
gameplay_original_19:
 .byte 0x2d,0xe9,0xf0,0x43,0x87,0xb0,0x5,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #76]
 bx ip
.global gameplay_original_20
.type gameplay_original_20,%function
.thumb_func
gameplay_original_20:
 .byte 0x2d,0xe9,0xf0,0x4f,0x8f,0xb0,0x7,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #80]
 bx ip
.global gameplay_original_21
.type gameplay_original_21,%function
.thumb_func
gameplay_original_21:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x4,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #84]
 bx ip
.global gameplay_original_22
.type gameplay_original_22,%function
.thumb_func
gameplay_original_22:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x6,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #88]
 bx ip

.global gameplay_original_23
.type gameplay_original_23,%function
.thumb_func
gameplay_original_23:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0xc,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #92]
 bx ip
.global gameplay_original_24
.type gameplay_original_24,%function
.thumb_func
gameplay_original_24:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0xe,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #96]
 bx ip
.global gameplay_original_25
.type gameplay_original_25,%function
.thumb_func
gameplay_original_25:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x2d,0xed,0x8,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #100]
 bx ip
.global gameplay_original_26
.type gameplay_original_26,%function
.thumb_func
gameplay_original_26:
 .byte 0x2d,0xe9,0xf0,0x41,0x2d,0xed,0x2,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #104]
 bx ip

.global gameplay_original_27
.type gameplay_original_27,%function
.thumb_func
gameplay_original_27:
 .byte 0x2d,0xe9,0xf0,0x43,0x81,0xb0,0x2d,0xed,0xe,0x8b
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #108]
 bx ip
.global gameplay_original_28
.type gameplay_original_28,%function
.thumb_func
gameplay_original_28:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x82,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #112]
 bx ip

.global gameplay_original_29
.type gameplay_original_29,%function
.thumb_func
gameplay_original_29:
 .byte 0x2d,0xe9,0xf0,0x4f,0x81,0xb0,0x4,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #116]
 bx ip

.global gameplay_original_30
.type gameplay_original_30,%function
.thumb_func
gameplay_original_30:
 .byte 0x2d,0xe9,0xf0,0x4f,0x85,0xb0,0x80,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #120]
 bx ip
.global gameplay_original_31
.type gameplay_original_31,%function
.thumb_func
gameplay_original_31:
 .byte 0x2d,0xe9,0xf0,0x4f,0x85,0xb0,0x82,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #124]
 bx ip
.global gameplay_original_32
.type gameplay_original_32,%function
.thumb_func
gameplay_original_32:
 .byte 0x2d,0xe9,0xf0,0x43,0x81,0xb0,0x80,0x46
 ldr ip, =gameplay_resumes
 ldr ip, [ip, #128]
 bx ip
.section .note.GNU-stack,"",%progbits
