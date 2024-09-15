/***************************************************************
文件名		: mpu6050.c
作者	  	: kun
版本	   	: V1.0
描述	   	: mpu6050驱动程序
其他	   	: 无
日志	   	: 初版V1.0 2023/12/5 kun创建
			 V1.1 2023/12/5	
***************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/buffer.h>
#include <linux/iio/trigger.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>
#include "mpu6050.h"

#define MPU6050_NAME "mpu6050"

#define MPU6050_CHAN(ty_pe, channel_2 , index)    \
    {                                              \
        .type = ty_pe,                             \
        .modified = 1,                             \
        .channel2 = channel_2,                     \
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE), \
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW)  |       \
                            BIT(IIO_CHAN_INFO_CALIBBIAS),     \
        .scan_index = index,                                 \
        .scan_type = {                                        \
            .sign = 's',                                      \
            .realbits = 16,                                   \
            .storagebits = 16,                                \
            .shift = 0,                                       \
            .endianness = IIO_BE,                             \
        },                                                    \
    }

enum inv_mpu6050_scan {
    INV_MPU6050_SCAN_ACCL_X,
    INV_MPU6050_SCAN_ACCL_Y,
    INV_MPU6050_SCAN_ACCL_Z,
    INV_MPU6050_SCAN_GYRO_X,
    INV_MPU6050_SCAN_GYRO_Y,
    INV_MPU6050_SCAN_GYRO_Z,
};

struct mpu6050_dev{
	struct i2c_client *client;	/* i2c 设备  保存mpu6050设备对应的i2c_client结构体，匹配成功后由.prob函数带回。*/
	struct mutex lock;
	struct iio_trigger  *trig;
};

/*
 * mpu6050陀螺仪分辨率，对应250、500、1000、2000，计算方法：
 * 以正负250度量程为例，500/2^16=0.007629，扩大1000000倍，就是7629
 */
static const int gyro_scale_mpu6050[] = {7629, 15258, 30517, 61035};

/* 
 * mpu6050加速度计分辨率，对应2、4、8、16 计算方法：
 * 以正负2g量程为例，4/2^16=0.000061035，扩大1000000000倍，就是61035
 */
static const int accel_scale_mpu6050[] = {61035, 122070, 244140, 488281};



/*
* MPU6050 通道 3路陀螺仪，3路加速度
*/

static const struct iio_chan_spec mpu6050_channels[]={
    MPU6050_CHAN(IIO_ANGL_VEL, IIO_MOD_X, INV_MPU6050_SCAN_GYRO_X),
    MPU6050_CHAN(IIO_ANGL_VEL, IIO_MOD_Y, INV_MPU6050_SCAN_GYRO_Y),
    MPU6050_CHAN(IIO_ANGL_VEL, IIO_MOD_Z, INV_MPU6050_SCAN_GYRO_Z),

    MPU6050_CHAN(IIO_ACCEL, IIO_MOD_X, INV_MPU6050_SCAN_ACCL_X),
    MPU6050_CHAN(IIO_ACCEL, IIO_MOD_Y, INV_MPU6050_SCAN_ACCL_Y),
    MPU6050_CHAN(IIO_ACCEL, IIO_MOD_Z, INV_MPU6050_SCAN_ACCL_Z),
};

/*------------------IIC设备内容----------------------*/

/*通过i2c 向mpu6050写入数据
*mpu6050_client：mpu6050的i2c_client结构体。
*address, 数据要写入的地址，
*data, 要写入的数据
*返回值，错误，-1。成功，0  
*/
static int i2c_write_mpu6050(struct i2c_client *mpu6050_client, u8 address, u8 data)
{
	int error = 0;
	u8 write_data[2];
	struct i2c_msg send_msg; //要发送的数据结构体

	/*设置要发送的数据*/
	write_data[0] = address;
	write_data[1] = data;

	/*发送 iic要写入的地址 reg*/
	send_msg.addr = mpu6050_client->addr; //mpu6050在 iic 总线上的地址
	send_msg.flags = 0;					  //标记为发送数据
	send_msg.buf = write_data;			  //写入的首地址
	send_msg.len = 2;					  //reg长度

	/*执行发送*/
	error = i2c_transfer(mpu6050_client->adapter, &send_msg, 1);
	if (error != 1)
	{
		printk(KERN_DEBUG "\n i2c_transfer error \n");
		return -1;
	}
	return 0;
}

