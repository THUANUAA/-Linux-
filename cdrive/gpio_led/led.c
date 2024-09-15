#include <linux/init.h>    // 初始化头文件
#include <linux/module.h>  // 最基本的文件，支持动态添加和卸载模块
#include <linux/moduleparam.h> // 驱动传参头文件
#include <linux/fs.h>             // 文件操作相关的struct定义
#include <linux/kdev_t.h>         // 设备号相关的宏定义
#include <linux/cdev.h>           // 字符设备相关的头文件
#include <linux/device.h>         // 设备模型相关的头文件
#include <linux/io.h>             // 内存映射IO操作相关头文件

#define DEVICE_NUMBER 1            // 次设备号个数
#define DEVICE_SNAME   "schrdev"   // 静态注册设备的名字
#define DEVICE_ANANME  "achrdev"   // 动态注册设备的名字
#define DEVICE_MINOR 0             // 次设备号起始地址

#define DEVICE_CLASS_NAME "chrdev_class"  // 设备类名称
#define DEVICE_NODE_NAME  "chrdev_led"   // 设备节点名称

// GPIO寄存器的物理地址定义
#define GPIO4_DR 0xfe200000
#define GPIO4_H  0xfe20001c
#define GPIO4_L  0xfe200028

// 定义虚拟地址指针
unsigned int *vir_gpio4_dr = NULL;
unsigned int *vir_gpio4_h = NULL;
unsigned int *vir_gpio4_l = NULL;

static int major_num, minor_num; // 定义主设备号和次设备号

struct cdev cdev; // 定义字符设备结构体

static dev_t dev_num; // 定义设备号

struct class *class; // 定义设备类指针
struct device *device; // 定义设备指针

// 打开设备时的操作
int chrdev_open(struct inode *inode, struct file *file)
{
    printk("hello chrdev_open \n"); // 打印打开信息
    return 0;
}

// 写操作
ssize_t chrdev_write(struct file *file, const char __user *ubuf, size_t size, loff_t *loff_t)
{
    char kbuf[64] = {0};
    // 将用户空间的数据复制到内核空间
    if (copy_from_user(kbuf, ubuf, size) != 0)
    {
        printk("copy_from_user error\n");
        return -1;
    }
    printk("kbuf is %s\n", kbuf);
    
    // 设置GPIO寄存器
    *vir_gpio4_dr |= (001 << (3 * 4));
    if (kbuf[0] == 1)
    {
        // 设置GPIO高电平
        *vir_gpio4_h |= (1 << 4);
    }
    else if (kbuf[0] == 0)
    {
        // 设置GPIO低电平
        *vir_gpio4_l |= (1 << 4);
    }
    return 0;
}

// 定义文件操作结构体
struct file_operations chrdev_ops =
{
    .owner = THIS_MODULE,
    .open = chrdev_open,
    .write = chrdev_write,
};

// 定义模块参数
module_param(major_num, int, S_IRUGO);
module_param(minor_num, int, S_IRUGO);

// 模块参数描述
MODULE_PARM_DESC(major_num, "e.g:a=1");
MODULE_PARM_DESC(minor_num, "e.g:a=1");

// 模块初始化函数
static int chrdev_init(void)
{
    int ret;

    if (major_num) // 如果提供了主设备号，进行静态注册
    {
        printk("mjor_num=%d \n", major_num);
        printk("minor_num=%d \n", minor_num);

        // 将cdev添加到核心的设备号
        dev_num = MKDEV(major_num, minor_num);
        ret = register_chrdev_region(dev_num, DEVICE_NUMBER, DEVICE_SNAME);

        if (ret < 0)
        {
            printk("register_chrdev_region error\n");
        }

        printk("register_chrdev_region success\n");
    }
    else // 动态注册设备号
    {
        ret = alloc_chrdev_region(&dev_num, DEVICE_MINOR, DEVICE_NUMBER, DEVICE_ANANME);
        if (ret < 0)
        {
            printk("alloc_chrdev_region error\n");
        }
        printk("alloc_chrdev_region success\n");

        major_num = MAJOR(dev_num); // 获取主设备号
        minor_num = MINOR(dev_num); // 获取次设备号

        printk("mjor_num=%d \n", major_num);
        printk("minor_num=%d \n", minor_num);
    }

    // 初始化cdev
    cdev.owner = THIS_MODULE;

    // 将file_operations结构体绑定到cdev
    cdev_init(&cdev, &chrdev_ops);
    // 向系统注册设备，使用户能够访问设备
    cdev_add(&cdev, dev_num, DEVICE_NUMBER);

    // 创建设备类
    class = class_create(THIS_MODULE, DEVICE_CLASS_NAME);

    // 创建设备节点
    device = device_create(class, NULL, dev_num, NULL, DEVICE_NODE_NAME);

    // 对物理地址进行虚拟地址映射,从而进行操作
    vir_gpio4_dr = ioremap(0xfe200000, 4);
    if (vir_gpio4_dr == NULL)
    {
        printk("gpio4dr ioremap error\n");
        return -EBUSY;
    }

    vir_gpio4_h = ioremap(GPIO4_H, 4);
    if (vir_gpio4_h == NULL)
    {
        printk("gpio4h ioremap error\n");
        return -EBUSY;
    }

    vir_gpio4_l = ioremap(GPIO4_L, 4);
    if (vir_gpio4_l == NULL)
    {
        printk("gpio4l ioremap error\n");
        return -EBUSY;
    }
    printk("gpio ioremap success\n");
    return 0;
}

// 模块卸载函数
static void chrdev_exit(void)
{
    unregister_chrdev_region(MKDEV(major_num, minor_num), DEVICE_NUMBER); // 释放设备号

    cdev_del(&cdev); // 删除cdev

    device_destroy(class, dev_num); // 销毁设备节点

    class_destroy(class); // 销毁设备类

    printk("bye bye \n");
}

// 指定模块初始化和卸载函数
module_init(chrdev_init);
module_exit(chrdev_exit);

MODULE_LICENSE("GPL"); // 指定模块的许可证为GPL

