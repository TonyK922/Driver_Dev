#include <linux/module.h>
#include <linux/poll.h>

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/major.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/stat.h>
#include <linux/timer.h>
#include <linux/tty.h>

struct gpio_desc
{
    int               gpio;
    int               irq;
    char*             name;
    int               key;
    struct timer_list key_timer;
};

static struct gpio_desc gpios = {
    115,
    0,
    "ds18b20",
};

/* 主设备号                                                                 */
static int           major = 0;
static struct class* gpio_class;

static spinlock_t ds18b20_spinlock;

static void ds18b20_udelay(int us)
{
    u64 time = ktime_get_ns();
    while (ktime_get_ns() - time < us * 1000)
        ;
}

static int ds18b20_reset_wait_ack(void)
{
    int timeout = 100;
    gpio_set_value(gpios.gpio, 0);
    ds18b20_udelay(480);

    gpio_direction_input(gpios.gpio);

    /* wait for ack */
    while (gpio_get_value(gpios.gpio) && timeout--)
        ds18b20_udelay(1);

    if (0 == timeout)
        return -EIO;

    timeout = 300;
    while (!gpio_get_value(gpios.gpio) && timeout--)
        ds18b20_udelay(1);

    if (0 == timeout)
        return -EIO;

    return 0;
}
static void ds18b20_send_cmd(unsigned char cmd)
{
    int i = 0;

    gpio_direction_output(gpios.gpio, 1);

    for (; i < 8; i++) {
        if (cmd & (1 << i)) { /* send 1 */
            gpio_direction_output(gpios.gpio, 0);
            ds18b20_udelay(2);
            gpio_direction_output(gpios.gpio, 1);
            ds18b20_udelay(60);
        }
        else { /* send 0 */
            gpio_direction_output(gpios.gpio, 0);
            ds18b20_udelay(60);
            gpio_direction_output(gpios.gpio, 1);
        }
    }
}

static void ds18b20_read_data(unsigned char* buf)
{
    int           i    = 0;
    unsigned char data = 0;
    if (!buf) {
        printk("%s %d arg is NULL!!\n", __FUNCTION__, __LINE__);
        return;
    }
    gpio_direction_output(gpios.gpio, 1);
    for (; i < 8; i++) {
        gpio_direction_output(gpios.gpio, 0);
        ds18b20_udelay(2);
        gpio_direction_input(gpios.gpio);
        ds18b20_udelay(7);
        if (gpio_get_value(gpios.gpio)) {
            data |= (1 << i);
        }
        ds18b20_udelay(60);
        gpio_direction_output(gpios.gpio, 1);
    }
    *buf = data;
}
static unsigned char calcrc_1byte(unsigned char abyte)
{
    unsigned char i, crc_1byte;
    crc_1byte = 0;  //设定crc_1byte初值为0
    for (i = 0; i < 8; i++) {
        if (((crc_1byte ^ abyte) & 0x01)) {
            crc_1byte ^= 0x18;
            crc_1byte >>= 1;
            crc_1byte |= 0x80;
        }
        else
            crc_1byte >>= 1;

        abyte >>= 1;
    }
    return crc_1byte;
}

/* 参考: https://www.cnblogs.com/yuanguanghui/p/12737740.html */
static unsigned char calcrc_bytes(unsigned char* p, unsigned char len)
{
    unsigned char crc = 0;
    while (len--)  // len为总共要校验的字节数
    {
        crc = calcrc_1byte(crc ^ *p++);
    }
    return crc;  //若最终返回的crc为0，则数据传输正确
}

static int ds18b20_verify_crc(unsigned char* buf, int len)
{
    unsigned char crc;

    crc = calcrc_bytes(buf, 8);

    if (crc == buf[8])
        return 0;
    else
        return -1;
}

