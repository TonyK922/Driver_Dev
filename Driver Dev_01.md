# Linux Driver Dev

>Linux 驱动 = 软件框架 + 硬件操作

# 嵌入式Linux驱动开发基础

# 准备工作:

驱动程序依赖于 Linux 内核，你为开发板 A 开发驱动，那就先在 Ubuntu 中得到、配置、编译开发板 A 所使用的 Linux 内核。

完成下面操作：

硬件部分：
- ① 开发板接线：串口线、电源线、网线
- ② 开发板烧写系统

软件部分：
- ① 下载 Linux 内核，Windows 和 Ubuntu 下各放一份
- ② Windows 下：使用 Source Insight 创建内核源码的工程，这是用来浏览内核、编辑驱动
- ③ Ubuntu 下：安装工具链，配置、编译 Linux 内核

- 编译内核 驱动等

## 为什么编译驱动程序之前要先编译内核？

- 驱动程序要用到内核文件：
	- 比如驱动程序中这样包含头文件：#include <asm/io.h>，其中的 asm 是一个链接文件，指向 asm-arm 或 asm-mips，这需要先配置、编译内核才会生成asm 这个链接文件。

- 编译驱动时用的内核、开发板上运行到内核，要一致：
	- 开发板上运行到内核是出厂时烧录的，你编译驱动时用的内核是你自己编译的，这两个内核不一致时会导致一些问题。所以我们编译驱动程序前，要把自己编译出来到内核放到板子上去，替代原来的内核。

- 更换板子上的内核后，板子上的其他驱动也要更换：
	- 板子使用新编译出来的内核时，板子上原来的其他驱动也要更换为新编译出来的。所以在编译我们自己的第 1 个驱动程序之前，要先编译内核、模块，并且放到板子上去。

- 编译内核

不同的开发板对应不同的配置文件，配置文件位于内核源码arch/arm/configs/目录。kernel 的编译过程如下：

```shell
make mrproper
make 100ask_imx6ull_defconfig
make zImage -j4
make dtbs
cp arch/arm/boot/zImage ~/nfs_rootfs
cp arch/arm/boot/dts/100ask_imx6ull-14x14.dtb ~/nfs_rootfs
```

- 编译完成后，在 arch/arm/boot 目录下生成 zImage 内核文件, 在arch/arm/boot/dts 目 录下 生成设备 树的二 进制文件 100ask_imx6ull-14x14.dtb。把这 2 个文件复制到/home/book/nfs_rootfs 目录下备用


- 编译内核模块
- 进入内核源码目录后，就可以编译内核模块了：
```shell
make modules
```

- 安装内核模块到 Ubuntu 某个目录下备用
- 可以先把内核模块安装到 nfs 目录(/home/book/nfs_rootfs)。注意：后面会使用 tree 命令查看目录结构，如果提示没有该命令，需要执行以下命令安装 tree 命令：
```shell
cd ~/100ask_imx6ull-sdk/Linux-4.9.88/
make ARCH=arm INSTALL_MOD_PATH=/home/book/nfs_rootfs modules_install
```

- 安装内核和模块到开发板上

假设：在 Ubuntu 的/home/book/nfs_rootfs 目录下，已经有了 zImage、dtb 文件，并且有 lib/modules 子目录(里面含有各种模块)。接下来要把这些文件复制到开发板上。假设 Ubuntu IP 为 192.168.5.11，在开发板上执行以下命令：
```shell
mount -t nfs -o nolock,vers=3 192.168.5.11:/home/book/nfs_rootfs /mnt
cp /mnt/zImage /boot
cp /mnt/100ask_imx6ull-14x14.dtb /boot
cp /mnt/lib/modules /lib -rfd
sync
```

最后重启开发板，它就使用新的 zImage、dtb、模块了。

```shell
insmod xx.ko  #安装驱动模块
lsmod  查看模块
```

- 如果你没有更新板子上的内核，会出现类似如图所示错误：
- ![](assets/Pasted%20image%2020230720233418.png)
- 可以强行安装驱动程序，比如使用“insmod -f hello_drv.ko”这样的命令，它会提示说“内核已经被污染了”，但是不影响学习、不影响使用，如图
- ![](assets/Pasted%20image%2020230720233437.png)

# Hello 驱动(不涉及硬件操作)

> APP 打开的文件在内核中如何表示

APP 打开文件时，可以得到一个整数，这个整数被称为文件句柄。对于 APP 的每一个文件句柄，在内核里面都有一个“struct file”与之对应。
- ![](assets/Pasted%20image%2020230719220748.png)

使用 open 打开文件时，传入的 flags、mode 等参数会被记录在内核中对应的 struct file 结构体里(f_flags、f_mode)：
- `int open(const char *pathname, int flags, mode_t mode);`

读写文件时，文件的当前偏移地址也会保存在 struct file 结构体的 f_pos 成员里。
- ![](assets/Pasted%20image%2020230720145852.png)

> 打开字符设备节点时，内核中也有对应的 struct file

- 注意这个结构体中的结构体：`struct file_operations *f_op`，这是由驱动程序提供的。
- ![](assets/Pasted%20image%2020230720104000.png)
- ![](assets/Pasted%20image%2020230720104004.png)
- ![](assets/Pasted%20image%2020230720151142.png)
	- 应用程序打开某个设备的节点时, 会根据这个设备的主设备号, 在内核文件管理数组中找到相应file_operations 结构体, 这个结构体里, 提供了驱动程序的操作函数. open这个节点, 就会调用file_ops 里的.open函数, 后面读写, 也是调用该结构体里的.read .write函数.

- 结构体 struct file_operations 的定义如下
- ![](assets/Pasted%20image%2020230720104025.png)

> 怎么编写驱动程序？

- ① 确定主设备号，也可以让内核分配
- ② 定义自己的 file_operations 结构体
- ③ 实现对应的 drv_open/drv_read/drv_write 等函数，填入 file_operations 结构体
- ④ 把 file_operations 结构体告诉内核：register_chrdev
- ⑤ 谁来注册驱动程序啊？得有一个入口函数：安装驱动程序时，就会去调用这个入口函数
- ⑥ 有入口函数就应该有出口函数：卸载驱动程序时，出口函数调用 unregister_chrdev
	- ⑦ 其他完善：提供设备信息，自动创建设备节点：class_create, device_create

![](assets/Pasted%20image%2020230720150012.png)

- 应用层通过系统调用, 陷入内核态, 内核态去执行相应的系统调用函数.
- 如果是访问普通文件, 那就走普通的文件系统那套操作函数. 
- 如果是访问驱动设备文件, 那就走驱动程序的操作函数. 驱动程序提供自己的操作函数.
- 实现的驱动操作函数, 封装在一个结构体中, 把这个结构体告诉内核, 方法是 register_chrdev 注册驱动程序.
- 把这个结构体的地址, 放到内核中存放操作函数的地方. 具体哪一项, 就是由主设备号来确定. 后面就会根据这个主设备号, 来寻找这个操作函数结构体, 进而调用.
- 入口函数来调用注册驱动程序函数.

> 编写代码

- 写驱动程序

参考 driver/char 中的程序，包含头文件，写框架，传输数据：
- 驱动中实现 open, read, write, release，APP 调用这些函数时，都打印内核信息
-  APP 调用 write 函数时，传入的数据保存在驱动中
-  APP 调用 read 函数时，把驱动中保存的数据返回给 APP

- 1. 确定主设备号
- 2. 定义自己的file_operations结构体
- 3. 实现对应的open/read/write等函数，填入file_operations结构体
- 4. 把file_operations结构体告诉内核：注册驱动程序
- 5. 入口函数：来注册驱动程序, 安装驱动程序时，就会去调用这个入口函数
- 6. 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数
- 7. 其他完善：提供设备信息，自动创建设备节点

阅读一个驱动程序，从它的入口函数开始，hello_init 就是入口函数。它的主要工作就是，向内核注册一个 file_operations 结构体：hello_drv，这就是字符设备驱动程序的核心。

file_operations 结构体 hello_drv 在前面定义，里面提供了open/read/write/release 成员，应用程序调用 open/read/write/close 时就会导致这些成员函数被调用。

file_operations 结构体 hello_drv 中的成员函数都比较简单，大多数只是打印而已。要注意的是，驱动程序和应用程序之间传递数据要使用copy_from_user/copy_to_user函数。

- 测试

驱动程序中包含了很多头文件，这些头文件来自内核，不同的 ARM 板它的某些头文件可能不同。所以编译驱动程序时，需要指定板子所用的内核的源码路径。

