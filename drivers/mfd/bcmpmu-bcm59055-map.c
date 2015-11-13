/*****************************************************************************
*  Copyright 2001 - 2008 Broadcom Corporation.  All rights reserved.
*
*  Unless you and Broadcom execute a separate written software license
*  agreement governing use of this software, this software is licensed to you
*  under the terms of the GNU General Public License version 2, available at
*  http://www.gnu.org/licenses/old-license/gpl-2.0.html (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a
*  license other than the GPL, without Broadcom's express prior written
*  consent.
*
*****************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/bug.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#include <linux/mfd/bcmpmu.h>

#define PMU_IRG_REG_MAX		14
#define PMU_ENV_REG_MAX		9
#define PMU_USB_ID_LVL_MAX	8
#define PMU_MIN_CHRG_CURR	85

static const struct bcmpmu_reg_map bcm59055_reg_map[PMU_REG_MAX] = {	/* revisit */
	[PMU_REG_SMPLCTRL] = {.map = 0x00, .addr = 0x06, .mask = 0xFF, .ro = 0},
	[PMU_REG_WRLOCKKEY] = {.map = 0x00, .addr = 0x07, .mask = 0xFF, .ro = 0},
	[PMU_REG_WRPROEN] = {.map = 0x00, .addr = 0x08, .mask = 0xFF, .ro = 0},
	[PMU_REG_PMUGID] = {.map = 0x00, .addr = 0x04, .mask = 0xFF, .ro = 0},
	[PMU_REG_PONKEYCTRL1] = {.map = 0x00, .addr = 0x0C, .mask = 0xFF, .ro = 0},
	[PMU_REG_PONKEYCTRL2] = {.map = 0x00, .addr = 0x0D, .mask = 0xFF, .ro = 0},
	[PMU_REG_PONKEYCTRL3] = {.map = 0x00, .addr = 0x0F, .mask = 0xFF, .ro = 0},
	[PMU_REG_AUXCTRL] = {.map = 0x00, .addr = 0x0E, .mask = 0xFF, .ro = 0},
	[PMU_REG_RTCSC] = {.map = 0x00, .addr = 0x20, .mask = 0x3F, .ro = 0},
	[PMU_REG_RTCMN] = {.map = 0x00, .addr = 0x21, .mask = 0x3F, .ro = 0},
	[PMU_REG_RTCHR] = {.map = 0x00, .addr = 0x22, .mask = 0x1F, .ro = 0},
	[PMU_REG_RTCDT] = {.map = 0x00, .addr = 0x24, .mask = 0x1F, .ro = 0},
	[PMU_REG_RTCMT] = {.map = 0x00, .addr = 0x25, .mask = 0x0F, .ro = 0},
	[PMU_REG_RTCYR] = {.map = 0x00, .addr = 0x26, .mask = 0xFF, .ro = 0},
	[PMU_REG_RTCSC_ALM] = {.map = 0x00, .addr = 0x27, .mask = 0x3F, .ro = 0},
	[PMU_REG_RTCMN_ALM] = {.map = 0x00, .addr = 0x28, .mask = 0x3F, .ro = 0},
	[PMU_REG_RTCHR_ALM] = {.map = 0x00, .addr = 0x29, .mask = 0x1F, .ro = 0},
	[PMU_REG_RTCDT_ALM] = {.map = 0x00, .addr = 0x2B, .mask = 0x1F, .ro = 0},
	[PMU_REG_RTCMT_ALM] = {.map = 0x00, .addr = 0x2C, .mask = 0x0F, .ro = 0},
	[PMU_REG_RTCYR_ALM] = {.map = 0x00, .addr = 0x2D, .mask = 0xFF, .ro = 0},
	[PMU_REG_RTC_CORE] = {.map = 0x00, .addr = 0x2E, .mask = 0xFF, .ro = 0},
	[PMU_REG_RFOPMODCTRL] = {.map = 0x00, .addr = 0xA0, .mask = 0xFF, .ro = 0},
	[PMU_REG_CAMOPMODCTRL] = {.map = 0x00, .addr = 0xA1, .mask = 0xFF, .ro =
				  0},
	[PMU_REG_HV1OPMODCTRL] = {.map = 0x00, .addr = 0xA2, .mask = 0xFF, .ro =
				  0},
	[PMU_REG_HV2OPMODCTRL] = {.map = 0x00, .addr = 0xA3, .mask = 0xFF, .ro =
				  0},
	[PMU_REG_HV3OPMODCTRL] = {.map = 0x00, .addr = 0xA4, .mask = 0xFF, .ro =
				  0},
	[PMU_REG_HV4OPMODCTRL] = {.map = 0x00, .addr = 0xA5, .mask = 0xFF, .ro =
				  0},
	[PMU_REG_HV5OPMODCTRL] = {.map = 0x00, .addr = 0xA6, .mask = 0xFF, .ro =
				  0},
	[PMU_REG_HV6OPMODCTRL] = {.map = 0x00, .addr = 0xA7, .mask = 0xFF, .ro =
				  0},
	[PMU_REG_HV7OPMODCTRL] = {.map = 0x00, .addr = 0xA8, .mask = 0xFF, .ro =
				  0},
	[PMU_REG_SIMOPMODCTRL] = {.map = 0x00, .addr = 0xA9, .mask = 0xFF, .ro =
				  0},
	[PMU_REG_CSROPMODCTRL] = {.map = 0x00, .addr = 0xAA, .mask = 0xFF, .ro =
				  0},
	[PMU_REG_IOSROPMODCTRL] = {.map = 0x00, .addr = 0xAB, .mask = 0xFF, .ro =
				   0},
	[PMU_REG_SDSROPMODCTRL] = {.map = 0x00, .addr = 0xAC, .mask = 0xFF, .ro =
				   0},
	[PMU_REG_RFLDOCTRL] = {.map = 0x00, .addr = 0xB0, .mask = 0x07, .ro = 0},
	[PMU_REG_CAMLDOCTRL] = {.map = 0x00, .addr = 0xB1, .mask = 0x07, .ro = 0},
	[PMU_REG_HVLDO1CTRL] = {.map = 0x00, .addr = 0xB2, .mask = 0x07, .ro = 0},
	[PMU_REG_HVLDO2CTRL] = {.map = 0x00, .addr = 0xB3, .mask = 0x07, .ro = 0},
	[PMU_REG_HVLDO3CTRL] = {.map = 0x00, .addr = 0xB4, .mask = 0x07, .ro = 0},
	[PMU_REG_HVLDO4CTRL] = {.map = 0x00, .addr = 0xB5, .mask = 0x07, .ro = 0},
	[PMU_REG_HVLDO5CTRL] = {.map = 0x00, .addr = 0xB6, .mask = 0x07, .ro = 0},
	[PMU_REG_HVLDO6CTRL] = {.map = 0x00, .addr = 0xB7, .mask = 0x07, .ro = 0},
	[PMU_REG_HVLDO7CTRL] = {.map = 0x00, .addr = 0xB8, .mask = 0x07, .ro = 0},
	[PMU_REG_SIMLDOCTRL] = {.map = 0x00, .addr = 0xB9, .mask = 0x07, .ro = 0},
	[PMU_REG_PWR_GRP_DLY] = {.map = 0x00, .addr = 0xBB, .mask = 0x03, .ro = 0},
	[PMU_REG_CSRCTRL1] = {.map = 0x00, .addr = 0xC0, .mask = 0x1F, .ro = 0},
	[PMU_REG_CSRCTRL2] = {.map = 0x00, .addr = 0xC1, .mask = 0x1F, .ro = 0},
	[PMU_REG_CSRCTRL3] = {.map = 0x00, .addr = 0xC2, .mask = 0x1F, .ro = 0},
	[PMU_REG_CSRCTRL4] = {.map = 0x00, .addr = 0xC3, .mask = 0xFF, .ro = 0},
	[PMU_REG_CSRCTRL5] = {.map = 0x00, .addr = 0xC4, .mask = 0xFF, .ro = 0},
	[PMU_REG_CSRCTRL6] = {.map = 0x00, .addr = 0xC5, .mask = 0xFF, .ro = 0},
	[PMU_REG_CSRCTRL7] = {.map = 0x00, .addr = 0xC6, .mask = 0xFF, .ro = 0},
	[PMU_REG_CSRCTRL8] = {.map = 0x00, .addr = 0xC7, .mask = 0xFF, .ro = 0},
	[PMU_REG_IOSRCTRL1] = {.map = 0x00, .addr = 0xC8, .mask = 0x1F, .ro = 0},
	[PMU_REG_IOSRCTRL2] = {.map = 0x00, .addr = 0xC9, .mask = 0x1F, .ro = 0},
	[PMU_REG_IOSRCTRL3] = {.map = 0x00, .addr = 0xCA, .mask = 0x1F, .ro = 0},
	[PMU_REG_IOSRCTRL4] = {.map = 0x00, .addr = 0xCB, .mask = 0xFF, .ro = 0},
	[PMU_REG_IOSRCTRL5] = {.map = 0x00, .addr = 0xCC, .mask = 0xFF, .ro = 0},
	[PMU_REG_IOSRCTRL6] = {.map = 0x00, .addr = 0xCD, .mask = 0xFF, .ro = 0},
	[PMU_REG_IOSRCTRL7] = {.map = 0x00, .addr = 0xCE, .mask = 0xFF, .ro = 0},
	[PMU_REG_IOSRCTRL8] = {.map = 0x00, .addr = 0xCF, .mask = 0xFF, .ro = 0},
	[PMU_REG_SDSRCTRL1] = {.map = 0x00, .addr = 0xD0, .mask = 0x1F, .ro = 0},
	[PMU_REG_SDSRCTRL2] = {.map = 0x00, .addr = 0xD1, .mask = 0x1F, .ro = 0},
	[PMU_REG_SDSRCTRL3] = {.map = 0x00, .addr = 0xD2, .mask = 0x1F, .ro = 0},
	[PMU_REG_SDSRCTRL4] = {.map = 0x00, .addr = 0xD3, .mask = 0xFF, .ro = 0},
	[PMU_REG_SDSRCTRL5] = {.map = 0x00, .addr = 0xD4, .mask = 0xFF, .ro = 0},
	[PMU_REG_SDSRCTRL6] = {.map = 0x00, .addr = 0xD5, .mask = 0xFF, .ro = 0},
	[PMU_REG_SDSRCTRL7] = {.map = 0x00, .addr = 0xD6, .mask = 0xFF, .ro = 0},
	[PMU_REG_SDSRCTRL8] = {.map = 0x00, .addr = 0xD7, .mask = 0xFF, .ro = 0},
	[PMU_REG_ENV1] = {.map = 0x00, .addr = 0xE0, .mask = 0xFF, .ro = 1},
	[PMU_REG_ENV2] = {.map = 0x00, .addr = 0xE1, .mask = 0xFF, .ro = 1},
	[PMU_REG_ENV3] = {.map = 0x00, .addr = 0xE2, .mask = 0xFF, .ro = 1},
	[PMU_REG_ENV4] = {.map = 0x00, .addr = 0xE3, .mask = 0xFF, .ro = 1},
	[PMU_REG_ENV5] = {.map = 0x00, .addr = 0xE4, .mask = 0xFF, .ro = 1},
	[PMU_REG_ENV6] = {.map = 0x00, .addr = 0xE5, .mask = 0xFF, .ro = 1},
	[PMU_REG_ENV7] = {.map = 0x00, .addr = 0xE6, .mask = 0xFF, .ro = 1},
	[PMU_REG_ENV8] = {.map = 0x00, .addr = 0xE7, .mask = 0xFF, .ro = 1},
	[PMU_REG_ENV9] = {.map = 0x00, .addr = 0xE8, .mask = 0xFF, .ro = 1},
	[PMU_REG_IHFTOP_IHF_IDDQ] = {.map = 0x01, .addr = 0x80, .mask = 0x01,
		.ro = 0},
	[PMU_REG_IHFTOP_BYPASS] = {.map = 0x01, .addr = 0x80, .mask = 0x40,
		.ro = 0, .shift = 6},
	[PMU_REG_IHFLDO_PUP] = {.map = 0x01, .addr = 0x82, .mask = 0x01,
		.ro = 0},
	[PMU_REG_IHFPOP_PUP] = {.map = 0x01, .addr = 0x83, .mask = 0x08,
		.ro = 0, .shift = 3},
	[PMU_REG_IHFAUTO_SEQ] = {.map = 0x01, .addr = 0x83, .mask = 0x40,
		.ro = 0, .shift = 6},
	[PMU_REG_IHFPGA2_GAIN] = {.map = 0x01, .addr = 0x8A, .mask = 0x3F, .ro =
		0, .shift = 0},
	[PMU_REG_HIGH_GAIN_MODE] = {.map = 0x01, .addr = 0x89, .mask = 0x08,
		.ro = 0, .shift = 3},
	[PMU_REG_IHF_SC_EDISABLE] = {.map = 0x01, .addr = 0x87, .mask = 0x04,
		.ro = 0},
	[PMU_REG_HS_SC_EDISABLE] = {.map = 0x01, .addr = 0x87, .mask = 0x02,
		.ro = 0},
	[PMU_REG_HSPUP_IDDQ_PWRDWN] = {.map = 0x01, .addr = 0xA2, .mask = 0x10,
		.ro = 0, .shift = 4},
	[PMU_REG_HSPUP_HS_PWRUP] = {.map = 0x01, .addr = 0xA3, .mask = 0x40,
		.ro = 0, .shift = 6},
	[PMU_REG_HSPGA1] = {.map = 0x01, .addr = 0x9E, .mask = 0xFF, .ro = 0},
	[PMU_REG_HSPGA2] = {.map = 0x01, .addr = 0x9F, .mask = 0xFF, .ro = 0},
	[PMU_REG_HSPGA1_LGAIN] = {
		.map = 0x01, .addr = 0x9E, .mask = 0x3F, .ro = 0},
	[PMU_REG_HSPGA2_RGAIN] = {
		.map = 0x01, .addr = 0x9F, .mask = 0x3F, .ro = 0},
	[PMU_REG_HSPGA3] = {.map = 0x01, .addr = 0xA0, .mask = 0xFF, .ro = 0},
	[PMU_REG_HSDRV_DISSC] = {.map = 0x01, .addr = 0x9B, .mask = 0x10,
		.ro = 0, .shift = 4},

	[PMU_REG_IHFCLK_PUP] =	{.map = 0x01, .addr = 0x81, .mask = 0x08,
		.ro = 0, .shift = 3},
	[PMU_REG_IHFBIAS_EN] =	{.map = 0x01, .addr = 0x81, .mask = 0x04,
		.ro = 0, .shift = 2},
	[PMU_REG_IHFRCCAL_PUP] = {.map = 0x01, .addr = 0x84, .mask = 0x01,
		.ro = 0, .shift = 0},
	[PMU_REG_IHFRAMP_PUP] =	{.map = 0x01, .addr = 0x88, .mask = 0x01,
		.ro = 0, .shift = 0},
	[PMU_REG_IHFLF_PUP] =	{.map = 0x01, .addr = 0x8B, .mask = 0x01,
		.ro = 0, .shift = 0},
	[PMU_REG_IHFCMPPD_PUP] = {.map = 0x01, .addr = 0x8C, .mask = 0x01,
		.ro = 0, .shift = 0},
	[PMU_REG_IHFFB_PUP] =	{.map = 0x01, .addr = 0x8E, .mask = 0x01,
		.ro = 0, .shift = 0},
	[PMU_REG_IHFPWRDRV_PUP] = {.map = 0x01, .addr = 0x8D, .mask = 0x01,
		.ro = 0, .shift = 0},
	[PMU_REG_IHFPOP_EN] =	{.map = 0x01, .addr = 0x83, .mask = 0x04,
				 .ro = 0, .shift = 2},
	[PMU_REG_IHFPSRCAL_PUP] =	{.map = 0x01, .addr = 0x85, .mask = 0x01,
			 .ro = 0, .shift = 0},
	/* charge */
	[PMU_REG_CHRGR_USB_EN] = {.map = 0x00, .addr = 0x52, .mask = 0x02, .ro =
				  0, .shift = 1},
	[PMU_REG_CHRGR_WAC_EN] = {.map = 0x00, .addr = 0x52, .mask = 0x01, .ro =
				  0, .shift = 0},
	[PMU_REG_CHRGR_ICC_FC] = {.map = 0x00, .addr = 0x59, .mask = 0x07, .ro = 0, .shift = 0},	/* USB_FC_CC, MBCCTRL10 for 59055 */
	[PMU_REG_CHRGR_ICC_QC] = {.map = 0x00, .addr = 0x59, .mask = 0x07, .ro = 0, .shift = 0},	/* no ICC_QC for 59055 */
	[PMU_REG_CHRGR_VFLOAT] = {.map = 0x00, .addr = 0x56, .mask = 0x0F, .ro = 0, .shift = 0},	/* MBCCTRL7 for 59055 */
	[PMU_REG_CHRGR_EOC] = {.map = 0x00, .addr = 0x00, .mask = 0x00, .ro =
			       0, .shift = 0},
	[PMU_REG_CHP_TYP] = {.map = 0x00, .addr = 0x54, .mask = 0x30, .ro =
			     0, .shift = 4},
	/* fuel gauge */
	[PMU_REG_FG_ACCM0] = {.map = 0x01, .addr = 0xC9, .mask = 0xFF, .ro =
			      1, .shift = 0},
	[PMU_REG_FG_ACCM1] = {.map = 0x01, .addr = 0xC8, .mask = 0xFF, .ro =
			      1, .shift = 0},
	[PMU_REG_FG_ACCM2] = {.map = 0x01, .addr = 0xC7, .mask = 0xFF, .ro =
			      1, .shift = 0},
	[PMU_REG_FG_ACCM3] = {.map = 0x01, .addr = 0xC6, .mask = 0xFF, .ro =
			      1, .shift = 0},
	[PMU_REG_FG_CNT0] = {.map = 0x01, .addr = 0xCB, .mask = 0xFF, .ro =
			     1, .shift = 0},
	[PMU_REG_FG_CNT1] = {.map = 0x01, .addr = 0xCA, .mask = 0xFF, .ro =
			     1, .shift = 0},
	[PMU_REG_FG_SLEEPCNT0] = {.map = 0x01, .addr = 0xCD, .mask = 0xFF, .ro =
				  1, .shift = 0},
	[PMU_REG_FG_SLEEPCNT1] = {.map = 0x01, .addr = 0xCC, .mask = 0xFF, .ro =
				  1, .shift = 0},
	[PMU_REG_FG_HOSTEN] = {.map = 0x01, .addr = 0xC0, .mask = 0x01, .ro =
			       0, .shift = 0},
	[PMU_REG_FG_RESET] = {.map = 0x01, .addr = 0xC1, .mask = 0x10, .ro =
			      0, .shift = 4},
	[PMU_REG_FG_CAL] = {.map = 0x01, .addr = 0xC1, .mask = 0x02, .ro =
			    0, .shift = 1},
	[PMU_REG_FG_FRZREAD] = {.map = 0x01, .addr = 0xC1, .mask = 0x20, .ro =
				0, .shift = 5},
	[PMU_REG_FG_FRZSMPL] = {.map = 0x01, .addr = 0xC1, .mask = 0x40, .ro =
				0, .shift = 6},
	[PMU_REG_FG_OFFSET0] = {.map = 0x01, .addr = 0xD4, .mask = 0xFF, .ro =
				1, .shift = 6},
	[PMU_REG_FG_OFFSET1] = {.map = 0x01, .addr = 0xD5, .mask = 0xFF, .ro =
				1, .shift = 6},
	[PMU_REG_FG_GAINTRIM] = {.map = 0x01, .addr = 0xC5, .mask = 0xFF, .ro =
				 0, .shift = 0},
	[PMU_REG_FG_DELTA] = {.map = 0x01, .addr = 0xDA, .mask = 0xFF, .ro =
			      0, .shift = 0},
	[PMU_REG_FG_CAP] = {.map = 0x01, .addr = 0xD9, .mask = 0xFF, .ro =
			    0, .shift = 0},
	/* usb control */
	[PMU_REG_OTG_VBUS_PULSE] = {.map = 0, .addr = 0x70, .mask = 0x01, .ro =
				    0, .shift = 0},
	[PMU_REG_OTG_VBUS_BOOST] = {.map = 0, .addr = 0x70, .mask = 0x04, .ro =
				    0, .shift = 2},
	[PMU_REG_OTG_VBUS_DISCHRG] = {.map = 0, .addr = 0x70, .mask = 0x02, .ro =
				      0, .shift = 1},
	[PMU_REG_OTG_ENABLE] = {.map = 0, .addr = 0x70, .mask = 0x40, .ro =
				0, .shift = 6},
