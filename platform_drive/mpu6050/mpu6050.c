#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>

#include <asm/io.h>
#include <linux/uaccess.h>
#include "mpu6050.h"

// 定义MPU6050寄存器的地址
#define SMPLRT_DIV              0x19 // 采样频率寄存器，典型值：0x07 (125Hz)
#define CONFIG                  0x1A // 配置寄存器，典型值：0x06 (5Hz)
#define GYRO_CONFIG             0x1B // 陀螺仪配置，典型值：0x18 (不自检，2000deg/s)
#define ACCEL_CONFIG            0x1C // 加速度配置，典型值：0x01 (2G，5Hz)
#define ACCEL_XOUT_H    0x3B // 加速度计X轴高字节
#define ACCEL_XOUT_L    0x3C // 加速度计X轴低字节
#define ACCEL_YOUT_H    0x3D // 加速度计Y轴高字节
#define ACCEL_YOUT_L    0x3E // 加速度计Y轴低字节
#define ACCEL_ZOUT_H    0x3F // 加速度计Z轴高字节
#define ACCEL_ZOUT_L    0x40 // 加速度计Z轴低字节
#define TEMP_OUT_H              0x41 // 温度传感器高字节
#define TEMP_OUT_L              0x42 // 温度传感器低字节
#define GYRO_XOUT_H             0x43 // 陀螺仪X轴高字节
#define GYRO_XOUT_L             0x44 // 陀螺仪X轴低字节
#define GYRO_YOUT_H             0x45 // 陀螺仪Y轴高字节
#define GYRO_YOUT_L             0x46 // 陀螺仪Y轴低字节
#define GYRO_ZOUT_H             0x47 // 陀螺仪Z轴高字节
#define GYRO_ZOUT_L             0x48 // 陀螺仪Z轴低字节
#define PWR_MGMT_1              0x6B // 电源管理寄存器，典型值：0x00 (正常启用)

// 定义设备结构体
struct mpu_sensor {
    int dev_major;              // 主设备号
    struct device *dev;         // 设备结构体指针
    struct class *cls;          // 设备类结构体指针
    struct i2c_client *client;  // i2c_client结构体指针，记录probe中的client
};

// 全局设备指针
struct mpu_sensor *mpu_dev;

// i2c写数据函数
int mpu6050_write_bytes(struct i2c_client *client, char *buf, int count) {
    int ret;
    struct i2c_adapter *adapter = client->adapter;
    struct i2c_msg msg;

    msg.addr = client->addr;  // 设置i2c设备地址
    msg.flags = 0;            // 0表示写操作
    msg.len = count;          // 数据长度
    msg.buf = buf;            // 数据缓冲区

    ret = i2c_transfer(adapter, &msg, 1); // 进行i2c传输

    return (ret == 1) ? count : ret; // 返回传输的字节数
}

// i2c读数据函数
int mpu6050_read_bytes(struct i2c_client *client, char *buf, int count) {
    int ret;
    struct i2c_adapter *adapter = client->adapter;
    struct i2c_msg msg;

    msg.addr = client->addr;  // 设置i2c设备地址
    msg.flags = I2C_M_RD;     // I2C_M_RD表示读操作
    msg.len = count;          // 数据长度
    msg.buf = buf;            // 数据缓冲区

    ret = i2c_transfer(adapter, &msg, 1); // 进行i2c传输

    return (ret == 1) ? count : ret; // 返回传输的字节数
}

// 读取特定寄存器的字节数据
int mpu6050_read_reg_byte(struct i2c_client *client, char reg) {
    int ret;
    struct i2c_adapter *adapter = client->adapter;
    struct i2c_msg msg[2];
    char rxbuf[1];

    msg[0].addr = client->addr;  // 设置i2c设备地址
    msg[0].flags = 0;            // 写操作
    msg[0].len = 1;              // 写1字节
    msg[0].buf = &reg;           // 寄存器地址

    msg[1].addr = client->addr;  // 设置i2c设备地址
    msg[1].flags = I2C_M_RD;     // 读操作
    msg[1].len = 1;              // 读1字节
    msg[1].buf = rxbuf;          // 读取数据缓冲区

    ret = i2c_transfer(adapter, msg, 2); // 进行i2c传输
    if (ret < 0) {
        printk("i2c_transfer read error\n"); // 错误日志
        return ret;
    }

    return rxbuf[0]; // 返回读取的数据
}

// 打开设备时的初始化操作
int mpu6050_drv_open(struct inode *inode, struct file *filp) {
    char buf1[2] = {PWR_MGMT_1, 0x0};  // 电源管理寄存器
    char buf2[2] = {SMPLRT_DIV, 0x07}; // 采样频率寄存器 125Hz
    char buf3[2] = {CONFIG, 0x06};     // 配置寄存器
    char buf4[2] = {GYRO_CONFIG, 0x18}; // 陀螺仪配置
    char buf5[2] = {ACCEL_CONFIG, 0x01}; // 加速度配置

    // 写入初始化配置到MPU6050
    mpu6050_write_bytes(mpu_dev->client, buf1, 2);
    mpu6050_write_bytes(mpu_dev->client, buf2, 2);
    mpu6050_write_bytes(mpu_dev->client, buf3, 2);
    mpu6050_write_bytes(mpu_dev->client, buf4, 2);
    mpu6050_write_bytes(mpu_dev->client, buf5, 2);

    return 0;
}