/*通过i2c 向mpu6050写入数据
*mpu6050_client：mpu6050的i2c_client结构体。
*address, 要读取的地址，
*data，保存读取得到的数据
*length，读长度
*返回值，错误，-1。成功，0
*/
static int i2c_read_mpu6050(struct i2c_client *mpu6050_client, u8 address, void *data, u32 length)
{
	int error = 0;
	u8 address_data = address;
	struct i2c_msg mpu6050_msg[2];
	/*设置读取位置msg*/
	mpu6050_msg[0].addr = mpu6050_client->addr; //mpu6050在 iic 总线上的地址
	mpu6050_msg[0].flags = 0;					//标记为发送数据
	mpu6050_msg[0].buf = &address_data;			//写入的首地址
	mpu6050_msg[0].len = 1;						//写入长度

	/*设置读取位置msg*/
	mpu6050_msg[1].addr = mpu6050_client->addr; //mpu6050在 iic 总线上的地址
	mpu6050_msg[1].flags = I2C_M_RD;			//标记为读取数据
	mpu6050_msg[1].buf = data;					//读取得到的数据保存位置
	mpu6050_msg[1].len = length;				//读取长度

	error = i2c_transfer(mpu6050_client->adapter, mpu6050_msg, 2);

	if (error != 2)
	{
		printk(KERN_DEBUG "\n i2c_read_mpu6050 error \n");
		return -1;
	}
	return 0;
}

/*初始化i2c
*返回值，成功，返回0。失败，返回 -1
*/
static int mpu6050_init(struct mpu6050_dev *dev)
{
#if 0
	int error = 0;
	/*配置mpu6050*/
	error += i2c_write_mpu6050(dev->client, PWR_MGMT_1, 0X80);
    mdelay(50);
    error += i2c_write_mpu6050(dev->client, PWR_MGMT_1, 0X01);
    mdelay(50);
	error += i2c_write_mpu6050(dev->client, SMPLRT_DIV, 0X00);       /* 输出速率是内部采样率*/ 
    error += i2c_write_mpu6050(dev->client, GYRO_CONFIG, 0X18);      /* 陀螺仪±2000dps量程 */
    error += i2c_write_mpu6050(dev->client, ACCEL_CONFIG, 0X18);     /* 加速度计±16G量程 */    
	error += i2c_write_mpu6050(dev->client, CONFIG, 0X04);           /* 陀螺仪低通滤波BW=20Hz */  
    error += i2c_write_mpu6050(dev->client, ACCEL_CONFIG2, 0X04);    /* 加速度计低通滤波BW=21.2Hz */
    error += i2c_write_mpu6050(dev->client, PWR_MGMT_2, 0X00);       /* 打开加速度计和陀螺仪所有轴 */
    error += i2c_write_mpu6050(dev->client, LP_MODE_CFG, 0X00);      /* 关闭低功耗 */
    
    error += i2c_write_mpu6050(dev->client, INT_ENABLE, 0X01);       /* 使能FIFO溢出以及数据就绪中断 */

	if (error < 0)
	{
		/*初始化错误*/
		printk(KERN_DEBUG "\n mpu6050_init error \n");
		return -1;
	}
	return 0;
#endif
#if 1
int error = 0;
	/*配置mpu6050*/
	error += i2c_write_mpu6050(dev->client, PWR_MGMT_1, 0X01);
    error += i2c_write_mpu6050(dev->client, PWR_MGMT_2, 0X00);
	error += i2c_write_mpu6050(dev->client, SMPLRT_DIV, 0X09);
	error += i2c_write_mpu6050(dev->client, CONFIG, 0X06);
	error += i2c_write_mpu6050(dev->client, GYRO_CONFIG, 0X18);      /* 陀螺仪±2000dps量程 */
    error += i2c_write_mpu6050(dev->client, ACCEL_CONFIG, 0X18);     /* 加速度计±16G量程 */ 

	if (error < 0)
	{
		/*初始化错误*/
		printk(KERN_DEBUG "\n mpu6050_init error \n");
		return -1;
	}
	return 0;
#endif
}

