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



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xd3fc359e, "i2c_register_driver" },
	{ 0x8a38c4d4, "device_destroy" },
	{ 0x7ba9eeeb, "class_destroy" },
	{ 0xfea0e755, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x92997ed8, "_printk" },
	{ 0x80df99b0, "i2c_del_driver" },
	{ 0x2196324, "__aeabi_idiv" },
	{ 0xdeb3a0d2, "i2c_smbus_write_byte_data" },
	{ 0xf9a482f9, "msleep" },
	{ 0xf0654627, "i2c_smbus_read_byte_data" },
	{ 0xe707d823, "__aeabi_uidiv" },
	{ 0x5f754e5a, "memset" },
	{ 0xb1aaaace, "i2c_smbus_read_i2c_block_data" },
	{ 0xc358aaf8, "snprintf" },
	{ 0x97255bdf, "strlen" },
	{ 0x528c709d, "simple_read_from_buffer" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xb482f127, "cdev_init" },
	{ 0xf7bde4bc, "cdev_add" },
	{ 0xb64449fe, "class_create" },
	{ 0xa4a61afc, "device_create" },
	{ 0x1aa733d9, "_dev_info" },
	{ 0x1b95b0fc, "_dev_err" },
	{ 0x7d5dce20, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cbosch,bmp180");
MODULE_ALIAS("of:N*T*Cbosch,bmp180C*");

MODULE_INFO(srcversion, "F7122C1D25EFD5314BD8F76");
