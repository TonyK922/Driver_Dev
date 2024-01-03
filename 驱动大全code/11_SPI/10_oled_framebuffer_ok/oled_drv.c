/*
 * Simple synchronous userspace interface to SPI devices
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/acpi.h>
#include <linux/compat.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <linux/gpio/consumer.h>
#include <linux/uaccess.h>

#include <linux/dma-mapping.h>
#include <linux/fb.h>

#define OLED_IOC_INIT 123
#define OLED_IOC_SET_POS 124

// 为0 表示命令，为1表示数据
#define OLED_CMD 0
#define OLED_DATA 1

static struct spi_device* spi_device;
static struct class*      spidev_class;
static struct gpio_desc*  dc_gpio;

static struct fb_info*     myfb_info;
static struct task_struct* oled_thread;
static unsigned int        pseudo_palette[16];
static unsigned char*      oled_buf;  //[1024];

int32_t major;

static int  oled_thread_func(void* para);
static void dc_pin_init(void);
static int  oled_init(void);

/* from pxafb.c */
static inline unsigned int chan_to_field(unsigned int        chan,
                                         struct fb_bitfield* bf)
{
    chan &= 0xffff;
    chan >>= 16 - bf->length;
    return chan << bf->offset;
}

static int mylcd_setcolreg(unsigned        regno,
                           unsigned        red,
                           unsigned        green,
                           unsigned        blue,
                           unsigned        transp,
                           struct fb_info* info)
{
    unsigned int val;

    /* dprintk("setcol: regno=%d, rgb=%d,%d,%d\n",
           regno, red, green, blue); */

    switch (info->fix.visual) {
        case FB_VISUAL_TRUECOLOR:
            /* true-colour, use pseudo-palette */

            if (regno < 16) {
                u32* pal = info->pseudo_palette;

                val = chan_to_field(red, &info->var.red);
                val |= chan_to_field(green, &info->var.green);
                val |= chan_to_field(blue, &info->var.blue);

                pal[regno] = val;
            }
            break;

        default: return 1; /* unknown type */
    }

    return 0;
}

static struct fb_ops myfb_ops = {
    .owner        = THIS_MODULE,
    .fb_setcolreg = mylcd_setcolreg,
    .fb_fillrect  = cfb_fillrect,
    .fb_copyarea  = cfb_copyarea,
    .fb_imageblit = cfb_imageblit,
};

static inline int lcd_fb_init(void)
{
    dma_addr_t phy_addr;
    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

    /* 1.1 分配fb_info */
    myfb_info = framebuffer_alloc(0, NULL);

    /* 1.2 设置fb_info */
    /* a. var : LCD分辨率、颜色格式 */
    myfb_info->var.xres_virtual = myfb_info->var.xres = 128;
    myfb_info->var.yres_virtual = myfb_info->var.yres = 64;

    myfb_info->var.bits_per_pixel = 1; /* rgb565 */

    /* b. fix */
    strcpy(myfb_info->fix.id, "100ask_OLED");
    myfb_info->fix.smem_len = myfb_info->var.xres * myfb_info->var.yres *
                              myfb_info->var.bits_per_pixel / 8;

    myfb_info->flags |= FBINFO_MODULE; /* 禁止显示LOGO 否则 segment fault */

    /* fb的虚拟地址 */
    myfb_info->screen_base =
        dma_alloc_wc(NULL, myfb_info->fix.smem_len, &phy_addr, GFP_KERNEL);
    myfb_info->fix.smem_start = phy_addr; /* fb的物理地址 */

    myfb_info->fix.type   = FB_TYPE_PACKED_PIXELS;
    myfb_info->fix.visual = FB_VISUAL_MONO10;

    myfb_info->fix.line_length =
        myfb_info->var.xres * myfb_info->var.bits_per_pixel / 8;

    /* c. fbops */
    myfb_info->fbops          = &myfb_ops;
    myfb_info->pseudo_palette = pseudo_palette;

    /* 1.3 注册fb_info */
    register_framebuffer(myfb_info);

    /* D. 创建内核线程 */
    oled_buf = kmalloc(1024, GFP_KERNEL);
    dc_pin_init();
    oled_init();

    oled_thread = kthread_run(oled_thread_func, NULL, "Oled_kthread");

    return 0;
}

