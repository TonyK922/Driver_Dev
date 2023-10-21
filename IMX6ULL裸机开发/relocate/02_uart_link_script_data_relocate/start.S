
.text
.global  _start
_start: 	

        /* 设置sp */
	ldr sp, =(0x80000000+0x100000)

	/* rodata/data重定位 */
	ldr r0, =__rodata_start

	ldr r2, =_start  /* link addr */
	adr r3, _start   /* load addr */
	sub r2, r2, r3
	sub r1, r0, r2   /* 源 */

	ldr r3, =__bss_start
	sub r2, r3, r0

	bl memcpy    /* r0: 目的, r1: 源, r2：长度 */

        /* 调用main函数 */
	bl main

