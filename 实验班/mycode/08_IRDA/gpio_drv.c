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
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/timekeeping.h>
#include <linux/timer.h>
#include <linux/tty.h>
#include <linux/types.h>

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
    "IRDA",
};

/* 主设备号*/
static int           major = 0;
static struct class* gpio_class;
static u64           g_irda_irq_time[68];
static int32_t       g_irda_irq_cnt;

/* 环形缓冲区 */
#define BUF_LEN 128
static uint8_t g_keys[BUF_LEN];
static int     r, w;

struct fasync_struct* button_fasync;

#define NEXT_POS(x) ((x + 1) % BUF_LEN)

static int is_key_buf_empty(void)
{
    return (r == w);
}

static int is_key_buf_full(void)
{
    return (r == NEXT_POS(w));
}

static void put_key(uint8_t key)
{
    if (!is_key_buf_full()) {
        g_keys[w] = key;
        w         = NEXT_POS(w);
    }
}

static uint8_t get_key(void)
{
    uint8_t key = 0;
    if (!is_key_buf_empty()) {
        key = g_keys[r];
        r   = NEXT_POS(r);
    }
    return key;
}

static DECLARE_WAIT_QUEUE_HEAD(gpio_wait);

// static void key_timer_expire(struct timer_list *t)
static void key_timer_expire(unsigned long data)
{
    /* time out */
    g_irda_irq_cnt = 0;

    put_key(-1);
    put_key(-1);

    wake_up_interruptible(&gpio_wait);
    kill_fasync(&button_fasync, SIGIO, POLL_IN);
}

/* 实现对应的open/read/write等函数，填入file_operations结构体 */
static ssize_t
gpio_drv_read(struct file* file, char __user* buf, size_t size, loff_t* offset)
{
    // printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    int     err;
    uint8_t kern_buf[2];

    if (2 != size)
        return -EINVAL;

    if (is_key_buf_empty() && (file->f_flags & O_NONBLOCK))
        return -EAGAIN;

    wait_event_interruptible(gpio_wait, !is_key_buf_empty());
    kern_buf[0] = get_key(); /* which device  */
    kern_buf[1] = get_key(); /* what data */

    if (kern_buf[0] == (uint8_t)-1 && kern_buf[1] == (uint8_t)-1)
        return -EIO;

    err = copy_to_user(buf, &kern_buf, 2);

    return 2;
}

static unsigned int gpio_drv_poll(struct file* fp, poll_table* wait)
{
    // printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    poll_wait(fp, &gpio_wait, wait);
    return is_key_buf_empty() ? 0 : POLLIN | POLLRDNORM;
}

static int gpio_drv_fasync(int fd, struct file* file, int on)
{
    if (fasync_helper(fd, file, on, &button_fasync) >= 0)
        return 0;
    else
        return -EIO;
}

/* 定义自己的file_operations结构体 */
static struct file_operations gpio_key_drv = {
    .owner  = THIS_MODULE,
    .read   = gpio_drv_read,
    .poll   = gpio_drv_poll,
    .fasync = gpio_drv_fasync,
};

