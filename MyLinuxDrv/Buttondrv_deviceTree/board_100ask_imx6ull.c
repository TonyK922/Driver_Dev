#include <linux/module.h>

#include <asm/io.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/major.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/tty.h>

#include "button_drv.h"

static int g_buttonpins[10];
static int g_buttoncnt = 0;

struct imx6ull_gpio
{
    volatile unsigned int dr;
    volatile unsigned int gdir;
    volatile unsigned int psr;
    volatile unsigned int icr1;
    volatile unsigned int icr2;
    volatile unsigned int imr;
    volatile unsigned int isr;
    volatile unsigned int edge_sel;
};

/* enable GPIO4 */
static volatile unsigned int* CCM_CCGR3;

/* enable GPIO5 */
static volatile unsigned int* CCM_CCGR1;

/* set GPIO5_IO01 as GPIO */
static volatile unsigned int* IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1;

/* set GPIO4_IO14 as GPIO */
static volatile unsigned int* IOMUXC_SW_MUX_CTL_PAD_NAND_CE1_B;

static struct imx6ull_gpio* gpio4;
static struct imx6ull_gpio* gpio5;

static int chip_6ull_led_init(int which)
{
    printk("init gpio: group %d, pin %d\n", GROUP(g_buttonpins[which]),
           PIN(g_buttonpins[which]));

    switch (GROUP(g_buttonpins[which])) {
        case 5: {
            printk("init pin of group 5 ...\n");
            if (!CCM_CCGR1) {
                CCM_CCGR1 = ioremap(CCM_CCGR1_ADDR, 4);
                // CCM_CCGR3 = ioremap(IOMUXC_SNVS_TAMPER3, 4);
                //  GPIO5_GDIR          = ioremap(0x020AC004, 4);
                //  GPIO5_DR            = ioremap(0x020AC000, 4);
                IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1 =
                    ioremap(IOMUXC_SNVS_TAMPER1, 4);
                gpio5 = ioremap(GPIO5_ADDR_BASE, sizeof(struct imx6ull_gpio));
            }
            /* 1. enable GPIO5
             * CG15, b[31:30] = 0b11
             */
            *CCM_CCGR1 |= (3 << 30);  // write it or not will be ok

            /* 2. set GPIO5_IO01 as GPIO
             * MUX_MODE, b[3:0] = 0b101
             */
            if (1 == PIN(g_buttonpins[which])) {
                unsigned int val = *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1;
                val &= ~(0xf);
                val |= (0x5);
                *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1 = val;

                /* 3. set GPIO5_IO01 as input
                 * GPIO5 GDIR, b[1] = 0b0
                 */
                gpio5->gdir &= ~(1 << 1);
            }
            break;
        }
        case 4: {
            printk("init pin of group 4 ...\n");
            if (!CCM_CCGR3) {
                CCM_CCGR3 = ioremap(CCM_CCGR3_ADDR, 4);
                //  GPIO5_GDIR          = ioremap(0x020AC004, 4);
                //  GPIO5_DR            = ioremap(0x020AC000, 4);
                IOMUXC_SW_MUX_CTL_PAD_NAND_CE1_B =
                    ioremap(IOMUXC_NAND_CE1_B, 4);
                gpio4 = ioremap(GPIO4_ADDR_BASE, sizeof(struct imx6ull_gpio));
            }
            /* 1. enable GPIO4
             * CG6, b[13:12] = 0b11
             */
            *CCM_CCGR3 |= (3 << 12);  // write it or not will be ok

            /* 2. set GPIO4_IO14 as GPIO
             * MUX_MODE, b[3:0] = 0b101
             */
            if (14 == PIN(g_buttonpins[which])) {
                unsigned int val = *IOMUXC_SW_MUX_CTL_PAD_NAND_CE1_B;
                val &= ~(0xf);
                val |= (0x5);
                *IOMUXC_SW_MUX_CTL_PAD_NAND_CE1_B = val;
                /* 3. set GPIO4_IO14 as input
                 * GPIO4 GDIR, b[14] = 0b0
                 */

                gpio4->gdir &= ~(1 << 14);
            }
            break;
        }
        case 1: {
            printk("init pin of group 1 ...\n");
            break;
        }
        case 2: {
            printk("init pin of group 2 ...\n");
            break;
        }
        case 3: {
            printk("init pin of group 3 ...\n");
            break;
        }
    }

    return 0;
}