/*
* @description : 读取MPU6050传感器数据，可用于陀螺仪、加速度
* @param - dev : mpu6050设备
* @param - reg : 要读取的通道寄存器首地址
* @param - axis :需要读取的通道，比如x,y,z
* @param - *val : 保存读取到的值
* @return     ：1(IIO_VAL_INT)，成功；其他值，错误 
*/
static int mpu6050_sensor_show( struct mpu6050_dev *dev, 
                                int reg,
                                int axis,
                                int *val)
{
    int ind;
    int error=0;
    char data_H;
	char data_L;
    short int data = -1;
    ind = (axis-IIO_MOD_X) * 2;
    error += i2c_read_mpu6050(dev->client, reg + ind, &data_H, 1);
	error += i2c_read_mpu6050(dev->client, reg + ind +1, &data_L, 1);
    printk("%d\r\n",data);
    printk("%d\r\n",(int)data_H);
    printk("%d\r\n",(int)data_L);
    data =(data_H<<8) +data_L;
    *val = (int)data;
    printk("%d\r\n",*val);
    if(error !=0 )
        return  -EINVAL;
    return IIO_VAL_INT;
}

/*
* @description :读取MPU6050 陀螺仪、加速度计值
* @param -indio_dev : iio设备
* @param -chan      :通道
* @param -val : 保存读取到的通道值
* @return ：1(IIO_VAL_INT)，成功；其他值：错误
*/
static int mpu6050_read_channel_data( struct iio_dev *indio_dev,
                                      struct iio_chan_spec const * chan,
                                      int *val)
{
    struct mpu6050_dev *dev = iio_priv(indio_dev);
    int ret = 0;
    switch(chan->type){
        case IIO_ANGL_VEL:  /* 读取陀螺仪数据 */
            ret = mpu6050_sensor_show(dev, GYRO_XOUT_H, chan->channel2 ,val);   /* channel2 为x ,y,z轴 */
            break;
        case IIO_ACCEL:    /* 读取加速度计 数据 */
            ret = mpu6050_sensor_show(dev, ACCEL_XOUT_H, chan->channel2,val);
            break;
        default:
            ret = -EINVAL;
    }
    return ret;
}

/*
  * @description     	: 读函数，当读取sysfs中的文件的时候最终此函数会执行，此函数
  * 					：里面会从传感器里面读取各种数据，然后上传给应用。
  * @param - indio_dev	: iio_dev
  * @param - chan   	: 通道
  * @param - val   		: 读取的值，如果是小数值的话，val是整数部分。
  * @param - val2   	: 读取的值，如果是小数值的话，val2是小数部分。
  * @return				: 0，成功；其他值，错误
  */
