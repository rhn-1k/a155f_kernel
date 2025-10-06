/*
 * linux/drivers/video/fbdev/exynos/panel/s6e8fc3/s6e8fc3_dimming.h
 *
 * Header file for S6E8FC3 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E8FC3_DIMMING_H__
#define __S6E8FC3_DIMMING_H__
#include <linux/types.h>
#include <linux/kernel.h>
#include "../dimming.h"
#include "s6e8fc3.h"

#define S6E8FC3_A15_NR_LUMINANCE (256)
#define S6E8FC3_A15_NR_EXTEND_BRIGHTNESS ((483) + 1)
#define S6E8FC3_A15_NR_HBM_LUMINANCE (S6E8FC3_A15_NR_EXTEND_BRIGHTNESS - S6E8FC3_A15_NR_LUMINANCE)

#define S6E8FC3_A15_TOTAL_NR_LUMINANCE (S6E8FC3_A15_NR_LUMINANCE + S6E8FC3_A15_NR_HBM_LUMINANCE)

#define S6E8FC3_A15X_NR_LUMINANCE (256)
#define S6E8FC3_A15X_NR_EXTEND_BRIGHTNESS ((483) + 1)
#define S6E8FC3_A15X_NR_HBM_LUMINANCE (S6E8FC3_A15X_NR_EXTEND_BRIGHTNESS - S6E8FC3_A15X_NR_LUMINANCE)

#define S6E8FC3_A15X_TOTAL_NR_LUMINANCE (S6E8FC3_A15X_NR_LUMINANCE + S6E8FC3_A15X_NR_HBM_LUMINANCE)

#endif /* __S6E8FC3_DIMMING_H__ */
