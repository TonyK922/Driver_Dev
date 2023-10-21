
#include "gic.h"

#define IRQ_GPIO4_0_15 (72 + 32)

/** GPIO - Register Layout Typedef */
typedef struct {
  volatile unsigned int DR;                                /**< GPIO data register, offset: 0x0 */
  volatile unsigned int GDIR;                              /**< GPIO direction register, offset: 0x4 */
  volatile unsigned int PSR;                               /**< GPIO pad status register, offset: 0x8 */
  volatile unsigned int ICR1;                              /**< GPIO interrupt configuration register1, offset: 0xC */
  volatile unsigned int ICR2;                              /**< GPIO interrupt configuration register2, offset: 0x10 */
  volatile unsigned int IMR;                               /**< GPIO interrupt mask register, offset: 0x14 */
  volatile unsigned int ISR;                               /**< GPIO interrupt status register, offset: 0x18 */
  volatile unsigned int EDGE_SEL;                          /**< GPIO edge select register, offset: 0x1C */
} GPIO_Type;

void key_init(void)
{
	/* 1. 配置GPIO04_IO14为输入/中断 */
	 volatile unsigned int *pRegKey;
	 GPIO_Type *gpio4 = (GPIO_Type *)0x020A8000;
	 
	/* 使能GPIO4, CCM_CCGR3, b[13:12]=0b11 */
	pRegKey = (volatile unsigned int *)(0x020C4074);
	*pRegKey |= (3<<12);

	/* 把GPIO4_14设置为GPIO功能
	 * IOMUXC_SW_MUX_CTL_PAD_NAND_CE1_B   地址：20E_0000h base + 1B0h offset = 0x020E01B0
	 */
	pRegKey = (volatile unsigned int *)(0x020E01B0);
	*pRegKey &= ~(0xf);
	*pRegKey |= (0x5);


	/* 把GPIO4_14设置为输入引脚 
	 * GPIO4_GDIR地址：0x020A8004 
	 */
	gpio4->GDIR &= ~(1<<14);

	/* 2. 设置中断触发方式:双边沿触发 */
	/* GPIO4_EDGE_SEL */
	gpio4->EDGE_SEL |= (1<<14);

	/* 为避免发生错误的中断, 先清除 */
	gpio4->ISR |= (1<<14);
	clear_gic_irq(IRQ_GPIO4_0_15);

	/* 3. 使能中断 */
	gpio4->IMR |= (1<<14);	
	gic_enable_irq(IRQ_GPIO4_0_15);
}

void do_irq_c(void)
{
	int irq;
	GPIO_Type *gpio4 = (GPIO_Type *)0x020A8000;
	
	/* 1. 分辨中断 */
	irq = get_gic_irq();

	/* 2. 调用处理函数 */
	if (irq == IRQ_GPIO4_0_15)
	{
		if (gpio4->DR & (1<<14))
		{
			puts("KEY2 pressed!\n\r");
		}
		else
		{
			puts("KEY2 released!\n\r");
		}
		gpio4->ISR |= (1<<14);
	}

	/* 3. 清除中断 */
	clear_gic_irq(irq);
}