static inline void lcd_fb_remove(void)
{
    kthread_stop(oled_thread);
    kfree(oled_buf);
    unregister_framebuffer(myfb_info);

    /* B. 释放内存 */
    dma_free_wc(
        NULL, myfb_info->fix.smem_len, myfb_info->screen_base,
        myfb_info->fix.smem_start); /* debug: 由于申请的时候没有给第一个参数,
                                       这里也不要给 */

    /* C. 释放fb_info */
    framebuffer_release(myfb_info);
}

static void dc_pin_init(void)
{
    gpiod_direction_output(dc_gpio, OLED_DATA);
}

static void oled_set_dc_pin(int val)
{
    gpiod_set_value(dc_gpio, val);
}

static void spi_write_datas(const char* buf, int len)
{
    spi_write(spi_device, buf, len);
}

void oled_write_datas(const unsigned char* buf, int len)
{
    oled_set_dc_pin(OLED_DATA);  // 拉高，表示写入数据
    spi_write_datas(buf, len);
}

void oled_write_cmd_data(unsigned char uc_data, unsigned char uc_cmd)
{
    if (uc_cmd == 0) {
        oled_set_dc_pin(OLED_CMD);  // 拉低，表示写入指令
    }
    else {
        oled_set_dc_pin(OLED_DATA);  // 拉高，表示写入数据
    }
    spi_write_datas(&uc_data, 1);
}

static int oled_init(void)
{
    oled_write_cmd_data(0xae, OLED_CMD);  // 关闭显示

    oled_write_cmd_data(0x00, OLED_CMD);  // 设置 lower column address
    oled_write_cmd_data(0x10, OLED_CMD);  // 设置 higher column address

    oled_write_cmd_data(0x40, OLED_CMD);  // 设置 display start line

    oled_write_cmd_data(0xB0, OLED_CMD);  // 设置page address

    oled_write_cmd_data(0x81, OLED_CMD);  // contract control
    oled_write_cmd_data(0x66, OLED_CMD);  // 128

    oled_write_cmd_data(0xa1, OLED_CMD);  // 设置 segment remap

    oled_write_cmd_data(0xa6, OLED_CMD);  // normal /reverse

    oled_write_cmd_data(0xa8, OLED_CMD);  // multiple ratio
    oled_write_cmd_data(0x3f, OLED_CMD);  // duty = 1/64

    oled_write_cmd_data(0xc8, OLED_CMD);  // com scan direction

    oled_write_cmd_data(0xd3, OLED_CMD);  // set displat offset
    oled_write_cmd_data(0x00, OLED_CMD);  //

    oled_write_cmd_data(0xd5, OLED_CMD);  // set osc division
    oled_write_cmd_data(0x80, OLED_CMD);  //

    oled_write_cmd_data(0xd9, OLED_CMD);  // ser pre-charge period
    oled_write_cmd_data(0x1f, OLED_CMD);  //

    oled_write_cmd_data(0xda, OLED_CMD);  // set com pins
    oled_write_cmd_data(0x12, OLED_CMD);  //

    oled_write_cmd_data(0xdb, OLED_CMD);  // set vcomh
    oled_write_cmd_data(0x30, OLED_CMD);  //

    oled_write_cmd_data(0x8d, OLED_CMD);  // set charge pump disable
    oled_write_cmd_data(0x14, OLED_CMD);  //

    oled_write_cmd_data(0xaf, OLED_CMD);  // set dispkay on

    return 0;
}

/**********************************************************************
     * 函数名称： OLED_DIsp_Set_Pos
     * 功能描述：设置要显示的位置
     * 输入参数：@ x ：要显示的column address
                @y :要显示的page address
     * 输出参数： 无
     * 返 回 值：
     * 修改日期 	   版本号	 修改人		  修改内容
     * -----------------------------------------------
     * 2020/03/15		 V1.0	  芯晓		  创建
 ***********************************************************************/
void OLED_DIsp_Set_Pos(int x, int y)
{
    oled_write_cmd_data(0xb0 + y, OLED_CMD);
    oled_write_cmd_data((x & 0x0f), OLED_CMD);
    oled_write_cmd_data(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
}

static long spidev_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    int x, y;
    switch (cmd) {
        case OLED_IOC_INIT: {
            dc_pin_init();
            oled_init();
            break;
        }
        case OLED_IOC_SET_POS: {
            x = arg & 0xFF;
            y = (arg >> 8) & 0xFF;
            OLED_DIsp_Set_Pos(x, y);

            break;
        }

        default: break;
    }
    return 0;
}

