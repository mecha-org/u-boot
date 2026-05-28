// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 */

#include <asm/arch/sys_proto.h>
#include <config.h>
#include <efi_loader.h>
#include <env.h>
#include <dm/uclass.h>
#include <dm.h>

#if CONFIG_IS_ENABLED(EFI_HAVE_CAPSULE_SUPPORT)
#define IMX_BOOT_IMAGE_GUID \
	EFI_GUID(0x928b33bc, 0xe58b, 0x4247, 0x9f, 0x1d, \
		 0x3b, 0xf1, 0xee, 0x1c, 0xda, 0xff)

struct efi_fw_image fw_images[] = {
	{
		.image_type_id = IMX_BOOT_IMAGE_GUID,
		.fw_name = u"Mecha Comet",
		.image_index = 1,
	},
};

struct efi_capsule_update_info update_info = {
	.dfu_string = "mmc 2=flash-bin raw 0 0x2000 mmcpart 1",
	.num_images = ARRAY_SIZE(fw_images),
	.images = fw_images,
};
#endif /* EFI_HAVE_CAPSULE_SUPPORT */

#include <clk.h>

#ifdef CONFIG_OF_BOARD_SETUP

int ft_board_setup(void *blob, struct bd_info *bd)
{
	int ret, node;
	int banks = 1;
	u64 base[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];

	/* fixup DT for the two DDR banks */
	base[0] = gd->bd->bi_dram[0].start;
	size[0] = gd->bd->bi_dram[0].size;
#ifndef CONFIG_IMX8M_LPDDR4_2GB
	base[1] = gd->bd->bi_dram[1].start;
	size[1] = gd->bd->bi_dram[1].size;
	banks = 2;
#endif

	ret = fdt_fixup_memory_banks(blob, base, size, banks);

	if (ret) {
		printf("Failed to setup memory bank values on fdt.\n");
		return ret;
	}

	return 0;
}

#endif

int board_init(void)
{
    return 0;
}

int board_early_init_r(void)
{
	return 0;
}

int board_late_init(void)
{
	if (is_usb_boot()) {
		env_set("bootcmd", "fastboot 0");
		env_set("bootdelay", "0");
	}
	return 0;
}