#ifdef PMU_BCM59055B0
	[PMU_REG_ADP_SENSE] = {.map = 0, .addr = 0x7A, .mask = 0x02, .ro =
			       0, .shift = 1},
	[PMU_REG_ADP_STATUS_RISE_TIMES_LSB] = {.map = 0x01, .addr = 0x71, .mask =
					       0xFF, .ro = 0, .shift = 0},
#else
	[PMU_REG_ADP_SENSE] = {.map = 0, .addr = 0x71, .mask = 0x10, .ro =
			       0, .shift = 4},
	[PMU_REG_ADP_STATUS_RISE_TIMES_LSB] = {.map = 0x01, .addr = 0x73, .mask =
					       0xFF, .ro = 0, .shift = 0},
#endif
	[PMU_REG_ADP_COMP_DB_TM] = {.map = 0, .addr = 0x71, .mask = 0x03, .ro =
				    0, .shift = 0},
	[PMU_REG_ADP_SNS_TM] =	{.map = 0, .addr = 0, .mask = 0,
		.ro = 0, .shift = 0},
	[PMU_REG_ADP_PRB] = {.map = 0, .addr = 0x7A, .mask = 0x01, .ro = 0, .shift =
			     0},
	[PMU_REG_ADP_CAL_PRB] = {.map = 0, .addr = 0x0, .mask = 0x0, .ro =
				 0, .shift = 0},
	[PMU_REG_ADP_PRB_MOD] = {.map = 0, .addr = 0x7A, .mask = 0x06, .ro =
				 0, .shift = 1},
	[PMU_REG_ADP_PRB_CYC_TIME] = {.map = 0, .addr = 0x71, .mask = 0x08, .ro =
				      0, .shift = 3},
	[PMU_REG_ADP_COMP_METHOD] = {.map = 0, .addr = 0x7B, .mask = 0x60, .ro =
				     0, .shift = 5},
	[PMU_REG_ADP_ENABLE] = {.map = 0, .addr = 0x7B, .mask = 0x08, .ro =
				0, .shift = 3},
	[PMU_REG_ADP_PRB_COMP] = {.map = 0, .addr = 0x7B, .mask = 0x01, .ro =
				  0, .shift = 0},
	[PMU_REG_ADP_PRB_REG_RST] = {.map = 0, .addr = 0x7B, .mask = 0x10, .ro =
				     0, .shift = 4},
	[PMU_REG_ADP_SNS_COMP] = {.map = 0, .addr = 0x7B, .mask = 0x02, .ro =
				  0, .shift = 1},
	[PMU_REG_ADP_SNS_AON] = {.map = 0, .addr = 0x7B, .mask = 0x04, .ro =
				 0, .shift = 2},
	[PMU_REG_OTGCTRL9] = {.map = 0, .addr = 0x78, .mask = 0xFF, .ro = 0},
	[PMU_REG_OTGCTRL10] = {.map = 0, .addr = 0x79, .mask = 0xFF, .ro = 0},
	/* usb status */
	[PMU_REG_USB_STATUS_ID_CODE] = {.map = 0, .addr = 0xE3, .mask = 0x38, .ro =
					0, .shift = 3},
	[PMU_REG_OTG_STATUS_VBUS] = {.map = 0, .addr = 0xE3, .mask = 0x01, .ro =
				     0, .shift = 0},
	[PMU_REG_OTG_STATUS_SESS] = {.map = 0, .addr = 0xE3, .mask = 0x02, .ro =
				     0, .shift = 1},
	[PMU_REG_OTG_STATUS_SESS_END] = {.map = 0, .addr = 0xE3, .mask =
					 0x04, .ro = 0, .shift = 2},
	[PMU_REG_ADP_STATUS_ATTACH_DET] = {.map = 0x01, .addr = 0x70, .mask =
					   0x04, .ro = 0, .shift = 3},
	[PMU_REG_ADP_STATUS_SNS_DET] = {.map = 0x01, .addr = 0x70, .mask =
					0x10, .ro = 0, .shift = 4},
	[PMU_REG_ADP_STATUS_RISE_TIMES_MSB] = {.map = 0x01, .addr = 0x74, .mask =
					       0x30, .ro = 0, .shift = 4},
	/* test */
	[PMU_REG_HSIST_I_HS_ENST] = {.map = 1, .addr = 0xA4, .mask = 0x03, .ro =
				     0, .shift = 0},
	[PMU_REG_HSIST_I_HS_IST] = {.map = 1, .addr = 0xA4, .mask = 0x04, .ro =
				    0, .shift = 2},
	[PMU_REG_IHFSTIN_I_IHFSELFTEST_EN] = {.map = 1, .addr = 0x8F, .mask =
					      0x03, .ro = 0, .shift = 0},
	[PMU_REG_IHFSTIN_I_IHFSTI] = {.map = 1, .addr = 0x8F, .mask = 0x0C, .ro =
				      0, .shift = 2},
	[PMU_REG_IHFSTIN_I_IHFSTO] = {.map = 1, .addr = 0x8F, .mask = 0x30, .ro =
				      0, .shift = 4},
	[PMU_REG_IHFSTO_O_IHFSTI] = {.map = 1, .addr = 0x90, .mask = 0x03, .ro =
				     0, .shift = 0},
	[PMU_REG_HSOUT1_O_HS_IST] = {.map = 1, .addr = 0xA9, .mask = 0xF0, .ro =
				     0, .shift = 4},

	/* interrupt */
	[PMU_REG_INT_START] = {.map = 0, .addr = 0x30, .mask = 0xFF, .ro =
			       0, .shift = 0},
	[PMU_REG_INT_MSK_START] = {.map = 0, .addr = 0x40, .mask = 0xFF, .ro =
				   0, .shift = 0},
	/* generic */
	[PMU_REG_SWUP] = {.map = 0x00, .addr = 0x58, .mask = 0x01, .ro = 0, .shift = 0},	/* MBCCTRL9 for 59055 */
	[PMU_REG_PMUID] = {.map = 0x01, .addr = 0xF7, .mask = 0xFF, .ro = 0},
	[PMU_REG_PMUREV] = {.map = 0x01, .addr = 0xF8, .mask = 0xFF, .ro = 0},
	[PMU_REG_PLLCTRL] = {.map = 0x00, .addr = 0x0b, .mask = 0xFF, .ro = 0},
	[PMU_REG_SW_SHDWN] = {.map = 0x00, .addr = 0x01, .mask = 0x04, .ro = 0},
	[PMU_REG_HOSTCTRL3] = {.map = 0x00, .addr = 0x03, .mask = 0xFF, .ro = 0},
	[PMU_REG_MBCCTRL5_USB_DET_LDO_EN] = {.map = 0x00, .addr = 0x54, .mask =
					     0x04, .ro = 0, .shift = 2},
	[PMU_REG_MBCCTRL5_CHARGE_DET] = {.map = 0x00, .addr = 0x54, .mask =
					 0x07, .ro = 0, .shift = 0},
	[PMU_REG_STATMUX] = {.map = 0x0, .addr = 0x01, .mask = 0x20,
		.ro = 0, .shift = 5},
	[PMU_REG_SIMOFF_EN] = {.map = 0x0, .addr = 0xB9, .mask = 0x40,
		.ro = 0, .shift = 6},

	/* PMU BUS */
	[PMU_REG_BUS_STATUS_WRITE_FIFO] = {.map = 0x00, .addr = 0x00, .mask =
					   0x2, .ro = 1, .shift = 1},
	[PMU_REG_BUS_STATUS_READ_FIFO] = {.map = 0x00, .addr = 0x00, .mask =
					  0x1, .ro = 1, .shift = 0},

	/* Watchdog */
	[PMU_REG_SYS_WDT_EN] = {.map = 0x00, .addr = 0x01, .mask = 0x01, .ro =
				0, .shift = 0},
	[PMU_REG_SYS_WDT_CLR] = {.map = 0x00, .addr = 0x01, .mask = 0x02, .ro =
				 0, .shift = 1},
	[PMU_REG_SYS_WDT_TIME] = {.map = 0x00, .addr = 0x02, .mask = 0x7F, .ro =
				  0, .shift = 0},

	/* BM access */
	[PMU_REG_MBCCTRL1]  = { .map = 0x00, .addr = 0x50, .mask = 0xff, },
	[PMU_REG_MBCCTRL2]  = { .map = 0x00, .addr = 0x51, .mask = 0xff, },
	[PMU_REG_MBCCTRL3]  = { .map = 0x00, .addr = 0x52, .mask = 0xff, },
	[PMU_REG_MBCCTRL4]  = { .map = 0x00, .addr = 0x53, .mask = 0xff, },
	[PMU_REG_MBCCTRL5]  = { .map = 0x00, .addr = 0x54, .mask = 0xff, },
	[PMU_REG_MBCCTRL6]  = { .map = 0x00, .addr = 0x55, .mask = 0xff, },
	[PMU_REG_MBCCTRL7]  = { .map = 0x00, .addr = 0x56, .mask = 0xff, },
	[PMU_REG_MBCCTRL8]  = { .map = 0x00, .addr = 0x57, .mask = 0xff, },
	[PMU_REG_MBCCTRL9]  = { .map = 0x00, .addr = 0x58, .mask = 0xff, },
	[PMU_REG_MBCCTRL10] = { .map = 0x00, .addr = 0x59, .mask = 0xff, },
	[PMU_REG_BBCCTRL]   = { .map = 0x00, .addr = 0x2f, .mask = 0xff, },

};

