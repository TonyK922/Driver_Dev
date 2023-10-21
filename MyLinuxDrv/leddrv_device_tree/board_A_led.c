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
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/tty.h>

#include "led_resource.h"

static struct resource resources[] = {
    {
        .start = GROUP_PIN(5, 3),
        .flags = IORESOURCE_IRQ,
        .name  = "100ask_led_pin",
    },

};

static void led_dev_release(struct device* dev)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
}

static struct platform_device board_A_led_dev = {
    .name          = "100ask_led",
    .num_resources = ARRAY_SIZE(resources),
    .resource      = resources,
    .dev =
        {
            .release = led_dev_release,
        },
};

static int __init led_dev_init(void)
{
    int err;

    err = platform_device_register(&board_A_led_dev);

    return 0;
}

static void __exit led_dev_exit(void)
{
    platform_device_unregister(&board_A_led_dev);
}

module_init(led_dev_init);
module_exit(led_dev_exit);

MODULE_LICENSE("GPL");