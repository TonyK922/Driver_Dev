
//#define STACK_BASE (0xc0000000 + 0x100000)  // stm32mp157
#define STACK_BASE (0x80000000 + 0x100000)  // imx6ull
#define STACK_SIZE (2048)


.text
.global  _start
_start: 				
    b	reset
	ldr	pc, =do_undefined
	ldr	pc, =do_svc  // ldr	pc, _software_interrupt
	.word 0  // ldr	pc, _prefetch_abort
	.word 0  // ldr	pc, _data_abort
	.word 0  // ldr	pc, _not_used
	ldr	pc, =do_irq // ldr	pc, _irq
	.word 0  // ldr	pc, _fiq

reset:
	/* 设置sp */
	/* 对于STM32MP157设置链接地址为0xC0200000, 对于IMX6ULL设为0x80200000 */
	ldr sp, =STACK_BASE

	adr r0, _start
	bl SystemInit

	bl uart_init

	/* 设置异常向量表基地址 : VBAR */
	ldr r0, =_start
	mcr p15, 0, r0, c12, c0, 0

	.word 0xffffffff
	
	svc #1

	CPSIE I  // 清除I位，使能中断
	
	/* 调用main函数 */
	//bl main
	ldr pc, =main

do_undefined:
	/* 设置SP_und */
	ldr sp, =STACK_BASE - STACK_SIZE
	
	/* 保存现场 */
	stmdb sp!, {R0-R3,R12,LR}

	/* 调用处理函数 */
	bl do_undefined_c

	/* 恢复现场 */
	ldmia sp!, {R0-R3,R12,PC}^


do_svc:
	/* 设置SP_und */
	ldr sp, =STACK_BASE - STACK_SIZE - STACK_SIZE
	
	/* 保存现场 */
	stmdb sp!, {R0-R3,R12,LR}

	/* 调用处理函数 */
	bl do_svc_c

	/* 恢复现场 */
	ldmia sp!, {R0-R3,R12,PC}^

do_irq:
		/* 设置SP_irq */
		ldr sp, =STACK_BASE - STACK_SIZE - STACK_SIZE - STACK_SIZE
		
		/* 保存现场 */
		subs lr, lr, #4
		stmdb sp!, {R0-R3,R12,LR}
		
		/* 调用处理函数 */
		bl do_irq_c
		
		/* 恢复现场 */
		ldmia sp!, {R0-R3,R12,PC}^