static void parse_irda_data(void)
{
    u64     time;
    int32_t i, m, n;
    uint8_t datas[4];
    uint8_t data = 0;
    int32_t bits = 0, byte = 0;

    /* 1. 9ms low level 4.5ms high level */
    time = g_irda_irq_time[1] - g_irda_irq_time[0];
    if (time < 8000000 || time > 10000000)
        goto err;

    time = g_irda_irq_time[2] - g_irda_irq_time[1];
    if (time < 3500000 || time > 5500000)
        goto err;

    for (i = 0; i < 32; i++) {
        m    = 3 + (i << 1);
        n    = m + 1;
        time = g_irda_irq_time[n] - g_irda_irq_time[m];
        data <<= 1;
        bits++;
        if (time > 1000000) {
            /* 得到了数据1 */
            data |= 1;
        }
        if (bits == 8) {
            datas[byte++] = data;
            data          = 0;
            bits          = 0;
        }
    }
    /* 判断数据正误 */
    datas[1] = ~datas[1];
    datas[3] = ~datas[3];

    if ((datas[0] != datas[1]) || (datas[2] != datas[3])) {
        printk("data verify err: %02x %02x %02x %02x\n", datas[0], datas[1],
               datas[2], datas[3]);
        goto err;
    }
    put_key(datas[0]);
    put_key(datas[2]);
    wake_up_interruptible(&gpio_wait);
    kill_fasync(&button_fasync, SIGIO, POLL_IN);
    return;

err:
    g_irda_irq_cnt = 0;
    put_key(-1);
    put_key(-1);
    wake_up_interruptible(&gpio_wait);
    kill_fasync(&button_fasync, SIGIO, POLL_IN);
}
static int get_irda_repeat_datas(void)
{
    u64 time;

    /* 1. 判断重复码 : 9ms的低脉冲, 2.25ms高脉冲  */
    time = g_irda_irq_time[1] - g_irda_irq_time[0];
    if (time < 8000000 || time > 10000000) {
        return -1;
    }

    time = g_irda_irq_time[2] - g_irda_irq_time[1];
    if (time < 2000000 || time > 2500000) {
        return -1;
    }

    return 0;
}

static irqreturn_t gpio_key_isr(int irq, void* dev_id)
{
    struct gpio_desc* gpio_desc = dev_id;
    u64               time;

    /* 1. record irq happen time */
    time = ktime_get_ns();

    g_irda_irq_time[g_irda_irq_cnt] = time;

    /* 2. sum irq times */
    g_irda_irq_cnt++;

    /* 3.times right parse data */
    if (g_irda_irq_cnt == 4) {
        /* 是否重复码 */
        if (0 == get_irda_repeat_datas()) {
            /* device: 0, val: 0, 表示重复码 */
            put_key(0);
            put_key(0);
            wake_up_interruptible(&gpio_wait);
            kill_fasync(&button_fasync, SIGIO, POLL_IN);
            del_timer(&gpio_desc->key_timer);
            g_irda_irq_cnt = 0;
            return IRQ_HANDLED;
        }
    }
    if (68 == g_irda_irq_cnt) {
        parse_irda_data();
        del_timer(&gpios.key_timer);
        g_irda_irq_cnt = 0;
        return IRQ_HANDLED;
    }

    /* 4. modify timer every time */
    mod_timer(&gpio_desc->key_timer, jiffies + msecs_to_jiffies(100));
    return IRQ_HANDLED;
}

/* 在入口函数 */
static int __init gpio_drv_init(void)
{
    int err;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    gpios.irq = gpio_to_irq(gpios.gpio);

    setup_timer(&gpios.key_timer, key_timer_expire, (unsigned long)&gpios);
    // timer_setup(&gpios[i].key_timer, key_timer_expire, 0);

    err = request_irq(gpios.irq, gpio_key_isr,
                      IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, gpios.name,
                      &gpios);

    /* 注册file_operations 	*/
    major = register_chrdev(0, "IRDA_Drv", &gpio_key_drv); /* /dev/gpio_desc */

    gpio_class = class_create(THIS_MODULE, "IRDA_Drv_class");
    if (IS_ERR(gpio_class)) {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        unregister_chrdev(major, "IRDA_Drv");
        return PTR_ERR(gpio_class);
    }

    device_create(gpio_class, NULL, MKDEV(major, 0), NULL,
                  "IRDA_Dev"); /* /dev/IRDA_Dev */

    return err;
}

/* 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数
 */
static void __exit gpio_drv_exit(void)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    device_destroy(gpio_class, MKDEV(major, 0));
    class_destroy(gpio_class);
    unregister_chrdev(major, "IRDA_Drv");

    free_irq(gpios.irq, &gpios);
    del_timer(&gpios.key_timer);
}

/* 7. 其他完善：提供设备信息，自动创建设备节点 */

module_init(gpio_drv_init);
module_exit(gpio_drv_exit);

MODULE_LICENSE("GPL");
