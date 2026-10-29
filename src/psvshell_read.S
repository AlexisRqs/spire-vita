.arch armv7a
/* Optional read-only PSVshellPlus API. NIDs verified against upstream stub. */
.section .vitalink.fstubs.PSVshellPlus_KernelForUser,"ax",%progbits
.align 4
.global psvsGetClockFrequency
.type psvsGetClockFrequency, %function
psvsGetClockFrequency:
.word 0x00010008
.word 0x0366b343
.word 0x3a7a3a18
.align 4
