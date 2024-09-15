#include <linux/init.h>  //包含宏定义的头文件
#include <linux/module.h>  //包含初始化加载模块的头文件


static int __init hello_init(void)
{                                                                                                                                                                    
        printk("hello world \n");                                                                                                                                           
        return 0;
}
 
static void __exit hello_exit(void)
{
        printk("Goodbye, the world\n");
}
 
module_init(hello_init);
module_exit(hello_exit);  


MODULE_LICENSE("GPL");
