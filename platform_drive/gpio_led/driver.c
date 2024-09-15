#include <linux/init.h>  
#include <linux/module.h> 
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/miscdevice.h> //注册杂项设备头文件
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>

unsigned int *vir_gpio4_dr=NULL;
unsigned int *vir_gpio4_h=NULL;
unsigned int *vir_gpio4_l=NULL;

struct resource *gpio4_dir;
struct resource *gpio4_h;
struct resource *gpio4_l;

struct resource *leddir_mem_test;
struct resource *ledh_mem_test;
struct resource *ledl_mem_test;

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
	
	int ret;

	printk( "led_probe\n");
	ret = misc_register( &misc_dev); //注册杂项设备
	if (ret < 0)
	{
		printk( "misc registe is error \n");
	}
	printk( "misc registe is succeed \n");

	gpio4_dir = platform_get_resource( pdev, IORESOURCE_MEM, 0);
	gpio4_h   = platform_get_resource( pdev, IORESOURCE_MEM, 1);
	gpio4_l   = platform_get_resource( pdev, IORESOURCE_MEM, 2);

	vir_gpio4_dr = ioremap( gpio4_dir->start,4);
	if( vir_gpio4_dr== NULL )
	{
		printk( "gpio4dr ioremap error\n");
		return EBUSY;
	}
	
	vir_gpio4_h = ioremap( gpio4_h->start,4);
	if( vir_gpio4_h== NULL)
	{
		printk( "gpio4h ioremap error\n");
		return EBUSY;
	}

	vir_gpio4_l = ioremap( gpio4_l->start,4);
	if( vir_gpio4_l == NULL)
	{
		printk( "gpio4l ioremap error\n");
		return EBUSY;
	}
	printk( "gpio ioremap success\n");

	return 0;
#if 0
	leddir_mem_test = request_mem_region( gpio4_dir->start, gpio4_dir->end - gpio4_dir->start +1, "led_dir");
	if( leddir_mem_test == NULL){
		printk( "platform_get_resource iserror \n");
		goto errdir_region;
	}
	
	ledh_mem_test = request_mem_region( gpio4_dir->start, gpio4_dir->end - gpio4_dir->start +1, "led_dir");
	if( ledh_mem_test == NULL){
		printk( "platform_get_resource iserror \n");
		goto errh_region;	
	}
	
	ledl_mem_test = request_mem_region( gpio4_dir->start, gpio4_dir->end - gpio4_dir->start +1, "led_dir");
	if( ledl_mem_test == NULL){
		printk( "platform_get_resource iserror \n");
		goto errl_region;
	}
	return 0;

errdir_region:
	release_mem_region( gpio4_dir->start, gpio4_dir->end - gpio4_dir->start +1);

	return -EBUSY;
errh_region:
	release_mem_region( gpio4_h->start, gpio4_h->end - gpio4_h->start +1);

	return -EBUSY;	
errl_region:
	release_mem_region(gpio4_l->start, gpio4_l->end - gpio4_l->start +1);

	return -EBUSY;
#endif
}

int led_remove( struct platform_device *pdev)
{
	printk("led_remove\n");
	return 0;
}
struct platform_driver led_driver ={
	.probe = led_probe,
	.remove = led_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "led_test"
	},
};

static int led_driver_init( void)
{
	int ret =0;
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

