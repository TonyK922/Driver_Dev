#include <linux/module.h>

#include <asm/pgtable.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/major.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/tty.h>

#define MIN(x, y)                                                              \
    ({                                                                         \
        typeof(x) _x = (x);                                                    \
        typeof(y) _y = (y);                                                    \
        (void)(&_x == &_y);                                                    \
        _x < _y ? _x : _y;                                                     \
    })

static ssize_t hello_drv_read(struct file* file,
                              char __user* buf,
                              size_t       size,
                              loff_t*      offset);
static ssize_t hello_drv_write(struct file*       file,
                               const char __user* buf,
                               size_t             size,
                               loff_t*            offset);
static int     hello_drv_open(struct inode* inode, struct file* file);
static int     hello_drv_release(struct inode* inode, struct file* file);
static int     hello_drv_mmap(struct file* file, struct vm_area_struct* vma);

static char*         hello_buf;
static struct class* hello_class;

/* 1. 确定主设备号 */
static int major = 0;

/* 2. 定义 驱动操作函数 结构体 */
static struct file_operations hello_drv = {
    .owner   = THIS_MODULE,
    .open    = hello_drv_open,
    .read    = hello_drv_read,
    .write   = hello_drv_write,
    .release = hello_drv_release,
    .mmap    = hello_drv_mmap,
};

/* 3. 实现 结构体中 相应的操作函数 */
static ssize_t
hello_drv_read(struct file* file, char __user* buf, size_t size, loff_t* offset)
{
    int ret;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    ret = copy_to_user(buf, hello_buf, MIN(1024, size));
    return MIN(1024, size);
}

static ssize_t hello_drv_write(struct file*       file,
                               const char __user* buf,
                               size_t             size,
                               loff_t*            offset)
{
    int ret;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    ret = copy_from_user(hello_buf, buf, MIN(1024, size));
    return MIN(1024, size);
}

static int hello_drv_open(struct inode* inode, struct file* file)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    return 0;
}
static int hello_drv_release(struct inode* inode, struct file* file)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    return 0;
}
static int hello_drv_mmap(struct file* file, struct vm_area_struct* vma)
{
    unsigned long phy = virt_to_phys(hello_buf);

    vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

    if (remap_pfn_range(vma, vma->vm_start, phy >> PAGE_SHIFT,
                        vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
        printk("mmap remap_pfn_range failed\n");
        return -ENOBUFS;
    }

    return 0;
}
/* 4. 注册 结构体 到内核中去 注册驱动程序 */
/* 5. 在入口函数中来完成注册 */
static int __init hello_init(void)
{
    int err;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    hello_buf = kmalloc(8 * 1024, GFP_KERNEL);
    strcpy(hello_buf, "old\n");

    major       = register_chrdev(0, "hello", &hello_drv);
    hello_class = class_create(THIS_MODULE, "hello_class");
    err         = PTR_ERR(hello_class);
    if (IS_ERR(hello_class)) {
        printk("ERR %s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        unregister_chrdev(major, "hello");
        return -1;
    }
    /* /dev/hello */
    device_create(hello_class, NULL, MKDEV(major, 0), NULL,
                  "hello"); /* 需要判返回值 */
    return 0;
}

/* 6. 出口函数 卸载驱动 */
static void __exit hello_exit(void)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    kfree(hello_buf);
    device_destroy(hello_class, MKDEV(major, 0));
    class_destroy(hello_class);
    unregister_chrdev(major, "hello");
}

/* 7. 其他完善：提供设备信息，自动创建设备节点 */

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");