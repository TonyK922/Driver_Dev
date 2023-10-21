
.text
.global  _start
_start: 	

        /* 设置sp */
	ldr sp, =(0x80000000+0x100000)

	adr r0, _start
	bl SystemInit
	
        /* 调用main函数 */
	//bl main
	ldr pc, =main

