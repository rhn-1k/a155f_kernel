// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/string.h>

#include "kd_camera_typedef.h"
#include "kd_camera_feature.h"
#include "kd_imgsensor.h"

#include "imgsensor_sensor.h"
#include "imgsensor_hw.h"

/*the index is consistent with enum IMGSENSOR_HW_PIN*/
char * const imgsensor_hw_pin_names[] = {
	"none",
	"pdn",
	"rst",
	"vcama",
	"vcama1",
	"vcamaf",
	"vcamd",
	"vcamio",
	"mipi_switch_en",
	"mipi_switch_sel",
	"mclk"
};

/*the index is consistent with enum IMGSENSOR_HW_ID*/
char * const imgsensor_hw_id_names[] = {
	"mclk",
	"regulator",
	"gpio"
};

#if defined(CONFIG_CAMERA_AAX_V16)
static bool uw_cam_status;
#endif

enum IMGSENSOR_RETURN imgsensor_hw_init(struct IMGSENSOR_HW *phw)
{
	struct IMGSENSOR_HW_SENSOR_POWER      *psensor_pwr;
	struct IMGSENSOR_HW_CFG               *pcust_pwr_cfg;
	struct IMGSENSOR_HW_CUSTOM_POWER_INFO *ppwr_info;
	unsigned int i, j;//, len;
	char str_prop_name[LENGTH_FOR_SNPRINTF];
	//const char *pin_hw_id_name;
	struct device_node *of_node
		= of_find_compatible_node(NULL, NULL, "mediatek,imgsensor");
	//int ret_snprintf = 0;

	mutex_init(&phw->common.pinctrl_mutex);

#if 0
	/* update the imgsensor_custom_cfg by dts */
	for (i = 0; i < IMGSENSOR_SENSOR_IDX_MAX_NUM; i++) {
		PK_DBG("IMGSENSOR_SENSOR_IDX: %d\n", i);

		pcust_pwr_cfg = imgsensor_custom_config;

		while (pcust_pwr_cfg->sensor_idx != i &&
		       pcust_pwr_cfg->sensor_idx != IMGSENSOR_SENSOR_IDX_NONE)
			pcust_pwr_cfg++;

		if (pcust_pwr_cfg->sensor_idx == IMGSENSOR_SENSOR_IDX_NONE)
			continue;

		/* i2c_dev */
		switch (i) {
		case IMGSENSOR_SENSOR_IDX_MAIN2:
			{
				if (IS_MT6877(phw->g_platform_id) ||
					IS_MT6833(phw->g_platform_id) ||
					IS_MT6781(phw->g_platform_id) ||
					IS_MT6779(phw->g_platform_id))
					pcust_pwr_cfg->i2c_dev = IMGSENSOR_I2C_DEV_1;
				else
					pcust_pwr_cfg->i2c_dev = IMGSENSOR_I2C_DEV_2;
			}
			break;
		case IMGSENSOR_SENSOR_IDX_SUB2:
			{
				if (IS_MT6785(phw->g_platform_id) ||
					IS_MT6779(phw->g_platform_id) ||
					IS_MT6768(phw->g_platform_id))
					pcust_pwr_cfg->i2c_dev = IMGSENSOR_I2C_DEV_1;
				else
					pcust_pwr_cfg->i2c_dev = IMGSENSOR_I2C_DEV_3;
			}
			break;
		case IMGSENSOR_SENSOR_IDX_MAIN3:
			{
				if (IS_MT6853(phw->g_platform_id) ||
					IS_MT6785(phw->g_platform_id) ||
					IS_MT6779(phw->g_platform_id))
					pcust_pwr_cfg->i2c_dev = IMGSENSOR_I2C_DEV_1;
				else if (IS_MT6893(phw->g_platform_id) ||
					IS_MT6885(phw->g_platform_id) ||
					IS_MT6873(phw->g_platform_id) ||
					IS_MT6855(phw->g_platform_id))
					pcust_pwr_cfg->i2c_dev = IMGSENSOR_I2C_DEV_4;
				else
					pcust_pwr_cfg->i2c_dev = IMGSENSOR_I2C_DEV_2;
			}
			break;
		default:
			break;
		}
		/* pwr_info */
		ppwr_info = pcust_pwr_cfg->pwr_info;
		while (ppwr_info->pin != IMGSENSOR_HW_PIN_NONE) {
			memset(str_prop_name, 0, sizeof(str_prop_name));
			ret_snprintf = snprintf(str_prop_name,
					sizeof(str_prop_name),
					"cam%d_pin_%s",
					i,
					imgsensor_hw_pin_names[ppwr_info->pin]);
			if (ret_snprintf < 0)
				PK_INFO("NOTICE: %s, snprintf err, %d\n", __func__, ret_snprintf);

			if (of_property_read_string(of_node, str_prop_name, &pin_hw_id_name) == 0) {
				for (j = 0; j < IMGSENSOR_HW_ID_MAX_NUM; j++) {
					len = strlen(imgsensor_hw_id_names[j]);
					if (strncmp(pin_hw_id_name, imgsensor_hw_id_names[j], len) == 0) {
						PK_INFO("imgsensor_hw_cfg hw_pin:%s,name:%s,id:%d\n", str_prop_name, pin_hw_id_name, j);
						ppwr_info->id = j;
						break;
					}
				}
			} else {
				PK_DBG("NOTICE: imgsensor_hw_cfg hw_pin:%s, id:%d\n",
					str_prop_name, IMGSENSOR_HW_ID_NONE);
				ppwr_info->id = IMGSENSOR_HW_ID_NONE;
			}
			ppwr_info++;
		}
	}
	/* update the imgsensor_custom_cfg by dts END */
#endif
	for (i = 0; i < IMGSENSOR_HW_ID_MAX_NUM; i++) {
		if (hw_open[i] != NULL)
			(hw_open[i]) (&phw->pdev[i]);

		if (phw->pdev[i]->init != NULL)
			(phw->pdev[i]->init)(
				phw->pdev[i]->pinstance, &phw->common);
	}

