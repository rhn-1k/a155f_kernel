/*
 * linux/drivers/video/fbdev/exynos/panel/tft_common/tft_common.c
 *
 * TFT_COMMON Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/of_gpio.h>
#include <video/mipi_display.h>
#include "../maptbl.h"
#include "../panel.h"
#include "../panel_debug.h"
#include "../panel_function.h"
#include "s6e8fc3_a16x_panel.h"
#include "s6e8fc3_a16x_ezop.h"

static int s6e8fc3_a16x_br_index_property_update(struct panel_property *prop)
{
	struct panel_device *panel = prop->panel;
	struct panel_bl_device *panel_bl = &panel->panel_bl;

	return panel_property_set_value(prop,
			get_brightness_pac_step_by_subdev_id(panel_bl,
				PANEL_BL_SUBDEV_TYPE_DISP, panel_bl->props.brightness));
}

static int s6e8fc3_a16x_first_br_property_update(struct panel_property *prop)
{
	struct panel_device *panel = prop->panel;
	struct panel_bl_device *panel_bl = &panel->panel_bl;

	int index;

	if (atomic_read(&panel_bl->props.brightness_non_zero_set_count) == 2)
		index = S6E8FC3_A16X_FIRST_BR_ON;
	else
		index = S6E8FC3_A16X_FIRST_BR_OFF;

	if (index == S6E8FC3_A16X_FIRST_BR_ON)
		panel_info("first non-zero br!\n");

	return panel_property_set_value(prop, index);
}

#ifdef CONFIG_USDM_MDNIE
static int oled_mdnie_night_level_property_update(struct panel_property *prop)
{
	struct panel_device *panel = prop->panel;
	struct mdnie_info *mdnie = &panel->mdnie;

	return panel_property_set_value(prop, mdnie->props.night_level);
}
#endif

static struct panel_prop_enum_item s6e8fc3_a16x_first_br_enum_items[MAX_S6E8FC3_A16X_FIRST_BR] = {
	__PANEL_PROPERTY_ENUM_ITEM_INITIALIZER(S6E8FC3_A16X_FIRST_BR_OFF),
	__PANEL_PROPERTY_ENUM_ITEM_INITIALIZER(S6E8FC3_A16X_FIRST_BR_ON),
};

static struct panel_prop_list s6e8fc3_a16x_property_array[] = {
	/* range property */
	__DIMEN_PROPERTY_RANGE_INITIALIZER(S6E8FC3_A16X_BR_INDEX_PROPERTY,
			0, 0, S6E8FC3_A16X_TOTAL_NR_LUMINANCE,
			s6e8fc3_a16x_br_index_property_update),
	__DIMEN_PROPERTY_ENUM_INITIALIZER(S6E8FC3_A16X_FIRST_BR_PROPERTY,
			S6E8FC3_A16X_FIRST_BR_OFF, s6e8fc3_a16x_first_br_enum_items,
			s6e8fc3_a16x_first_br_property_update),

#ifdef CONFIG_USDM_MDNIE
	__DIMEN_PROPERTY_RANGE_INITIALIZER(OLED_MDNIE_NIGHT_LEVEL_PROPERTY,
			0, 0, S6E8FC3_A16X_MAX_NIGHT_LEVEL,
			oled_mdnie_night_level_property_update),
#endif
};

static struct dump_expect s6e8fc3_a16x_self_mask_crc_expects[] = {
	{ .offset = 0, .mask = 0xFF, .value = 0x35, .msg = "Self Mask CRC[0] Error(NG)" },
	{ .offset = 1, .mask = 0xFF, .value = 0xf6, .msg = "Self Mask CRC[1] Error(NG)" },
};
static struct dumpinfo s6e8fc3_a16x_dump_self_mask_crc = DUMPINFO_INIT_V2(self_mask_crc,
		&s6e8fc3_restbl[RES_SELF_MASK_CRC], &OLED_FUNC(OLED_DUMP_SHOW_EXPECTS), s6e8fc3_a16x_self_mask_crc_expects);

static struct dump_expect s6e8fc3_a16x_self_mask_checksum_expects[] = {
	{ .offset = 0, .mask = 0xFF, .value = 0x74, .msg = "Self Mask CHKSUM[0] Error(NG)" },
	{ .offset = 1, .mask = 0xFF, .value = 0x3D, .msg = "Self Mask CHKSUM[1] Error(NG)" },
	{ .offset = 2, .mask = 0xFF, .value = 0x64, .msg = "Self Mask CHKSUM[2] Error(NG)" },
	{ .offset = 3, .mask = 0xFF, .value = 0x8D, .msg = "Self Mask CHKSUM[3] Error(NG)" },
};
static struct dumpinfo s6e8fc3_a16x_dump_self_mask_checksum = DUMPINFO_INIT_V2(self_mask_checksum,
		&s6e8fc3_restbl[RES_SELF_MASK_CHECKSUM], &OLED_FUNC(OLED_DUMP_SHOW_EXPECTS), s6e8fc3_a16x_self_mask_checksum_expects);

static int __init s6e8fc3_a16x_panel_init(void)
{
	struct common_panel_info *cpi = &s6e8fc3_a16x_panel_info;
	s6e8fc3_init(cpi);
	cpi->prop_lists[USDM_DRV_LEVEL_MODEL] = s6e8fc3_a16x_property_array;
	cpi->num_prop_lists[USDM_DRV_LEVEL_MODEL] = ARRAY_SIZE(s6e8fc3_a16x_property_array);
	cpi->ezop_json = EZOP_JSON_BUFFER;
	memcpy(&cpi->dumpinfo[DUMP_SELF_MASK_CRC],
		&s6e8fc3_a16x_dump_self_mask_crc, sizeof(struct dumpinfo));
	memcpy(&cpi->dumpinfo[DUMP_SELF_MASK_CHECKSUM],
		&s6e8fc3_a16x_dump_self_mask_checksum, sizeof(struct dumpinfo));

	register_common_panel(&s6e8fc3_a16x_panel_info);

	return 0;
}

static void __exit s6e8fc3_a16x_panel_exit(void)
{
	deregister_common_panel(&s6e8fc3_a16x_panel_info);
}

module_init(s6e8fc3_a16x_panel_init);
module_exit(s6e8fc3_a16x_panel_exit);
MODULE_DESCRIPTION("Samsung Mobile Panel Driver");
MODULE_LICENSE("GPL");