static int mpu6050_read_raw(struct iio_dev *indio_dev,
			   struct iio_chan_spec const *chan,
			   int *val, int *val2, long mask)
{
    int ret = 0;
    int error = 0;
    struct mpu6050_dev *dev = iio_priv(indio_dev);
    unsigned char regdata = 0;
    printk("mpu6050_read_raw\r\n");

    switch (mask) {
        case IIO_CHAN_INFO_RAW:      /*读取加速度、陀螺仪原始值*/
            printk("read raw data\r\n");
            mutex_lock(&dev->lock);	                 /* 上锁 */
            ret = mpu6050_read_channel_data(indio_dev, chan, val);
            mutex_unlock(&dev->lock);
            return ret;
        case IIO_CHAN_INFO_SCALE:
            switch (chan->type){
                case IIO_ANGL_VEL:
                    mutex_lock(&dev->lock);
                    printk("read gyro scale\r\n");
                    error = i2c_read_mpu6050(dev->client, GYRO_CONFIG, &regdata, 1);  
                    if(error != 0){
                        return -EINVAL;
                    }  
                    regdata = (regdata & 0x18) >>3;  /* 提取陀螺仪量程*/
                    *val = 0;
                    *val2 = gyro_scale_mpu6050[regdata];
                    mutex_unlock(&dev->lock);
                    return IIO_VAL_INT_PLUS_MICRO;   /* 值为val + val2/1000000 */
                case IIO_ACCEL:
                    printk("read accel sacle\r\n");
                    mutex_lock(&dev->lock);
                    error = i2c_read_mpu6050(dev->client, ACCEL_CONFIG, &regdata, 1);  
                    if(error != 0){
                        return -EINVAL;
                    }  
                    regdata = (regdata & 0x18) >>3;  /* 提取陀螺仪量程*/
                    *val = 0;
                    *val2 = accel_scale_mpu6050[regdata];
                    mutex_unlock(&dev->lock);
                    return IIO_VAL_INT_PLUS_NANO;   /* 值为val + val2/1000000000 */
                default:
                    return -EINVAL;
            }
        case IIO_CHAN_INFO_CALIBBIAS:         /*mpu6050 加速度计和陀螺仪校准值 */
            switch (chan->type){
                case IIO_ANGL_VEL:          /*陀螺仪的校准值*/
                    printk("read gyro calibbias \r\n");
                    mutex_lock(&dev->lock);
                    ret = mpu6050_sensor_show(dev, XG_OFFS_USRH ,chan->channel2, val);
                    mutex_unlock(&dev->lock);
                    return ret;
                case IIO_ACCEL:                /* 加速度计的校准值 */
                    printk("read accel calibbias \r\n"); 
                    mutex_lock(&dev->lock);
                    ret = mpu6050_sensor_show(dev, XA_OFFSET_H ,chan->channel2, val);
                    mutex_unlock(&dev->lock);    
                    return ret;
                default:
                    return -EINVAL;
            }
        return -EINVAL;
    }
	return ret;
}

/*
* @description : 设置MPU6050传感器，可用于陀螺仪校准
* @param - dev : mpu6050设备
* @param - reg : 要设置的通道寄存器首地址
* @param - axis :要设置的通道，比如x,y,z
* @param - *val : 要设置的值
* @return     ：0，成功；其他值，错误 
*/
static int mpu6050_sensor_set_1( struct mpu6050_dev *dev, 
                                int reg,
                                int axis,
                                int val)
{
    int ind;
    int error=0;
    char data_H;
	char data_L;
    ind = (axis-IIO_MOD_X) * 2;
    data_H = val>>8;
    data_L = val&0xff;
    error = i2c_write_mpu6050(dev->client, reg + ind, data_H);
    error = i2c_write_mpu6050(dev->client, reg + ind + 1, data_L);
    if(error !=0 )
        return  -EINVAL;
    return 0;
}

/*
* @description : 设置MPU6050传感器，可用于加速度计校准
* @param - dev : mpu6050设备
* @param - reg : 要设置的通道寄存器首地址
* @param - axis :要设置的通道，比如x,y,z
* @param - *val : 要设置的值
* @return     ：0，成功；其他值，错误 
*/
static int mpu6050_sensor_set_2( struct mpu6050_dev *dev, 
                                int reg,
                                int axis,
                                int val)
{
    int ind;
    int error=0;
    char data_H;
	char data_L;
    ind = (axis-IIO_MOD_X) * 3;
    data_H = val>>8;
    data_L = val&0xff;
    error = i2c_write_mpu6050(dev->client, reg + ind, data_H);
    error = i2c_write_mpu6050(dev->client, reg + ind + 1, data_L);
    if(error !=0 )
        return  -EINVAL;
    return 0;
}

/*
  * @description  	: 设置mpu6050的陀螺仪计量程(分辨率)
  * @param - dev	: icm20608设备
  * @param - val   	: 量程(分辨率值)。
  * @return			: 0，成功；其他值，错误
  */