static const struct bcmpmu_irq_map bcm59055_irq_map[PMU_IRQ_MAX] = {	/* revisit */
	[PMU_IRQ_RTC_ALARM] = {.int_addr = 0x37, .mask_addr = 0x47, .bit_mask =
			       0x01},
	[PMU_IRQ_RTC_SEC] = {.int_addr = 0x37, .mask_addr = 0x47, .bit_mask =
			     0x02},
	[PMU_IRQ_RTC_MIN] = {.int_addr = 0x37, .mask_addr = 0x47, .bit_mask =
			     0x04},
	[PMU_IRQ_RTCADJ] = {.int_addr = 0x37, .mask_addr = 0x47, .bit_mask =
			    0x08},
	[PMU_IRQ_BATINS] = {.int_addr = 0x35, .mask_addr = 0x45, .bit_mask =
			    0x10},
	[PMU_IRQ_BATRM] = {.int_addr = 0x35, .mask_addr = 0x45, .bit_mask = 0x02},
	[PMU_IRQ_GBAT_PLUG_IN] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
				  0x00},
	[PMU_IRQ_SMPL_INT] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
			      0x00},
	[PMU_IRQ_USBINS] = {.int_addr = 0x31, .mask_addr = 0x41, .bit_mask =
			    0x10},
	[PMU_IRQ_USBRM] = {.int_addr = 0x31, .mask_addr = 0x41, .bit_mask = 0x20},
	[PMU_IRQ_USBOV] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask = 0x00},
	[PMU_IRQ_EOC] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask = 0x00},
	[PMU_IRQ_RESUME_VBUS] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
				 0x00},
	[PMU_IRQ_CHG_HW_TTR_EXP] = {.int_addr = 0x00, .mask_addr =
				    0x00, .bit_mask = 0x00},
	[PMU_IRQ_CHG_HW_TCH_EXP] = {.int_addr = 0x00, .mask_addr =
				    0x00, .bit_mask = 0x00},
	[PMU_IRQ_CHG_SW_TMR_EXP] = {.int_addr = 0x00, .mask_addr =
				    0x00, .bit_mask = 0x00},
	[PMU_IRQ_CHGDET_LATCH] = {.int_addr = 0x34, .mask_addr = 0x44, .bit_mask =
				  0x02},
	[PMU_IRQ_CHGDET_TO] = {.int_addr = 0x34, .mask_addr = 0x44, .bit_mask =
			       0x04},
	[PMU_IRQ_MBTEMPLOW] = {.int_addr = 0x35, .mask_addr = 0x45, .bit_mask =
			       0x04},
	[PMU_IRQ_MBTEMPHIGH] = {.int_addr = 0x35, .mask_addr = 0x45, .bit_mask =
				0x08},
	[PMU_IRQ_MBOV] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask = 0x00},
	[PMU_IRQ_MBOV_DIS] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
			      0x00},
	[PMU_IRQ_USBOV_DIS] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
			       0x00},
	[PMU_IRQ_CHGERRDIS] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
			       0x00},
	[PMU_IRQ_VBUS_1V5_R] = {.int_addr = 0x33, .mask_addr = 0x43, .bit_mask =
				0x20},
	[PMU_IRQ_VBUS_4V5_R] = {.int_addr = 0x33, .mask_addr = 0x43, .bit_mask =
				0x10},
	[PMU_IRQ_VBUS_1V5_F] = {.int_addr = 0x33, .mask_addr = 0x43, .bit_mask =
				0x02},
	[PMU_IRQ_VBUS_4V5_F] = {.int_addr = 0x33, .mask_addr = 0x43, .bit_mask =
				0x01},
	[PMU_IRQ_MBWV_R_10S_WAIT] = {.int_addr = 0x00, .mask_addr =
				     0x00, .bit_mask = 0x00},
	[PMU_IRQ_BBLOW] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask = 0x00},
	[PMU_IRQ_LOWBAT] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
			    0x00},
	[PMU_IRQ_VERYLOWBAT] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
				0x00},
	[PMU_IRQ_RTM_DATA_RDY] = {.int_addr = 0x38, .mask_addr = 0x48, .bit_mask =
				  0x01},
	[PMU_IRQ_RTM_IN_CON_MEAS] = {.int_addr = 0x38, .mask_addr =
				     0x48, .bit_mask = 0x02},
	[PMU_IRQ_RTM_UPPER] = {.int_addr = 0x38, .mask_addr = 0x48, .bit_mask =
			       0x04},
	[PMU_IRQ_RTM_IGNORE] = {.int_addr = 0x38, .mask_addr = 0x48, .bit_mask =
				0x08},
	[PMU_IRQ_RTM_OVERRIDDEN] = {.int_addr = 0x38, .mask_addr =
				    0x48, .bit_mask = 0x10},
	[PMU_IRQ_AUD_HSAB_SHCKT] = {.int_addr = 0x00, .mask_addr =
				    0x00, .bit_mask = 0x00},
	[PMU_IRQ_AUD_IHFD_SHCKT] = {.int_addr = 0x00, .mask_addr =
				    0x00, .bit_mask = 0x00},
	[PMU_IRQ_MBC_TF] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
			    0x00},
	[PMU_IRQ_CSROVRI] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
			     0x00},
	[PMU_IRQ_IOSROVRI] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
			      0x00},
	[PMU_IRQ_SDSROVRI] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
			      0x00},
	[PMU_IRQ_ASROVRI] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
			     0x00},
	[PMU_IRQ_UBPD_CHG_F] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
				0x00},
	[PMU_IRQ_ACD_INS] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
			     0x00},
	[PMU_IRQ_ACD_RM] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
			    0x00},
	[PMU_IRQ_PONKEYB_HOLD] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
				  0x00},
	[PMU_IRQ_PONKEYB_F] = {.int_addr = 0x30, .mask_addr = 0x40, .bit_mask =
			       0x01},
	[PMU_IRQ_PONKEYB_R] = {.int_addr = 0x30, .mask_addr = 0x40, .bit_mask =
			       0x02},
	[PMU_IRQ_PONKEYB_OFFHOLD] = {.int_addr = 0x00, .mask_addr =
				     0x00, .bit_mask = 0x00},
	[PMU_IRQ_PONKEYB_RESTART] = {.int_addr = 0x00, .mask_addr =
				     0x00, .bit_mask = 0x00},
	[PMU_IRQ_IDCHG] = {.int_addr = 0x34, .mask_addr = 0x44, .bit_mask = 0x01},
	[PMU_IRQ_JIG_USB_INS] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
				 0x00},
	[PMU_IRQ_JIG_UART_INS] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
			      0x00},
	[PMU_IRQ_ID_INS] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask =
			    0x00},
	[PMU_IRQ_ID_RM] = {.int_addr = 0x00, .mask_addr = 0x00, .bit_mask = 0x00},
	[PMU_IRQ_ADP_CHANGE] = {.int_addr = 0x34, .mask_addr = 0x44, .bit_mask =
				0x10},
	[PMU_IRQ_ADP_SNS_END] = {.int_addr = 0x34, .mask_addr = 0x44, .bit_mask =
				 0x20},
	[PMU_IRQ_SESSION_END_VLD] = {.int_addr = 0x33, .mask_addr =
				     0x43, .bit_mask = 0x04},
	[PMU_IRQ_SESSION_END_INVLD] = {.int_addr = 0x33, .mask_addr =
				       0x43, .bit_mask = 0x40},
	[PMU_IRQ_VBUS_OVERCURRENT] = {.int_addr = 0x37, .mask_addr =
				      0x47, .bit_mask = 0x80},
};

