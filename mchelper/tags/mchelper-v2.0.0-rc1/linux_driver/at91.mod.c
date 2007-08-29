#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

#undef unix
struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = __stringify(KBUILD_MODNAME),
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
};

static const struct modversion_info ____versions[]
__attribute_used__
__attribute__((section("__versions"))) = {
	{ 0xf0b8c1db, "struct_module" },
	{ 0x66cb0a76, "usb_deregister" },
	{ 0x372cdc21, "usb_register" },
	{ 0x5dfe8f1a, "unlock_kernel" },
	{ 0x7c456125, "usb_deregister_dev" },
	{ 0x5568be43, "lock_kernel" },
	{ 0xc35131dd, "usb_register_dev" },
	{ 0x77414029, "__kmalloc" },
	{ 0xa42caaab, "usb_get_dev" },
	{ 0x3ae831b6, "kref_init" },
	{ 0x490baed2, "kmem_cache_alloc" },
	{ 0xc1f34dfc, "malloc_sizes" },
	{ 0xc13a3a1a, "usb_submit_urb" },
	{ 0xc463f92d, "usb_free_urb" },
	{ 0xf2a644fb, "copy_from_user" },
	{ 0xecbee2b1, "usb_buffer_alloc" },
	{ 0x914aa763, "usb_alloc_urb" },
	{ 0xe9d9239e, "usb_buffer_free" },
	{ 0x2da418b5, "copy_to_user" },
	{ 0xfed1f240, "usb_bulk_msg" },
	{ 0xcff53400, "kref_put" },
	{ 0x1b7d4074, "printk" },
	{ 0x8a1203a9, "kref_get" },
	{ 0xb57d6cb8, "usb_find_interface" },
	{ 0x37a0cba, "kfree" },
	{ 0x2be7cb54, "usb_put_dev" },
};

static const char __module_depends[]
__attribute_used__
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("usb:v03EBp6124d*dc*dsc*dp*ic*isc*ip*");

MODULE_INFO(srcversion, "7DE458E1264D2476CE45F78");
