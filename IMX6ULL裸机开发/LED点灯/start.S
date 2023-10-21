.text
.global _start
_start:

    ldr sp, =0x80100000
    bl main