#include <linux/module.h>

#include <linux/device.h>
#include <linux/errno.h>
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
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/tty.h>

struct gpio_key
{
    struct gpio_desc* gpiod;
    int               irq;
    int               gpio;
    int               flags;
};

static struct gpio_key* gpio_keys = NULL;

static irqreturn_t gpio_key_irq(int irq, void* dev_id)
{
    int val;

    struct gpio_key* gpio_key = (struct gpio_key*)dev_id;

    val = gpiod_get_value(gpio_key->gpiod);

    printk("key %d  val %d \n", gpio_key->gpio, val);

    return IRQ_HANDLED;
}

static int gpio_key_probe(struct platform_device* pdev)
{
    int                count, i, err;
    enum of_gpio_flags flags;
    unsigned           irq_flag = GPIOF_IN;

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

        if (flags & OF_GPIO_ACTIVE_LOW)
            irq_flag |= GPIOF_ACTIVE_LOW;

        err = devm_gpio_request_one(&pdev->dev, gpio_keys[i].gpio, irq_flag,
                                    NULL);
    }

    for (i = 0; i < count; i++) {
        err = request_irq(gpio_keys[i].irq, gpio_key_irq,
                          IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                          "my_gpio_key", &gpio_keys[i]);
    }
    return 0;
}
static int gpio_key_remove(struct platform_device* pdev)
{
    // int err;

    int count, i;

    struct device_node* node = pdev->dev.of_node;

    count = of_gpio_count(node);
    if (!count) {
        printk("%s %s line %d, there isn't any gpio available\n", __FILE__,
               __FUNCTION__, __LINE__);
        return -1;
    }

    for (i = 0; i < count; i++) {
        free_irq(gpio_keys[i].irq, &gpio_keys[i]);
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