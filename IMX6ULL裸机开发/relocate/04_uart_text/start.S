
.text
.global  _start
_start: 	

        /* 设置sp */
	ldr sp, =(0x80000000+0x100000)

	/* text/rodata/data重定位 */
	ldr r0, =_start

	adr r1, _start  /* 源 */

	ldr r3, =__bss_start
	sub r2, r3, r0

	bl memcpy    /* r0: 目的, r1: 源, r2：长度 */

	/* clear bss */
	ldr r0, =__bss_start
	mov r1, #0
	ldr r2, =__bss_end
	sub r2, r2, r0
	bl memset   /* r0: dest, r1: val(0), r2: len */
	
        /* 调用main函数 */
	//bl main
	ldr pc, =main

