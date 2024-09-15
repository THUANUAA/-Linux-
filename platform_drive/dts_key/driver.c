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
#include <linux/uaccess.h>
#include <linux/cdev.h>

#define DEVICE_NAME "gpio_key_device"

struct gpio_key {
    int gpio;
    int irq;
    enum of_gpio_flags flag;
};

static struct gpio_key *gpio_keys;
static int num_keys;
static dev_t dev;
static struct cdev my_cdev;
static struct class *cl;
static char irq_msg[256] = {0};

static irqreturn_t gpio_key_irq_handle(int irq, void *dev_id) {
    struct gpio_key *gpio_key = dev_id;
    snprintf(irq_msg, sizeof(irq_msg), "key %d val %d\n", irq, gpio_get_value(gpio_key->gpio));
	printk("key %d val %d\n", irq, gpio_get_value(gpio_key->gpio));
    return IRQ_HANDLED;
}

static ssize_t gpio_key_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
    return simple_read_from_buffer(buf, count, offset, irq_msg, strlen(irq_msg));
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = gpio_key_read,
};

static int chip_demo_gpio_probe(struct platform_device *pdev) {
    struct device_node *node = pdev->dev.of_node;
    int i, ret = 0;

    num_keys = of_gpio_count(node);
    if (num_keys <= 0) {
        ret = -EINVAL;
        goto fail_count;
    }

    gpio_keys = kzalloc(num_keys * sizeof(struct gpio_key), GFP_KERNEL);
    if (!gpio_keys) {
        printk("内存分配失败\n");
        ret = -ENOMEM;
        goto fail_kzalloc;
    }

    for (i = 0; i < num_keys; i++) {
        gpio_keys[i].gpio = of_get_gpio_flags(node, i, &gpio_keys[i].flag);
        if (!gpio_is_valid(gpio_keys[i].gpio)) {
            printk("设备树获取失败 key: %d\n", i);
            ret = -EINVAL;
            goto fail_flags;
        }
        gpio_keys[i].irq = gpio_to_irq(gpio_keys[i].gpio);
        if (gpio_keys[i].irq < 0) {
            printk("中断号获取失败 key: %d\n", i);
            ret = gpio_keys[i].irq;
            goto fail_irq;
        }
        ret = request_irq(gpio_keys[i].irq, gpio_key_irq_handle, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "test_gpio_key", &gpio_keys[i]);
        if (ret != 0) {
            printk("无法请求 gpio_keys irq\n");
            free_irq(gpio_keys[i].irq, &gpio_keys[i]);
            goto fail_request;
        }
    }

    // 注册字符设备
    if (alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME) < 0) {
        printk(KERN_ALERT "设备号分配失败\n");
        goto fail_chrdev;
    }
    cl = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(cl)) {
        unregister_chrdev_region(dev, 1);
        printk(KERN_ALERT "类创建失败\n");
        goto fail_class;
    }
    if (device_create(cl, NULL, dev, NULL, DEVICE_NAME) == NULL) {
        class_destroy(cl);
        unregister_chrdev_region(dev, 1);
        printk(KERN_ALERT "设备创建失败\n");
        goto fail_device;
    }
    cdev_init(&my_cdev, &fops);
    if (cdev_add(&my_cdev, dev, 1) == -1) {
        device_destroy(cl, dev);
        class_destroy(cl);
        unregister_chrdev_region(dev, 1);
        printk(KERN_ALERT "字符设备添加失败\n");
        goto fail_cdev;
    }

    return 0;

fail_cdev:
    device_destroy(cl, dev);
fail_device:
    class_destroy(cl);
fail_class:
    unregister_chrdev_region(dev, 1);
fail_chrdev:
fail_request:
fail_irq:
fail_flags:
    kfree(gpio_keys);
fail_kzalloc:
fail_count:
    return ret;
}

static const struct of_device_id key_gpios[] = {
    {.compatible = "keytest"},
    {},
};

static int chip_demo_gpio_remove(struct platform_device *pdev) {
    int i;

    for (i = 0; i < num_keys; i++) {
        free_irq(gpio_keys[i].irq, &gpio_keys[i]);
    }
    kfree(gpio_keys);

    // 注销字符设备
    cdev_del(&my_cdev);
    device_destroy(cl, dev);
    class_destroy(cl);
    unregister_chrdev_region(dev, 1);

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

static __init int test_gpio_init(void) {
    printk("test_gpio_init\n");
    return platform_driver_register(&test_gpio_drv);
}

static __exit void test_gpio_exit(void) {
    printk("test_gpio_exit\n");
    platform_driver_unregister(&test_gpio_drv);
}

module_init(test_gpio_init);
module_exit(test_gpio_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xxx");