	for (i = 0; i < IMGSENSOR_SENSOR_IDX_MAX_NUM; i++) {
		psensor_pwr = &phw->sensor_pwr[i];

		pcust_pwr_cfg = imgsensor_custom_config;

		while (pcust_pwr_cfg->sensor_idx != i &&
		       pcust_pwr_cfg->sensor_idx != IMGSENSOR_SENSOR_IDX_NONE)
			pcust_pwr_cfg++;

		if (pcust_pwr_cfg->sensor_idx == IMGSENSOR_SENSOR_IDX_NONE)
			continue;

		ppwr_info = pcust_pwr_cfg->pwr_info;
		while (ppwr_info->pin != IMGSENSOR_HW_PIN_NONE) {
			if (ppwr_info->pin != IMGSENSOR_HW_PIN_UNDEF) {
				for (j = 0; j < IMGSENSOR_HW_ID_MAX_NUM && ppwr_info->id != phw->pdev[j]->id; j++) {
				}
				psensor_pwr->id[ppwr_info->pin] = j;
			}
			ppwr_info++;
		}
	}

	for (i = 0; i < IMGSENSOR_SENSOR_IDX_MAX_NUM; i++) {
		memset(str_prop_name, 0, sizeof(str_prop_name));
		snprintf(str_prop_name,
			sizeof(str_prop_name),
			"cam%d_%s",
			i,
			"enable_sensor");
		if (of_property_read_string(
			of_node, str_prop_name,
			&phw->enable_sensor_by_index[i]) < 0) {
			PK_INFO("Property cust-sensor not defined\n");
			phw->enable_sensor_by_index[i] = NULL;
		}
	}

#if defined(CONFIG_CAMERA_AAX_V16)
	uw_cam_status = false;
#endif

	return IMGSENSOR_RETURN_SUCCESS;
}

enum IMGSENSOR_RETURN imgsensor_hw_release_all(struct IMGSENSOR_HW *phw)
{
	int i;

	for (i = 0; i < IMGSENSOR_HW_ID_MAX_NUM; i++) {
		if (phw->pdev[i]->release != NULL)
			(phw->pdev[i]->release)(phw->pdev[i]->pinstance);
	}

#if defined(CONFIG_CAMERA_AAX_V16)
	uw_cam_status = false;
#endif

