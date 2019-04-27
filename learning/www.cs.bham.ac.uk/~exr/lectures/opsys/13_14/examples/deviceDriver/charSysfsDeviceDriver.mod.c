#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x84b3ef83, "module_layout" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x39d636e4, "cdev_del" },
	{ 0xd1515064, "cdev_add" },
	{ 0x685acc2b, "cdev_init" },
	{ 0xdbf43d82, "cdev_alloc" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0xc3aaf0a9, "__put_user_1" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0x59177771, "try_module_get" },
	{ 0x91715312, "sprintf" },
	{ 0x125059d3, "module_put" },
	{ 0x50eedeb8, "printk" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "BA5DB58E0A2195D24FACE4F");
