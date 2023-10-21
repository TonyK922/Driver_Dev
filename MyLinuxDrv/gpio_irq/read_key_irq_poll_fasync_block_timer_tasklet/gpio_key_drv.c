#include <linux/module.h>

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/major.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/timer.h>
#include <linux/tty.h>

#define BUFF_LEN 128
#define NEXT_POS(x) (((x) + 1) % BUFF_LEN)

struct gpio_key
{
    int               gpio;
    struct gpio_desc* gpiod; /*must be like this due to
                                key_timer_expires(unsigned long data)*/
    int                   irq;
    int                   flags;
    struct timer_list     key_timer;
    struct tasklet_struct tasklet;
};

struct cycle_buffer
{
    int      g_keys[BUFF_LEN];
    unsigned read;
    unsigned write;
};

static struct cycle_buffer   keys_buffer;
static struct fasync_struct* key_fasync;

static int              major          = 0;
static struct class*    gpio_key_class = NULL;
static struct gpio_key* gpio_keys      = NULL;
// static int              g_key;

static DECLARE_WAIT_QUEUE_HEAD(gpio_key_wait);

static void buf_init(struct cycle_buffer* buf)
{
    buf->read = buf->write = 0;
}

static inline int is_key_buf_empty(struct cycle_buffer* buf)
{
    return (buf->read == buf->write);
}
static inline int is_key_buf_full(struct cycle_buffer* buf)
{
    return (buf->read == NEXT_POS(buf->write));
}

static void put_key(int key, struct cycle_buffer* buf)
{
    if (!is_key_buf_full(buf)) {
        buf->g_keys[buf->write] = key;
        buf->write              = NEXT_POS(buf->write);
    }
}

static int get_key(struct cycle_buffer* buf)
{
    int key = 0;
    if (!is_key_buf_empty(buf)) {
        key       = buf->g_keys[buf->read];
        buf->read = NEXT_POS(buf->read);
    }
    return key;
}

static void key_timer_expires(unsigned long data)
{
    // printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    int val, key;

    struct gpio_key* gpio_key = data;

    val = gpiod_get_value(gpio_key->gpiod);

    printk("%s key %d  val %d \n", __FUNCTION__, gpio_key->gpio, val);

    key = (gpio_key->gpio << 8) | val;
    put_key(key, &keys_buffer);
    wake_up_interruptible(&gpio_key_wait);

    kill_fasync(&key_fasync, SIGIO, POLL_IN);
}

static void key_tasklet_func(unsigned long data)
{
    int val;

    struct gpio_key* gpio_key = data;

    val = gpiod_get_value(gpio_key->gpiod);

    printk("%s key %d  val %d \n", __FUNCTION__, gpio_key->gpio, val);
}

/* 实现对应的open/read/write等函数，填入file_operations结构体 */
static ssize_t gpio_key_drv_read(struct file* file,
                                 char __user* buf,
                                 size_t       size,
                                 loff_t*      offset)
{
    // printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    int err, key;

    /* support NONBLOCK */
    if (is_key_buf_empty(&keys_buffer) && (file->f_flags & O_NONBLOCK)) {
        return -EAGAIN;
    }

    wait_event_interruptible(gpio_key_wait, !is_key_buf_empty(&keys_buffer));
    key = get_key(&keys_buffer);
    err = copy_to_user(buf, &key, 4);
    // g_key = 0;

    return 4;
}

static unsigned gpio_key_drv_poll(struct file* fp, poll_table* wait)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    poll_wait(fp, &gpio_key_wait, wait);
    return (is_key_buf_empty(&keys_buffer) ? 0 : POLLIN | POLLRDNORM);
}

static int gpio_key_drv_fasync(int fd, struct file* file, int on)
{
    if (fasync_helper(fd, file, on, &key_fasync) >= 0) {
        return 0;
    }
    else {
        return -EIO;
    }
}

/* 定义自己的file_operations结构体 */
static struct file_operations gpio_key_drv = {
    .owner  = THIS_MODULE,
    .read   = gpio_key_drv_read,
    .poll   = gpio_key_drv_poll,
    .fasync = gpio_key_drv_fasync,
};