要编译哪个文件？这也需要指定，设置 obj-m 变量即可怎么把.c 文件编译为驱动程序.ko？这要借助内核的顶层 Makefile。
- ![](assets/Pasted%20image%2020230720234328.png)
- ![](assets/Pasted%20image%2020230720234602.png)
- ![](assets/Pasted%20image%2020230720234634.png)
- ![](assets/Pasted%20image%2020230720234710.png)
- 内核打印没开, 用 dmesg 查看保存的内核打印.
- ![](assets/Pasted%20image%2020230720234802.png)


# 一些补充知识

## module_init/module_exit 的实现

- 一个驱动程序有入口函数、出口函数，代码如下：
```c
module_init(hello_init);
module_exit(hello_exit);
```

- 驱动程序可以被编进内核里，也可以被编译为 ko 文件后手工加载。对于这两种形式，“`module_init/module_exit`”这 2 个宏是不一样的。在内核文件“`include\linux\module.h`”中可以看到这 2 个宏：
```c
01 #ifndef MODULE
02
03 #define module_init(x) __initcall(x);
04 #define module_exit(x) __exitcall(x);
05
06 #else /* MODULE */
07
08 #define module_init(initfn) \
09
10 /* Each module must use one module_init(). */
11 static inline initcall_t __inittest(void) \
12 { return initfn; } \
13 int init_module(void) __attribute__((alias(#initfn)));
14
15 /* This is only required if you want to be unloadable. */
16 #define module_exit(exitfn) \
17 static inline exitcall_t __exittest(void) \
18 { return exitfn; } \
19 void cleanup_module(void) __attribute__((alias(#exitfn)));
20
21 #endif
```

- 编译驱动程序时，我们执行“make modules”这样的命令，它在编译 c 文件时会定义宏 MODULE，比如：
- `arm-buildroot-linux-gnueabihf-gcc -DMODULE -c -o hello_drv.o hello_drv.c`

在编译内核时，并不会定义宏 MODULE。所以，“module_init/module_exit”这 2 个宏在驱动程序被编进内核时，如上面代码中第 3、4 行那样定义；在驱动程序被编译为 ko 文件时，如上面代码中第 11~19 行那样定义。

把上述代码里的宏全部展开后，得到如下代码：
```c
01 #ifndef MODULE
01
01 #define module_init(fn)
02 static initcall_t __initcall_##fn##id __used \
03 __attribute__((__section__(".initcall" #6 ".init"))) = fn;
04
05 #define module_exit(x) static exitcall_t __exitcall_##x __used __section(.exitcall.exit) = x;
06
07 #else /* MODULE */
08
09 #define module_init(initfn) \
10
11 /* Each module must use one module_init(). */
12 static inline initcall_t __inittest(void) \
13 { return initfn; } \
14 int init_module(void) __attribute__((alias(#initfn)));
15
16 /* This is only required if you want to be unloadable. */
17 #define module_exit(exitfn) \
18 static inline exitcall_t __exittest(void) \
19 { return exitfn; } \
20 void cleanup_module(void) __attribute__((alias(#exitfn)));
21
22 #endif
```

- 驱动程序被编进内核时，把“module_init(hello_init)”、“module_exit(hello_exit)”展开，得到如下代码：
```c
static initcall_t __initcall_hello_init6 __used \
__attribute__((__section__(".initcall6.init"))) = hello_init;
static exitcall_t __exitcall_hello_exit __used __section(.exitcall.exit) = hello_exit;
```

- 其中的“initcall_t”、“exitcall_t”就是函数指针类型，所以上述代码就是定义了两个函数指针：第 1 个函数指针名为__initcall_hello_init6，放在段".initcall6.init"里；第 2 个函数指针名为__exitcall_hello_exit，放在段“.exitcall.exit”里。
- 内核启动时，会去段".initcall6.init"里取出这些函数指针来执行，所以驱动程序的入口函数就被执行了。
- 一个驱动被编进内核后，它是不会被卸载的，所以段“.exitcall.exit”不会被用到，内核启动后会释放这块段空间。

- 驱动程序被编译为ko文件时，把“module_init(hello_init)”、“module_exit(hello_exit)”展开,得到如下代码：
```c
static inline initcall_t __inittest(void) \
{ return hello_init; } \
int init_module(void) __attribute__((alias("hello_init")));

static inline exitcall_t __exittest(void) \
{ return hello_exit; } \
void cleanup_module(void) __attribute__((alias("hello_exit")));
```

- 分别定义了2个函数：第1个函数名为init_module，它是 hello_init 函数的别名；第 2 个函数名为cleanup_module，它是 hello_exit 函数的别名。

- 以后我们使用 insmod 命令加载驱动时，内核都是调用 init_module 函数，实际上就是调用 hello_init 函数；使用 rmmod 命令卸载驱动时，内核都是调用cleanup_module 函数，实际上就是调用 hello_exit 函数。

## register_chrdev 的内部实现

register_chrdev 函数源码如下：
```c
static inline int register_chrdev(unsigned int major, const char *name,
								  const struct file_operations *fops)
{
	return __register_chrdev(major, 0, 256, name, fops);
}
```

它调用__register_chrdev 函数，这个函数的代码精简如下：

```c
01 int __register_chrdev(unsigned int major, unsigned int baseminor,
02                       unsigned int count, const char *name,
03                       const struct file_operations *fops)
04 {
05     struct char_device_struct *cd;
06     struct cdev *cdev;
07     int err = -ENOMEM;
08
09     cd = __register_chrdev_region(major, baseminor, count, name);
10
11     cdev = cdev_alloc();
12
13     cdev->owner = fops->owner;
14     cdev->ops = fops;
15     kobject_set_name(&cdev->kobj, "%s", name);
16
17     err = cdev_add(cdev, MKDEV(cd->major, baseminor), count);
18 }
```

- 这个函数主要的代码是第 09 行、第 11~15 行、第 17 行。
- 第 09 行，调用__register_chrdev_region 函数来“注册字符设备的区域”，它仅仅是查看设备号(major, baseminor)到(major, baseminor+count-1)有没有被占用，如果未被占用的话，就使用这块区域。
- 在前面的课程里，在引入驱动程序时为了便于理解，我们说内核里有一个chrdevs数组，根据主设备号major在`chrdevs[major]`中放入file_operations 结构体，以后 open/read/write 某个设备文件时，就是根据主设备号从`chrdevs[major]`中取出 file_operations 结构体，调用里面的open/read/write 函数指针。上述说法并不准确，内核中确实有一个 chrdevs 数组：
```c
static struct char_device_struct {
	struct char_device_struct *next;
	unsigned int major;
	unsigned int baseminor;
	int minorct;
	char name[64];
	struct cdev *cdev; /* will die */
} *chrdevs[CHRDEV_MAJOR_HASH_SIZE];
```

- 去访问它的时候，并不是直接使用主设备号 major 来确定数组项，而是使用如下函数来确定数组项：
```c
/* index in the above */
static inline int major_to_index(unsigned major)
{
	return major % CHRDEV_MAJOR_HASH_SIZE;
}
```

- 上述代码中，CHRDEV_MAJOR_HASH_SIZE 等于 255。比如主设备号1、256, 都会使用 chardevs[1]。 chardevs[1] 是一个链表，链表里有多个char_device_struct 结构体，某个结构体表示主设备号为 1 的设备，某个结构体表示主设备号为 256 的设备。
	- 数据结构中, 即为 哈希表 的 链地址法.
	- chardevs 的结构图如图 2.6 所示：
	- ![](assets/Pasted%20image%2020230721105446.png)
- 可以得出如下结论：
- ① `chrdevs[i]`数组项是一个链表头
- 链表里每一个元素都是一个 char_device_struct 结构体，每个元素表示一个驱动程序。char_device_struct 结构体内容如下：
```c
struct char_device_struct {
	struct char_device_struct *next;
	unsigned int major;
	unsigned int baseminor;
	int minorct;
	char name[64];
	struct cdev *cdev; /* will die */
}
```
- 它指定了主设备号 major、次设备号 baseminor、个数 minorct，在 cdev中含有 file_operations 结构体。
- char_device_struct结构体的含义是: 主次设备号为(major,baseminor)、(major, baseminor+1)、(major, baseminor+2)、(major, baseminor+ minorct-1)的这些设备, 都使用同一个 file_operations 来操作。
- 以前为了更容易理解驱动程序时，说“内核通过主设备号找到对应的file_operations 结构体”，这并不准确。应该改成：“内核通过主、次设备号，找到对应的 file_operations 结构体”。

