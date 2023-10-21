#include <linux/module.h>

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/major.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/tty.h>

#include "led_opr.h"

#define READ_OPR 2

/* 1. 确定主设备号 */

static int             major = 0;
static struct class*   led_class;
struct led_operations* p_led_opr;

#define MIN(x, y)                                                              \
    ({                                                                         \
        typeof(x) _x = (x);                                                    \
        typeof(y) _y = (y);                                                    \
        (void)(&_x == &_y);                                                    \
        _x < _y ? _x : _y;                                                     \
    })

void led_device_create(int minor)
{
    device_create(led_class, NULL, MKDEV(major, minor), NULL, "100ask_led%d",
                  minor);
}

void led_device_destroy(int minor)
{
    device_destroy(led_class, MKDEV(major, minor));
}

void register_led_operations(struct led_operations* opr)
{
    p_led_opr = opr;
}

EXPORT_SYMBOL(led_device_create);
EXPORT_SYMBOL(led_device_destroy);
EXPORT_SYMBOL(register_led_operations);

static int led_drv_open(struct inode* node, struct file* file)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    int minor;
    /* 根据次设备号 初始化 LED */
    minor = iminor(node);
    p_led_opr->init(minor);

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

    val = p_led_opr->ctl(minor, READ_OPR);

    ret = copy_to_user(buf, &val, 1);

    return 1;
}

/* write(fd, &val, 1); */
static ssize_t led_drv_write(struct file*       file,
                             const char __user* buf,
                             size_t             size,
                             loff_t*            offset)
{
    int           err;
    char          status;
    struct inode* inode = file_inode(file);
    int           minor = iminor(inode);

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    err = copy_from_user(&status, buf, 1);

    /* 根据次设备号和status控制LED */
    p_led_opr->ctl(minor, status);

    return 1;
}

static int led_drv_close(struct inode* node, struct file* file)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    return 0;
}

/* 2. 定义自己的file_operations结构体 */
static struct file_operations led_drv = {
    .owner   = THIS_MODULE,
    .open    = led_drv_open,
    .read    = led_drv_read,
    .write   = led_drv_write,
    .release = led_drv_close,
};

static int __init led_init(void)
{
    int err;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    major = register_chrdev(0, "100ask_led", &led_drv); /* /dev/led */

    led_class = class_create(THIS_MODULE, "100ask_led_class");
    err       = PTR_ERR(led_class);
    if (IS_ERR(led_class))
    {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        unregister_chrdev(major, "100ask_led");
        return -1;
    }

    return 0;
}

static void __exit led_exit(void)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    class_destroy(led_class);
    unregister_chrdev(major, "100ask_led");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");