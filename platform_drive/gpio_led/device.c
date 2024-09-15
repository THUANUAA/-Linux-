#include <linux/init.h>  
#include <linux/module.h> 
#include <linux/platform_device.h>
#include <linux/ioport.h>

void led_release(struct device *dev)
{
	printk("led_release \n");
}

struct resource led_res[] = {
	[0] = {
		.start = 0xfe200000,
		.end = 0xfe200003,
		.flags = IORESOURCE_MEM,
		.name = "GPIO1_IO4DIR",
	},
	[1] = {
		.start = 0xfe20001c,
		.end = 0xfe20001f,
		.flags = IORESOURCE_MEM,
		.name = "GPIO1_IO4H"
	},
	[2] = {
		.start = 0xfe200028,
		.end = 0xfe20002b,
		.flags = IORESOURCE_MEM,
		.name = "GPIO1_IO4L"
	},
};

struct platform_device led_device = {
	/* name必须与static struct platform_driver.name的name字段相同 */
	.name = "led_test",
	.id = -1,
	.resource = led_res,
	.num_resources = ARRAY_SIZE(led_res),
	.dev={
		.release = led_release,
	},

};

static int device_init(void)
{
	/*  
		注册驱动程序并将其放入由内核维护的驱动程序列表中
		以便，每当发现新的匹配时，就可以按需调用其probe（）函数	
	*/
	platform_device_register(&led_device);
	printk("platform_device_register ok \n");
	
	return 0;
}

static void device_exit(void)
{
	platform_device_unregister(&led_device);

	printk("goodbye! \n");
}

module_init(device_init);
module_exit(device_exit);

MODULE_LICENSE("GPL");


