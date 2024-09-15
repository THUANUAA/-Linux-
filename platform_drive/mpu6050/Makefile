# 判断是否在内核构建系统内。如果没有定义 KERNELRELEASE，则表示这是从命令行调用。
ifneq ($(KERNELRELEASE),)
        # 如果不是内核构建系统，则定义需要编译的模块对象文件。
        # obj-m 是内核模块的编译变量，+= 表示添加模块文件（.o）
		obj-m += mpu6050.o
else
        # 定义内核头文件的位置，使用当前正在运行的内核版本。
        KDIR := /home/interest/linux/lib/modules/$(shell uname -r)/build
 
        # 定义当前的工作目录。
        PWD := $(shell pwd)
 
        # 默认目标。如果调用了 make 而没有指定目标，会执行这个部分。
        # -C $(KDIR) 表示切换到内核源码目录进行编译
        # M=$(PWD) 表示在当前模块的目录下执行内核模块编译
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
 
        # 清理目标：用于清理编译产生的中间文件。
clean:
	rm -f *.mod.c *.order *.ko *.o *.mod *.symvers
endif
