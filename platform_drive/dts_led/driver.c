#include <linux/init.h>  
#include <linux/module.h> 
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/miscdevice.h> //注册杂项设备头文件
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>

unsigned int *vir_gpio4_dr=NULL;
unsigned int *vir_gpio4_h=NULL;
unsigned int *vir_gpio4_l=NULL;

u32 out_values[6]; //用于存储从设备树获取的reg数据

struct resource *gpio4_dir;
struct resource *gpio4_h;
struct resource *gpio4_l;


const struct of_device_id of_match_table_test[] = {
	{ .compatible = "test_led"},
	{}

};

ssize_t misc_write(struct file *file, const char __user *ubuf, size_t size, loff_t *loff_t)
{
	char kbuf[64] = {0};
	if ( copy_from_user( kbuf, ubuf, size) != 0)
	{
		printk( "copy_from_user error\n ");
		return -1;
	}
	printk( "kbuf is %s\n ", kbuf);
	*vir_gpio4_dr |= (001<<(3*4));
	if( kbuf[0] == 1)
	{
		
		*vir_gpio4_h |=(1<<4);
	}
	else if( kbuf[0]==0)
	{
		*vir_gpio4_l |=(1<<4);
	}
	return 0;
}

int misc_release( struct inode *inode, struct file *file)
{
	printk( "hello misc_relaease bye bye \n ");
	return 0;
}

int misc_open( struct inode *inode, struct file *file)
{
	printk( "hello misc_open\n ");
	return 0;
}
//文件操作集
struct file_operations misc_fops = {
		.owner = THIS_MODULE, 
		.open = misc_open,
		.release = misc_release,
		.write = misc_write,
		};
//miscdevice 结构体
struct miscdevice misc_dev = {
	.minor = MISC_DYNAMIC_MINOR, 
	.name = "hello_misc",
	.fops = &misc_fops,
};


int led_probe( struct platform_device *pdev)
{
	//5匹配成功后进入probe函数
	int ret;

	printk( "led_probe\n");

	//注册杂项设备
	ret = misc_register( &misc_dev); 
	if (ret < 0)
	{
		printk( "misc registe is error \n");
	}
	printk( "misc registe is succeed \n");

	//获取设备树里面reg属性的值
	
	ret = of_property_read_u32_array( pdev->dev.of_node, "reg", out_values, 6);
	
	if( ret<0){
		printk("of_property_read_u32_array is error \n");
		return -1;
	}

	//对物理地址进行虚拟映射
	
	vir_gpio4_dr = ioremap( out_values[0],out_values[1]);
	if( vir_gpio4_dr== NULL )
	{
		printk( "gpio4dr ioremap error\n");
		return EBUSY;
	}
	
	vir_gpio4_h = ioremap( out_values[2],out_values[3]);
	if( vir_gpio4_h== NULL)
	{
		printk( "gpio4h ioremap error\n");
		return EBUSY;
	}

	vir_gpio4_l = ioremap( out_values[4],out_values[5]);
	if( vir_gpio4_l == NULL)
	{
		printk( "gpio4l ioremap error\n");
		return EBUSY;
	}
	printk( "gpio ioremap success\n");

	return 0;

}

int led_remove( struct platform_device *pdev)
{
	printk("led_remove\n");
	return 0;
}
struct platform_driver led_driver ={   
	//3.在led_driver结构体中完成了led_probe和led_remove

	.probe = led_probe,
	.remove = led_remove,

	//4.在driver结构体里面填写匹配名字，让他匹配设备树里面的led_test节点
	.driver = {
		.owner = THIS_MODULE,
		.name = "led_test",                //匹配名字，匹配成功后进入probe函数
		.of_match_table = of_match_table_test//优先匹配of_match_table
	},
};
  
static int led_driver_init( void)
{
	//1.看驱动文件先从init函数看
	int ret = 0;
	//2.在init函数里面注册了platform_driver
	ret = platform_driver_register( &led_driver);
	if( ret<0)
	{
		printk( "platform_driver_register error \n");
	}
	printk( "platform_driver_register ok \n");
	return 0;
}

static void led_driver_exit(void)
{
	misc_deregister( &misc_dev); //卸载杂项设备
	printk( "misc gooodbye! \n");
	iounmap( vir_gpio4_dr);
	iounmap( vir_gpio4_h);
	iounmap( vir_gpio4_l);

	// platform 驱动卸载
	platform_driver_unregister( &led_driver);
	printk( "goodbye! \n");
}

module_init( led_driver_init);
module_exit( led_driver_exit);

MODULE_LICENSE( "GPL");