static const struct bcmpmu_env_info bcm59055_env_reg_map[PMU_ENV_MAX] = {	/* revisit */
	[PMU_ENV_MBWV_DELTA] = {.regmap = {
				.addr = 0xE0, .mask = 0x02, .shift = 1, .ro =
				 1}, .bitmask = PMU_ENV_BITMASK_MBWV_DELTA},
	[PMU_ENV_CGPD_ENV] = {.regmap = {
			      .addr = 0xE1, .mask = 0x01, .shift = 0, .ro =
			       1}, .bitmask = PMU_ENV_BITMASK_CGPD_ENV},
	[PMU_ENV_UBPD_ENV] = {.regmap = {
			      .addr = 0xE1, .mask = 0x02, .shift = 1, .ro =
			       1}, .bitmask = PMU_ENV_BITMASK_UBPD_ENV},
	[PMU_ENV_UBPD_USBDET] = {.regmap = {
				 .addr = 0xE1, .mask = 0x04, .shift = 2, .ro =
				  1}, .bitmask = PMU_ENV_BITMASK_UBPD_USBDET},
	[PMU_ENV_CGPD_PRI] = {.regmap = {
			      .addr = 0xE1, .mask = 0x10, .shift = 4, .ro =
			       1}, .bitmask = PMU_ENV_BITMASK_CGPD_PRI},
	[PMU_ENV_UBPD_PRI] = {.regmap = {
			      .addr = 0xE1, .mask = 0x20, .shift = 5, .ro =
			       1}, .bitmask = PMU_ENV_BITMASK_UBPD_PRI},
	[PMU_ENV_WAC_VALID] = {.regmap = {
			       .addr = 0xE1, .mask = 0x40, .shift = 6, .ro =
				1}, .bitmask = PMU_ENV_BITMASK_WAC_VALID},
	[PMU_ENV_USB_VALID] = {.regmap = {
			       .addr = 0xE1, .mask = 0x80, .shift = 7, .ro =
				1}, .bitmask = PMU_ENV_BITMASK_USB_VALID},
	[PMU_ENV_P_UBPD_INT] =	{.regmap = {
				.addr = 0xE1, .mask = 0x08, .shift = 3, .ro =
				 1}, .bitmask = PMU_ENV_BITMASK_P_UBPD_INT},
	[PMU_ENV_P_CGPD_CHG] = {.regmap = {
				.addr = 0x00, .mask = 0x00, .shift = 0, .ro =
				 1}, .bitmask = PMU_ENV_BITMASK_P_CGPD_CHG},
	[PMU_ENV_P_UBPD_CHR] = {.regmap = {
				.addr = 0xE1, .mask = 0x04, .shift = 2, .ro =
				 1}, .bitmask = PMU_ENV_BITMASK_P_UBPD_CHR},
	[PMU_ENV_PORT_DISABLE] = {.regmap = {
				  .addr = 0x00, .mask = 0x00, .shift = 0, .ro =
				   1}, .bitmask = PMU_ENV_BITMASK_PORT_DISABLE},
	[PMU_ENV_MBPD] = {.regmap = {
			  .addr = 0xE4, .mask = 0x01, .shift = 0, .ro =
			   1}, .bitmask = PMU_ENV_BITMASK_MBPD},
	[PMU_ENV_MBOV] = {.regmap = {
			  .addr = 0xE0, .mask = 0x10, .shift = 4, .ro =
			   1}, .bitmask = PMU_ENV_BITMASK_MBOV},
	[PMU_ENV_MBMC] = {.regmap = {
			  .addr = 0xE0, .mask = 0x08, .shift = 3, .ro =
			   1}, .bitmask = PMU_ENV_BITMASK_MBMC},
};