static int board_6ull_led_exit(int which)
{
    printk("exit gpio: group %d, pin %d\n", GROUP(g_buttonpins[which]),
           PIN(g_buttonpins[which]));

    if (GROUP(g_buttonpins[which]) == 5) {
        iounmap(CCM_CCGR1);
        iounmap(IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1);
        iounmap(gpio5);
    }
    else if (4 == GROUP(g_buttonpins[which])) {
        iounmap(CCM_CCGR3);
        iounmap(IOMUXC_SW_MUX_CTL_PAD_NAND_CE1_B);
        iounmap(gpio4);
    }
    return 0;
}

static int chip_6ull_led_ctl(int which)
{
    // printk("%s %s line %d, led %d, %s\n", __FILE__, __FUNCTION__,
    // __LINE__, which, status ? "on" : "off");
    printk("read group %d, pin %d\n", GROUP(g_buttonpins[which]),
           PIN(g_buttonpins[which]));

    if ((PIN(g_buttonpins[which]) == 1) && (GROUP(g_buttonpins[which]) == 5)) {
        printk("read pin 1 group 5 ...\n");

        return ((gpio5->psr & (1 << 1)) ? 1 : 0);
    }
    else {
        return (gpio4->psr & (1 << 14)) ? 1 : 0;
    }
}

static int board_100ask_gpio_probe(struct platform_device* dev)
{
    int err = 0;
    // struct resource*    rsc;
    struct device_node* pnode;
    int                 button_pin;

    /* 此platform_driver 支持的platform_device不一定来自设备树,要判断下 */
    pnode = dev->dev.of_node;
    if (!pnode)
        return -1;

    err = of_property_read_u32(pnode, "pin", &button_pin);

    /* 记录引脚 */
    g_buttonpins[g_buttoncnt] = button_pin;

    /* device_create */
    button_device_create(g_buttoncnt);
    g_buttoncnt++;

    return 0;
}

static int board_100ask_gpio_remove(struct platform_device* dev)
{
    /* device_destory */
    // struct resource* res;
    struct device_node* pnode;
    int                 i = 0;
    int                 button_pin, err;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    // while (1) {
    //  res = platform_get_resource(dev, IORESOURCE_IRQ, i);
    pnode = dev->dev.of_node;
    if (!pnode)
        return -1;

    err = of_property_read_u32(pnode, "pin", &button_pin);

    for (i = 0; i < g_buttoncnt; i++) {
        if (g_buttonpins[i] == button_pin) {
            board_6ull_led_exit(i);
            button_device_destroy(i);
            g_buttonpins[i] = -1;
            break;
        }
    }

    for (i = 0; i < g_buttoncnt; i++) {
        if (g_buttonpins[i] != -1)
            break;
    }

    if (i == g_buttoncnt)
        g_buttoncnt = 0;

    return 0;
}

/* add this for device tree  */
static const struct of_device_id gpio_100ask_match_table[] = {
    {.compatible = "100ask,buttondrv"},
    {},
};

static struct platform_driver board_100ask_gpio_drv = {
    .probe  = board_100ask_gpio_probe,
    .remove = board_100ask_gpio_remove,
    .driver =
        {
            .name = "100ask_button",
            .of_match_table =
                gpio_100ask_match_table, /* add this for device tree  */

        },
};

static struct button_operations board_6ull_button_opr = {
    .init = chip_6ull_led_init,
    .ctl  = chip_6ull_led_ctl,
};

static int __init board_6ull_gpio_drv_init(void)
{
    int err;

    err = platform_driver_register(&board_100ask_gpio_drv);

    register_button_operations(&board_6ull_button_opr);

    return 0;
}

static void __init board_6ull_gpio_drv_exit(void)
{
    platform_driver_unregister(&board_100ask_gpio_drv);
}

module_init(board_6ull_gpio_drv_init);
module_exit(board_6ull_gpio_drv_exit);

MODULE_LICENSE("GPL");