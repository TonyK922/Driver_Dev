#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/spi/spi.h>

static struct spi_master *g_virtual_master;
static struct work_struct g_virtual_ws;

static const struct of_device_id spi_virtual_dt_ids[] = {
	{ .compatible = "100ask,virtual_spi_master", },
	{ /* sentinel */ }
};


static void spi_virtual_work(struct work_struct *work)
{
	struct spi_message *mesg;
	
	while (!list_empty(&g_virtual_master->queue)) {
		mesg = list_entry(g_virtual_master->queue.next, struct spi_message, queue);
		list_del_init(&mesg->queue);
		
		/* 假装硬件传输已经完成 */

		mesg->status = 0;
		if (mesg->complete)
			mesg->complete(mesg->context);

	}	
}

static int spi_virtual_transfer(struct spi_device *spi, struct spi_message *mesg)
{
#if 1	
	/* 方法1: 直接实现spi传输 */
	/* 假装传输完成, 直接唤醒 */
	mesg->complete(mesg->context);
	return 0;
	
#else
	/* 方法2: 使用工作队列启动SPI传输、等待完成 */
	/* 把消息放入队列 */
	mesg->actual_length = 0;
	mesg->status = -EINPROGRESS;
	list_add_tail(&mesg->queue, &spi->master->queue);
	
	/* 启动工作队列 */
	schedule_work(&g_virtual_ws);
	
	/* 直接返回 */
	return 0;
#endif	
}


static int spi_virtual_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	int ret;
	
	/* 分配/设置/注册spi_master */
	g_virtual_master = master = spi_alloc_master(&pdev->dev, 0);
	if (master == NULL) {
		dev_err(&pdev->dev, "spi_alloc_master error.\n");
		return -ENOMEM;
	}

	master->transfer = spi_virtual_transfer;
	INIT_WORK(&g_virtual_ws, spi_virtual_work);


	ret = spi_register_master(master);
	if (ret < 0) {
		printk(KERN_ERR "spi_register_master error.\n");
		spi_master_put(master);
		return ret;
	}

	return 0;

	
}

static int spi_virtual_remove(struct platform_device *pdev)
{
	/* 反注册spi_master */
	spi_unregister_master(g_virtual_master);
	return 0;
}


static struct platform_driver spi_virtual_driver = {
	.probe = spi_virtual_probe,
	.remove = spi_virtual_remove,
	.driver = {
		.name = "virtual_spi",
		.of_match_table = spi_virtual_dt_ids,
	},
};

static int virtual_master_init(void)
{
	return platform_driver_register(&spi_virtual_driver);
}

static void virtual_master_exit(void)
{
	platform_driver_unregister(&spi_virtual_driver);
}

module_init(virtual_master_init);
module_exit(virtual_master_exit);

MODULE_DESCRIPTION("Virtual SPI bus driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("www.100ask.net");