- ② 在图中，chardevs[1]中有 3 个驱动程序
- 第 1 个 char_device_struct 结构体对应主次设备号(1, 0)、(1, 1)，这是第 1 个驱动程序。
- 第2个char_device_struct结构体对应主次设备号(1, 2)、(1, 2)、……、(1, 11)，这是第 2 个驱动程序。
- 第 3 个 char_device_struct 结构体对应主次设备号(256, 0)，这是第 3个驱动程序。
- 第 11~15 行分配一个 cdev 结构体，并设置它：它含有 file_operations结构体。
- 第 17 行调用 cdev_add 把 cdev 结构体注册进内核里，cdev_add 函数代码如下：
```c
01 int cdev_add(struct cdev *p, dev_t dev, unsigned count)
02 {
03     int error;
04
05     p->dev = dev;
06     p->count = count;
07
08     error = kobj_map(cdev_map, dev, count, NULL,
09     exact_match, exact_lock, p);
10     if (error)
11         return error;
12
13     kobject_get(p->kobj.parent);
14
15     return 0;
16 }
```

- 这个函数涉及 kobj 的操作，这是一个通用的链表操作函数。它的作用是：把 cdev 结构体放入 cdev_map 链表中，对应的索引值是“dev”到“dev+count-1”。以后可以从 `cdev_map` 链表中快速地使用索引值取出对应的 cdev。
- 比如执行以下代码：`err = cdev_add(cdev, MKDEV(1, 2), 10);`
- 其中的 `MKDEV(1,2)` 构造出一个整数“`1<<8 | 2`”, 即 `0x102`; 上述代码将 `cdev` 放入 `cdev_map` 链表中，对应的索引值是 `0x102` 到 `0x10c(即 0x102+10)`。以后根据这 10 个数值(0x102、0x103、0x104、……、0x10c)中任意一个，都可以快速地从 cdev_map 链表中取出 cdev 结构体。
- APP 打开某个字符设备节点时，进入内核。在内核里根据字符设备节点的主、次设备号，计算出一个数值(major<<8 | minor，即 inode->i_rdev)，然后使用这个数值从 cdev_map 中快速得到 cdev，再从 cdev 中得到 file_operations结构体。
- 关键函数如下：
- 找到驱动程序的关键函数
	- ![](assets/Pasted%20image%2020230723155504.png)
- 在打开文件的过程中，可以看到并未涉及 chrdevs，都是使用 cdev_map。所以可以看到在 chrdevs 的定义中看到如下注释：
	- ![](assets/Pasted%20image%2020230723160044.png)

## class_destroy/device_create 浅析

驱动程序的核心是 file_operations 结构体：分配、设置、注册它。“class_destroy/device_create”函数知识起一些辅助作用：在/sys 目录下创建一些目录、文件，这样 Linux 系统中的 APP(比如 udev、mdev)就可以根据这些目录或文件来创建设备节点。

以下代码将会在“/sys/class”目录下创建一个子目录“hello_class”：`hello_class = class_create(THIS_MODULE, "hello_class");`

以下代码将会在“/sys/class/hello_class”目录下创建 一个文 件“hello”：`device_create(hello_class, NULL, MKDEV(major, 0), NULL, "hello");`

![](assets/Pasted%20image%2020230723160225.png)


# 普适的 GPIO 引脚操作方法

GPIO: General-purpose input/output，通用的输入输出口

## GPIO 模块一般结构

- a. 有多组 GPIO，每组有多个 GPIO
- b. 使能：电源/时钟
- c. 模式(Mode)：引脚可用于 GPIO 或其他功能
- d. 方向：引脚 Mode 设置为 GPIO 时，可以继续设置它是输出引脚，还是输入引脚
- e. 数值：
	- 对于输出引脚，可以设置寄存器让它输出高、低电平
	- 对于输入引脚，可以读取寄存器得到引脚的当前电平

## GPIO 寄存器操作

- a. 芯片手册一般有相关章节，用来介绍：power/clock 可以设置对应寄存器使能某个 GPIO 模块(Module)有些芯片的 GPIO 是没有使能开关的，即它总是使能的
- b. 一个引脚可以用于 GPIO、串口、USB 或其他功能，有对应的寄存器来选择引脚的功能
- c. 对于已经设置为 GPIO 功能的引脚，有方向寄存器用来设置它的方向：输出、输入
- d. 对于已经设置为 GPIO 功能的引脚，有数据寄存器用来写、读引脚电平状态

GPIO寄存器的 2 种操作方法：

- 原则：不能影响到其他位

- a. 直接读写：读出、修改对应位、写入

```C
要设置 bit n：

val = *data_reg;
val = val | (1<<n);
*data_reg = val;

要清除 bit n：

val = *data_reg;
val = val & ~(1<<n);
*data_reg = val;
```

- b. `set-and-clear protocol`(有些处理器, 不是所有): 
	- set_reg, clr_reg, data_reg 三个寄存器对应的是同一个物理寄存器,
		- 要设置 bit n：set_reg = (1<<n);
		- 要清除 bit n：clr_reg = (1<<n);

GPIO 的其他功能：防抖动、中断、唤醒：

6ULL GPIO 见裸机开发部分. Driver Dev_00.md

# 最简单的 LED 驱动程序

## 字符设备驱动程序框架

- 字符设备驱动程序的框架：
	- ![](assets/Pasted%20image%2020230731200200.png)
	- ![](assets/Pasted%20image%2020230731200646.png)
- 编写驱动程序的套路：
	- ① 确定主设备号，也可以让内核分配
	- ② 定义自己的 file_operations 结构体(驱动核心)
	- ③ 实现对应的 drv_open/drv_read/drv_write 等函数，填入 file_operations 结构体
	- ④ 把 file_operations 结构体`告诉内核`：register_chrdev. 系统会分配(或你自己指定)一个主设备号(一个int型整数), 来索引这个结构体地址.
	- ⑤ 谁来注册驱动程序啊？得有一个`入口函数`：安装驱动程序时，就会去调用这个入口函数
	- ⑥ 有入口函数就应该有`出口函数`：卸载驱动程序时，出口函数调用`unregister_chrdev`
	- ⑦ 其他完善：提供设备信息，`自动创建设备节点`：class_create, device_create

- 驱动怎么操作硬件？
	- 通过 ioremap 映射寄存器的物理地址得到虚拟地址，读写虚拟地址。
	- 实际上，它是按页(4096 字节)进行映射的，是整页整页地映射的。
	- 假设 phys_addr = 0x10002，size=4，ioremap 的内部实现是：
		- a. phys_addr 按页取整，得到地址 0x10000
		- b. size 按页取整，得到 4096
		- c. 把起始地址 0x10000，大小为 4096 的这一块物理地址空间，映射到虚拟地址空间，假设得到的虚拟空间起始地址为 0xf0010000
		- d. 那么 phys_addr = 0x10002 对应的 virt_addr = 0xf0010002
- 驱动怎么和 APP 传输数据？
	- 通过 copy_to_user、copy_from_user 这 2 个函数。

# LED 驱动程序框架

## 回顾字符设备驱动程序框架

![](assets/Pasted%20image%2020230802172549.png)

# 对于LED驱动, 我们想要什么样的接口?

- LED 驱动程序接口定义构思
- ![](assets/Pasted%20image%2020230802185744.png)

# LED 驱动能支持多个板子的基础: 分层思想

- ① 把驱动拆分为通用的框架(leddrv.c)、具体的硬件操作(board_X.c)：
- 拆分驱动构成通用框架
- ![](assets/Pasted%20image%2020230802173744.png)
- ![](assets/Pasted%20image%2020230803223331.png)

- ② 以面向对象的思想，改进代码，抽象出一个结构体：
- 面向对象的思想抽象出结构体
- ![](assets/Pasted%20image%2020230802173806.png)

- 每个单板相关的 board_X.c 实现自己的 led_operations 结构体，供上层的 leddrv.c 调用：
- ![](assets/Pasted%20image%2020230802173826.png)
- ![](assets/Pasted%20image%2020230803223348.png)

# 具体单板的 LED 驱动程序

## 怎么写 LED 驱动程序？

详细步骤如下：
- ① 看原理图确定引脚，确定引脚输出什么电平才能点亮/熄灭 LED
- ② 看主芯片手册，确定寄存器操作方法：哪些寄存器？哪些位？地址是？
- ③ 编写驱动：先写框架，再写硬件操作的代码

注意：在芯片手册中确定的寄存器地址被称为物理地址，在 Linux 内核中无法直接使用。
需要使用内核提供的 ioremap 把物理地址映射为虚拟地址，使用虚拟地址。

ioremap 函数的使用：

