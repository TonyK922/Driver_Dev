#include "uart1.h"

/*根据IMX6ULL芯片手册<<55.15 UART Memory Map/Register
 * Definition>>的3608页，定义UART的结构体,*/
typedef struct
{
    volatile unsigned int URXD; /**< UART Receiver Register, offset:
                                   0x0串口接收寄存器，偏移地址0x0     */
    unsigned char         RESERVED_0[60];
    volatile unsigned int UTXD; /**< UART Transmitter Register, offset: 0x40
                                   串口发送寄存器，偏移地址0x40*/
    unsigned char         RESERVED_1[60];
    volatile unsigned int UCR1; /**< UART Control Register 1, offset: 0x80
                                   串口控制寄存器1，偏移地址0x80*/
    volatile unsigned int UCR2; /**< UART Control Register 2, offset:
                                   0x84串口控制寄存器2，偏移地址0x84*/
    volatile unsigned int UCR3; /**< UART Control Register 3, offset:
                                   0x88串口控制寄存器3，偏移地址0x88*/
    volatile unsigned int UCR4; /**< UART Control Register 4, offset:
                                   0x8C串口控制寄存器4，偏移地址0x8C*/
    volatile unsigned int UFCR; /**< UART FIFO Control Register, offset:
                                   0x90串口FIFO控制寄存器，偏移地址0x90*/
    volatile unsigned int USR1; /**< UART Status Register 1, offset:
                                   0x94串口状态寄存器1，偏移地址0x94*/
    volatile unsigned int USR2; /**< UART Status Register 2, offset:
                                   0x98串口状态寄存器2，偏移地址0x98*/
    volatile unsigned int UESC; /**< UART Escape Character Register, offset:0x9C
                                   串口转义字符寄存器，偏移地址0x9C*/
    volatile unsigned int UTIM; /**< UART Escape Timer Register, offset:
                                   0xA0串口转义定时器寄存器 偏移地址0xA0*/
    volatile unsigned int UBIR; /**< UART BRM Incremental Register, offset:
                                   0xA4串口二进制倍率增加寄存器 偏移地址0xA4*/
    volatile unsigned int UBMR; /**< UART BRM Modulator Register, offset:
                                   0xA8串口二进制倍率调节寄存器 偏移地址0xA8*/
    volatile unsigned int UBRC; /**< UART Baud Rate Count Register, offset:
                                   0xAC串口波特率计数寄存器 偏移地址0xAC*/
    volatile unsigned int ONEMS; /**< UART One Millisecond Register, offset:0xB0
                                    串口一毫秒寄存器 偏移地址0xB0*/
    volatile unsigned int
        UTS; /**< UART Test Register, offset: 0xB4 串口测试寄存器 偏移地址0xB4*/
    volatile unsigned int
        UMCR; /**< UART RS-485 Mode Control Register, offset:0xB8
                 串口485模式控制寄存器 偏移地址0xB8*/
} UART_Type;

UART_Type* uart1 = (UART_Type*)0x02020000;

void uart_init()
{
    volatile unsigned int* IOMUXC_SW_MUX_CTL_PAD_UART1_TX_DATA;
    volatile unsigned int* IOMUXC_SW_MUX_CTL_PAD_UART1_RX_DATA;
    volatile unsigned int* IOMUXC_UART1_RX_DATA_SELECT_INPUT;
    volatile unsigned int* CCM_CSCDR1;
    volatile unsigned int* CCM_CCGR5;

    IOMUXC_SW_MUX_CTL_PAD_UART1_TX_DATA = (volatile unsigned*)(0x020E0084);
    IOMUXC_SW_MUX_CTL_PAD_UART1_RX_DATA = (volatile unsigned*)(0x020E0088);
    IOMUXC_UART1_RX_DATA_SELECT_INPUT   = (volatile unsigned*)(0x020E0624);
    CCM_CSCDR1                          = (volatile unsigned*)(0x020C4024);
    CCM_CCGR5                           = (volatile unsigned*)(0x020C407C);

    /* 设置 UART 总时钟
     * UART_CLK_ROOT = 80MHz
     */
    *CCM_CSCDR1 &= ~((1 << 6) | (0x3f));

    /*给UART模块提供时钟
     *UART1_CLK_ENABLE
     */
    *CCM_CCGR5 |= (3 << 24);

    /*配置引脚功能*/
    *IOMUXC_SW_MUX_CTL_PAD_UART1_TX_DATA &= ~0xf;
    *IOMUXC_SW_MUX_CTL_PAD_UART1_RX_DATA &= ~0xf;

    /*6ull 特殊的设置*/
    *IOMUXC_UART1_RX_DATA_SELECT_INPUT |= 3;

    /* 设置波特率
     * 115200 = 80M/(16*(UBMR+1)/(UBIR+1))
     * UBIR = 15 那么 UBMR = 694 - 1 = 693
     * 实际波特率 就是 115274
     * 先设置 UBIR 再 UBMR
     */
    uart1->UFCR |= (5 << 7);
    uart1->UBIR = 15;
    uart1->UBMR = 693;

    /*设置数据格式*/
    uart1->UCR2 |=
        (1 << 14) | (0 << 8) | (0 << 6) | (1 << 5) | (1 << 2) | (1 << 1);

    /* 6ULL 芯片要求的必设配置*/
    uart1->UCR3 |= (1 << 2);

    /*使能 UART*/
    uart1->UCR1 |= (1 << 0);
}

int getchar()
{
    UART_Type* UART1 = (UART_Type*)0x02020000;
    while ((UART1->USR2 & (1 << 0)) == 0)
        ;
    return UART1->URXD;
}
int putchar(char Ch)
{
    UART_Type* UART1 = (UART_Type*)0x02020000;
    while ((UART1->USR2 & (1 << 3)) == 0)
        ;
    UART1->UTXD = Ch;
    return Ch;
}
