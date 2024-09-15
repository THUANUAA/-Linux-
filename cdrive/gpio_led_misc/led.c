#include <linux/init.h> //初始化头文件
#include <linux/module.h> //最基本的文件，支持动态添加和卸载模块。
#include <linux/miscdevice.h> //注册杂项设备头文件
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>

#define GPIO4_DR 0xfe200000
#define GPIO4_H  0xfe20001c
#define GPIO4_L  0xfe200028

unsigned int *vir_gpio4_dr=NULL;
unsigned int *vir_gpio4_h=NULL;
unsigned int *vir_gpio4_l=NULL;
ssize_t misc_read(struct file *file, char __user *ubuf, size_t size, loff_t *loff_t)
{
	char kbuf[] = "hehe";
	if (copy_to_user(ubuf, kbuf, strlen(kbuf)) != 0)
	{
		printk("copy_to_user error\n ");
		return -1;
	}
	printk("misc_read\n ");
	return 0;
}

ssize_t misc_write(struct file *file, const char __user *ubuf, size_t size, loff_t *loff_t)
{
	char kbuf[64] = {0};
	if (copy_from_user(kbuf, ubuf, size) != 0)
	{
		printk("copy_from_user error\n ");
		return -1;
	}
	printk("kbuf is %s\n ", kbuf);
	*vir_gpio4_dr |=(1<<(3*4));
	if(kbuf[0]==1)
	{
		
		*vir_gpio4_h |=(1<<4);
	}
	else if(kbuf[0]==0)
	{
		*vir_gpio4_l |=(1<<4);
	}
	return 0;
}

int misc_release(struct inode *inode, struct file *file)
{
	printk("hello misc_relaease bye bye \n ");
	return 0;
}

int misc_open(struct inode *inode, struct file *file)
{
	printk("hello misc_open\n ");
	return 0;
}
//文件操作集
struct file_operations misc_fops = {
		.owner = THIS_MODULE, 
		.open = misc_open,
		.release = misc_release,
		.read = misc_read,
		.write = misc_write,
		};
//miscdevice 结构体
struct miscdevice misc_dev = {
	.minor = MISC_DYNAMIC_MINOR, 
	.name = "hello_misc",
	.fops = &misc_fops,
};
static int misc_init(void)
{
	int ret;
	ret = misc_register(&misc_dev); //注册杂项设备
	if (ret < 0)
	{
		printk("misc registe is error \n");
	}
	printk("misc registe is succeed \n");
	
	vir_gpio4_dr=ioremap(GPIO4_DR,4);
	if(vir_gpio4_dr==NULL )
	{
		printk("gpio4dr ioremap error\n");
		return EBUSY;
	}
	
	vir_gpio4_h=ioremap(GPIO4_H,4);
	if( vir_gpio4_h==NULL)
	{
		printk("gpio4h ioremap error\n");
		return EBUSY;
	}

	vir_gpio4_l=ioremap(GPIO4_L,4);
	if(vir_gpio4_l==NULL)
	{
		printk("gpio4l ioremap error\n");
		return EBUSY;
	}
	printk("gpio ioremap success\n");
	return 0;
}
static void misc_exit(void)
{
	misc_deregister(&misc_dev); //卸载杂项设备
	printk(" misc gooodbye! \n");
	iounmap(vir_gpio4_dr);
	iounmap(vir_gpio4_h);
	iounmap(vir_gpio4_l);
}
module_init(misc_init);
module_exit(misc_exit);
MODULE_LICENSE("GPL");

