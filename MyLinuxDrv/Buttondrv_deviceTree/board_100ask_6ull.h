#ifndef BORAD_100ASK_6ULL_H
#define BORAD_100ASK_6ULL_H

#define CCM_ADDR_BASE 0x020C4000
#define CCM_CCGR1_ADDR (CCM_ADDR_BASE + 0x6c)
#define CCM_CCGR3_ADDR (CCM_ADDR_BASE + 0x74) /* GPIO4 enable*/

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

#define GROUP(x) (x >> 16)
#define PIN(x) (x & 0xffff)
#define GROUP_PIN(g,p) ((g<<16)|(p))

struct iomux
{
    // volatile unsigned int unnames[23];
    volatile unsigned int IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO00; /* offset 0x5c */
    volatile unsigned int IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO01;
    volatile unsigned int IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO02;
    volatile unsigned int IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03;
    volatile unsigned int IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO04;
    volatile unsigned int IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO05;
    volatile unsigned int IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO06;
    volatile unsigned int IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO07;
    volatile unsigned int IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO08;
    volatile unsigned int IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO09;
    volatile unsigned int IOMUXC_SW_MUX_CTL_PAD_UART1_TX_DATA;
    volatile unsigned int IOMUXC_SW_MUX_CTL_PAD_UART1_RX_DATA;
    volatile unsigned int IOMUXC_SW_MUX_CTL_PAD_UART1_CTS_B;
};

struct button_operations {
	int (*init) (int which); /* 初始化LED, which-哪个LED */       
	int (*ctl) (int which); /* 控制LED, which-哪个LED, status:1-亮,0-灭 */
};

struct button_operations *get_board_button_opr(void);

#endif // !BORAD_100ASK_6ULL_H
