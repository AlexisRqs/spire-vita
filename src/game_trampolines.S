.syntax unified
.thumb
.fpu neon
.text
.macro RESUME index
 ldr ip, =game_resumes
 ldr ip, [ip, #(\index*4)]
 bx ip
.endm
.global game_original_create
.type game_original_create,%function
.thumb_func
game_original_create:
 push {r4-r11,lr}
 sub sp, #4
 vpush {d8}
 RESUME 0
.global game_original_room
.type game_original_room,%function
.thumb_func
game_original_room:
 push {r4-r11,lr}
 sub sp, #76
 mov r10,r0
 RESUME 1
.global game_original_character
.type game_original_character,%function
.thumb_func
game_original_character:
 push {r4-r8,lr}
 mov r8,r0
 ldr r0, =game_character_literal
 ldr r0, [r0]
 RESUME 2
.global game_original_map
.type game_original_map,%function
.thumb_func
game_original_map:
 push {r4-r6,lr}
 vpush {d8}
 mov r4,r0
 RESUME 3
.global game_original_reward
.type game_original_reward,%function
.thumb_func
game_original_reward:
 push {r4-r11,lr}
 sub sp, #4
 vpush {d8}
 RESUME 4
.section .note.GNU-stack,"",%progbits