static void ds18b20_calc_val(unsigned char* buf, int* result)
{
    unsigned char tempL = 0, tempH = 0;
    unsigned int  integer;
    unsigned char decimal1, decimal2, decimal;

    tempL = buf[0];  //读温度低8位
    tempH = buf[1];  //读温度高8位

    if (tempH > 0x7f)  //最高位为1时温度是负
    {
        tempL    = ~tempL;  //补码转换，取反加一
        tempH    = ~tempH + 1;
        integer  = (tempL >> 4) + tempH * 16;         //整数部分
        decimal1 = ((tempL & 0x0f) * 10) >> 4;        //小数第一位
        decimal2 = ((tempL & 0x0f) * 100 >> 4) % 10;  //小数第二位
        decimal  = decimal1 * 10 + decimal2;          //小数两位
    }
    else {
        integer  = (tempL >> 4) + tempH * 16;         //整数部分
        decimal1 = ((tempL & 0x0f) * 10) >> 4;        //小数第一位
        decimal2 = ((tempL & 0x0f) * 100 >> 4) % 10;  //小数第二位
        decimal  = decimal1 * 10 + decimal2;          //小数两位
    }
    result[0] = integer;
    result[1] = decimal;
}

/* 实现对应的open/read/write等函数，填入file_operations结构体 */
static ssize_t
ds18b20_read(struct file* file, char __user* buf, size_t size, loff_t* offset)
{
    unsigned long flags       = -1;
    int           err         = -1;
    unsigned char kern_buf[9] = {0};
    int           i           = 0;
    int           result_buf[2];

    if (size != 8)
        return -EINVAL;

    /*  turn off irq */
    spin_lock_irqsave(&ds18b20_spinlock, flags);

    /* send reset sig & wait for response */
    err = ds18b20_reset_wait_ack();
    if (err) {
        spin_unlock_irqrestore(&ds18b20_spinlock, flags);
        return err;
    }

    /* send cmd: skip rom (0xcc) */
    ds18b20_send_cmd(0xcc);

    /* send cmd: start temperature transform 0x44 */
    ds18b20_send_cmd(0x44);

    /* restore irq */
    spin_unlock_irqrestore(&ds18b20_spinlock, flags);

    /* wait for slave finish temperature transform */
    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(msecs_to_jiffies(1000));

    /* read temperature: */
    /*  turn off irq */
    spin_lock_irqsave(&ds18b20_spinlock, flags);

    /* send reset sig & wait for response */
    err = ds18b20_reset_wait_ack();
    if (err) {
        spin_unlock_irqrestore(&ds18b20_spinlock, flags);
        return err;
    }
    /* send cmd: skip rom (0xcc) */
    ds18b20_send_cmd(0xcc);

    /* send cmd: read scratchpad(0xbe) */
    ds18b20_send_cmd(0xbe);

    /* read 9 bytes data */
    for (i = 0; i < 9; i++) {
        ds18b20_read_data(&kern_buf[i]);
    }
    /* restore irq */
    spin_unlock_irqrestore(&ds18b20_spinlock, flags);
    // return simple_read_from_buffer(buf, size, offset, kern_buf, 9);
    /* calc CRC  */
    err = ds18b20_verify_crc(kern_buf, 9);
    if (err) {
        return err;
    }
    /* copy data to user */
    ds18b20_calc_val(kern_buf, result_buf);

    err = copy_to_user(buf, result_buf, 8);
    return 8;
}

/* 定义自己的file_operations结构体 */
static struct file_operations ds18b20_drv = {
    .owner = THIS_MODULE,
    .read  = ds18b20_read,
};

/* 在入口函数 */
static int __init gpio_drv_init(void)
{
    int err;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    spin_lock_init(&ds18b20_spinlock);

    err = gpio_request(gpios.gpio, gpios.name);
    gpio_direction_output(gpios.gpio, 1);

    /* 注册file_operations 	*/
    major =
        register_chrdev(0, "ds18b20_drv", &ds18b20_drv); /* /dev/gpio_desc */

    gpio_class = class_create(THIS_MODULE, "ds18b20_class");
    if (IS_ERR(gpio_class)) {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        unregister_chrdev(major, "ds18b20_drv");
        return PTR_ERR(gpio_class);
    }

    device_create(gpio_class, NULL, MKDEV(major, 0), NULL,
                  "ds18b20_dev"); /* /dev/ds18b20_dev */

    return err;
}

/* 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数
 */
static void __exit gpio_drv_exit(void)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    device_destroy(gpio_class, MKDEV(major, 0));
    class_destroy(gpio_class);
    unregister_chrdev(major, "ds18b20_drv");
    gpio_free(gpios.gpio);
}

/* 7. 其他完善：提供设备信息，自动创建设备节点 */

module_init(gpio_drv_init);
module_exit(gpio_drv_exit);

MODULE_LICENSE("GPL");