- ① 函数原型：
	- ![](assets/Pasted%20image%2020230803223947.png)
	- 使用时，要包含头文件：`#include <asm/io.h>`

- ② 它的作用：
- 把物理地址 phys_addr 开始的一段空间(大小为 size)，映射为虚拟地址；返回值是该段虚拟地址的首地址。
	- `virt_addr = ioremap(phys_addr, size);`

实际上，它是按页(4096 字节)进行映射的，是整页整页地映射的。

假设 phys_addr = 0x10002，size=4，ioremap 的内部实现是：
- a. phys_addr 按页取整，得到地址 0x10000
- b. size 按页取整，得到 4096
- c. 把起始地址 0x10000，大小为 4096 的这一块物理地址空间，映射到虚拟地址空间，假设得到的虚拟空间起始地址为 0xf0010000
- d. 那么 phys_addr = 0x10002 对应的 virt_addr = 0xf0010002

- ③ 不再使用该段虚拟地址时，要 iounmap(virt_addr)：
	- ![](assets/Pasted%20image%2020230803224137.png)

volatile 的使用：

为了避免编译器自动优化，需要加上 volatile，告诉它“这是容易出错的，别乱优化”.

# 驱动设计的思想：面向对象/分层/分离

## 面向对象

- 字符设备驱动程序抽象出一个 file_operations 结构体；
- 我们写的程序针对硬件部分抽象出 led_operations 结构体。

## 分层

- 上下分层，比如我们前面写的 LED 驱动程序就分为 2 层：
- ① 上层实现硬件无关的操作，比如注册字符设备驱动：leddrv.c
- ② 下层实现硬件相关的操作，比如 board_A.c 实现单板 A 的 LED 操作
- ![](assets/Pasted%20image%2020230809121449.png)

## 分离

还能不能改进？`分离`。

在 board_A.c 中，实现了一个 led_operations，为 LED 引脚实现了初始化函数、控制函数：
```c
static struct led_operations board_demo_led_opr = {
.num = 1,
.init = board_demo_led_init,
.ctl = board_demo_led_ctl,
};
```

如果硬件上更换一个引脚来控制 LED 怎么办？你要去修改上面结构体中的 init、ctl 函数。

实际情况是，每一款芯片它的 GPIO 操作都是类似的。以假设举例，比如：GPIO1_3、GPIO5_4 这 2 个引脚接到 LED：
- ① GPIO1_3 属于第 1 组，即 GPIO1。
	- 有方向寄存器 DIR、数据寄存器 DR 等，基础地址是 addr_base_addr_gpio1。
	- 设置为 output 引脚：修改 GPIO1 的 DIR 寄存器的 bit3。
	- 设置输出电平：修改 GPIO1 的 DR 寄存器的 bit3。
- ② GPIO5_4 属于第 5 组，即 GPIO5。
	- 有方向寄存器 DIR、数据寄存器 DR 等，基础地址是 addr_base_addr_gpio5。
	- 设置为 output 引脚：修改 GPIO5 的 DIR 寄存器的 bit4。
	- 设置输出电平：修改 GPIO5 的 DR 寄存器的 bit4。

既然引脚操作那么有规律，并且这是跟主芯片相关的，那可以针对该芯片写出比较通用的硬件操作代码。

比如 board_A.c 使用芯片 chipY，那就可以写出：chipY_gpio.c，它实现芯片 Y 的 GPIO操作，适用于芯片 Y 的所有 GPIO 引脚。

使用时，我们只需要在 board_A_led.c 中指定使用哪一个引脚即可。

程序结构如下：

- ![](assets/Pasted%20image%2020230809121753.png)
- ![](assets/Pasted%20image%2020230809121808.png)

以面向对象的思想，在 board_A_led.c 中实现 led_resouce 结构体，它定义“资源”──要用哪一个引脚。

在 chipY_gpio.c 中仍是实现 led_operations 结构体，它要写得更完善，支持所有 GPIO。

# 驱动进化之路：总线设备驱动模型

- 驱动模型示例
- ![](assets/Pasted%20image%2020230809181753.png)

## 驱动编写的 3 种方法

以 LED 驱动为例。

- 传统写法
- 传统写法架构
- ![](assets/Pasted%20image%2020230809181855.png)
	- 使用哪个引脚，怎么操作引脚，都写死在代码中。
	- 最简单，不考虑扩展性，可以快速实现功能。
	- 修改引脚时，需要重新编译。

- 总线设备驱动模型
- ![](assets/Pasted%20image%2020230809182538.png)
- 引入 platform_device/platform_driver，将`“资源”与“驱动”分离`开来
- 代码稍微复杂，但是易于扩展。不仅支持led设备还有其他各种设备.
- 冗余代码太多，修改引脚时`设备端的代码需要重新编译内核`
- 更换引脚时, 上图中的 led_drv.c 基本不用改, 但是需要修改 led_dev.c。

- 设备树
- 设备树驱动模型
- ![](assets/Pasted%20image%2020230809182718.png)
- 通过配置文件 ── 设备树来定义“资源”。
- 代码稍微复杂，但是易于扩展。
- 无冗余代码，`修改引脚时只需要修改 dts 文件`并`编译得到 dtb 文件`，`把它传给内核`. dts就是在内核之外定义设备资源, 避免了增加设备就得动内核代码, 导致内核代码垃圾.
- 无需重新编译内核/驱动。

## 在 Linux 中实现“分离”：Bus/Dev/Drv 模型

- BUS/Dev/Drv 模型

![](assets/Pasted%20image%2020230809182915.png)

![](assets/Pasted%20image%2020230809183013.png)

## 匹配规则

- 最先比较
	- `platform_device.driver_override` 和 `platform_driver.driver.name`
	- 可以设置 platform_device 的 driver_override，强制选择某个 platform_driver。

- 然后比较
	- `platform_device.name` 和 `platform_driver.id_table[i].name`
	- platform_driver.id_table 是“platform_device_id”指针，表示该 drv 支持若干个 device，它里面列出了各个 device 的{`.name`, `.driver_data`}，其中的“name”表示该drv 支持的设备的名字，driver_data 是些提供给该 device 的私有数据。

- 最后比较
	- `platform_device.name` 和 `platform_driver.driver.name`
	- platform_driver.id_table 可能为空，这时可以根据 platform_driver.driver.name 来寻找同名的 platform_device。

- platform_match()
- ![](assets/Pasted%20image%2020230809215010.png)
	
- 函数调用关系
- ![](assets/Pasted%20image%2020230809183215.png)
- 从上面的 设备注册 跟 驱动注册函数来看, 都会先放入链表, 然后去匹配两边的链表里的设备跟驱动.
## 常用函数

这些函数可查看内核源码：drivers/base/platform.c，根据函数名即可知道其含义。

下面摘取常用的几个函数。

- 注册/反注册
	- platform_device_register/ platform_device_unregister
	- platform_driver_register/ platform_driver_unregister
	- platform_add_devices // 注册多个 device

- 获得资源
- 返回该 dev 中某类型(type)资源中的第几个(num)：
	- `struct resource *platform_get_resource(struct platform_device *dev, unsigned int type, unsigned int num)`
- 返回该 dev 所用的第几个(num)中断：
	- `int platform_get_irq(struct platform_device *dev, unsigned int num)`
- 通过名字(name)返回该 dev 的某类型(type)资源：
	- `struct resource *platform_get_resource_byname(struct platform_device *dev, unsigned int type, const char *name)`
- 通过名字(name)返回该 dev 的中断号：
	- `int platform_get_irq_byname(struct platform_device *dev， const char *name)`

## 怎么写程序

- 分配/设置/注册 platform_device 结构体
	- 在里面定义所用资源，指定设备名字

- 分配/设置/注册 platform_driver 结构体
	- 在其中的 probe 函数里，分配/设置/注册 file_operations 结构体，并从 platform_device 中确实所用硬件资源。指定 platform_driver 的名字。

# LED 模板驱动程序的改造：总线设备驱动模型

看代码吧: /MyLinuxDrv/led_bus_dev_drv 下的代码.

# 驱动进化之路：设备树的引入及简明教程

官方文档(可以下载到 devicetree-specification-v0.2.pdf):
https://www.devicetree.org/specifications/

内核文档:
Documentation/devicetree/booting-without-of.txt

韦东山录制“设备树视频”时写的文档：设备树详细分析.txt
这个 txt 文件也同步上传到 wiki 了：http://wiki.100ask.org/Linux_devicetree

## 设备树的引入与作用

以 LED 驱动为例，如果你要更换 LED 所用的 GPIO 引脚，需要修改驱动程序源码、重新编译驱动、重新加载驱动。

