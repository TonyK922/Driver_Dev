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
#include <linux/spi/spi_bitbang.h>

static struct spi_master *g_virtual_master;
static struct spi_bitbang *g_virtual_bitbang;
static struct completion g_xfer_done;


static const struct of_device_id spi_virtual_dt_ids[] = {
	{ .compatible = "100ask,virtual_spi_master", },
	{ /* sentinel */ }
};

/* xxx_isr() { complete(&g_xfer_done)  } */

static int spi_virtual_transfer(struct spi_device *spi,
				struct spi_transfer *transfer)
{
	int timeout;

#if 1	
	/* 1. init complete */
	reinit_completion(&g_xfer_done);

	/* 2. 启动硬件传输 */
	complete(&g_xfer_done);

	/* 3. wait for complete */
	timeout = wait_for_completion_timeout(&g_xfer_done,
					      100);
	if (!timeout) {
		dev_err(&spi->dev, "I/O Error in PIO\n");
		return -ETIMEDOUT;
	}
#endif
	return transfer->len;
}

static void	spi_virtual_chipselect(struct spi_device *spi, int is_on)
{
}


static int spi_virtual_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	int ret;
	
	/* 分配/设置/注册spi_master */
	g_virtual_master = master = spi_alloc_master(&pdev->dev, sizeof(struct spi_bitbang));
	if (master == NULL) {
		dev_err(&pdev->dev, "spi_alloc_master error.\n");
		return -ENOMEM;
	}

	g_virtual_bitbang = spi_master_get_devdata(master);

	init_completion(&g_xfer_done);

	/* 怎么设置spi_master?
	 * 1. spi_master使用默认的函数
	 * 2. 分配/设置 spi_bitbang结构体: 主要是实现里面的txrx_bufs函数
	 * 3. spi_master要能找到spi_bitbang
	 */
	g_virtual_bitbang->master = master;
	g_virtual_bitbang->txrx_bufs  = spi_virtual_transfer;
	g_virtual_bitbang->chipselect = spi_virtual_chipselect;
	master->dev.of_node = pdev->dev.of_node;

#if 0
	ret = spi_register_master(master);
	if (ret < 0) {
		printk(KERN_ERR "spi_register_master error.\n");
		spi_master_put(master);
		return ret;
	}
#else
	ret = spi_bitbang_start(g_virtual_bitbang);
	if (ret) {
		printk("bitbang start failed with %d\n", ret);
		return ret;
	}

#endif

	return 0;

	
}

static int spi_virtual_remove(struct platform_device *pdev)
{
#if 0	
	/* 反注册spi_master */
	spi_unregister_master(g_virtual_master);
#endif
	spi_bitbang_stop(g_virtual_bitbang);
	spi_master_put(g_virtual_master);
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