	return IMGSENSOR_RETURN_SUCCESS;
}

static enum IMGSENSOR_RETURN imgsensor_hw_power_sequence(
		struct IMGSENSOR_HW             *phw,
		enum   IMGSENSOR_SENSOR_IDX      sensor_idx,
		enum   IMGSENSOR_HW_POWER_STATUS pwr_status,
		struct IMGSENSOR_HW_POWER_SEQ   *ppower_sequence,
		char *pcurr_idx)
{
	struct IMGSENSOR_HW_SENSOR_POWER *psensor_pwr =
					&phw->sensor_pwr[sensor_idx];
	struct IMGSENSOR_HW_POWER_SEQ    *ppwr_seq = ppower_sequence;
	struct IMGSENSOR_HW_POWER_INFO   *ppwr_info;
	struct IMGSENSOR_HW_DEVICE       *pdev;
	int                               pin_cnt = 0;

	static DEFINE_RATELIMIT_STATE(ratelimit, 1 * HZ, 30);

#ifdef CONFIG_FPGA_EARLY_PORTING  /*for FPGA*/
	if (1) {
		PK_INFO("FPGA return true for power control\n");
		return IMGSENSOR_RETURN_SUCCESS;
	}
#endif

	while (ppwr_seq < ppower_sequence + IMGSENSOR_HW_SENSOR_MAX_NUM &&
		ppwr_seq->name != NULL) {
		if (!strcmp(ppwr_seq->name, PLATFORM_POWER_SEQ_NAME)) {
			if (sensor_idx == ppwr_seq->_idx)
				break;
		} else {
			if (!strcmp(ppwr_seq->name, pcurr_idx))
				break;
		}
		ppwr_seq++;
	}

	if (ppwr_seq->name == NULL)
		return IMGSENSOR_RETURN_ERROR;

	ppwr_info = ppwr_seq->pwr_info;

#if defined(CONFIG_CAMERA_AAX_V16)
		if (sensor_idx == IMGSENSOR_SENSOR_IDX_MAIN2 && pwr_status == IMGSENSOR_HW_POWER_STATUS_ON) {
			PK_INFO("uw_cam_status: ON\n");
			uw_cam_status = true;
		}
#endif

	while (ppwr_info->pin != IMGSENSOR_HW_PIN_NONE &&
	       ppwr_info->pin < IMGSENSOR_HW_PIN_MAX_NUM &&
	       ppwr_info < ppwr_seq->pwr_info + IMGSENSOR_HW_POWER_INFO_MAX) {

		if (pwr_status == IMGSENSOR_HW_POWER_STATUS_ON) {
			if (ppwr_info->pin != IMGSENSOR_HW_PIN_UNDEF) {
				if (psensor_pwr->id[ppwr_info->pin] != IMGSENSOR_HW_ID_MAX_NUM) {
					pdev = phw->pdev[psensor_pwr->id[ppwr_info->pin]];

#if defined(CONFIG_CAMERA_AAX_V16)
					if (sensor_idx != IMGSENSOR_SENSOR_IDX_MAIN2 && ppwr_info->pin == IMGSENSOR_HW_PIN_RST2 && uw_cam_status) {
						PK_INFO("[Power on] sensor_idx = %d, reset2->[skip], pin_state = %d, delay = 0 ms\n",
							sensor_idx, ppwr_info->pin_state_on);
						ppwr_info++;
						pin_cnt++;
						continue;
					}
#endif

					if (__ratelimit(&ratelimit))
						PK_INFO("[Power on] sensor_idx = %d, pin = %d, pin_state_on = %d, delay after setting = %u ms",
						sensor_idx, ppwr_info->pin, ppwr_info->pin_state_on, ppwr_info->pin_on_delay);

					if (pdev->set != NULL)
						pdev->set(
							pdev->pinstance,
							sensor_idx,
							ppwr_info->pin,
							ppwr_info->pin_state_on);

					if (ppwr_info->pin_on_delay)
						mDELAY(ppwr_info->pin_on_delay);
				}
			}
		}

		ppwr_info++;
		pin_cnt++;
	}

	if (pwr_status == IMGSENSOR_HW_POWER_STATUS_OFF) {
		while (pin_cnt) {
			ppwr_info--;
			pin_cnt--;

			if (ppwr_info->pin != IMGSENSOR_HW_PIN_UNDEF) {
				if (psensor_pwr->id[ppwr_info->pin] != IMGSENSOR_HW_ID_MAX_NUM) {
					pdev = phw->pdev[psensor_pwr->id[ppwr_info->pin]];

					if (ppwr_info->pin_on_delay)
						mDELAY(ppwr_info->pin_on_delay);

#if defined(CONFIG_CAMERA_AAX_V16)
					if (sensor_idx != IMGSENSOR_SENSOR_IDX_MAIN2 && ppwr_info->pin == IMGSENSOR_HW_PIN_RST2) {
						PK_INFO("[Power off]  sensor_idx = %d, reset2->[skip], pin_state = %d, delay = %u ms\n",
							sensor_idx, ppwr_info->pin_state_off, ppwr_info->pin_on_delay);
						continue;
					}
#endif

					if (__ratelimit(&ratelimit))
						PK_INFO("[Power off] sensor_idx = %d, pin = %d, pin_state_off = %d, delay before setting = %u ms",
							sensor_idx,	ppwr_info->pin,	ppwr_info->pin_state_off, ppwr_info->pin_on_delay);

					if (pdev->set != NULL)
						pdev->set(
							pdev->pinstance,
							sensor_idx,
							ppwr_info->pin,
							ppwr_info->pin_state_off);
				}
			}
		}
	}

#if defined(CONFIG_CAMERA_AAX_V16)
	if (sensor_idx == IMGSENSOR_SENSOR_IDX_MAIN2 && pwr_status == IMGSENSOR_HW_POWER_STATUS_OFF) {
		PK_INFO("uw_cam_status: OFF\n");
		uw_cam_status = false;
	}
#endif

	return IMGSENSOR_RETURN_SUCCESS;
}