在内核中，使用同一个芯片的板子，它们所用的外设资源不一样，比如 A 板用 GPIO A，B 板用 GPIO B。而 GPIO 的驱动程序既支持 GPIO A 也支持 GPIO B，你需要指定使用哪一个引脚，怎么指定？在 c 代码中指定。

随着 ARM 芯片的流行，内核中针对这些 ARM 板保存有大量的、没有技术含量的文件。
Linus 大发雷霆：`"this whole ARM thing is a f*cking pain in the ass"`。

于是，Linux 内核开始引入设备树。

设备树并不是重新发明出来的，在 Linux 内核中其他平台如 PowerPC，早就使用设备树来描述硬件了。

Linus 发火之后，内核开始全面使用设备树来改造，神人就神人！

有一种错误的观点，说“新驱动都是用设备树来写了”。设备树不可能用来写驱动。请想想，要操作硬件就需要去操作复杂的寄存器，如果设备树可以操作寄存器，那么它就是“驱动”，它就一样很复杂。

设备树只是用来`给内核里的驱动程序, 指定硬件的信息`。比如 LED 驱动，在内核的驱动程序里去操作寄存器，但是操作哪一个引脚？这由设备树指定。

你可以事先体验一下设备树，板子启动后执行下面的命令：
```
# ls /sys/firmware/
devicetree fdt
```

/sys/firmware/devicetree 目录下是以目录结构程现的 dtb 文件, 根节点对应 base 目录,每一个节点对应一个目录, 每一个属性对应一个文件。

这些属性的值如果是字符串，可以使用 cat 命令把它打印出来；对于数值，可以用hexdump 把它打印出来。

一个单板启动时，u-boot 先运行，它的作用是启动内核。U-boot 会把内核和设备树文件都读入内存，然后启动内核。在启动内核时会把设备树在内存中的地址告诉内核。

## 设备树的语法

- 为什么叫“树”？
- 总线“树”:
- ![](assets/Pasted%20image%2020230815184510.png)

怎么描述这棵树？

我们需要编写设备树文件(dts: device tree source), 它需要编译为dtb(device tree blob)文件, 内核使用的是 dtb 文件.

dts 文件是根本, 它的语法很简单.
- 下面是一个设备树示例：
- ![](assets/Pasted%20image%2020230815184859.png)

- 它对应的 dts 文件如下：
	- ![](assets/Pasted%20image%2020230815193832.png)

### Devicetree 格式

1. DTS 文件的格式
- DTS 文件布局(layout):
```
/dts-v1/; // 表示版本
[memory reservations] // 格式为: /memreserve/ <address> <length>;
/ {
	[property definitions]
	[child nodes]
};
```

2. node 的格式
- 设备树中的基本单元，被称为“node”，其格式为：
```
[label:] node-name[@unit-address] {
	[properties definitions]
	[child nodes]
};
```
- label 是标号，可以省略。label 的作用是为了方便地引用 node，比如：
```
/dts-v1/;
/ {
	uart0: uart@fe001000 {
		compatible="ns16550";
		reg=<0xfe001000 0x100>;
	};
};
```
- 可以使用下面 2 种方法来修改 uart@fe001000 这个 node：
```
// 在根节点之外使用 label 引用 node：
&uart0 {
	status = “disabled”;
};

或在根节点之外使用全路径：
&{/uart@fe001000} {
	status = “disabled”;
};
```

3. properties 的格式
- 简单地说，properties 就是“name=value”，value 有多种取值方式。
	- Property 格式 1:  `[label:] property-name = value;`
	- Property 格式 2(没有值):  `[label:] property-name;`
- Property 取值只有 3 种:
```
1. arrays of cells(1个或多个32位数据, 64位数据使用2个32位数据表示),
2. string(字符串),
3. bytestring(1个或多个字节)
```

示例:

a) Arrays of cells : cell 就是一个 32 位的数据，用尖括号包围起来 `interrupts = <17 0xc>;`

b) 64bit 数据使用2个cell来表示，用尖括号包围起来: `clock-frequency = <0x00000001 0x00000000>;`

c) A null-terminated string (有结束符的字符串)，用双引号包围起来: `compatible = "simple-bus";`

d) A bytestring(字节序列) ，用中括号包围起来:
- `local-mac-address = [00 00 12 34 56 78]; //每个byte使用2个16进制数来表示`
- `local-mac-address = [000012345678]; //每个byte使用2个16进制数来表示`

e) 可以是各种值的组合, 用逗号隔开:
- `compatible = "ns16550", "ns8250";
- `example = <0xf00f0000 19>, "a strange property format";`

### dts 文件包含 dtsi 文件

设备树文件不需要我们从零写出来，内核支持了某款芯片比如 imx6ull，在内核的arch/arm/boot/dts 目录下就有了能用的设备树模板，一般命名为 xxxx.dtsi。

“i”表示“include”，被别的文件引用的。

我们使用某款芯片制作出了自己的单板，所用资源跟 xxxx.dtsi 是大部分相同，小部分不同，所以需要引脚 xxxx.dtsi 并修改。

dtsi 文件跟 dts 文件的语法是完全一样的。
dts 中可以包含.h 头文件，也可以包含 dtsi 文件，在.h 头文件中可以定义一些宏。

- 示例imx6ull pro 14x14.dts：
```
/dts-v1/;
#include <dt-bindings/input/input.h>
#include "imx6ull.dtsi"

/ {
	……
};
```

### 常用的节点(node)

① 根节点

- dts 文件中必须有一个根节点：
```
/dts-v1/;
/ {
	model = "SMDK24440";
	compatible = "samsung,smdk2440";
	
	#address-cells = <1>;
	#size-cells = <1>;
};
```

- 根节点中`必须`有这些属性：
```
#address-cells // 在它的子节点的 reg 属性中, 使用多少个 u32 整数来描述地址(address)
#size-cells   // 在它的子节点的 reg 属性中, 使用多少个 u32 整数来描述大小(size)
compatible   // 定义一系列的字符串, 用来指定内核中哪个machine_desc可以支持本设备
			// 即这个板子兼容哪些平台
		   // uImage : smdk2410 smdk2440 mini2440 ==> machine_desc

model    // 咱这个板子是什么
		// 比如有 2 款板子配置基本一致, 它们的 compatible 是一样的
	   // 那么就通过 model 来分辨这 2 款板子
```

② CPU 节点

一般不需要我们设置，在 dtsi 文件中都定义好了：
```
cpus {
	#address-cells = <1>;
	#size-cells = <0>;
	cpu0: cpu@0 {
		.......
	};
};
```

③ memory 节点

芯片厂家不可能事先确定你的板子使用多大的内存，所以 memory 节点需要板厂设置，比如:

```
memory {
	reg = <0x80000000 0x20000000>;
};
```

④ chosen 节点

我们可以通过设备树文件给内核传入一些参数，这要在 chosen 节点中设置 bootargs 属性：

```
chosen {
    bootargs = "noinitrd root=/dev/mtdblock4 rw init=/linuxrc  console=ttySAC0,115200";
};
```

### 常用的属性

① `#address-cells 和 #size-cells`

cell 指一个32位(字或半字)的值.

- address-cells：address要用多少个32位数来表示；
- size-cells：size 要用多少个32位数来表示。

比如一段内存，怎么描述它的起始地址和大小？

下例中，address-cells 为 1，所以 reg 中用 1 个数来表示地址，即用 0x80000000 来表示地址；size-cells 为 1，所以 reg 中用 1 个数来表示大小，即用 0x20000000 表示大小：

```
/ {
	#address-cells = <1>;
	#size-cells = <1>;
	memory {
		reg = <0x80000000 0x20000000>;
	};
};
```
- 这里的address-cells 跟 size-cells 影响的是子节点里的reg属性.

② compatible

“compatible”表示“兼容”，对于某个 LED，内核中可能有 A、B、C 三个驱动都支持它，那可以这样写：
```
led {
	compatible = “A”, “B”, “C”;
};
```
- 内核启动时，就会为这个 LED 按这样的优先顺序为它找到驱动程序：A、B、C。

- 根节点下也有 compatible 属性，用来选择哪一个“machine desc”：一个内核可以支持machine A，也支持 machine B，内核启动后会根据根节点的 compatible 属性找到对应的machine desc 结构体，执行其中的初始化函数。

- compatible 的值，建议取这样的形式："manufacturer,model"，即“厂家名,模块名”。
	- ![](assets/Pasted%20image%2020230816165212.png)

- 注意：machine desc 的意思就是“机器描述”，学到内核启动流程时才涉及。

③ model

