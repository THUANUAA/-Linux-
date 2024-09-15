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
	{ 0x551ab123, "class_destroy" },
	{ 0x13868293, "platform_driver_unregister" },
	{ 0x81cbe3cb, "__platform_driver_register" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x42824447, "__class_create" },
	{ 0x845319c9, "__register_chrdev" },
	{ 0x8da6585d, "__stack_chk_fail" },
	{ 0xec3d2e1b, "trace_hardirqs_off" },
	{ 0xd697e69a, "trace_hardirqs_on" },
	{ 0x4b0a3f52, "gic_nonsecure_priorities" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x9714e0bb, "ktime_get_raw" },
	{ 0xaf18e39b, "gpiod_get_value" },
	{ 0xf28855b, "gpiod_direction_input" },
	{ 0xb127e82, "gpiod_set_value" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0xca2da281, "gpiod_direction_output" },
	{ 0x12284cf8, "device_create" },
	{ 0xee5b0c9b, "gpiod_get" },
	{ 0x82395752, "gpiod_put" },
	{ 0xc6bfeb6c, "device_destroy" },
	{ 0x92997ed8, "_printk" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "9421D23C5C99100169F33B5");
