// #include <stdio.h>
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

void delay(volatile unsigned count)
{
    while (count--)
        ;
}

int main(void)
{
    volatile unsigned* pRegLED;
    volatile unsigned* pRegKEY;
    volatile unsigned* pRegKEY1;
    /* 使能 gpio 5 6ull 默认使能*/

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
    pRegKEY = (volatile unsigned*)(GPIO4_GDIR);
    *pRegKEY &= ~(1 << 14);

    /*GPIO5_1 设置为输入引脚*/
    pRegKEY1 = (volatile unsigned*)(GPIO5_GDIR);
    *pRegKEY1 &= ~(1 << 1);

    /* GPIO5_3 设置为 GPIO*/
    pRegLED = (volatile unsigned*)IOMUXC_SNVS_TAMPER3;
    *pRegLED |= 0x5;
    /*GPIO5_3 设置为 输出引脚*/
    pRegLED = (volatile unsigned*)GPIO5_GDIR;
    *pRegLED |= (1 << 3);

    pRegLED = (volatile unsigned*)GPIO5_DR;

    /*GPIO4_DR */
    pRegKEY = (volatile unsigned*)(GPIO4_DR);

    /*GPIO5_DR*/
    pRegKEY1 = (volatile unsigned*)(GPIO5_DR);

    while (1)
    {
        /*读GPIO4_14 引脚*/
        if ((*pRegKEY & (1 << 14)) == 0 ||
            ((*pRegKEY1 & (1 << 1)) == 0))  // 按下
            *pRegLED &= ~(1 << 3);          // 输出0
        // delay(1000000);
        else                       // 没有按下
            *pRegLED |= (1 << 3);  // 输出1
        // delay(1000000);
    }
    return 0;
}