static int mpu6050_write_gyro_scale(struct mpu6050_dev *dev, int val)
{
	int result, i;
	u8 d;

	for (i = 0; i < ARRAY_SIZE(gyro_scale_mpu6050); ++i) {
		if (gyro_scale_mpu6050[i] == val) {
			d = (i << 3);
			result = i2c_write_mpu6050(dev->client, GYRO_CONFIG, d);
			if (result)
				return result;
			return 0;
		}
	}
	return -EINVAL;
}

/*
  * @description  	: 设置mpu6050的加速度计量程(分辨率)
  * @param - dev	: mpu6050设备
  * @param - val   	: 量程(分辨率值)。
  * @return			: 0，成功；其他值，错误
  */
static int mpu6050_write_accel_scale(struct mpu6050_dev *dev, int val)
{
	int result, i;
	u8 d;

	for (i = 0; i < ARRAY_SIZE(accel_scale_mpu6050); ++i) {
		if (accel_scale_mpu6050[i] == val) {
			d = (i << 3);
            result = i2c_write_mpu6050(dev->client, ACCEL_CONFIG, d);
			if (result)
				return result;
			return 0;
		}
	}
	return -EINVAL;
}

/* @description     	: 写函数，当向sysfs中的文件写数据的时候最终此函数会执行，一般在此函数
  * 					：里面设置传感器，比如量程等。
  * @param - indio_dev	: iio_dev
  * @param - chan   	: 通道
  * @param - val   		: 应用程序写入的值，如果是小数值的话，val是整数部分。
  * @param - val2   	: 应用程序写入的值，如果是小数值的话，val2是小数部分。
  * @param - mask       : 掩码，用于指定我们读取的是什么数据
  * @return				: 0，成功；其他值，错误
  */
static int mpu6050_write_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int val, int val2, long mask)
{
    int ret = 0;
    struct mpu6050_dev *dev = iio_priv(indio_dev);
    printk("mpu6050_write_raw\r\n");
    switch(mask){
        case IIO_CHAN_INFO_SCALE:      /* 设置陀螺仪和加速度计的分辨率*/
            switch (chan->type){
                case IIO_ANGL_VEL:     /* 设置陀螺仪 */
                    mutex_lock(&dev->lock);
                    ret = mpu6050_write_gyro_scale(dev,val2);
                    mutex_unlock(&dev->lock);
                    break;
                case IIO_ACCEL:       /* 设置加速度计*/
                    mutex_lock(&dev->lock);
                    ret = mpu6050_write_accel_scale(dev,val2);
                    mutex_unlock(&dev->lock);
                    break;
                default:
                    ret = -EINVAL;
                    break;
            }
            break;
        case IIO_CHAN_INFO_CALIBBIAS:   /* 设置陀螺仪和加速度计的校准值 */
            switch (chan->type){
                case IIO_ANGL_VEL:       /* 设置陀螺仪校准值 */
                    mutex_lock(&dev->lock);
                    ret = mpu6050_sensor_set_1(dev, XG_OFFS_USRH, chan->channel2, val);
                    mutex_unlock(&dev->lock);
                    break;
                case IIO_ACCEL:          /* 加速度计校准值 */
                    mutex_lock(&dev->lock);
                    ret = mpu6050_sensor_set_2(dev, XA_OFFSET_H, chan->channel2, val);
                    mutex_unlock(&dev->lock);
                    break;
                default:
                    ret = -EINVAL;
                    break;
            }
            break;
        default: 
            ret = -EINVAL; 
            break;
    }
    return ret;
}

/*
  * @description     	: 用户空间写数据格式，比如我们在用户空间操作sysfs来设置传感器的分辨率，
  * 					：如果分辨率带小数，那么这个小数传递到内核空间应该扩大多少倍，此函数就是
  *						: 用来设置这个的。
  * @param - indio_dev	: iio_dev
  * @param - chan   	: 通道
  * @param - mask   	: 掩码
  * @return				: 0，成功；其他值，错误
  */
