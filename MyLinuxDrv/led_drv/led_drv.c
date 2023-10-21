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

static int           major;
static struct class* led_class;

/* register addr */
/* IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 addr：0x02290000 + 0x14 */
static volatile unsigned int* IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3;
/* GPIO5_GDIR addr :0x020AC004 */
static volatile unsigned int* GPIO5_GDIR;
/* GPIO5_DR addr:0x020AC000 */
static volatile unsigned int* GPIO5_DR;

static ssize_t
led_drv_read(struct file* file, char __user* buf, size_t size, loff_t* offset)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    int  ret;
    char val;

    /* configure GPIO5_IO3 pin as output */
    *GPIO5_GDIR &= ~(1 << 3);

    if ((*GPIO5_DR & (1 << 3)) == 0)
        val = 0;
    else
        val = 1;

    ret = copy_to_user(buf, &val, 1);
    *GPIO5_GDIR |= (1 << 3);
    return 1;
}

static ssize_t
led_write(struct file* filp, const char __user* buf, size_t count, loff_t* ppos)
{
    char val;
    int  ret;
    /* copy_from_user : get data from app */
    ret = copy_from_user(&val, buf, 1);

    /* set gpio register  */
    if (val) {
        /* set gpio to drive led on */
        *GPIO5_DR &= ~(1 << 3);
    }
    else {
        /* set gpio to drive led off */
        *GPIO5_DR |= (1 << 3);
    }
    return 1;
}

static int led_open(struct inode* inode, struct file* filp)
{
    /* enable gpio */

    /* configure GPIO5_IO3 pin as gpio */
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 &= (~0xf);
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 |= (0x5);

    /* configure GPIO5_IO3 pin as output */
    *GPIO5_GDIR |= (1 << 3);

    return 0;
}

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open  = led_open,
    .write = led_write,
    .read  = led_drv_read,
};

/* entry function */

static int __init led_init(void)
{
    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

    major = register_chrdev(0, "led_drv", &led_fops);

    /* ioremap  */
    IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 = ioremap(0x02290014, 4);
    GPIO5_GDIR                              = ioremap(0x020AC004, 4);
    GPIO5_DR                                = ioremap(0x020AC000, 4);

    /* os create /dev/myled for us*/
    led_class = class_create(THIS_MODULE, "myled");
    device_create(led_class, NULL, MKDEV(major, 0), NULL, "myled");

    return 0;
}

/* exit function */

static void __exit led_exit(void)
{
    iounmap(IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3);
    iounmap(GPIO5_GDIR);
    iounmap(GPIO5_DR);
    device_destroy(led_class, MKDEV(major, 0));
    class_destroy(led_class);
    unregister_chrdev(major, "led_drv");
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");