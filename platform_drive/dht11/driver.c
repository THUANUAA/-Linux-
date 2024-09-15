/***************************************************************
Copyright  2024-2029. All rights reserved.
文件名     : drv_14_dht11.c
作者       : tangmingfei2013@126.com
版本       : V1.0
描述       : dht11 驱动程序, GPIO4_PIN19-----DHT11 IO port
其他       : 无
日志       : 初版V1.0 2024/1/30  
使用方法：
1) 在.dts文件中定义节点信息
    //mftang: user's dht11, 2024-2-14
    // IO: GPIO-4-PIN19
    mftangdht11 {
        compatible = "atk-dl6y2c,dht11";
        pinctrl-names = "default";
        pinctrl-0 = <&pinctrl_gpio_mftang_1_wire>;
        gpios = <&gpio4 19 GPIO_ACTIVE_HIGH>;
        status = "okay";
    };
    
2) 在驱动匹配列表 
static const struct of_device_id dht11_of_match[] = {
    { .compatible = "atk-dl6y2c,dht11" },
    { } // Sentinel
};
***************************************************************/
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ktime.h>
//#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
//#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
 
#define DEVICE_NAME      "treedht11"     // dev/treedht11
 
/* dht11dev设备结构体 */
struct dht11stru_dev{
    dev_t   devid;                /* 设备号         */
    struct  cdev cdev;            /* cdev           */
    struct  class *class;         /* 类             */
    struct  device *device;       /* 设备           */
    int     major;                /* 主设备号       */
    struct  device_node *node;    /* dht11设备节点 */
    int     userdht11;            /* dht11 GPIO标号*/
    struct  gpio_desc *pin;
};
 
struct dht11stru_dev dht11dev;    /* dht11设备 */ 
 
int us_low_array[40];
int us_low_index;
int us_array[40];
int time_array[40];
int us_index;
 
/*
    dht11 driver 
*/
static void dht11_release( void )
{
    gpiod_direction_output(dht11dev.pin, 1);
}
 
static void dht11_start(void)
{
    gpiod_direction_output(dht11dev.pin, 1);
    mdelay(30);
    
    gpiod_set_value( dht11dev.pin, 0);
    mdelay(20);
    
    gpiod_set_value(dht11dev.pin, 1);
    udelay(40);
    
    gpiod_direction_input(dht11dev.pin);
}
 
static int dht11_wait_ack(void)
{
    int timeout_us = 20000;
 
    /* 等待低电平 */
    while (gpiod_get_value(dht11dev.pin) && --timeout_us)
    {
        udelay(1);
    }
    if (!timeout_us)
    {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        return -1;
    }
 
    /* 现在是低电平 */
    /* 等待高电平 */
    timeout_us = 200;
    
    while (!gpiod_get_value(dht11dev.pin) && --timeout_us)
    {
        udelay(1);
    }
    
    if (!timeout_us)
    {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        return -1;
    }
 
    /* 现在是高电平 */
    /* 等待低电平 */
    timeout_us = 200;
    while (gpiod_get_value(dht11dev.pin) && --timeout_us)
    {
        udelay(1);
    }
    
    if (!timeout_us)
    {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        return -1;
    }
    
    return 0;
}
 
 
 
static int dht11_read_byte( unsigned char *datalist )
{
    int i;
    int us = 0;
    unsigned char data = 0;
    int timeout_us = 200;
    u64 pre, last;
    
    for (i = 0; i < 8; i++)
    {
        /* 现在是低电平 */
        /* 等待高电平 */
        timeout_us = 400;
        us = 0;
        while (!gpiod_get_value(dht11dev.pin) && --timeout_us)
        {
            udelay(1);
            us++;
        }
        
        if (!timeout_us)
        {
            printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
            return -1;
        }
        us_low_array[us_low_index++] = us;
 
        /* 现在是高电平 */
        /* 等待低电平,累加高电平的时间 */
        timeout_us = 20000000;
        us = 0;
 
        /* set another gpio low  */
        pre = ktime_get_raw_ns();
        while (1) 
        {
            last = ktime_get_raw_ns();
            if (last - pre >= 40000)
                break;
        }
 
        if (gpiod_get_value(dht11dev.pin))
        {
            /* get bit 1 */
            data = (data << 1) | 1;
            /* 当前位的高电平未结束, 等待 */
            timeout_us = 400;
            us = 0;
            while (gpiod_get_value(dht11dev.pin) && --timeout_us)
            {
                udelay(1);
                us++;
            }
            if (!timeout_us)
            {
                printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
                return -1;
            }
        }
        else
        {
            /* get bit 0 */
            data = (data << 1) | 0;
        }
    }
 
    *datalist = data;
    return 0;
}
 
 
static int dht11_get_value( unsigned char *data )
{
    unsigned long flags;
    int i;
 
    local_irq_save(flags);  // 关中断
    us_index = 0;
    us_low_index = 0;
 
    /* 1. 发送高脉冲启动DHT11 */
    dht11_start();
    
    /* 2. 等待DHT11就绪 */
    if (dht11_wait_ack())
    {
        local_irq_restore(flags); // 恢复中断
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        return -EAGAIN;
    }
    
    /* 3. 读5字节数据 */
    for (i = 0; i < 5; i++)
    {
        if (dht11_read_byte(&data[i]))
        {
            local_irq_restore(flags); // 恢复中断
            printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
            return -EAGAIN;
        }
    }
    
    /* 4. 释放总线 */
    dht11_release();
    local_irq_restore(flags); // 恢复中断
    
    /* 5. 根据校验码验证数据 */
    if (data[4] != (data[0] + data[1] + data[2] + data[3]))
    {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        return -1;
    }
    
    return 0;
}
 
 
/*
    linux driver 驱动接口： 
    实现对应的open/read/write等函数，填入file_operations结构体
*/
static ssize_t dht11_drv_read ( struct file *file, char __user *buf, 
                                size_t size, loff_t *offset)
{
    unsigned char data[4];
    int err;
    
    if( !dht11_get_value( data ) ){
        printk(" %s line %d \r\n",  __FUNCTION__, __LINE__);
        err = copy_to_user(buf, data, 4);
        return 4;
    }
 
    return -1;
}
 