- model 属性与 compatible 属性有些类似，但是有差别。
- compatible 属性是一个字符串列表，表示可以你的硬件兼容 A、B、C 等驱动；
- model 用来`准确地定义这个硬件是什么`。

- 比如根节点中可以这样写：
```
/ {
	compatible = "samsung,smdk2440", "samsung,mini2440";
	model = "jz2440_v3";
};
```

- 它表示这个单板，可以兼容内核中的“smdk2440”，也兼容“mini2440”。
- 从 compatible 属性中可以知道它兼容哪些板，但是它到底是什么板？用 model 属性来明确。
- 上面的例子表示我这个设备就是jz2440, 但是可以用smdk2440(优先选这个)跟mini2440来初始化.


③ status

`dtsi 文件`中定义了很多设备，但是在你的板子上某些设备是没有的。这时你可以给这个设备节点添加一个 status 属性，设置为“disabled”：

```
&uart1 {
	status = "disabled";
};
```

- ![](assets/Pasted%20image%2020230816170636.png)

④ reg

reg 的本意是 register，用来表示寄存器地址。

但是在设备树里，它可以用来描述一段空间。反正对于 ARM 系统，寄存器和内存是统一编址的，即访问寄存器时用某块地址，访问内存时用某块地址，在访问方法上没有区别。

reg 属性的值，是一系列的“address size”，用多少个 32 位的数来表示 address 和 size，由其父节点的#address-cells、#size-cells 决定。

- 示例：
```
/dts-v1/;
/ {
	#address-cells = <1>;
	#size-cells = <1>;
	memory {
		reg = <0x80000000 0x20000000>;
	};
};
```

- reg属性也可以用来表示CPU
- ![](assets/Pasted%20image%2020230816204340.png)

⑤ name(过时了，建议不用)

它的值是字符串，用来表示节点的名字。在跟 platform_driver 匹配时，优先级最低。compatible 属性在匹配过程中，优先级最高。

⑥ device_type(过时了，建议不用)

它的值是字符串，用来表示节点的类型。在跟 platform_driver 匹配时，优先级为中。compatible 属性在匹配过程中，优先级最高。

但这俩在内核匹配的时候要用到.

## 编译、更换设备树

我们一般不会从零写 dts 文件，而是修改。程序员水平有高有低，改得对不对？需要编译一下。并且内核直接使用 dts 文件的话，就太低效了，它也需要使用二进制格式的 dtb 文件。

- 看一个内核的dts文件
![](assets/Pasted%20image%2020230816211204.png)

- 在内核中直接 make

设置 ARCH、CROSS_COMPILE、PATH 这三个环境变量后，进入 ubuntu 上板子内核源码的目录，执行如下命令即可编译 dtb 文件：`make dtbs V=1`

以 IMX6UL 为例，可以看到如下输出：
```
mkdir -p arch/arm/boot/dts/ ;
arm-linux-gnueabihf-gcc -E
-Wp,-MD,arch/arm/boot/dts/.imx6ull-14x14-ebf-mini.dtb.d.pre.tmp
-nostdinc
-I./arch/arm/boot/dts
-I./arch/arm/boot/dts/include
-I./drivers/of/testcase-data
-undef -D__DTS__ -x assembler-with-cpp
-o arch/arm/boot/dts/.imx6ull-14x14-ebf-mini.dtb.dts.tmp
arch/arm/boot/dts/imx6ull-14x14-ebf-mini.dts ;

./scripts/dtc/dtc -O dtb
-o arch/arm/boot/dts/imx6ull-14x14-ebf-mini.dtb
-b 0 -i arch/arm/boot/dts/ -Wno-unit_address_vs_reg
-d arch/arm/boot/dts/.imx6ull-14x14-ebf-mini.dtb.d.dtc.tmp
arch/arm/boot/dts/.imx6ull-14x14-ebf-mini.dtb.dts.tmp ;
```

它首先用 arm-linux-gnueabihf-gcc 预处理 dts 文件，把其中的.h 头文件包含进来，把宏展开。
然后使用设备树编译器 scripts/dtc/dtc 生成 dtb 文件。
可见，dts 文件之所以支持“#include”语法，是因为 arm-linux-gnueabihf-gcc 帮忙。
如果只用 dtc 工具，它是不支持”#include”语法的，只支持“/include”语法。

- 手工编译

除非你对设备树比较了解，否则不建议手工使用 dtc 工具直接编译。

内核目录下 scripts/dtc/dtc 是设备树的编译工具，直接使用它的话，包含其他文件时不能使用“#include”，而必须使用“/incldue”。

编译、反编译的示例命令如下，“-I”指定输入格式，“-O”指定输出格式，“-o”指定输出文件：
```
// 编译 dts 为 dtb
./scripts/dtc/dtc -I dts -O dtb -o tmp.dtb arch/arm/boot/dts/xxx.dts 

// 反编译 dtb 为 dts
./scripts/dtc/dtc -I dtb -O dts -o tmp.dts arch/arm/boot/dts/xxx.dtb 
```

- 给开发板更换设备树文件

怎么给各个单板编译出设备树文件，它们的设备树文件是哪一个？这些操作步骤在各个开发板的高级用户使用手册.

基本方法都是：设置 ARCH、CROSS_COMPILE、PATH 这三个环境变量后，在内核源码目录中执行：`make dtbs` 

对于我们的6ull板子:
设备树文件是：内核源码目录中 arch/arm/boot/dts/100ask_imx6ull-14x14.dtb
然后把这个dtb文件拷贝到 板子的/boot下.

- 板子启动后查看设备树

板子启动后执行下面的命令：
```
# ls /sys/firmware/
devicetree fdt
```

/sys/firmware/devicetree 目录下是以目录结构程现的 dtb 文件, 根节点对应 base 目录, 每一个节点对应一个目录, 每一个属性对应一个文件。

这些属性的值如果是字符串，可以使用 cat 命令把它打印出来；对于数值，可以用hexdump 把它打印出来。

还可以看到/sys/firmware/fdt 文件，它就是 dtb 格式的设备树文件，可以把它复制出来放到 ubuntu 上，执行下面的命令反编译出来(-I dtb：输入格式是 dtb，-O dts：输出格式是dts)：

```
cd 板子所用的内核源码目录
./scripts/dtc/dtc -I dtb -O dts /从板子上/复制出来的/fdt -o tmp.dts
```

- ![](assets/Pasted%20image%2020230817013429.png)
- ![](assets/Pasted%20image%2020230817013622.png)

## 内核对设备树的处理

从源代码文件 dts 文件开始，设备树的处理过程为：
- ![](assets/Pasted%20image%2020230817190623.png)

- ① dts 在 PC 机上被编译为 dtb 文件;
- ② u-boot 把 dtb 文件传给内核;
- ③ 内核解析 dtb 文件, 把 `每一个节点` 都转换为 `device_node` 结构体;
- ④ 对于 `某些device_node结构体`，会被转换为 platform_device 结构体.


- dtb 中每一个节点都被转换为 device_node 结构体

![](assets/Pasted%20image%2020230817195423.png)
- name 跟 type 字符指针, 就是来自dts里的name 和 device_type
- properties 是指可能有多个属性. property结构体里 value 的长度由length指定
- 内核处理节点 是从根节点开始的

根节点被保存在 `全局变量of_root` 中, 从of_root开始可以访问到任意节点.


- 哪些设备树节点会被转换为 platform_device

- ① `根节点`下含有 `compatible 属性` 的`子节点`
- ② 含有特定 compatible 属性的节点的子节点
	- 如果一个节点的 compatible 属性, 它的值是这 4 者之一：`"simple-bus","simple-mfd","isa","arm,amba-bus"`, 那么它的子结点(需含compatible属性)也可以转换为 platform_device. (根节点的子节点A有这几个属性, 那A的子节点(根节点的孙节点)有compatible 就可以转换为 platform_device)
- ③ 总线 I2C、SPI 节点下的子节点：不转换为 platform_device
	- 某个总线下到子节点, 应该交给对应的总线驱动程序来处理, 它们不应该被转换为 platform_device.

- 比如以下的节点中：
```
/ {
	mytest {
		compatible = "mytest", "simple-bus";
		mytest@0 {
			compatible = "mytest_0";
		};
	};
	
	i2c {
		compatible = "samsung,i2c";
		at24c02 {
			compatible = "at24c02";
		};
	};
	
	spi {
		compatible = "samsung,spi";
		flash@0 {
			compatible = "winbond,w25q32dw";
			spi-max-frequency = <25000000>;
			reg = <0>;
		};
	};
};
```

