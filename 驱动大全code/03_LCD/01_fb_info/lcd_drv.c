#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>

#include <asm/div64.h>

#include <asm/mach/map.h>
#include <mach/fb.h>
#include <mach/regs-gpio.h>
#include <mach/regs-lcd.h>

static struct fb_info* myfb_info;

static struct fb_ops myfb_ops = {
    .owner        = THIS_MODULE,
    .fb_fillrect  = cfb_fillrect,
    .fb_copyarea  = cfb_copyarea,
    .fb_imageblit = cfb_imageblit,
};
/* init func */
static int __init lcd_drv_init(void)
{
    dma_addr_t phy_addr;

    /* alloc fb_info */
    myfb_info = framebuffer_alloc(0, NULL);
    if (!myfb_info) {
        printk("framebuffer_alloc err !\n");
        return -1;
    }
    /* setting fb_info */
    /* a. var : LCD分辨率 颜色格式 */
    myfb_info->var.xres           = 1024;
    myfb_info->var.yres           = 600;
    myfb_info->var.bits_per_pixel = 16; /* rgb565 */

    myfb_info->var.red.offset   = 11; /* 从bit11 开始 5位 */
    myfb_info->var.red.length   = 5;
    myfb_info->var.green.offset = 5;
    myfb_info->var.green.length = 6;
    myfb_info->var.blue.offset  = 0;
    myfb_info->var.blue.length  = 5;

    /* b. fix */
    myfb_info->fix.smem_len = (myfb_info->var.xres * myfb_info->var.yres *
                               myfb_info->var.bits_per_pixel) >>
                              3;
    if (myfb_info->var.bits_per_pixel == 24)
        myfb_info->fix.smem_len = myfb_info->var.xres * myfb_info->var.yres * 4;
    /* 24bpp 实际一个像素是32位 */

    myfb_info->fix.type   = FB_TYPE_PACKED_PIXELS;
    myfb_info->fix.visual = FB_VISUAL_TRUECOLOR;

    /* fb的虚拟地址 */
    myfb_info->screen_base =
        dma_alloc_wc(NULL, myfb_info->fix.smem_len, &phy_addr, GFP_KERNEL);

    myfb_info->fix.smem_start = phy_addr;

    /* c. fbops */
    myfb_info->fbops = &myfb_ops;

    /* register fb_info */
    register_framebuffer(myfb_info);

    /* hardware ops */

    return 0;
}

/* exit func */
static void __exit lcd_drv_exit(void)
{
    /* unregister fb_info */
    unregister_framebuffer(myfb_info);

    /* free fb_info */
    framebuffer_release(myfb_info);
}

moudule_init(lcd_drv_init);
moudule_exit(lcd_drv_exit);

MODULE_AUTHOR("Tony");
MODULE_DESCRIPTION("Framebuffer driver frame for the linux");
MODULE_LICENSE("GPL");