static const struct bcmpmu_adc_map bcm59055_adc_map[PMU_ADC_MAX] = {
	[PMU_ADC_VMBATT] = {.map = 0, .addr0 = 0x83, .addr1 = 0x82, .dmask =
			    0x3FF, .vmask = 0x0400, .rtmsel = 0x00, .vrng = 4800},
	[PMU_ADC_VBBATT] = {.map = 0, .addr0 = 0x85, .addr1 = 0x84, .dmask =
			    0x3FF, .vmask = 0x0400, .rtmsel = 0x01, .vrng = 4800},
	[PMU_ADC_VWALL] = {.map = 0, .addr0 = 0x87, .addr1 = 0x86, .dmask =
			   0x3FF, .vmask = 0x0400, .rtmsel = 0x02, .vrng = 14400},
	[PMU_ADC_VBUS] = {.map = 0, .addr0 = 0x89, .addr1 = 0x88, .dmask =
			  0x3FF, .vmask = 0x0400, .rtmsel = 0x03, .vrng = 14400},
	[PMU_ADC_ID] = {.map = 0, .addr0 = 0x8B, .addr1 = 0x8A, .dmask =
			0x3FF, .vmask = 0x0400, .rtmsel = 0x04, .vrng = 4800},
	[PMU_ADC_NTC] = {.map = 0, .addr0 = 0x8D, .addr1 = 0x8C, .dmask =
			 0x3FF, .vmask = 0x0400, .rtmsel = 0x05, .vrng = 1200},
	[PMU_ADC_BSI] = {.map = 0, .addr0 = 0x8F, .addr1 = 0x8E, .dmask =
			 0x3FF, .vmask = 0x0400, .rtmsel = 0x06, .vrng = 1200},
	[PMU_ADC_BOM] = {.map = 0, .addr0 = 0x91, .addr1 = 0x90, .dmask =
			 0x3FF, .vmask = 0x0400, .rtmsel = 0x07, .vrng = 1200},
	[PMU_ADC_32KTEMP] = {.map = 0, .addr0 = 0x93, .addr1 = 0x92, .dmask =
			     0x3FF, .vmask = 0x0400, .rtmsel = 0x08, .vrng = 1200},
	[PMU_ADC_PATEMP] = {.map = 0, .addr0 = 0x95, .addr1 = 0x94, .dmask =
			    0x3FF, .vmask = 0x0400, .rtmsel = 0x09, .vrng = 1200},
	[PMU_ADC_ALS] = {.map = 0, .addr0 = 0x97, .addr1 = 0x96, .dmask =
			 0x3FF, .vmask = 0x0400, .rtmsel = 0x0a, .vrng = 1200},
	[PMU_ADC_RTM] = {.map = 0, .addr0 = 0x99, .addr1 = 0x98, .dmask =
			 0x3FF, .vmask = 0x0000, .rtmsel = 0x00, .vrng = 0000},
	[PMU_ADC_FG_CURRSMPL] = {.map = 1, .addr0 = 0xD1, .addr1 = 0xD0, .dmask =
				 0xFFFF, .vmask = 0x0000, .rtmsel = 0x00, .vrng =
				 0000},
	[PMU_ADC_FG_RAW] = {.map = 1, .addr0 = 0xD3, .addr1 = 0xD2, .dmask =
			    0xFFFF, .vmask = 0x0000, .rtmsel = 0x00, .vrng = 0000},
	[PMU_ADC_FG_VMBATT] = {.map = 1, .addr0 = 0xD7, .addr1 = 0xD6, .dmask =
			       0x03FF, .vmask = 0x0400, .rtmsel = 0x00, .vrng =
			       4800},
	[PMU_ADC_BSI_CAL_LO] = {.map = 0, .addr0 = 0x99, .addr1 = 0x98, .dmask =
				0x3FF, .vmask = 0x0000, .rtmsel = 0x0b, .vrng =
				0000},
	[PMU_ADC_BSI_CAL_HI] = {.map = 0, .addr0 = 0x99, .addr1 = 0x98, .dmask =
				0x3FF, .vmask = 0x0000, .rtmsel = 0x0f, .vrng =
				0000},
	[PMU_ADC_NTC_CAL_LO] = {.map = 0, .addr0 = 0x99, .addr1 = 0x98, .dmask =
				0x3FF, .vmask = 0x0000, .rtmsel = 0x0c, .vrng =
				0000},
	[PMU_ADC_NTC_CAL_HI] = {.map = 0, .addr0 = 0x99, .addr1 = 0x98, .dmask =
				0x3FF, .vmask = 0x0000, .rtmsel = 0x0d, .vrng =
				0000},
};