static int dht11_drv_close(struct inode *node, struct file *file)
{
    printk(" %s line %d \r\n",  __FUNCTION__, __LINE__);
 
    return 0;
}
 
static int dht11_drv_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &dht11dev; /* 设置私有数据  */
    return 0;
}
 
/* 
    定义driver的file_operations结构体
*/
static struct file_operations dht11_fops = {
    .owner   = THIS_MODULE,
    .read    = dht11_drv_read,
    .open    = dht11_drv_open,
 
    .release = dht11_drv_close,
};
 
 
/* 1. 从platform_device获得GPIO
 * 2. gpio=>irq
 * 3. request_irq
 */
static int dht11_probe(struct platform_device *pdev)
{
    printk("dht11 driver and device was matched!\r\n");
    
    /* 1. 获得硬件信息 */
    dht11dev.pin = gpiod_get(&pdev->dev, NULL, 0);
    if (IS_ERR(dht11dev.pin))
    {
        printk("%s line %d get pin parameter error! \n", __FUNCTION__, __LINE__);
    }
    
    /* 2. device_create */
    device_create( dht11dev.class, NULL, 
                   MKDEV( dht11dev.major, 0 ), NULL, 
                   DEVICE_NAME);        // device name 
    
    return 0;
}
 
static int dht11_remove(struct platform_device *pdev)
{
    printk("%s line %d\n", __FUNCTION__, __LINE__);
    
    device_destroy( dht11dev.class, MKDEV( dht11dev.major, 0));
    gpiod_put(dht11dev.pin);
    
    return 0;
}
 
 
static const struct of_device_id atk_dl6y2c_dht11[] = {
    { .compatible = "test_gpio4" },
    { },
};
 
/* 1. 定义platform_driver */
static struct platform_driver dht11_driver = {
    .probe      = dht11_probe,
    .remove     = dht11_remove,
    .driver     = {
        .name   = "atk_dht11",
        .of_match_table = atk_dl6y2c_dht11,
    },
};
 
/* 
  2. 在入口函数注册platform_driver 
*/
static int __init dht11_init(void)
{
    int err;
    
    printk("%s line %d\n",__FUNCTION__, __LINE__);
     
    /* register file_operations  */
    dht11dev.major = register_chrdev( 0, 
                                    DEVICE_NAME,     /* device name */
                                    &dht11_fops);  
 
    /* create the device class  */
    dht11dev.class = class_create(THIS_MODULE, "dht11_class");
    
    if (IS_ERR(dht11dev.class)) {
        printk("%s line %d\n", __FUNCTION__, __LINE__);
        unregister_chrdev( dht11dev.major, DEVICE_NAME);
        return PTR_ERR( dht11dev.class );
    }
    
    err = platform_driver_register(&dht11_driver); 
    
    return err;
}
 
/* 3. 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数
 *    卸载platform_driver
 */
static void __exit dht11_exit(void)
{
    printk("%s line %d\n", __FUNCTION__, __LINE__);
 
    platform_driver_unregister(&dht11_driver);
    class_destroy(dht11dev.class);
    unregister_chrdev(dht11dev.major, DEVICE_NAME);
}
 
/*
 4. 驱动入口和出口函数
*/
module_init(dht11_init);
module_exit(dht11_exit);
 
MODULE_LICENSE("GPL");
