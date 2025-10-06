// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "kd_imgsensor.h"

#include "mclk/mclk.h"
#include "regulator/regulator.h"
#include "gpio/gpio.h"

#include "imgsensor_hw.h"
#include "imgsensor_cfg_table.h"
enum IMGSENSOR_RETURN (*hw_open[IMGSENSOR_HW_ID_MAX_NUM])
	(struct IMGSENSOR_HW_DEVICE **) = {
	imgsensor_hw_mclk_open,
	imgsensor_hw_regulator_open,
	imgsensor_hw_gpio_open
};

struct IMGSENSOR_HW_CFG imgsensor_custom_config[] = {
	{
		IMGSENSOR_SENSOR_IDX_MAIN,
		IMGSENSOR_I2C_DEV_0,
		{
			{IMGSENSOR_HW_PIN_MCLK,  IMGSENSOR_HW_ID_MCLK},
			{IMGSENSOR_HW_PIN_AVDD,  IMGSENSOR_HW_ID_REGULATOR},
			{IMGSENSOR_HW_PIN_DOVDD, IMGSENSOR_HW_ID_REGULATOR},
			{IMGSENSOR_HW_PIN_DVDD,  IMGSENSOR_HW_ID_REGULATOR},
			{IMGSENSOR_HW_PIN_AFVDD, IMGSENSOR_HW_ID_REGULATOR},
			{IMGSENSOR_HW_PIN_RST,   IMGSENSOR_HW_ID_GPIO},
			{IMGSENSOR_HW_PIN_NONE,  IMGSENSOR_HW_ID_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_SUB,
		IMGSENSOR_I2C_DEV_1,
		{
			{IMGSENSOR_HW_PIN_MCLK,  IMGSENSOR_HW_ID_MCLK},
			{IMGSENSOR_HW_PIN_AVDD,  IMGSENSOR_HW_ID_REGULATOR},
			{IMGSENSOR_HW_PIN_DOVDD, IMGSENSOR_HW_ID_REGULATOR},
			{IMGSENSOR_HW_PIN_DVDD,  IMGSENSOR_HW_ID_REGULATOR},
			{IMGSENSOR_HW_PIN_DVDD1,  IMGSENSOR_HW_ID_REGULATOR},
			{IMGSENSOR_HW_PIN_RST,   IMGSENSOR_HW_ID_GPIO},
			{IMGSENSOR_HW_PIN_NONE, IMGSENSOR_HW_ID_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_MAIN2,
		IMGSENSOR_I2C_DEV_2,
		{
			{IMGSENSOR_HW_PIN_MCLK,  IMGSENSOR_HW_ID_MCLK},
			{IMGSENSOR_HW_PIN_AVDD,  IMGSENSOR_HW_ID_REGULATOR},
			{IMGSENSOR_HW_PIN_DOVDD, IMGSENSOR_HW_ID_REGULATOR},
			{IMGSENSOR_HW_PIN_DVDD,  IMGSENSOR_HW_ID_REGULATOR},
			{IMGSENSOR_HW_PIN_RST,	 IMGSENSOR_HW_ID_GPIO},
			{IMGSENSOR_HW_PIN_NONE,  IMGSENSOR_HW_ID_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_SUB2,
		IMGSENSOR_I2C_DEV_2,
		{
			{IMGSENSOR_HW_PIN_MCLK,  IMGSENSOR_HW_ID_MCLK},
			{IMGSENSOR_HW_PIN_AVDD,  IMGSENSOR_HW_ID_REGULATOR},
			{IMGSENSOR_HW_PIN_DOVDD, IMGSENSOR_HW_ID_REGULATOR},
			{IMGSENSOR_HW_PIN_DVDD1,  IMGSENSOR_HW_ID_REGULATOR},
			{IMGSENSOR_HW_PIN_RST,   IMGSENSOR_HW_ID_GPIO},
			{IMGSENSOR_HW_PIN_NONE,  IMGSENSOR_HW_ID_NONE},
		},
	},

	{IMGSENSOR_SENSOR_IDX_NONE}
};

struct IMGSENSOR_HW_POWER_SEQ platform_power_sequence[] = {
	{NULL}
};

struct IMGSENSOR_HW_POWER_SEQ sensor_power_sequence[] = {
#if defined(HI5022Q_MIPI_RAW)
	{
		SENSOR_DRVNAME_HI5022Q_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1100, 0},
			{AFVDD, Vol_2800, 0},
			{AVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(S5KJN1_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5KJN1_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1050, 1},
			{AFVDD, Vol_2800, 1},
			{AVDD, Vol_2800, 1},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(HI1339_MIPI_RAW)
	{
		SENSOR_DRVNAME_HI1339_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1100, 0},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 2},
		},
	},
#endif
#if defined(GC13A0_MIPI_RAW)
	{
		SENSOR_DRVNAME_GC13A0_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{DVDD1, Vol_1200, 0},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 2},
		},
	},
#endif
#if defined(SC501CS_MIPI_RAW)
	{
		SENSOR_DRVNAME_SC501CS_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 5},
		},
	},
#endif
#if defined(GC02M2_MIPI_RAW)
	{
		SENSOR_DRVNAME_GC02M2_MIPI_RAW,
		{
			{RST, Vol_Low, 2},
			{DOVDD, Vol_1800, 1},
			{DVDD1, Vol_1200, 0},
			{AVDD, Vol_2800, 1},
			{SensorMCLK, Vol_High, 2},
			{RST, Vol_High, 2},
		},
	},
#endif
};