/* all voltages are in uV */
static struct bcmpmu_adc_unit bcm59055_adc_unit[PMU_ADC_MAX] = {
	[PMU_ADC_VMBATT] = {.vstep = 4687, .voffset = 0, .rpullup = 0, .lut_ptr =
			    NULL, .lut_len = 0, .fg_k = 0, .vmax = 4800000},
	[PMU_ADC_VBBATT] = {.vstep = 4687, .voffset = 0, .rpullup = 0, .lut_ptr =
			    NULL, .lut_len = 0, .fg_k = 0, .vmax = 4800000},
	[PMU_ADC_VWALL] = {.vstep = 14063, .voffset = 0, .rpullup = 0, .lut_ptr =
			   NULL, .lut_len = 0, .fg_k = 0, .vmax = 14400000},
	[PMU_ADC_VBUS] = {.vstep = 14063, .voffset = 0, .rpullup = 0, .lut_ptr =
			  NULL, .lut_len = 0, .fg_k = 0, .vmax = 14400000},
	[PMU_ADC_ID] = {.vstep = 4687, .voffset = 0, .rpullup = 0, .lut_ptr =
			NULL, .lut_len = 0, .fg_k = 0, .vmax = 4800000},
	[PMU_ADC_NTC] = {.vstep = 1172, .voffset = 0, .rpullup = 162000, .lut_ptr =
			 NULL, .lut_len = 0, .fg_k = 0, .vmax = 1200000},
	[PMU_ADC_BSI] = {.vstep = 1172, .voffset = 0, .rpullup = 162000, .lut_ptr =
			 NULL, .lut_len = 0, .fg_k = 0, .vmax = 1800000},
	[PMU_ADC_BOM] = {.vstep = 1172, .voffset = 0, .rpullup = 162000, .lut_ptr =
			 NULL, .lut_len = 0, .fg_k = 0, .vmax = 1200000},
	[PMU_ADC_32KTEMP] = {.vstep = 1172, .voffset = 0, .rpullup =
			     162000, .lut_ptr = NULL, .lut_len = 0, .fg_k =
			     0, .vmax = 1200000},
	[PMU_ADC_PATEMP] = {.vstep = 1172, .voffset = 0, .rpullup =
			    162000, .lut_ptr = NULL, .lut_len = 0, .fg_k =
			    0, .vmax = 1200000},
	[PMU_ADC_ALS] = {.vstep = 1, .voffset = 0, .rpullup = 0, .lut_ptr =
			 NULL, .lut_len = 0, .fg_k = 0, .vmax = 1200000},
	[PMU_ADC_RTM] = {.vstep = 1, .voffset = 0, .rpullup = 0, .lut_ptr =
			 NULL, .lut_len = 0, .fg_k = 0, .vmax = 0000000},
	[PMU_ADC_FG_CURRSMPL] = {.vstep = 1, .voffset = 0, .rpullup = 0, .lut_ptr =
				 NULL, .lut_len = 0, .fg_k = 0, .vmax = 0000000},
	[PMU_ADC_FG_RAW] = {.vstep = 1, .voffset = 0, .rpullup = 0, .lut_ptr =
			    NULL, .lut_len = 0, .fg_k = 0, .vmax = 0000000},
	[PMU_ADC_FG_VMBATT] = {.vstep = 4687, .voffset = 0, .rpullup =
			       0, .lut_ptr = NULL, .lut_len = 0, .fg_k = 0, .vmax =
			       4800000},
	[PMU_ADC_BSI_CAL_LO] = {.vstep = 1, .voffset = 0, .rpullup = 0, .lut_ptr =
				NULL, .lut_len = 0, .fg_k = 0, .vmax = 1200000},
	[PMU_ADC_BSI_CAL_HI] = {.vstep = 1, .voffset = 0, .rpullup = 0, .lut_ptr =
				NULL, .lut_len = 0, .fg_k = 0, .vmax = 1200000},
	[PMU_ADC_NTC_CAL_LO] = {.vstep = 1, .voffset = 0, .rpullup = 0, .lut_ptr =
				NULL, .lut_len = 0, .fg_k = 0, .vmax = 1200000},
	[PMU_ADC_NTC_CAL_HI] = {.vstep = 1, .voffset = 0, .rpullup = 0, .lut_ptr =
				NULL, .lut_len = 0, .fg_k = 0, .vmax = 1200000},
};

