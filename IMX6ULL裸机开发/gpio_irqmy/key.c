
#define CCM_ADDR_BASE 0x020C4000
#define CCM_CCGR1 (CCM_ADDR_BASE + 0x6c)
#define CCM_CCGR3 (CCM_ADDR_BASE + 0x74) /* GPIO4 enable*/

#define IOMUXC_SNVS_TAMPER3 0x02290014
#define IOMUXC_SNVS_TAMPER1 0x0229000C /*GPIO5_IO01 enable value: 101*/
#define IOMUXC_NAND_CE1_B 0x020E01B0   /*GPIO4_IO14 enable value: 0101*/

#define GPIO4_ADDR_BASE 0x020A8000
#define GPIO4_DR (GPIO4_ADDR_BASE + 0x0)
#define GPIO4_GDIR (GPIO4_ADDR_BASE + 0x4)
#define GPIO4_PSR (GPIO4_ADDR_BASE + 0x8)

#define GPIO5_ADDR_BASE 0x020AC000
#define GPIO5_DR (GPIO5_ADDR_BASE + 0x0)
#define GPIO5_GDIR (GPIO5_ADDR_BASE + 0x4)
#define GPIO5_PSR (GPIO5_ADDR_BASE + 0x8)

#include "key.h"
#include "gic.h"
#include "string.h"

typedef struct
{
    volatile unsigned DR;
    volatile unsigned GDIR;
    volatile unsigned PSR;
    volatile unsigned ICR1;
    volatile unsigned ICR2;
    volatile unsigned IMR;
    volatile unsigned ISR;
    volatile unsigned EDGE_SEL;

} GPIO_Type;

void key_init(void)
{
    /* 1.配置GPIO04_IO14引脚为输入/中断引脚 */

    volatile unsigned* pRegKEY;
    /* 使能 gpio 5 6ull 默认使能*/
    volatile unsigned* pRegKEY1;

    GPIO_Type* gpio4 = (GPIO_Type*)(GPIO4_ADDR_BASE);
    GPIO_Type* gpio5 = (GPIO_Type*)(GPIO5_ADDR_BASE);

    /*使能 GPIO4 CCM_CCGR3 b[13:12] = 0b11*/
    pRegKEY = (volatile unsigned*)(CCM_CCGR3);
    *pRegKEY |= (3 << 12);

    /*配置 GPIO4_IO14 为GPIO*/
    pRegKEY = (volatile unsigned*)(IOMUXC_NAND_CE1_B);
    *pRegKEY &= ~(0xf);
    *pRegKEY |= (0x5);

    /*配置GPIO5_1 为GPIO*/
    pRegKEY1 = (volatile unsigned*)(IOMUXC_SNVS_TAMPER1);
    *pRegKEY1 &= ~(0xf);
    *pRegKEY1 |= (0x5);

    /*GPIO4_14 设置为输入引脚*/
    // pRegKEY = (volatile unsigned*)(GPIO4_GDIR);
    //*pRegKEY &= ~(1 << 14);
    gpio4->GDIR &= ~(1 << 14);

    /*GPIO5_1 设置为输入引脚*/
    // pRegKEY1 = (volatile unsigned*)(GPIO5_GDIR);
    //*pRegKEY1 &= ~(1 << 1);
    gpio5->GDIR &= ~(1 << 1);

    /*GPIO4_DR */
    pRegKEY = (volatile unsigned*)(GPIO4_DR);

    /*GPIO5_DR*/
    pRegKEY1 = (volatile unsigned*)(GPIO5_DR);

    /* 2.设置中断触发方式: 双边沿触发 */
    /* GPIO4_EDGE_SEL */
    gpio4->EDGE_SEL |= (1 << 14);
    gpio5->EDGE_SEL |= (1 << 1);

    gpio4->ISR |= (1 << 14);
    clear_gic_irq(IRQ_GPIO4_0_15);
    gpio5->ISR |= (1 << 1);
    clear_gic_irq(IRQ_GPIO5_0_15);

    /* 3.使能中断 */
    gpio4->IMR |= (1 << 14);
    gpio5->IMR |= (1 << 1);

    gic_enable_irq(IRQ_GPIO4_0_15);
    gic_enable_irq(IRQ_GPIO5_0_15);
}

void do_irq_c(void)
{
    GPIO_Type* gpio4 = (GPIO_Type*)(GPIO4_ADDR_BASE);
    GPIO_Type* gpio5 = (GPIO_Type*)(GPIO5_ADDR_BASE);

    int irq;

    /* 1.分辨中断 */
    irq = get_gic_irq();

    /* 2.调用处理函数 */
    if (irq == IRQ_GPIO4_0_15)
    {
        if (gpio4->DR & (1 << 14))
            puts("KEY2 released !\r\n");
        else
        {
            puts("KEY2 pressed !\r\n");
        }
        gpio4->ISR |= (1 << 14);
    }
    else if (irq == IRQ_GPIO5_0_15)
    {
        if (gpio5->DR & (1 << 1))
            puts("KEY1 released !\r\n");
        else
        {
            puts("KEY1 pressed !\r\n");
        }
        gpio5->ISR |= (1 << 1);
    }

    /* 3.清楚中断 */
    clear_gic_irq(irq);
}