- /mytest 会被转换为 platform_device, 因为它兼容"simple-bus"; 它的子节点/mytest/mytest@0 也会被转换为 platform_device
- /i2c 节点一般表示 i2c 控制器, 它会被转换为 platform_device, 在内核中有对应的 platform_driver;
- /i2c/at24c02 节点不会被转换为 platform_device, 它被如何处理完全由父节点的 platform_driver 决定, 一般是被创建为一个 i2c_client。
- 类似的也有/spi 节点, 它一般也是用来表示 SPI 控制器, 它会被转换为platform_device, 在内核中有对应的 platform_driver;
- /spi/flash@0 节点不会被转换为 platform_device, 它被如何处理完全由父节点的 platform_driver 决定, 一般是被创建为一个 spi_device.


- 怎么转换为 platform_device

内核处理设备树的函数调用过程，这里不去分析；我们只需要得到如下结论：
- platform_device 中含有 resource 数组, 它来自 device_node 的 reg, interrupts 属性;
- platform_device.dev.of_node 指向 device_node, 可以通过它获得其他属性


## platform_device 如何与 platform_driver 配对

从设备树转换得来的 platform_device 会被注册进内核里，以后当我们每注册一个platform_driver 时，它们就会两两确定能否配对，如果能配对成功就调用 platform_driver 的probe 函数。(变成了总线设备驱动)

套路是一样的。我们需要将前面讲过的“匹配规则”再完善一下：

先贴源码：
- ![](assets/Pasted%20image%2020230818143129.png)

- ① 最先比较：是否强制选择某个 driver

比较 platform_device. driver_override 和 platform_driver.driver.name
可以设置 platform_device 的 driver_override，强制选择某个 platform_driver。

- ② 然后比较：设备树信息

比较：platform_device. dev.of_node 和 platform_driver.driver.of_match_table。

由设备树节点转换得来的 platform_device 中，struct device下, 含有一个结构体struct device_node：of_node。它的类型如下：
- ![](assets/Pasted%20image%2020230818143230.png)
- ![](assets/Pasted%20image%2020230820012026.png)

如果一个 platform_driver 支持设备树，它的 platform_driver.driver.of_match_table 是一个数组，类型如下：
- ![](assets/Pasted%20image%2020230820005418.png)
- ![](assets/Pasted%20image%2020230818143346.png)

使用设备树信息来判断 dev 和 drv 是否配对时:
- 首先，如果 of_match_table 中含有 compatible 值，就跟 dev 的 compatible 属性比较，若一致则成功，否则返回失败；优先级最高这个
- 其次，如果 of_match_table 中含有 type 值，就跟 dev 的 device_type 属性比较，若一致则成功，否则返回失败；优先级其次
- 最后，如果 of_match_table 中含有 name 值，就跟 dev 的 name 属性比较，若一致则成功，否则返回失败。

而设备树中建议不再使用 devcie_type 和 name 属性，所以基本上只使用设备节点的 compatible 属性来寻找匹配的 platform_driver。


- ③ 接下来比较：platform_device_id

比较platform_device. name和platform_driver.id_table[i].name，id_table中可能有多项。

platform_driver.id_table 是“platform_device_id”指针，表示该 drv 支持若干个 device，它里面列出了各个 device 的{.name, .driver_data}，其中的“name”表示该 drv 支持的设备的名字，driver_data 是些提供给该 device 的私有数据。

- ④ 最后比较：platform_device.name 和 platform_driver.driver.name

platform_driver.id_table 可能为空，这时可以根据 platform_driver.driver.name 来寻找同名的 platform_device。

概括出了这个图：

![](assets/Pasted%20image%2020230818144949.png)

![](assets/Pasted%20image%2020230820213923.png)

![](assets/Pasted%20image%2020230818145014.png)

![](assets/Pasted%20image%2020230818145018.png)


## 没有转换为 platform_device 的节点，如何使用

任意驱动程序里，都可以直接访问设备树。

你可以使用下面介绍的函数找到节点，读出里面的值。


## 内核里操作设备树的常用函数

内核源码中 include/linux/目录下有很多 of 开头的头文件，of 表示“open firmware”即开放固件。

> 内核中设备树相关的头文件介绍

内核源码中 include/linux/目录下有很多 of 开头的头文件，of 表示“open firmware”即开放固件。

设备树的处理过程是：dtb -> device_node -> platform_device

### ① 处理 DTB

```c
of_fdt.h // dtb 文件的相关操作函数, 我们一般用不到,
		// 因为 dtb 文件在内核中已经被转换为 device_node 树(它更易于使用)
```

### ② 处理 device_node

```c
of.h // 提供设备树的一般处理函数,
	// 比如 of_property_read_u32(读取某个属性的 u32 值),
	// of_get_child_count(获取某个 device_node 的子节点数)
of_address.h // 地址相关的函数,
			// 比如 of_get_address(获得 reg 属性中的 addr, size 值)
			// of_match_device (从 matches 数组中取出与当前设备最匹配的一项)
of_dma.h // 设备树中 DMA 相关属性的函数
of_gpio.h // GPIO 相关的函数
of_graph.h // GPU 相关驱动中用到的函数, 从设备树中获得 GPU 信息
of_iommu.h // 很少用到
of_irq.h // 中断相关的函数
of_mdio.h // MDIO (Ethernet PHY) API
of_net.h // OF helpers for network devices.
of_pci.h // PCI 相关函数
of_pdt.h // 很少用到
of_reserved_mem.h // reserved_mem 的相关函数
```

### ③ 处理 platform_device

```c
of_platform.h // 把 device_node 转换为 platform_device 时用到的函数,
			// 比如 of_device_alloc(根据 device_node 分配设置 platform_device),
			// of_find_device_by_node (根据 device_node 查找到 platform_device),
			// of_platform_bus_probe (处理 device_node 及它的子节点)
of_device.h // 设备相关的函数, 比如 of_match_device
```


> platform_device 相关的函数

of_platform.h 中声明了很多函数，但是作为驱动开发者，我们只使用其中的 1、2 个。
其他的都是给内核自己使用的，内核使用它们来处理设备树，转换得到 platform_device。

① of_find_device_by_node

函数原型为：`extern struct platform_device *of_find_device_by_node(struct device_node *np);`

设备树中的每一个节点，在内核里都有一个 device_node；你可以使用 device_node 去找到对应的 platform_device。

② `platform_get_resource`

这个函数跟设备树没什么关系，但是设备树中的节点被转换为 platform_device 后，设备树中的 reg 属性、interrupts 属性也会被转换为`"resource"`。
- 原来在内核的`.c` 文件中定义的平台设备资源 的方法:
	- ![](assets/Pasted%20image%2020230823162355.png)
	- ![](assets/Pasted%20image%2020230823162451.png)

这时，你可以使用这个函数取出这些资源。

函数原型为：
```c
/**
* platform_get_resource - get a resource for a device
* @dev: platform device
* @type: resource type // 取哪类资源？IORESOURCE_MEM、IORESOURCE_REG
* // IORESOURCE_IRQ 等
* @num: resource index // 这类资源中的哪一个？
*/

struct resource *platform_get_resource(struct platform_device *dev,
unsigned int type, unsigned int num);
```

- 对于设备树节点中的 `reg` 属性，它属性 IORESOURCE_MEM 类型的资源；
- 对于设备树节点中的 `interrupts` 属性，它属性 IORESOURCE_IRQ 类型的资源。


> 有些节点不会生成 platform_device，怎么访问它们

内核会把 dtb 文件解析出一系列的 device_node 结构体，我们可以直接访问这些device_node。
内核源码 incldue/linux/of.h 中声明了 device_node 和属性 property 的操作函数device_node 和 property 的结构体定义如下：

![](assets/Pasted%20image%2020230818145503.png)

### ① 找到节点

a. of_find_node_by_path

根据路径找到节点，比如“/”就对应根节点，“/memory”对应 memory 节点。函数原型：
`static inline struct device_node *of_find_node_by_path(const char *path);`

b. of_find_node_by_name

根据名字找到节点，节点如果定义了 name 属性，那我们可以根据名字找到它。函数原型：
`extern struct device_node *of_find_node_by_name(struct device_node *from, const char *name);`

参数 from 表示从哪一个节点开始寻找，传入 NULL 表示从根节点开始寻找。
但是在设备树的官方规范中不建议使用“name”属性，所以这函数也不建议使用。

c. of_find_node_by_type

根据类型找到节点，节点如果定义了 device_type 属性，那我们可以根据类型找到它。函数原型：`extern struct device_node *of_find_node_by_type(struct device_node *from, const char *type);`