static const struct bcmpmu_reg_map bcm59055_adc_ctrl_map[PMU_ADC_CTRL_MAX] = {	/* revisit */
	[PMU_ADC_RST_CNT] = {.map = 0, .addr = 0x80, .mask = 0x03, .shift = 0},
	[PMU_ADC_RTM_START] = {.map = 0, .addr = 0x80, .mask = 0x04, .shift = 2},
	[PMU_ADC_RTM_MASK] = {.map = 0, .addr = 0x80, .mask = 0x08, .shift = 3},
	[PMU_ADC_RTM_SEL] = {.map = 0, .addr = 0x80, .mask = 0xF0, .shift = 4},
	[PMU_ADC_RTM_DLY] = {.map = 0, .addr = 0x81, .mask = 0x1F, .shift = 0},
	[PMU_ADC_GSM_DBNC] = {.map = 0, .addr = 0x81, .mask = 0x20, .shift = 5},
};

static const struct bcmpmu_reg_map bcm59055_irq_reg_map[PMU_IRG_REG_MAX] = {
	[PMU_REG_INT1] = {.map = 0, .addr = 0x30, .mask = 0xFF, .ro = 0},
	[PMU_REG_INT2] = {.map = 0, .addr = 0x31, .mask = 0xFF, .ro = 0},
	[PMU_REG_INT3] = {.map = 0, .addr = 0x32, .mask = 0xFF, .ro = 0},
	[PMU_REG_INT4] = {.map = 0, .addr = 0x33, .mask = 0xFF, .ro = 0},
	[PMU_REG_INT5] = {.map = 0, .addr = 0x34, .mask = 0xFF, .ro = 0},
	[PMU_REG_INT6] = {.map = 0, .addr = 0x35, .mask = 0xFF, .ro = 0},
	[PMU_REG_INT7] = {.map = 0, .addr = 0x36, .mask = 0xFF, .ro = 0},
	[PMU_REG_INT8] = {.map = 0, .addr = 0x37, .mask = 0xFF, .ro = 0},
	[PMU_REG_INT9] = {.map = 0, .addr = 0x38, .mask = 0xFF, .ro = 0},
	[PMU_REG_INT10] = {.map = 0, .addr = 0x39, .mask = 0xFF, .ro = 0},
	[PMU_REG_INT11] = {.map = 0, .addr = 0x3a, .mask = 0xFF, .ro = 0},
	[PMU_REG_INT12] = {.map = 0, .addr = 0x3b, .mask = 0xFF, .ro = 0},
	[PMU_REG_INT13] = {.map = 0, .addr = 0x3c, .mask = 0xFF, .ro = 0},
	[PMU_REG_INT14] = {.map = 0, .addr = 0x3d, .mask = 0xFF, .ro = 0},
};

const unsigned int bcmpmu_chrgr_icc_fc_settings[PMU_CHRGR_CURR_MAX] = {
	[PMU_CHRGR_CURR_50] = 0x00,
	[PMU_CHRGR_CURR_100] = 0x00,
	[PMU_CHRGR_CURR_150] = 0x00,
	[PMU_CHRGR_CURR_200] = 0x00,
	[PMU_CHRGR_CURR_250] = 0x00,
	[PMU_CHRGR_CURR_300] = 0x00,
	[PMU_CHRGR_CURR_350] = 0x01,
	[PMU_CHRGR_CURR_400] = 0x01,
	[PMU_CHRGR_CURR_450] = 0x01,
	[PMU_CHRGR_CURR_500] = 0x02,
	[PMU_CHRGR_CURR_550] = 0x02,
	[PMU_CHRGR_CURR_600] = 0x03,
	[PMU_CHRGR_CURR_650] = 0x03,
	[PMU_CHRGR_CURR_700] = 0x03,
	[PMU_CHRGR_CURR_750] = 0x03,
	[PMU_CHRGR_CURR_800] = 0x04,
	[PMU_CHRGR_CURR_850] = 0x04,
	[PMU_CHRGR_CURR_900] = 0x05,
	[PMU_CHRGR_CURR_950] = 0x05,
	[PMU_CHRGR_CURR_1000] = 0x05,
	[PMU_CHRGR_CURR_90] = 0x00,
};