static irqreturn_t gpio_key_irq(int irq, void* dev_id)
{
    struct gpio_key* gpio_key = dev_id;
    // printk("gpio_key_isr key %d irq happened\n", gpio_key->gpio);
    tasklet_schedule(&gpio_key->tasklet);
    mod_timer(&gpio_key->key_timer, jiffies + HZ / 50);

    return IRQ_HANDLED;
}

static int gpio_key_probe(struct platform_device* pdev)
{
    int                count, i, err;
    enum of_gpio_flags flags;
    // unsigned           irq_flag = GPIOF_IN;

    struct device_node* node = pdev->dev.of_node;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    count = of_gpio_count(node);
    if (!count) {
        printk("%s %s line %d, there isn't any gpio available\n", __FILE__,
               __FUNCTION__, __LINE__);
        return -1;
    }

    gpio_keys = kzalloc(count * sizeof(struct gpio_key),
                        GFP_KERNEL);  // free in gpio_key_remove

    for (i = 0; i < count; i++) {
        gpio_keys[i].gpio = of_get_gpio_flags(node, i, &flags);

        if (gpio_keys[i].gpio < 0) {
            printk("%s %s line %d, of_get_gpio_flags fail\n", __FILE__,
                   __FUNCTION__, __LINE__);
            return -1;
        }

        gpio_keys[i].gpiod = gpio_to_desc(gpio_keys[i].gpio);
        gpio_keys[i].irq   = gpio_to_irq(gpio_keys[i].gpio);
        gpio_keys[i].flags = flags & OF_GPIO_ACTIVE_LOW;
        /*
                if (flags & OF_GPIO_ACTIVE_LOW)
                    irq_flag |= GPIOF_ACTIVE_LOW;

                err = devm_gpio_request_one(&pdev->dev, gpio_keys[i].gpio,
           irq_flag, NULL);
        */
        setup_timer(&gpio_keys[i].key_timer, key_timer_expires, &gpio_keys[i]);
        gpio_keys[i].key_timer.expires = ~0;
        add_timer(&gpio_keys[i].key_timer);

        tasklet_init(&gpio_keys[i].tasklet, key_tasklet_func, &gpio_keys[i]);
    }

    for (i = 0; i < count; i++) {
        err = request_irq(gpio_keys[i].irq, gpio_key_irq,
                          IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                          "my_gpio_key", &gpio_keys[i]);
    }

    major          = register_chrdev(0, "my_gpio_key", &gpio_key_drv);
    gpio_key_class = class_create(THIS_MODULE, "my_gpio_key_class");
    if (IS_ERR(gpio_key_class)) {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        unregister_chrdev(major, "my_gpio_key");
        return PTR_ERR(gpio_key_class);
    }
    device_create(gpio_key_class, NULL, MKDEV(major, 0), NULL, "my_gpio_key");
    /* /dev/100ask_gpio_key */

    return 0;
}
static int gpio_key_remove(struct platform_device* pdev)
{
    // int err;

    int count, i;

    struct device_node* node = pdev->dev.of_node;

    device_destroy(gpio_key_class, MKDEV(major, 0));
    class_destroy(gpio_key_class);
    unregister_chrdev(major, "my_gpio_key");

    count = of_gpio_count(node);
    if (!count) {
        printk("%s %s line %d, there isn't any gpio available\n", __FILE__,
               __FUNCTION__, __LINE__);
        return -1;
    }

    for (i = 0; i < count; i++) {
        free_irq(gpio_keys[i].irq, &gpio_keys[i]);
        del_timer(&gpio_keys[i].key_timer);
        tasklet_kill(&gpio_keys[i].tasklet);
    }

    kfree(gpio_keys);
    return 0;
}

static const struct of_device_id keys_match_table[] = {
    {.compatible = "100ask,gpio_key"},
    {},
};

static struct platform_driver gpio_keys_driver = {
    .probe  = gpio_key_probe,
    .remove = gpio_key_remove,
    .driver =
        {
            .name           = "my_gpio_key",
            .of_match_table = keys_match_table,
        },
};

static int __init gpio_key_init(void)
{
    int err;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    err = platform_driver_register(&gpio_keys_driver);
    buf_init(&keys_buffer);

    return err;
}

static void __exit gpio_key_exit(void)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    platform_driver_unregister(&gpio_keys_driver);
}

module_init(gpio_key_init);
module_exit(gpio_key_exit);

MODULE_LICENSE("GPL");