enum IMGSENSOR_RETURN imgsensor_hw_power(
		struct IMGSENSOR_HW *phw,
		struct IMGSENSOR_SENSOR *psensor,
		enum IMGSENSOR_HW_POWER_STATUS pwr_status)
{
	int ret = 0;
	enum IMGSENSOR_SENSOR_IDX sensor_idx = psensor->inst.sensor_idx;
	char *curr_sensor_name = psensor->inst.psensor_list->name;
	char str_index[LENGTH_FOR_SNPRINTF];

	PK_INFO("[Power %s] sensor_idx = %d, curr_sensor_name = %s, enable list = %s",
		pwr_status ? "on" : "off",
		sensor_idx,
		curr_sensor_name,
		phw->enable_sensor_by_index[(uint32_t)sensor_idx] == NULL
		? "NULL"
		: phw->enable_sensor_by_index[(uint32_t)sensor_idx]);

	if (phw->enable_sensor_by_index[(uint32_t)sensor_idx] &&
	!strstr(phw->enable_sensor_by_index[(uint32_t)sensor_idx], curr_sensor_name))
		return IMGSENSOR_RETURN_ERROR;

	ret = snprintf(str_index, sizeof(str_index), "%d", sensor_idx);
	if (ret < 0) {
		PK_PR_ERR("Error! snprintf allocate 0");
		ret = IMGSENSOR_RETURN_ERROR;
		return ret;
	}

	imgsensor_hw_power_sequence(
			phw,
			sensor_idx,
			pwr_status, platform_power_sequence, str_index);

	imgsensor_hw_power_sequence(
			phw,
			sensor_idx,
			pwr_status, sensor_power_sequence, curr_sensor_name);

	return IMGSENSOR_RETURN_SUCCESS;
}

enum IMGSENSOR_RETURN imgsensor_hw_dump(struct IMGSENSOR_HW *phw)
{
	int i;

	for (i = 0; i < IMGSENSOR_HW_ID_MAX_NUM; i++) {
		if (phw->pdev[i]->dump != NULL)
			(phw->pdev[i]->dump)(phw->pdev[i]->pinstance);
	}
	return IMGSENSOR_RETURN_SUCCESS;
}

