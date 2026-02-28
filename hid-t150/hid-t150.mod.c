#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

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


MODULE_INFO(depends, "");

MODULE_ALIAS("hid:b0003g*v0000044Fp0000B677");
MODULE_ALIAS("hid:b0003g*v0000044Fp0000B67F");

MODULE_INFO(srcversion, "928FA9AFFDA3E6B20F8973B");
