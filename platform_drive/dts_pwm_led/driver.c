#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
//#include <linux/ide.h>
#include <linux/errno.h>
#include <linux/gpio.h>
// #include <asm/mach/map.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/io.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

#define LED_PWM_CMD_SET_DUTY         0x01
#define LED_PWM_CMD_SET_PERIOD       0x02
#define LED_PWM_CMD_SET_BOTH         0x03
#define LED_PWM_CMD_ENABLE           0x04
#define LED_PWM_CMD_DISABLE          0x05

struct my_pwm_param_struct 
{
    int duty_ns;        /* 占空比 */
    int period_ns;      /* 周期 */
};

struct my_pwm_led_dev_struct 
{
    dev_t dev_no;                    
    struct cdev chrdev;             /* 字符设备 */    
    struct class *led_class;
    struct device_node *dev_node;
    struct pwm_device *led_pwm;     /* pwm设备结构体 */
};

static struct my_pwm_param_struct 	 my_pwm_para;
static struct my_pwm_led_dev_struct   my_pwm_led_dev;

static int red_led_drv_open (struct inode *node, struct file *file)
{
    int ret = 0;
    
	//pwm_set_polarity(my_pwm_led_dev.led_pwm, PWM_POLARITY_INVERSED);
	pwm_enable(my_pwm_led_dev.led_pwm);     /* 使能pwm设备 */

    printk("led_pwm open\r\n");
    return ret;
}

static ssize_t red_led_drv_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    int err;

    if (size != sizeof(my_pwm_para)) return -EINVAL;

    /* 接收用户空间拷贝过来的数据，也就是write函数的数据 */
	err = copy_from_user(&my_pwm_para, buf, size);
    if (err > 0) return -EFAULT;

	pwm_config(my_pwm_led_dev.led_pwm, my_pwm_para.duty_ns, my_pwm_para.period_ns);     /* 配置pwm */

	return 1;
}

static long _drv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    void __user *my_user_space = (void __user *)arg;
    
    switch (cmd)
    {
        case LED_PWM_CMD_SET_DUTY:
            ret = copy_from_user(&my_pwm_para.duty_ns, my_user_space, sizeof(my_pwm_para.duty_ns));
            if (ret > 0) 
				return -EFAULT;
            pwm_config(my_pwm_led_dev.led_pwm, my_pwm_para.duty_ns, my_pwm_para.period_ns);
            break;
        case LED_PWM_CMD_SET_PERIOD:
            ret = copy_from_user(&my_pwm_para.period_ns, my_user_space, sizeof(my_pwm_para.period_ns));
            if (ret > 0) 
				return -EFAULT;
            pwm_config(my_pwm_led_dev.led_pwm, my_pwm_para.duty_ns, my_pwm_para.period_ns);
            break;
        case LED_PWM_CMD_SET_BOTH: 
            ret = copy_from_user(&my_pwm_para, my_user_space, sizeof(my_pwm_para));
            if (ret > 0) 
				return -EFAULT;
            pwm_config(my_pwm_led_dev.led_pwm, my_pwm_para.duty_ns, my_pwm_para.period_ns);
            break;
        case LED_PWM_CMD_ENABLE:
            pwm_enable(my_pwm_led_dev.led_pwm);
            break;
        case LED_PWM_CMD_DISABLE:
            pwm_disable(my_pwm_led_dev.led_pwm);
            break;
    }
	return 0;
}

static int red_led_drv_release(struct inode *node, struct file *filp)
{
    int ret = 0;

    pwm_config(my_pwm_led_dev.led_pwm, 0, 5000);
    printk("led pwm dev close\r\n");
//    pwm_disable(my_pwm_led_dev.led_pwm);
    return ret;
}

static struct file_operations red_led_drv = {
	.owner	 = THIS_MODULE,
	.open    = red_led_drv_open,
	.write   = red_led_drv_write,
    .unlocked_ioctl = _drv_ioctl,
    .release  = red_led_drv_release,
};



static int led_red_driver_probe(struct platform_device *pdev)
{
    int err;
    int ret;
    struct device *tdev;
    struct device_node *child;

	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	
    tdev = &pdev->dev;
    child = of_get_next_child(tdev->of_node, NULL);      /* 获取设备树子节点 */
	if (!child) 
	{
        return -EINVAL;
    }

    my_pwm_led_dev.led_pwm = devm_of_pwm_get(tdev, child, NULL);     /* 从子节点中获取PWM设备 */
    if (IS_ERR(my_pwm_led_dev.led_pwm)) 
	{
        printk(KERN_ERR"can't get led_pwm!!\n");
        return -EFAULT;
    }

    ret = alloc_chrdev_region(&my_pwm_led_dev.dev_no, 0, 1, "led_pwm");
	if (ret < 0) 
	{
		pr_err("Error: failed to register mbochs_dev, err: %d\n", ret);
		return ret;
	}

	cdev_init(&my_pwm_led_dev.chrdev, &red_led_drv);
	cdev_add(&my_pwm_led_dev.chrdev, my_pwm_led_dev.dev_no, 1);

    my_pwm_led_dev.led_class = class_create(THIS_MODULE, "led_pwm");
	
	err = PTR_ERR(my_pwm_led_dev.led_class);
	if (IS_ERR(my_pwm_led_dev.led_class)) 
	{
        goto failed1;
	}

    tdev = device_create(my_pwm_led_dev.led_class , NULL, my_pwm_led_dev.dev_no, NULL, "my_pwm_led"); 
    if (IS_ERR(tdev))
	{
        ret = -EINVAL;
		goto failed2;
	}

   	printk(KERN_INFO"%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    
    return 0;
failed2:
    device_destroy(my_pwm_led_dev.led_class, my_pwm_led_dev.dev_no);
    class_destroy(my_pwm_led_dev.led_class);
failed1:
    cdev_del(&my_pwm_led_dev.chrdev);
	unregister_chrdev_region(my_pwm_led_dev.dev_no, 1);
    return ret;
}

int led_red_driver_remove(struct platform_device *dev)
{
    // pwm_disable(my_pwm_led_dev.led_pwm);
    // pwm_free(my_pwm_led_dev.led_pwm);
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	
    device_destroy(my_pwm_led_dev.led_class, my_pwm_led_dev.dev_no);
	class_destroy(my_pwm_led_dev.led_class);
	unregister_chrdev_region(my_pwm_led_dev.dev_no, 1);
    cdev_del(&my_pwm_led_dev.chrdev);
     
    return 0;
}

static struct of_device_id dts_match_table[] = {
    {.compatible = "pgg,my_pwm_led", },  
    {},                  
};

static struct platform_driver my_pwm_led_platform_driver = 
{
      .probe = led_red_driver_probe,
      .remove = led_red_driver_remove,
      .driver = 
      {
        .name = "pgg,my_pwm_led",
        .owner = THIS_MODULE,
        .of_match_table = dts_match_table,//通过设备树匹配
      },
};

module_platform_driver(my_pwm_led_platform_driver);

MODULE_AUTHOR("PGG");
MODULE_LICENSE("GPL");