// 关闭设备时的操作（目前不做任何处理）
int mpu6050_drv_close(struct inode *inode, struct file *filp) {
    return 0;
}

// 处理ioctl请求，读取传感器数据
long mpu6050_drv_ioctl(struct file *filp, unsigned int cmd, unsigned long args) {
    union mpu6050_data data;

    switch (cmd) {
        case IOC_GET_ACCEL:
            // 读取加速度数据
            data.accel.x = mpu6050_read_reg_byte(mpu_dev->client, ACCEL_XOUT_L);
            data.accel.x |= mpu6050_read_reg_byte(mpu_dev->client, ACCEL_XOUT_H) << 8;

            data.accel.y = mpu6050_read_reg_byte(mpu_dev->client, ACCEL_YOUT_L);
            data.accel.y |= mpu6050_read_reg_byte(mpu_dev->client, ACCEL_YOUT_H) << 8;

            data.accel.z = mpu6050_read_reg_byte(mpu_dev->client, ACCEL_ZOUT_L);
            data.accel.z |= mpu6050_read_reg_byte(mpu_dev->client, ACCEL_ZOUT_H) << 8;
            break;
        case IOC_GET_GYRO:
            // 读取陀螺仪数据
            data.gyro.x = mpu6050_read_reg_byte(mpu_dev->client, GYRO_XOUT_L);
            data.gyro.x |= mpu6050_read_reg_byte(mpu_dev->client, GYRO_XOUT_H) << 8;

            data.gyro.y = mpu6050_read_reg_byte(mpu_dev->client, GYRO_YOUT_L);
            data.gyro.y |= mpu6050_read_reg_byte(mpu_dev->client, GYRO_YOUT_H) << 8;

            data.gyro.z = mpu6050_read_reg_byte(mpu_dev->client, GYRO_ZOUT_L);
            data.gyro.z |= mpu6050_read_reg_byte(mpu_dev->client, GYRO_ZOUT_H) << 8;
            break;
        case IOC_GET_TEMP:
            // 读取温度数据
            data.temp = mpu6050_read_reg_byte(mpu_dev->client, TEMP_OUT_L);
            data.temp |= mpu6050_read_reg_byte(mpu_dev->client, TEMP_OUT_H) << 8;
            break;
        default:
            printk("invalid cmd\n"); // 无效命令
            return -EINVAL;
    }
    // 将数据传递给用户空间
    if (copy_to_user((void __user *)args, &data, sizeof(data)) > 0)
        return -EFAULT;
    return 0;
}

// 文件操作结构体
const struct file_operations mpu6050_fops = {
    .open = mpu6050_drv_open,
    .release = mpu6050_drv_close,
    .unlocked_ioctl = mpu6050_drv_ioctl,
};

// 设备驱动的probe函数
int mpu6050_drv_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    printk("-----%s----\n", __FUNCTION__);

    // 初始化设备结构体
    mpu_dev = kzalloc(sizeof(struct mpu_sensor), GFP_KERNEL);
    mpu_dev->client = client;

    // 注册字符设备
    mpu_dev->dev_major = register_chrdev(0, "mpu_drv", &mpu6050_fops);

    // 创建设备类
    mpu_dev->cls = class_create(THIS_MODULE, "mpu_cls");

    // 创建设备文件
    mpu_dev->dev = device_create(mpu_dev->cls, NULL, MKDEV(mpu_dev->dev_major, 0), NULL, "mpu_sensor");

    return 0;
}

// 设备驱动的remove函数
int mpu6050_drv_remove(struct i2c_client *client) {
    printk("-----%s----\n", __FUNCTION__);
    // 清理设备
    device_destroy(mpu_dev->cls, MKDEV(mpu_dev->dev_major, 0));
    class_destroy(mpu_dev->cls);
    unregister_chrdev(mpu_dev->dev_major, "mpu_drv");
    kfree(mpu_dev);
    return 0;
}

// 设备树匹配表
const struct of_device_id of_mpu6050_id[] = {
    { .compatible = "lch-mpu6050", },
    { /* sentinel */ },
};

// 传统I2C设备匹配表
const struct i2c_device_id mpu_id_table[] = {
    { "mpu6050_drv", 0x1111 },
    { /* sentinel */ },
};

// I2C驱动结构体
struct i2c_driver mpu6050_drv = {
    .probe = mpu6050_drv_probe,
    .remove = mpu6050_drv_remove,
    .driver = {
        .name = "lch-mpu6050",
        .of_match_table = of_match_ptr(of_mpu6050_id),
    },
    .id_table = mpu_id_table,
};

// 驱动初始化函数
static int __init mpu6050_drv_init(void) {
    // 注册I2C驱动
    return i2c_add_driver(&mpu6050_drv);
}

// 驱动退出函数
static void __exit mpu6050_drv_exit(void) {
    // 注销I2C驱动
    i2c_del_driver(&mpu6050_drv);
}

module_init(mpu6050_drv_init);
module_exit(mpu6050_drv_exit);
MODULE_LICENSE("GPL");

