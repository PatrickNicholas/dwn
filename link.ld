SECTIONS
{
	.rodata.module_kernel ALIGN(0x10) : {
		PROVIDE(module_kernel_start = .);
		*(.module_kernel)
		PROVIDE(module_kernel_end = .);
	}
	.rodata.module_config ALIGN(0x10): {
		PROVIDE(module_config_start = .);
		*(.module_config)
		PROVIDE(module_config_end = .);
	}
}
INSERT AFTER .text;

