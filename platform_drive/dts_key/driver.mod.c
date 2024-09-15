#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xd4b13e21, "module_layout" },
	{ 0x13868293, "platform_driver_unregister" },
	{ 0x81cbe3cb, "__platform_driver_register" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x6f414e08, "gpiod_get_raw_value" },
	{ 0x92997ed8, "_printk" },
	{ 0xf355108, "cdev_add" },
	{ 0x4392e2ef, "cdev_init" },
	{ 0x12284cf8, "device_create" },
	{ 0x42824447, "__class_create" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0x3ba9b8b7, "gpiod_to_irq" },
	{ 0x3a109e75, "gpio_to_desc" },
	{ 0x95fc5f46, "of_get_named_gpio_flags" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x2cfe9c2b, "of_count_phandle_with_args" },
	{ 0x619cb7dd, "simple_read_from_buffer" },
	{ 0x98cf60b3, "strlen" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x551ab123, "class_destroy" },
	{ 0xc6bfeb6c, "device_destroy" },
	{ 0xc793df81, "cdev_del" },
	{ 0x37a0cba, "kfree" },
	{ 0xc1514a3b, "free_irq" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "29D14EB80035913B8D33F90");
