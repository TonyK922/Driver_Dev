#include "led_opr.h"
#include "led_resource.h"
#include <asm/io.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/major.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/tty.h>

static struct led_resource* led_rsc;

static volatile unsigned* CCM_CCGR1;
static volatile unsigned* IOMUXC_SNVS_TAMPER3;
static volatile unsigned* GPIO5_GDIR;
static volatile unsigned* GPIO5_DR;

static int chip_6ull_led_init(void)
{
    if (!led_rsc) {
        led_rsc = get_led_resource();
    }

    printk("init gpio: group %d, pin %d\n", GROUP(led_rsc->pin),
           PIN(led_rsc->pin));

    switch (GROUP(led_rsc->pin)) {
        case 5: {
            printk("init pin of group 5 ...\n");
            if (!CCM_CCGR1) {
                CCM_CCGR1           = ioremap(0x020C406C, 4);
                IOMUXC_SNVS_TAMPER3 = ioremap(0x02290014, 4);
                GPIO5_GDIR          = ioremap(0x020AC004, 4);
                GPIO5_DR            = ioremap(0x020AC000, 4);
            }
            /* GPIO5_IO03 */

            /* 1. 使能GPIO5
             * set CCM to enable GPIO5
             * CCM_CCGR1[CG15] 0x20C406C
             * bit[31:30] = 0b11
             * reserved
             */
            *CCM_CCGR1 |= (3 << 30);  // write it or not will be ok

            /* 2. 设置GPIO5_IO03用于GPIO
             * set IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3
             *      to configure GPIO5_IO03 as GPIO
             * IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3  0x2290014
             * bit[3:0] = 0b0101 alt5
             */
            unsigned int val = *IOMUXC_SNVS_TAMPER3;
            val &= ~(0xf);
            val |= (0x5);
            *IOMUXC_SNVS_TAMPER3 = val;

            /* 3. 设置GPIO5_IO03作为output引脚
             * set GPIO5_GDIR to configure GPIO5_IO03 as output
             * GPIO5_GDIR  0x020AC000 + 0x4
             * bit[3] = 0b1
             */
            *GPIO5_GDIR |= (1 << 3);
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
        case 4: {
            printk("init pin of group 4 ...\n");
            break;
        }
    }

    return 0;
}

/* 控制LED, which-哪个LED, status:1-亮,0-灭 */
static int chip_6ull_led_ctl(char status)
{
    // printk("%s %s line %d, led %d, %s\n", __FILE__, __FUNCTION__, __LINE__,
    // which, status ? "on" : "off");
    printk("set led %s: group %d, pin %d\n", status ? "on" : "off",
           GROUP(led_rsc->pin), PIN(led_rsc->pin));

    switch (GROUP(led_rsc->pin)) {
        case 5: {
            printk("set pin of group 5 ...\n");
            unsigned int pin = (unsigned int)PIN(led_rsc->pin);

            if (1 == status) { /* on : output 0 */
                /* 设置GPIO5_DR输出低电平
                 * set GPIO5_DR to configure GPIO5_IO03 output 0
                 * GPIO5_DR 0x020AC000 + 0
                 * bit[3] = 0b0
                 */
                *GPIO5_DR &= ~(1 << pin);
            }
            else if (status == 0) { /* off: output 1*/
                /* 设置GPIO5_IO3输出高电平
                 * set GPIO5_DR to configure GPIO5_IO03 output 1
                 * GPIO5_DR 0x020AC000 + 0
                 * bit[3] = 0b1
                 */
                *GPIO5_DR |= (1 << pin);
            }
            else if (status == 2) {
                char val;

                /* configure GPIO5_IO3 pin as input */
                *GPIO5_GDIR &= ~(1 << pin);

                if ((*GPIO5_DR & (1 << pin)) == 0)
                    val = 0;
                else
                    val = 1;

                *GPIO5_GDIR |= (1 << pin);  // as output
                // printk("kernel read val = %d\n", val);
                return val;
            }
            break;
        }
        case 1: {
            printk("set pin of group 1 ...\n");
            break;
        }
        case 2: {
            printk("set pin of group 2 ...\n");
            break;
        }
        case 3: {
            printk("set pin of group 3 ...\n");
            break;
        }
    }

    return 0;
}

static int board_6ull_led_exit(void)
{
    if (GROUP(led_rsc->pin) == 5) {
        iounmap(CCM_CCGR1);
        iounmap(IOMUXC_SNVS_TAMPER3);
        iounmap(GPIO5_GDIR);
        iounmap(GPIO5_DR);
    }
    return 0;
}

static struct led_operations board_6ull_led_opr = {
    .init = chip_6ull_led_init,
    .ctl  = chip_6ull_led_ctl,
    .exit = board_6ull_led_exit,
};

struct led_operations* get_board_led_opr(void)
{
    return &board_6ull_led_opr;
}