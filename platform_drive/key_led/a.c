#include <linux/init.h>            //初始化头文件
#include <linux/module.h>          //最基本的文件，支持动态添加和卸载模块
#include <linux/platform_device.h> //platform平台设备相关的头文件
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/miscdevice.h> //注册杂项设备头文件
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

int size;
int led_gpio = 0;
int ret = 0;
u32 out_values[2]={0};  //用于存储从设备树获取的reg数据
const char *str;

struct device_node *test_device_node;
struct property *test_node_property;



ssize_t misc_write(struct file *file, const char __user *ubuf, size_t size, loff_t *loff_t)
{
	char kbuf[64] = {0};
	if ( copy_from_user( kbuf, ubuf, size) != 0)
	{
		printk( "copy_from_user error\n ");
		return -1;
	}
	printk( "kbuf is %s\n ", kbuf);

	//如果传递进内核的数据是1，则led亮
	if(kbuf[0]==1){

		gpio_set_value(led_gpio, 1);		
	}
	else if(kbuf[0]==0){

		gpio_set_value(led_gpio, 0);		
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

	//6查找我们要查找的节点

	test_device_node = of_find_node_by_path("/led_test@0xfe200000");

	if(test_device_node == NULL){

		printk("of_find_node_by_path error");
		return -1;
	}

	printk("of_find_node_by_path success");

	
	//从设备树获取要使用的GPIO
	/* of_get_named_gpio 函数获取 GPIO 编号，
	因为 Linux 内核中关于 GPIO 的 API 函数都要使用 GPIO 编号，
	此函数会将设备树中类似<&gpio 4 0>的属性信息转换为对应的 GPIO 编号 */
	led_gpio = of_get_named_gpio(test_device_node, "gpios", 0);
	if (led_gpio < 0)
	{
	printk("of_get_named_gpio is error \n");
	return -1;
	}
	printk("led_gpio is %d \n", led_gpio);
	//申请一个 GPIO 管脚
	ret = gpio_request(led_gpio, "led");
	if (ret < 0)
	{
	printk("gpio_request is error \n");
	return -1;
	}
	//设置某个 GPIO 为输出，并且设置默认输出值
	gpio_direction_output(led_gpio, 1);
	//注册杂项设备
	ret = misc_register( &misc_dev); 
	if (ret < 0)
	{
		printk( "misc registe is error \n");
	}
	printk( "misc registe is succeed \n");

	return 0;

}

int led_remove( struct platform_device *pdev)
{
	gpio_free(led_gpio);
	printk("led_remove\n");
	return 0;
}

const struct of_device_id of_match_table_test[] = {
{	
	.compatible = {"test_led"}, 
	{}, 
};


struct platform_driver led_driver ={   
	//3.在led_driver结构体中完成了led_probe和led_remove

	.probe = led_probe,
	.remove = led_remove,

	//4.在driver结构体里面填写匹配名字，让他匹配设备树里面的led_test节点
	.driver = {
		.owner = THIS_MODULE,
		.name = "test_led",                //匹配名字(匹配的是设备树)，匹配成功后进入probe函数
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
	
	// platform 驱动卸载
	platform_driver_unregister( &led_driver);
	printk( "goodbye! \n");
}

module_init( led_driver_init);
module_exit( led_driver_exit);

MODULE_LICENSE( "GPL");