const unsigned int bcmpmu_chrgr_icc_qc_settings[PMU_CHRGR_QC_CURR_MAX] = {
	[PMU_CHRGR_QC_CURR_50] = 0x00,
	[PMU_CHRGR_QC_CURR_60] = 0x00,
	[PMU_CHRGR_QC_CURR_70] = 0x00,
	[PMU_CHRGR_QC_CURR_80] = 0x00,
	[PMU_CHRGR_QC_CURR_90] = 0x00,
	[PMU_CHRGR_QC_CURR_100] = 0x00,
};

const unsigned int bcmpmu_chrgr_eoc_settings[PMU_CHRGR_EOC_CURR_MAX] = {
	[PMU_CHRGR_EOC_CURR_50] = 0x00,
	[PMU_CHRGR_EOC_CURR_60] = 0x00,
	[PMU_CHRGR_EOC_CURR_70] = 0x00,
	[PMU_CHRGR_EOC_CURR_80] = 0x00,
	[PMU_CHRGR_EOC_CURR_90] = 0x00,
	[PMU_CHRGR_EOC_CURR_100] = 0x00,
	[PMU_CHRGR_EOC_CURR_110] = 0x00,
	[PMU_CHRGR_EOC_CURR_120] = 0x00,
	[PMU_CHRGR_EOC_CURR_130] = 0x00,
	[PMU_CHRGR_EOC_CURR_140] = 0x00,
	[PMU_CHRGR_EOC_CURR_150] = 0x00,
	[PMU_CHRGR_EOC_CURR_160] = 0x00,
	[PMU_CHRGR_EOC_CURR_170] = 0x00,
	[PMU_CHRGR_EOC_CURR_180] = 0x00,
	[PMU_CHRGR_EOC_CURR_190] = 0x00,
	[PMU_CHRGR_EOC_CURR_200] = 0x00,
};

const unsigned int bcmpmu_chrgr_vfloat_settings[PMU_CHRGR_VOLT_MAX] = {
	[PMU_CHRGR_VOLT_3600] = 0x00,
	[PMU_CHRGR_VOLT_3625] = 0x00,
	[PMU_CHRGR_VOLT_3650] = 0x00,
	[PMU_CHRGR_VOLT_3675] = 0x00,
	[PMU_CHRGR_VOLT_3700] = 0x00,
	[PMU_CHRGR_VOLT_3725] = 0x00,
	[PMU_CHRGR_VOLT_3750] = 0x00,
	[PMU_CHRGR_VOLT_3775] = 0x00,
	[PMU_CHRGR_VOLT_3800] = 0x00,
	[PMU_CHRGR_VOLT_3825] = 0x00,
	[PMU_CHRGR_VOLT_3850] = 0x00,
	[PMU_CHRGR_VOLT_3875] = 0x00,
	[PMU_CHRGR_VOLT_3900] = 0x01,
	[PMU_CHRGR_VOLT_3925] = 0x01,
	[PMU_CHRGR_VOLT_3950] = 0x01,
	[PMU_CHRGR_VOLT_3975] = 0x01,
	[PMU_CHRGR_VOLT_4000] = 0x02,
	[PMU_CHRGR_VOLT_4025] = 0x02,
	[PMU_CHRGR_VOLT_4050] = 0x03,
	[PMU_CHRGR_VOLT_4075] = 0x03,
	[PMU_CHRGR_VOLT_4100] = 0x04,
	[PMU_CHRGR_VOLT_4125] = 0x05,
	[PMU_CHRGR_VOLT_4150] = 0x06,
	[PMU_CHRGR_VOLT_4175] = 0x07,
	[PMU_CHRGR_VOLT_4200] = 0x08,
	[PMU_CHRGR_VOLT_4225] = 0x09,
	[PMU_CHRGR_VOLT_4250] = 0x0A,
	[PMU_CHRGR_VOLT_4275] = 0x0B,
	[PMU_CHRGR_VOLT_4300] = 0x0C,
	[PMU_CHRGR_VOLT_4325] = 0x0D,
	[PMU_CHRGR_VOLT_4350] = 0x0E,
	[PMU_CHRGR_VOLT_4375] = 0x0F,
};

static const int bcm59055_usb_id_map[PMU_USB_ID_LVL_MAX] = {
	[0] = PMU_USB_ID_GROUND,
	[1] = PMU_USB_ID_NOT_SUPPORTED,
	[2] = PMU_USB_ID_NOT_SUPPORTED,
	[3] = PMU_USB_ID_RID_A,
	[4] = PMU_USB_ID_RID_C,
	[5] = PMU_USB_ID_RID_C,
	[6] = PMU_USB_ID_NOT_SUPPORTED,
	[7] = PMU_USB_ID_FLOAT,
};

const struct bcmpmu_reg_map *bcmpmu_get_regmap(struct bcmpmu *bcmpmu)
{
	return bcm59055_reg_map;
}

const struct bcmpmu_irq_map *bcmpmu_get_irqmap(struct bcmpmu *bcmpmu)
{
	return bcm59055_irq_map;
}

const struct bcmpmu_adc_map *bcmpmu_get_adcmap(struct bcmpmu *bcmpmu)
{
	return bcm59055_adc_map;
}

struct bcmpmu_adc_unit *bcmpmu_get_adcunit(struct bcmpmu *bcmpmu)
{
	return bcm59055_adc_unit;
}

const struct bcmpmu_reg_map *bcmpmu_get_irqregmap(struct bcmpmu *bcmpmu,
			int *len)
{
	*len = PMU_IRG_REG_MAX;
	return bcm59055_irq_reg_map;
}

const struct bcmpmu_reg_map *bcmpmu_get_adc_ctrl_map(struct bcmpmu *bcmpmu)
{
	return bcm59055_adc_ctrl_map;
}

const struct bcmpmu_env_info *bcmpmu_get_envregmap(struct bcmpmu *bcmpmu,
	 int *len)
{
	*len = PMU_ENV_REG_MAX;
	return bcm59055_env_reg_map;
}

int bcmpmu_sel_adcsync(enum bcmpmu_adc_timing_t timing)
{
	return 0;
}

const int *bcmpmu_get_usb_id_map(struct bcmpmu *bcmpmu, int *len)
{
	*len = PMU_USB_ID_LVL_MAX;
	return bcm59055_usb_id_map;
}

const int bcmpmu_min_supported_curr(void)
{
	return PMU_MIN_CHRG_CURR;
}

int bcmpmu_init_pmurev_info(struct bcmpmu *bcmpmu)
{
	int val;
	int ret;

	ret = bcmpmu->read_dev_drct(bcmpmu,
				bcm59055_reg_map[PMU_REG_PMUGID].map,
				bcm59055_reg_map[PMU_REG_PMUGID].addr, &val,
				bcm59055_reg_map[PMU_REG_PMUGID].mask);
	if (unlikely(ret))
		return ret;
	bcmpmu->rev_info.gen_id = (u8)val;

	ret = bcmpmu->read_dev_drct(bcmpmu, bcm59055_reg_map[PMU_REG_PMUID].map,
				bcm59055_reg_map[PMU_REG_PMUID].addr, &val,
				bcm59055_reg_map[PMU_REG_PMUID].mask);
	if (unlikely(ret))
		return ret;
	bcmpmu->rev_info.prj_id = (u8)val;

	ret = bcmpmu->read_dev_drct(bcmpmu,
				bcm59055_reg_map[PMU_REG_PMUREV].map,
				bcm59055_reg_map[PMU_REG_PMUREV].addr, &val,
				bcm59055_reg_map[PMU_REG_PMUREV].mask);

	if (unlikely(ret))
		return ret;
	bcmpmu->rev_info.dig_rev = ((u8)val) & 0xF;
	bcmpmu->rev_info.ana_rev = (((u8)val) >> 4) & 0xF;

	return ret;
}


MODULE_DESCRIPTION("BCM590XX PMIC bcm59055 driver");
MODULE_LICENSE("GPL");
