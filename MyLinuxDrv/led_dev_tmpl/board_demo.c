#include "led_opr.h"
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

static int board_demo_led_init(int which)
{
    printk("%s %s line %d, led %d\n", __FILE__, __FUNCTION__, __LINE__, which);
    return 0;
}

static int board_demo_led_ctl(int which, char status)
{
    printk("%s %s line %d, led %d, %s\n", __FILE__, __FUNCTION__, __LINE__,
           which, status ? "on" : "off");
    return 0;
}

static struct led_operations board_demo_led_opr = {
    .num  = 1,
    .init = board_demo_led_init,
    .ctl  = board_demo_led_ctl,
};

struct led_operations* get_board_led_opr(void)
{
    return &board_demo_led_opr;
}