static int mpu6050_write_raw_get_fmt(struct iio_dev *indio_dev,
				 struct iio_chan_spec const *chan, long mask)
{
    printk("mpu6050_write_raw_get_fmt\r\n");
    switch (mask){
        case IIO_CHAN_INFO_SCALE:
            switch (chan->type) {
                case IIO_ANGL_VEL:                  /* 用户空间写的陀螺仪分辨率数据要乘以1000000*/
                    return IIO_VAL_INT_PLUS_MICRO;
                default:                           /* 用户空间写的加速度计分辨率数据要乘以1000000000 */
                    return IIO_VAL_INT_PLUS_NANO;
            }
        default:
            return IIO_VAL_INT_PLUS_MICRO;
    }
    return -EINVAL;
}
/*
 * iio_info结构体变量
 */
static const struct iio_info mpu6050_info = {
	.read_raw		= mpu6050_read_raw,
	.write_raw		= mpu6050_write_raw,
	.write_raw_get_fmt = &mpu6050_write_raw_get_fmt,	/* 用户空间写数据格式 */
};

/*----------------平台驱动函数集-----------------*/
static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    struct mpu6050_dev *dev;
    struct iio_dev *indio_dev;

    /* 1、申请iio_dev内存*/
    indio_dev = devm_iio_device_alloc(&client->dev,sizeof(*dev));

    /* 2、获取mpu6050结构体地址*/
    dev = iio_priv(indio_dev);
    dev->client =client;
    i2c_set_clientdata(client, indio_dev);    /*保存indio_dev数据至i2c_client*/
    mutex_init(&dev->lock);
  

    /* 3、iio_dev的其他成员变量初始化*/
    indio_dev->dev.parent = &client->dev;
    indio_dev->info = &mpu6050_info;   /*驱动开发人员编写，从用户空间读取IIO设备内部数据，最终调用的就是iio_info里面的函数*/
    indio_dev->name = MPU6050_NAME; 
    indio_dev->modes = INDIO_DIRECT_MODE;  /*直接模式，提供sysfs接口*/
    indio_dev->channels = mpu6050_channels;
    indio_dev->num_channels = ARRAY_SIZE(mpu6050_channels);

    /*  4、注册iio_dev*/
    ret = iio_device_register(indio_dev);
    if (ret <0){
        dev_err(&client->dev, "iio_device_register failed \n");
        goto err_iio_register;
    }
    mpu6050_init(dev);             /*初始化MPU6050*/
    printk("iio_device_register successfully\r\n");
    return 0;
err_iio_register:
    return ret;
}


static int mpu6050_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);
	struct ap3216c_dev *dev;
	
	dev = iio_priv(indio_dev);

	/* 2、注销IIO */
	iio_device_unregister(indio_dev);
	return 0;
}



/*定义ID 匹配表*/
static const struct i2c_device_id gtp_device_id[] = {
	{"kun,i2c_mpu6050", 0},
	{}};

/*定义设备树匹配表*/
static const struct of_device_id mpu6050_of_match_table[] = {
	{.compatible = "kun,i2c_mpu6050"},
	{/* sentinel */}};

/*定义i2c总线设备结构体*/
struct i2c_driver mpu6050_driver = {
	.probe = mpu6050_probe,
	.remove = mpu6050_remove,
	.id_table = gtp_device_id,
	.driver = {
		.name = "kun,i2c_mpu6050",
		.owner = THIS_MODULE,
		.of_match_table = mpu6050_of_match_table,
	},
};

/*
*驱动初始化函数
*/
static int __init mpu6050_driver_init(void)
{
	int ret;
	pr_info("mpu6050_driver_init\n");
	ret = i2c_add_driver(&mpu6050_driver);
	return ret;
}

/*
*驱动注销函数
*/
static void __exit mpu6050_driver_exit(void)
{
	pr_info("mpu6050_driver_exit\n");
	i2c_del_driver(&mpu6050_driver);
}

module_init(mpu6050_driver_init);
module_exit(mpu6050_driver_exit);

MODULE_LICENSE("GPL");


