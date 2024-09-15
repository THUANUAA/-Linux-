#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/interrupt.h>    /* Tasklet API */ 
 
char tasklet_data[]="We use a string; but it could be pointer to a structure"; 
 
/* Tasklet处理程序，只打印数据 */ 
void tasklet_work(unsigned long data) 
{ 
    printk("%s\n", (char *)data); 
} 
 
tasklet_init(my_tasklet, tasklet_work,(unsigned long)tasklet_data); 
 
static int __init my_init(void) 
{ 
    /* 
     * 安排处理程序 
     * 从中断处理程序调度Tasklet arealso  
     */ 
    tasklet_schedule(&my_tasklet); 
    return 0;  
} 
 
void my_exit(void) 
{ 
    tasklet_kill(&my_tasklet); 
} 
 
module_init(my_init); 
module_exit(my_exit);  
MODULE_LICENSE("GPL");