static ssize_t spidev_write(struct file*       filp,
                            const char __user* buf,
                            size_t             count,
                            loff_t*            f_pos)
{
    char* ker_buf;
    int   err;

    ker_buf = kmalloc(count, GFP_KERNEL);
    err     = copy_from_user(ker_buf, buf, count);

    oled_set_dc_pin(1);  // 拉高，表示写入数据
    spi_write_datas(ker_buf, count);
    kfree(ker_buf);
    return count;
}

static int oled_thread_func(void* para)
{
    unsigned char* p[8];
    unsigned char  data[8];
    int            i, j, line, bit, k;
    unsigned char  byte;
    unsigned char* fb = myfb_info->screen_base;

    while (!kthread_should_stop()) {
        /* 1. 从Framebuffer得到数据 */
        /* 2. 转换格式 */
        k = 0;
        for (i = 0; i < 8; i++) {
            for (line = 0; line < 8; line++)
                p[line] = &fb[i * 128 + line * 16];

            for (j = 0; j < 16; j++) {
                for (line = 0; line < 8; line++) {
                    data[line] = *p[line];
                    p[line] += 1;
                }

                for (bit = 0; bit < 8; bit++) {
                    byte = (((data[0] >> bit) & 1) << 0) |
                           (((data[1] >> bit) & 1) << 1) |
                           (((data[2] >> bit) & 1) << 2) |
                           (((data[3] >> bit) & 1) << 3) |
                           (((data[4] >> bit) & 1) << 4) |
                           (((data[5] >> bit) & 1) << 5) |
                           (((data[6] >> bit) & 1) << 6) |
                           (((data[7] >> bit) & 1) << 7);

                    oled_buf[k++] = byte;
                }
            }
        }
        /* 3. 通过SPI发送给OLED */
        for (i = 0; i < 8; i++) {
            OLED_DIsp_Set_Pos(0, i);
            oled_set_dc_pin(1);
            spi_write_datas(&oled_buf[i * 128], 128);
        }

        /* 4. 休眠一会 */
        schedule_timeout_interruptible(HZ);
    }
    return 0;
}
static const struct file_operations spidev_fops = {
    .owner = THIS_MODULE,
    /* REVISIT switch to aio primitives, so that userspace
     * gets more complete API coverage.  It'll simplify things
     * too, except for the locking.
     */
    .write          = spidev_write,
    .unlocked_ioctl = spidev_ioctl,
};

static int spidev_probe(struct spi_device* spi)
{
    spi_device = spi;

    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

    major        = register_chrdev(0, "spi_oled", &spidev_fops);
    spidev_class = class_create(THIS_MODULE, "spi_oled_class");
    device_create(spidev_class, NULL, MKDEV(major, 0), NULL, "spi_oled");

    dc_gpio = gpiod_get(&spi->dev, "dc", 0);

    /* 分配 fb_info */
    /* 设置 fb_info */
    /* 注册 fb_info */
    lcd_fb_init();

    return 0;
}

static int spidev_remove(struct spi_device* spi)
{
    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

    lcd_fb_remove();
    gpiod_put(dc_gpio);
    device_destroy(spidev_class, MKDEV(major, 0));
    class_destroy(spidev_class);
    unregister_chrdev(major, "spi_oled");
    return 0;
}

static const struct of_device_id spidev_dt_ids[] = {
    {
        .compatible = "100ask,oled",
    },
    {},
};
static struct spi_driver spidev_spi_driver = {
    .driver =
        {
            .name           = "spi_oled_drv",
            .of_match_table = of_match_ptr(spidev_dt_ids),
        },
    .probe  = spidev_probe,
    .remove = spidev_remove,
};

static int __init spidev_init(void)
{
    int status;

    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

    status = spi_register_driver(&spidev_spi_driver);
    if (status < 0) {}
    return status;
}
module_init(spidev_init);

static void __exit spidev_exit(void)
{
    printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
    spi_unregister_driver(&spidev_spi_driver);
}
module_exit(spidev_exit);

MODULE_LICENSE("GPL");
