#include "led_opr.h"
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#define READ_OPR 2

static int             major = 0;
static struct class*   led_class;
struct led_operations* p_led_fops;

#define MIN(x, y)                                                              \
    ({                                                                         \
        typeof(x) _x = (x);                                                    \
        typeof(y) _y = (y);                                                    \
        (void)(&_x == &_y);                                                    \
        _x < _y ? _x : _y;                                                     \
    })

static int led_drv_open(struct inode* node, struct file* file)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    int minor;
    /* 根据次设备号 初始化 LED */
    minor = iminor(node);
    p_led_fops->init(minor);

    return 0;
}

static ssize_t
led_drv_read(struct file* file, char __user* buf, size_t size, loff_t* offset)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    int           ret;
    char          val;
    struct inode* node  = file_inode(file);
    int           minor = iminor(node);

    val = p_led_fops->ctl(minor, READ_OPR);

    ret = copy_to_user(buf, &val, 1);

    return 1;
}

static ssize_t led_drv_write(struct file*       file,
                             const char __user* buf,
                             size_t             size,
                             loff_t*            offset)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    char          status;
    struct inode* node  = file_inode(file);
    int           minor = iminor(node);

    int err = copy_from_user(&status, buf, 1);

    /* 根据次设备号 和 status 控制 对应led */
    p_led_fops->ctl(minor, status);

    return 1;
}

static int led_drv_close(struct inode* node, struct file* file)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    return 0;
}

/* drv operations */

static struct file_operations led_drv_ops = {
    .owner   = THIS_MODULE,
    .open    = led_drv_open,
    .read    = led_drv_read,
    .write   = led_drv_write,
    .release = led_drv_close,
};

/* entry function */

static int __init led_init(void)
{
    printk("%s %s line : %d \n", __FILE__, __FUNCTION__, __LINE__);
    int i, err;
    major = register_chrdev(0, "100ask_led_drv", &led_drv_ops); /* /dev/led */

    led_class = class_create(THIS_MODULE, "100ask_led");

    printk("major = %d\n", major);

    err = PTR_ERR(led_class);
    if (IS_ERR(led_class)) {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        unregister_chrdev(major, "100ask_led");
        return -1;
    }

    p_led_fops = get_board_led_opr();

    for (i = 0; i < p_led_fops->num; i++) {
        device_create(led_class, NULL, MKDEV(major, i), NULL, "100ask_led%d",
                      i);
    }

    return 0;
}

/* exit function */

static void __exit led_exit(void)
{
    printk("%s %s line : %d \n", __FILE__, __FUNCTION__, __LINE__);
    int i;

    for (i = 0; i < p_led_fops->num; i++)
        device_destroy(led_class, MKDEV(major, i));

    class_destroy(led_class);
    unregister_chrdev(major, "100ask_led");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");