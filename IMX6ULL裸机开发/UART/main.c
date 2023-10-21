
#include "uart1.h"

#define CCM_ADDR_BASE 0x020C4000
#define CCM_CCGR1 (CCM_ADDR_BASE + 0x6c)

#define IOMUXC_SNVS_TAMPER3 0x02290014

#define GPIO_ADDR_BASE 0x020AC000
#define GPIO5_GDIR (GPIO_ADDR_BASE + 0x4)
#define GPIO5_DR (GPIO_ADDR_BASE + 0x0)

void delay(volatile unsigned count)
{
    while (count--)
        ;
}
#if 0
int main(void)
{
    volatile unsigned* pReg;
    /* 使能 gpio 5 6ull 默认使能*/

    /* GPIO5_3 设置为 GPIO*/
    pReg = (volatile unsigned*)IOMUXC_SNVS_TAMPER3;
    *pReg |= 0x5;
    /*GPIO5_3 设置为 输出引脚*/
    pReg = (volatile unsigned*)GPIO5_GDIR;
    *pReg |= (1 << 3);

    pReg = (volatile unsigned*)GPIO5_DR;
    while (1)
    {
        *pReg |= (1 << 3);
        delay(1000000);
        *pReg &= ~(1 << 3);
        delay(1000000);
    }
    return 0;
}
#endif

int main(void)
{
    char c;
    uart_init();
    putchar('h');
    putchar('e');
    putchar('l');
    putchar('l');
    putchar('o');
    putchar('\r');
    putchar('\n');

    while (1)
    {
        c = getchar();
        putchar(c);
        putchar(c + 1);
    }
    return 0;
}