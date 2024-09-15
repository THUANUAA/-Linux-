#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

/* 定义宏指令 */
#define TIMER_NAME        "timer"           // 定时器设备名称
#define TIMER_COUNT       1                 // 定时器设备数量
#define CLOSE_CMD         _IO(0XEF, 0x1)    // 关闭定时器命令
#define OPEN_CMD          _IO(0XEF, 0x2)    // 打开定时器命令
#define SETPERIOD_CMD     _IO(0XEF, 0x3)    // 设置定时器周期命令

/* 设备结构体 */
struct timer_dev {
    struct cdev cdev;         // 字符设备
    dev_t devid;              // 设备号
    struct class *class;      // 类
    struct device *device;    // 设备
    int major;                // 主设备号
    int minor;                // 次设备号
    struct device_node *nd;   // 设备节点
    int led_gpio;             // LED使用的GPIO编号
    struct timer_list timer;  // 定时器
    int timePeriod;           // 定时周期，单位为毫秒(ms)
    spinlock_t lock;          // 自旋锁，用于保护共享资源
};

/* 定义定时器设备结构体变量 */
static struct timer_dev timerdev;

/* 定时器处理函数 */
static void timer_function(struct timer_list *t)
{
    struct timer_dev *dev = &timerdev;
    int timerPeriod;
    unsigned long flags;
    static int sta = 1;

    sta = !sta;  // 切换LED状态
    gpio_set_value(dev->led_gpio, sta);  // 设置LED的GPIO输出电平

    /* 使用自旋锁更新定时器周期，保护共享资源 */
    spin_lock_irqsave(&dev->lock, flags);
    timerPeriod = dev->timePeriod;  // 获取定时器周期
    spin_unlock_irqrestore(&dev->lock, flags);

    /* 重新设置定时器，周期到后再次调用此函数 */
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timerPeriod));
}

/* 初始化LED GPIO */
static int led_gpio_init(void)
{
    int ret;

    /* 获取设备树中的LED设备节点 */
    timerdev.nd = of_find_node_by_path("/ledtest");
    if (timerdev.nd == NULL) {
        printk("ledtest node not found!\r\n");
        return -EINVAL;
    }

    /* 获取设备树中的GPIO属性 */
    timerdev.led_gpio = of_get_named_gpio(timerdev.nd, "gpios", 0);
    if (timerdev.led_gpio < 0) {
        printk("can't get gpios");
        return -EINVAL;
    }
    printk("gpios num = %d\n", timerdev.led_gpio);

    /* 设置GPIO方向为输出并初始化状态为高电平 */
    ret = gpio_direction_output(timerdev.led_gpio, 1);
    if (ret < 0) {
        printk("can't set gpio!\n");
    }

    return 0;
}

/* 打开设备文件时调用的函数 */
static int timer_open(struct inode *inode, struct file *filp)
{
    printk("timer_open\n");
    filp->private_data = &timerdev;  // 将设备结构体保存到文件私有数据中
    return 0;
}

/* 释放设备文件时调用的函数 */
static int timer_release(struct inode *inode, struct file *filp)
{
    printk("timer_release\n");
    return 0;
}

/* 处理设备的ioctl系统调用 */
static long timer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct timer_dev *dev = filp->private_data;
    int timerPeriod;
    unsigned long flags;

    switch (cmd) {
        case CLOSE_CMD:  // 关闭定时器
            del_timer_sync(&dev->timer);
            break;
        case OPEN_CMD:  // 打开定时器
            spin_lock_irqsave(&dev->lock, flags);
            timerPeriod = dev->timePeriod;  // 获取定时器周期
            spin_unlock_irqrestore(&dev->lock, flags);
            mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timerPeriod));  // 设置定时器
            break;
        case SETPERIOD_CMD:  // 设置定时器周期
            spin_lock_irqsave(&dev->lock, flags);
            dev->timePeriod = arg;  // 更新定时器周期
            spin_unlock_irqrestore(&dev->lock, flags);
            mod_timer(&dev->timer, jiffies + msecs_to_jiffies(arg));  // 重新设置定时器
            break;
        default:
            printk("Invalid input!\n");
            break;
    }
    return 0;
}

/* 文件操作结构体 */
static const struct file_operations timer_fops = {
    .owner = THIS_MODULE,
    .open = timer_open,
    .release = timer_release,
    .unlocked_ioctl = timer_unlocked_ioctl,
};

/* 驱动入口函数 */
static int __init timer_init(void)
{
    int ret;
    unsigned long flags;

    printk("timer_init\n");
    timerdev.major = 0;  // 初始化主设备号

    /* 初始化自旋锁 */
    spin_lock_init(&timerdev.lock);

    /* 初始化LED GPIO */
    ret = led_gpio_init();
    if (ret < 0) {
        return ret;
    }

    /* 分配或注册字符设备号 */
    if (timerdev.major) {
        timerdev.devid = MKDEV(timerdev.major, 0);
        ret = register_chrdev_region(timerdev.devid, TIMER_COUNT, TIMER_NAME);
    } else {
        ret = alloc_chrdev_region(&timerdev.devid, 0, TIMER_COUNT, TIMER_NAME);
        timerdev.major = MAJOR(timerdev.devid);
        timerdev.minor = MINOR(timerdev.devid);
    }
    if (ret < 0) {
        printk("Failed to register chrdev region!\n");
        goto fail_devid;
    }
    printk("timer major:%d, minor:%d\n", timerdev.major, timerdev.minor);

    /* 初始化字符设备 */
    timerdev.cdev.owner = THIS_MODULE;
    cdev_init(&timerdev.cdev, &timer_fops);
    ret = cdev_add(&timerdev.cdev, timerdev.devid, TIMER_COUNT);
    if (ret < 0) {
        goto fail_cdev;
    }

    /* 创建设备类和设备节点 */
    timerdev.class = class_create(THIS_MODULE, TIMER_NAME);
    if (IS_ERR(timerdev.class)) {
        ret = PTR_ERR(timerdev.class);
        goto fail_class;
    }
    timerdev.device = device_create(timerdev.class, NULL, timerdev.devid, NULL, TIMER_NAME);
    if (IS_ERR(timerdev.device)) {
        ret = PTR_ERR(timerdev.device);
        goto fail_device;
    }

    /* 初始化定时器 */
    timer_setup(&timerdev.timer, timer_function, 0);
    spin_lock_irqsave(&timerdev.lock, flags);
    timerdev.timePeriod = 500;  // 设置定时器周期为500ms
    spin_unlock_irqrestore(&timerdev.lock, flags);
    timerdev.timer.expires = jiffies + msecs_to_jiffies(timerdev.timePeriod);  // 设置超时时间
    add_timer(&timerdev.timer);  // 将定时器添加到系统

    return 0;

fail_device:
    class_destroy(timerdev.class);
fail_class:
    cdev_del(&timerdev.cdev);
fail_cdev:
    unregister_chrdev_region(timerdev.devid, TIMER_COUNT);
fail_devid:
    return ret;
}

/* 驱动出口函数 */
static void __exit timer_exit(void)
{
    printk("timer_exit\n");

    /* 删除定时器 */
    del_timer_sync(&timerdev.timer);

    /* 删除字符设备 */
    cdev_del(&timerdev.cdev);

    /* 注销设备号 */
    unregister_chrdev_region(timerdev.devid, TIMER_COUNT);

    /* 销毁设备和类 */
    device_destroy(timerdev.class, timerdev.devid);
    class_destroy(timerdev.class);
}

/* 注册模块的初始化和退出函数 */
module_init(timer_init);
module_exit(timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("xxx");

