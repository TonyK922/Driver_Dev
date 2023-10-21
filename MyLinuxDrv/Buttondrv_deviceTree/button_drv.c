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

#include "board_100ask_6ull.h"
#include "button_drv.h"

static int                major = 0;
static struct class*      button_class;
struct button_operations* p_button_opr;

#define MIN(x, y)                                                              \
    ({                                                                         \
        typeof(x) _x = (x);                                                    \
        typeof(y) _y = (y);                                                    \
        (void)(&_x == &_y);                                                    \
        _x < _y ? _x : _y;                                                     \
    })

void button_device_create(int minor)
{
    device_create(button_class, NULL, MKDEV(major, minor), NULL,
                  "100ask_button%d", minor);
}

void button_device_destroy(int minor)
{
    device_destroy(button_class, MKDEV(major, minor));
}

void register_button_operations(struct button_operations* opr)
{
    p_button_opr = opr;
}

EXPORT_SYMBOL(button_device_create);
EXPORT_SYMBOL(button_device_destroy);
EXPORT_SYMBOL(register_button_operations);

static int button_drv_open(struct inode* node, struct file* file)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    int minor;
    /* 根据次设备号 初始化 LED */
    minor = iminor(node);
    p_button_opr->init(minor);

    return 0;
}

static ssize_t button_drv_read(struct file* file,
                               char __user* buf,
                               size_t       size,
                               loff_t*      offset)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    int  ret;
    char val;
    int  minor = iminor(file_inode(file));

    val = p_button_opr->ctl(minor);

    ret = copy_to_user(buf, &val, 1);

    return 1;
}

static int button_drv_close(struct inode* node, struct file* file)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    return 0;
}

static struct file_operations button_fops = {
    .owner   = THIS_MODULE,
    .open    = button_drv_open,
    .read    = button_drv_read,
    .release = button_drv_close,
};

static int __init button_drv_init(void)
{
    int err = 0;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    major        = register_chrdev(0, "100ask_button", &button_fops);
    button_class = class_create(THIS_MODULE, "100ask_button_class");

    err = PTR_ERR(button_class);
    if (IS_ERR(button_class)) {
        printk("class_create ERROR %s %s line %d\n", __FILE__, __FUNCTION__,
               __LINE__);
        unregister_chrdev(major, "100ask_button");
        return -1;
    }

    return 0;
}

static void __exit button_drv_exit(void)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    class_destroy(button_class);
    unregister_chrdev(major, "100ask_button");
}

module_init(button_drv_init);
module_exit(button_drv_exit);

MODULE_LICENSE("GPL");