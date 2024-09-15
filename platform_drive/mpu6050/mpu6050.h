#ifndef MPU6050_H
#define MPU6050_H

#include <linux/ioctl.h>  // 用于 _IOR 宏
#include <linux/i2c.h>    // 用于 struct i2c_client
#include <linux/fs.h>     // 用于 struct file 和 struct inode

// 定义 IOC_GET_* 命令
#define IOC_GET_ACCEL _IOR('M', 0x01, union mpu6050_data)
#define IOC_GET_GYRO  _IOR('M', 0x02, union mpu6050_data)
#define IOC_GET_TEMP  _IOR('M', 0x03, union mpu6050_data)

// 定义 MPU6050 数据的联合体
union mpu6050_data {
    struct {
        int16_t x;
        int16_t y;
        int16_t z;
    } accel;  // 加速度数据

    struct {
        int16_t x;
        int16_t y;
        int16_t z;
    } gyro;   // 陀螺仪数据

    int16_t temp;  // 温度数据
};

// 函数声明

// I2C 写操作的函数
int mpu6050_write_bytes(struct i2c_client *client, char *buf, int count);

// I2C 读操作的函数
int mpu6050_read_bytes(struct i2c_client *client, char *buf, int count);

// 读取 MPU6050 寄存器的值
int mpu6050_read_reg_byte(struct i2c_client *client, char reg);

// 文件操作接口
int mpu6050_drv_open(struct inode *inode, struct file *filp);
int mpu6050_drv_close(struct inode *inode, struct file *filp);
long mpu6050_drv_ioctl(struct file *filp, unsigned int cmd, unsigned long args);

// I2C 驱动的探测函数和移除函数
int mpu6050_drv_probe(struct i2c_client *client, const struct i2c_device_id *id);
int mpu5060_drv_remove(struct i2c_client *client);

#endif

