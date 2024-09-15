#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/jiffies.h>


struct gpio_key{
	int gpio;
	int irq;
	enum of_gpio_flags flag;
};

struct pi4irq_dev{
	struct tasklet_struct tasklet;        /* 中断下半部 */
	struct timer_list timer;              /* 定时器 */
    int timePeriod;                       /* 定时周期，单位为ms */
};


static struct gpio_key *gpio_keys;
static struct pi4irq_dev pi4irq;

/* 中断上半部，中断服务函数 */
static irqreturn_t gpio_key_irq_handle(int irq, void *dev_id)
{
	/* 调度tasklet */
    tasklet_schedule(&pi4irq.tasklet);

	return IRQ_HANDLED;
}

/* 中断下半部tasklet */
static void key_tasklet(unsigned long data)
{
	struct pi4irq_dev *dev = &pi4irq;
	int timerPeriod;

	timerPeriod = dev->timePeriod;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timerPeriod));
}


/* 定时器 */
static void timer_function(struct timer_list* time)
{
	struct gpio_key *dev = gpio_keys;
	int value;

	value = gpio_get_value(dev->gpio);
	if(value == 0)
	{
		printk("中断号:%d  松开\n", dev->irq);
	}
	else
	{
		printk("中断号:%d  按下\n", dev->irq);
	}
}


static int chip_demo_gpio_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	int count, i;
	int ret = 0;

	count = of_gpio_count(node);
	if(count <= 0)
	{
		ret = -EINVAL;
		goto fail_count;
	}

	gpio_keys = kzalloc(count * sizeof(struct gpio_key), GFP_KERNEL);
	if(!gpio_keys)
	{
		printk("内存分配失败\n");
		ret = -ENOMEM;
		goto fail_kzalloc;
	}
	for(i = 0; i < count; i++)
	{
		gpio_keys[i].gpio = of_get_gpio_flags(node, i, &gpio_keys[i].flag);
		if (!gpio_is_valid(gpio_keys[i].gpio))
		{
			printk("设备树获取失败 key: %d\n", i);
			ret =  -EINVAL;
			goto fail_flags;
		}
		gpio_keys[i].irq = gpio_to_irq(gpio_keys[i].gpio);
		if(gpio_keys[i].irq < 0)
		{
			printk("中断号获取失败 key: %d\n", i);
			ret = gpio_keys[i].irq;
			goto fail_irq;
		}
		ret = request_irq(gpio_keys[i].irq, gpio_key_irq_handle, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "test_gpio_key", &gpio_keys[i]);
		if (ret != 0)
		{
			ret = -1;
			printk("无法请求 gpio_keys irq\n");
			free_irq(gpio_keys[i].irq, &gpio_keys[i]);
			goto fail_request;
		}
	}
	tasklet_init(&pi4irq.tasklet, key_tasklet, (unsigned long)gpio_keys);

	return 0;

fail_request:
fail_irq:
fail_flags:
fail_kzalloc:
	kfree((struct gpio_key*)gpio_keys);
fail_count:
	return ret;
}

static const struct of_device_id key_gpios[] = {
	{.compatible = "keytest"},
	{},
};

static int chip_demo_gpio_remove(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	int count, i;

	count = of_gpio_count(node);
	for(i = 0; i < count; i++)
	{
		free_irq(gpio_keys[i].irq, &gpio_keys[i]);
	}
	kfree((struct gpio_key*)gpio_keys);

	return 0;
}


static struct platform_driver test_gpio_drv = {
	.probe = chip_demo_gpio_probe,
	.remove = chip_demo_gpio_remove,
	.driver = {
		.name = "keytest",
		.of_match_table = key_gpios,
	},
};

static __init int test_gpio_init(void)
{
	printk("test_gpio_init\n");
	platform_driver_register(&test_gpio_drv);

	/* 初始化定时器 */
    timer_setup(&pi4irq.timer, timer_function, 0);
    pi4irq.timePeriod = 20;
    pi4irq.timer.expires = jiffies + msecs_to_jiffies(pi4irq.timePeriod);

	return 0;
}

static __exit void test_gpio_exit(void)
{
	printk("test_gpio_exit\n");
	platform_driver_unregister(&test_gpio_drv);

	/* 删除 timer */
    del_timer_sync(&pi4irq.timer);
}

module_init(test_gpio_init);
module_exit(test_gpio_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xxx");