参数 from 表示从哪一个节点开始寻找，传入NULL表示从根节点开始寻找。
但是在设备树的官方规范中不建议使用“device_type”属性，所以这函数也不建议使用.

d. of_find_compatible_node

根据 compatible 找到节点，节点如果定义了 compatible 属性，那我们可以根据compatible 属性找到它。函数原型：

`extern struct device_node *of_find_compatible_node(struct device_node *from, const char *type, const char *compat);`

参数 from 表示从哪一个节点开始寻找，传入 NULL 表示从根节点开始寻找。
参数 compat 是一个字符串，用来指定 compatible 属性的值；
参数 type 是一个字符串，用来指定 device_type 属性的值，可以传入 NULL。

e. of_find_node_by_phandle

根据 phandle 找到节点。

dts 文件被编译为 dtb 文件时，每一个节点都有一个数字 ID，这些数字 ID 彼此不同。可以使用数字 ID 来找到 device_node。这些数字 ID 就是 phandle。函数原型：

`extern struct device_node *of_find_node_by_phandle(phandle handle);`

参数 from 表示从哪一个节点开始寻找，传入 NULL 表示从根节点开始寻找。

f. of_get_parent

找到 device_node 的父节点。函数原型：
`extern struct device_node *of_get_parent(const struct device_node *node);`

参数 from 表示从哪一个节点开始寻找，传入 NULL 表示从根节点开始寻找。

g. of_get_next_parent

这个函数名比较奇怪，怎么可能有“next parent”？
它实际上也是找到 device_node 的父节点，跟 of_get_parent 的返回结果是一样的。
差别在于它多调用下列函数，把 node 节点的引用计数减少了 1。这意味着调用
of_get_next_parent 之后，你不再需要调用 of_node_put 释放 node 节点。

`of_node_put(node);`

函数原型：
`extern struct device_node *of_get_next_parent(struct device_node *node);`

参数 from 表示从哪一个节点开始寻找，传入 NULL 表示从根节点开始寻找

h. of_get_next_child

取出下一个子节点. 函数原型：
`extern struct device_node *of_get_next_child(const struct device_node *node, struct device_node *prev);`

参数 node 表示父节点；
prev 表示上一个子节点，设为 NULL 时表示想找到第 1 个子节点。
不断调用 of_get_next_child 时，不断更新 pre 参数，就可以得到所有的子节点。

i. of_get_next_available_child

取出下一个“可用”的子节点，有些节点的 status 是“disabled”，那就会跳过这些节点。函数原型：
```c
struct device_node *of_get_next_available_child(const struct device_node *node,
												struct device_node *prev);
```

参数 node 表示父节点；
prev 表示上一个子节点，设为 NULL 时表示想找到第 1 个子节点。.


j. of_get_child_by_name

根据名字取出子节点。函数原型：
```c
extern struct device_node *of_get_child_by_name(const struct device_node *node,
												const char *name);
```

参数 node 表示父节点； name 表示子节点的名字。

### ② 找到属性

内核源码 incldue/linux/of.h 中声明了 device_node 的操作函数，当然也包括属性的操作函数。

a. of_find_property

找到节点中的属性。函数原型：
```c
extern struct property *of_find_property(const struct device_node *np,
										const char *name,
										int *lenp);
```

- 参数 np 表示节点，我们要在这个节点中找到名为 name 的属性。
- lenp 用来保存这个属性的长度，即它的值的长度。

在设备树中，节点大概是这样：
```
xxx_node {
		xxx_pp_name = “hello”;
};
```

上述节点中，“xxx_pp_name”就是属性的名字，值的长度是 6。


### ③ 获取属性的值

a. of_get_property

根据名字找到节点的属性，并且返回它的值。函数原型：
```c
/*
* Find a property with a given name for a given node
* and return the value.
*/
const void *of_get_property(const struct device_node *np, const char *name,
							int *lenp)
```

参数 np 表示节点，我们要在这个节点中找到名为 name 的属性，然后返回它的值。
lenp 用来保存这个属性的长度，即它的值的长度。

b. of_property_count_elems_of_size

根据名字找到节点的属性，确定它的值有多少个元素(elem)。函数原型：

```c
/* of_property_count_elems_of_size - Count the number of elements in a property
*
* @np:
device node from which the property value is to be read.
* @propname:
name of the property to be searched.
* @elem_size:
size of the individual element
*
* Search for a property in a device node and count the number of elements of
* size elem_size in it. Returns number of elements on sucess, -EINVAL if the
* property does not exist or its length does not match a multiple of elem_size
* and -ENODATA if the property does not have a value.
*/
int of_property_count_elems_of_size(const struct device_node *np,
								const char *propname, int elem_size)
```

参数 np 表示节点，我们要在这个节点中找到名为 propname 的属性，然后返回下列结果：
`return prop->length / elem_size;`

在设备树中，节点大概是这样：

```
xxx_node {
		xxx_pp_name = <0x50000000 1024> <0x60000000 2048>;
};
```

调用 of_property_count_elems_of_size(np, “xxx_pp_name”, 8)时，返回值是 2；
调用 of_property_count_elems_of_size(np, “xxx_pp_name”, 4)时，返回值是 4。

c. 读整数 u32/u64

函数原型为：
```c
static inline int of_property_read_u32(const struct device_node *np,
									const char *propname,
									u32 *out_value);

extern int of_property_read_u64(const struct device_node *np,
								const char *propname, u64 *out_value);
```

在设备树中，节点大概是这样：

```
xxx_node {
	name1 = <0x50000000>;
	name2 = <0x50000000 0x60000000>;
};
```

调用 of_property_read_u32 (np, “name1”, &val)时，val 将得到值 0x50000000；

调用 of_property_read_u64 (np, “name2”, &val)时，val 将得到值 0x0x6000000050000000。

d. 读某个整数 u32/u64

函数原型为：

```c
extern int of_property_read_u32_index(const struct device_node *np,
									const char *propname,
									u32 index, u32 *out_value);
```

在设备树中，节点大概是这样：
```
xxx_node {
	name2 = <0x50000000 0x60000000>;
};
```

调用 of_property_read_u32 (np, “name2”, 1, &val)时，val 将得到值 0x0x60000000。

e. 读数组

函数原型为：

```c
int of_property_read_variable_u8_array(const struct device_node *np,
								const char *propname, u8 *out_values,
								size_t sz_min, size_t sz_max);

int of_property_read_variable_u16_array(const struct device_node *np,
									const char *propname, u16 *out_values,
									size_t sz_min, size_t sz_max);

int of_property_read_variable_u32_array(const struct device_node *np,
									const char *propname, u32 *out_values,
									size_t sz_min, size_t sz_max);

int of_property_read_variable_u64_array(const struct device_node *np,
									const char *propname, u64 *out_values,
									size_t sz_min, size_t sz_max);
```

在设备树中，节点大概是这样：
```
xxx_node {
	name2 = <0x50000012 0x60000034>;
};
```

上述例子中属性 name2 的值，长度为 8。

调用 of_property_read_variable_u8_array (np, “name2”, out_values, 1, 10)时，out_values中将会保存这 8 个字节： 0x12,0x00,0x00,0x50,0x34,0x00,0x00,0x60。

调用 of_property_read_variable_u16_array (np, “name2”, out_values, 1, 10)时，out_values中将会保存这 4 个 16 位数值： 0x0012, 0x5000,0x0034,0x6000。

总之，这些函数要么能取到全部的数值，要么一个数值都取不到；

如果值的长度在 sz_min 和 sz_max 之间，就返回全部的数值；否则一个数值都不返回。

f. 读字符串

函数原型为：
```c
int of_property_read_string(const struct device_node *np, const char *propname,
							const char **out_string);
```

返回节点 np 的属性(名为 propname)的值，(`*out_string`)指向这个值，把它当作字符串。


### 怎么修改设备树文件

一个写得好的驱动程序, 它会尽量确定所用资源。
只把不能确定的资源留给设备树, 让设备树来指定。
根据原理图确定`"驱动程序无法确定的硬件资源"`, 再在设备树文件中填写对应内容。
那么, 所填写内容的格式是什么?

① 使用芯片厂家提供的工具

有些芯片，厂家提供了对应的设备树生成工具，可以选择某个引脚用于某些功能，就可以自动生成设备树节点。
你再把这些节点复制到内核的设备树文件里即可。

② 看绑定文档

内核文档 Documentation/devicetree/bindings/
做得好的厂家也会提供设备树的说明文档

③ 参考同类型单板的设备树文件

④ 网上搜索

⑤ 实在没办法时, 只能去研究驱动源码

## LED 模板驱动程序的改造：设备树

See part2

