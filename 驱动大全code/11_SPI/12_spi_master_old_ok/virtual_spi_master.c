#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/spi/spi.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#define METHOD1

/* 参考 spi-sh.c  */
static struct spi_master* g_virtual_master;
static struct work_struct virtual_ws;

static int spi_virtual_transfer(struct spi_device*  spi,
                                struct spi_message* mesg)
{
    /* 方法1: 直接实现spi传输 */
    /* 假装传输完成, 直接唤醒 */
#ifdef METHOD1
    mesg->status = 0;
    mesg->complete(mesg->context);
    return 0;
#else
    /* 方法2: 使用工作队列启动SPI传输 等待完成 */
    /* 消息放入队列, 启动工作队列 */
    mesg->actual_length = 0;
    mesg->status        = -EINPROGRESS;
    list_add_tail(&mesg->queue, &spi->master->queue);
    schedule_work(&virtual_ws);
#endif
    return 0;
}

static void spi_virtual_work(struct work_struct* work)
{
    struct spi_message* mesg;

    while (!list_empty(&g_virtual_master->queue)) {
        mesg =
            list_entry(g_virtual_master->queue.next, struct spi_message, queue);
        list_del_init(&mesg->queue);

        /* 硬件传输过程省略 */
        /* 假装硬件传输完成. */
        mesg->status = 0;
        if (mesg->complete)
            mesg->complete(mesg->context);
    }

    return;
}

static int spi_virtual_probe(struct platform_device* pdev)
{
    int ret;
    /* 分配 设置 注册 spi_master */
    struct spi_master* master;

    g_virtual_master = master = spi_alloc_master(&pdev->dev, 0);
    if (master == NULL) {
        dev_err(&pdev->dev, "spi_alloc_master error.\n");
        return -ENOMEM;
    }
    master->transfer = spi_virtual_transfer;
    INIT_WORK(&virtual_ws, spi_virtual_work);

    master->dev.of_node = pdev->dev.of_node;
    ret                 = spi_register_master(master);
    if (ret < 0) {
        printk(KERN_ERR "spi_register_master error.\n");
        goto error;
    }

error:
    spi_master_put(master);
    return ret;
}

static int spi_virtual_remove(struct platform_device* pdev)
{
    spi_unregister_master(g_virtual_master);
    flush_work(&virtual_ws);
    return 0;
}
static const struct of_device_id spi_virtual_dt_ids[] = {
    {
        .compatible = "my,virtual_spi_master",
    },
    {/* sentinel */},
};
static struct platform_driver spi_virtual_driver = {
    .probe  = spi_virtual_probe,
    .remove = spi_virtual_remove,
    .driver =
        {
            .name           = "virtual_spi",
            .of_match_table = spi_virtual_dt_ids,
        },
};

static int __init virtual_master_init(void)
{
    return platform_driver_register(&spi_virtual_driver);
}
module_init(virtual_master_init);

static void __exit virtual_master_exit(void)
{
    platform_driver_unregister(&spi_virtual_driver);
}
module_exit(virtual_master_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("virtual_spi_master");