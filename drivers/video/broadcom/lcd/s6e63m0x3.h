/*******************************************************************************
* Copyright 2010 Broadcom Corporation.  All rights reserved.
*
* 	@file	drivers/video/broadcom/displays/lcd_S6E63M0X.h
*
* Unless you and Broadcom execute a separate DISPCTRL_WRitten software license agreement
* governing use of this software, this software is licensed to you under the
* terms of the GNU General Public License version 2, available at
* http://www.gnu.org/copyleft/gpl.html (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a license
* other than the GPL, without Broadcom's express prior DISPCTRL_WRitten consent.
*******************************************************************************/

/****************************************************************************
*
*  S6E63M0X.h
*
*  PURPOSE:
*    This is the LCD-specific code for a S6E63M0X module.
*
*****************************************************************************/

/* ---- Include Files ---------------------------------------------------- */
#ifndef __BCM_LCD_S6E63M0X_H
#define __BCM_LCD_S6E63M0X_H

//  LCD command definitions
#define PIXEL_FORMAT_RGB565	0x05   // for 16 bits
#define PIXEL_FORMAT_RGB666	0x06   // for 18 bits
#define PIXEL_FORMAT_RGB888	0x07   // for 24 bits

#define MAX_BRIGHTNESS		255
#define DEFAULT_BRIGHTNESS	160
#define DEFAULT_GAMMA_LEVEL	14/*180cd*/
#define MAX_GAMMA_VALUE 	24
#define MAX_GAMMA_LEVEL		25

#define DIM_BL 20
#define MIN_BL 30
#define MAX_BL 255

//#define GAMMA_PARAM_LEN	21

//#define ELVSS_SET_START_IDX 2
//#define ELVSS_SET_END_IDX 5

#define GAMMA_INDEX_NOT_SET	0xFFFF

/*
#define ID_VALUE_M2			0xA4
#define ID_VALUE_SM2			0xB4
#define ID_VALUE_SM2_1		0xB6
*/

const char *LCD_panel_name = "S6E63M0X LCD";

typedef enum {
	S6E63M0X_CMD_NOP					= 0x00,
	S6E63M0X_CMD_SWRESET				= 0x01,
	S6E63M0X_CMD_RDNUMERR				= 0x05,
	S6E63M0X_CMD_RDDPM					= 0x0A,
	S6E63M0X_CMD_RDDCOLMOD				= 0x0C,
	S6E63M0X_CMD_RDDIM					= 0x0D,
	S6E63M0X_CMD_RDDSM					= 0x0E,
	S6E63M0X_CMD_SLPIN					= 0x10,
	S6E63M0X_CMD_SLPOUT					= 0x11,
	S6E63M0X_CMD_NORON					= 0x13,
	S6E63M0X_CMD_ALLPOFF				= 0x22,
	S6E63M0X_CMD_ALLPON					= 0x23,
	S6E63M0X_CMD_GAMSET					= 0x26,
	S6E63M0X_CMD_DISPOFF				= 0x28,
	S6E63M0X_CMD_DISPON					= 0x29,
	S6E63M0X_CMD_TEOFF					= 0x34,
	S6E63M0X_CMD_TEON					= 0x35,
	S6E63M0X_CMD_COLMOD					= 0x3A,
	S6E63M0X_CMD_RDDDBSTART				= 0xA1,
	S6E63M0X_CMD_RDDDBCONT				= 0xA8,
	S6E63M0X_CMD_GLOBAL_PARAM 			= 0xB0,
	S6E63M0X_CMD_ELVSS_CON				= 0xB1,
	S6E63M0X_CMD_TEMP_SWIRE				= 0xB2,
	S6E63M0X_CMD_PENTILE_1				= 0xB3,
	S6E63M0X_CMD_PENTILE_2				= 0xB4,
	S6E63M0X_CMD_GAMMA_DELTA_Y_RED		= 0xB5,
	S6E63M0X_CMD_GAMMA_DELTA_X_RED		= 0xB6,
	S6E63M0X_CMD_GAMMA_DELTA_Y_GREEN	= 0xB7,
	S6E63M0X_CMD_GAMMA_DELTA_X_GREEN	= 0xB8,
	S6E63M0X_CMD_GAMMA_DELTA_Y_BLUE		= 0xB9,
	S6E63M0X_CMD_GAMMA_DELTA_X_BLUE		= 0xBA,
	S6E63M0X_CMD_ACL_CTL				= 0xC0,
	S6E63M0X_CMD_ACL_UPDATE				= 0xC1,
	S6E63M0X_CMD_RDID1					= 0xDA,
	S6E63M0X_CMD_RDID2					= 0xDB,
	S6E63M0X_CMD_RDID3					= 0xDC,
	S6E63M0X_CMD_ERROR_CHECK			= 0xD5,
	S6E63M0X_CMD_LEVEL_2_KEY			= 0xF0,
	S6E63M0X_CMD_MTP_KEY				= 0xF1,
	S6E63M0X_CMD_DISPCTL				= 0xF2,
	S6E63M0X_CMD_SRC_CTL				= 0xF6,
	S6E63M0X_CMD_IF_CTL					= 0xF7,
	S6E63M0X_CMD_PANEL_CON				= 0xF8,
	S6E63M0X_CMD_GAMMA_CTL				= 0xFA,
} S6E63M0X_CMD_T;

__initdata struct DSI_COUNTER s6e63m0x3_timing[] = {
	/* LP Data Symbol Rate Calc - MUST BE FIRST RECORD */
	{"ESC2LP_RATIO", DSI_C_TIME_ESC2LPDT, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0x0000003F, 1, 1},
	/* SPEC:  min =  100[us] + 0[UI] */
	/* SET:   min = 1000[us] + 0[UI]                             <= */
	{"HS_INIT", DSI_C_TIME_HS, 0,
		0, 100000, 0, 0, 0, 0, 0, 0, 0x00FFFFFF, 0, 0},
	/* SPEC:  min = 1[ms] + 0[UI] */
	/* SET:   min = 1[ms] + 0[UI] */
	{"HS_WAKEUP", DSI_C_TIME_HS, 0,
		0, 1000000, 0, 0, 0, 0, 0, 0, 0x00FFFFFF, 0, 0},
	/* SPEC:  min = 1[ms] + 0[UI] */
	/* SET:   min = 1[ms] + 0[UI] */
	{"LP_WAKEUP", DSI_C_TIME_ESC, 0,
		0, 1000000, 0, 0, 0, 0, 0, 0, 0x00FFFFFF, 1, 1},
	/* SPEC:  min = 0[ns] +  8[UI] */
	/* SET:   min = 0[ns] + 12[UI]                               <= */
	{"HS_CLK_PRE", DSI_C_TIME_HS, 0,
		0, 0, 12, 0, 0, 0, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min = 38[ns] + 0[UI]   max= 95[ns] + 0[UI] */
	/* SET:   min = 48[ns] + 0[UI]   max= 95[ns] + 0[UI]         <= */
	{"HS_CLK_PREPARE", DSI_C_TIME_HS, DSI_C_HAS_MAX,
		0, 48, 0, 0, 0, 95, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min = 262[ns] + 0[UI] */
	/* SET:   min = 262[ns] + 0[UI]                              <= */
	{"HS_CLK_ZERO", DSI_C_TIME_HS, 0,
		0, 320, 0, 0, 0, 0, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min =  60[ns] + 52[UI] */
	/* SET:   min =  70[ns] + 52[UI]                             <= */
	{"HS_CLK_POST", DSI_C_TIME_HS, 0,
		0, 70, 52, 0, 0, 0, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min =  60[ns] + 0[UI] */
	/* SET:   min =  70[ns] + 0[UI]                              <= */
	{"HS_CLK_TRAIL", DSI_C_TIME_HS, 0,
		0, 70, 0, 0, 0, 0, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min =  50[ns] + 0[UI] */
	/* SET:   min =  60[ns] + 0[UI]                              <= */
	{"HS_LPX", DSI_C_TIME_HS, 0,
		0, 60, 0, 0, 0, 75, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min = 40[ns] + 4[UI]      max= 85[ns] + 6[UI] */
	/* SET:   min = 50[ns] + 4[UI]      max= 85[ns] + 6[UI]      <= */
	{"HS_PRE", DSI_C_TIME_HS, DSI_C_HAS_MAX,
		0, 50, 4, 0, 0, 85, 6, 0, 0x000001FF, 0, 0},
	/* SPEC:  min = 105[ns] + 6[UI] */
	/* SET:   min = 105[ns] + 6[UI]                              <= */
	{"HS_ZERO", DSI_C_TIME_HS, 0,
		0, 105, 6, 0, 0, 0, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min = max(0[ns]+32[UI],60[ns]+16[UI])  n=4 */
	/* SET:   min = max(0[ns]+32[UI],60[ns]+16[UI])  n=4 */
	{"HS_TRAIL", DSI_C_TIME_HS, DSI_C_MIN_MAX_OF_2,
		0, 60, 4, 60, 16, 0, 0, 0, 0x000001FF, 0, 0},
	/* SPEC:  min = 100[ns] + 0[UI] */
	/* SET:   min = 110[ns] + 0[UI]                              <= */
	{"HS_EXIT", DSI_C_TIME_HS, 0,
		0, 110, 0, 0, 0, 0, 0, 0, 0x000001FF, 0, 0},
	/* min = 50[ns] + 0[UI] */
	/* LP esc counters are speced in LP LPX units.
	   LP_LPX is calculated by chal_dsi_set_timing
	   and equals LP data clock */
	{"LPX", DSI_C_TIME_ESC, 0,
		1, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1},
	/* min = 4*[Tlpx]  max = 4[Tlpx], set to 4 */
	{"LP-TA-GO", DSI_C_TIME_ESC, 0,
		4, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1},
	/* min = 1*[Tlpx]  max = 2[Tlpx], set to 2 */
	{"LP-TA-SURE", DSI_C_TIME_ESC, 0,
		2, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1},
	/* min = 5*[Tlpx]  max = 5[Tlpx], set to 5 */
	{"LP-TA-GET", DSI_C_TIME_ESC, 0,
		5, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1},
};

__initdata DISPCTRL_REC_T s6e63m0x3_scrn_on[] = {
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_DISPON},
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_IF_CTL}, // Display condition set 
	{DISPCTRL_WR_DATA, 0x03},	
	{DISPCTRL_LIST_END, 0}
};

__initdata DISPCTRL_REC_T s6e63m0x3_scrn_off[] = {
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_DISPOFF},
	{DISPCTRL_LIST_END, 0}
};

__initdata DISPCTRL_REC_T s6e63m0x3_slp_in[] = {
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_DISPOFF},
	{DISPCTRL_SLEEP_MS, 10},
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_SLPIN},
	{DISPCTRL_SLEEP_MS, 120},
	{DISPCTRL_LIST_END, 0}
};

__initdata DISPCTRL_REC_T s6e63m0x3_slp_out[] = {
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_SLPOUT},
	{DISPCTRL_SLEEP_MS, 120},
	{DISPCTRL_LIST_END, 0}
};

__initdata DISPCTRL_REC_T s6e63m0x3_init_panel_vid[] = {
  {DISPCTRL_WR_CMND, S6E63M0X_CMD_LEVEL_2_KEY},
	{DISPCTRL_WR_DATA, 0x5A},
	{DISPCTRL_WR_DATA, 0x5A},
	
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_MTP_KEY},
	{DISPCTRL_WR_DATA, 0x5A},
	{DISPCTRL_WR_DATA, 0x5A},
	
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_ACL_CTL}, //ACL on
	{DISPCTRL_WR_DATA, 0x01},	
	
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_SLPOUT},
	{DISPCTRL_SLEEP_MS, 120},	

  {DISPCTRL_WR_CMND, S6E63M0X_CMD_ERROR_CHECK},
	{DISPCTRL_WR_DATA, 0xE7},
	{DISPCTRL_WR_DATA, 0x14},
	{DISPCTRL_WR_DATA, 0x60},
	{DISPCTRL_WR_DATA, 0x17},
	{DISPCTRL_WR_DATA, 0x0A},
	{DISPCTRL_WR_DATA, 0x49},
	{DISPCTRL_WR_DATA, 0xC3},
	{DISPCTRL_WR_DATA, 0x8F},
	{DISPCTRL_WR_DATA, 0x19},
	{DISPCTRL_WR_DATA, 0x64},
	{DISPCTRL_WR_DATA, 0x91},
	{DISPCTRL_WR_DATA, 0x84},
	{DISPCTRL_WR_DATA, 0x76},
	{DISPCTRL_WR_DATA, 0x20},
	{DISPCTRL_WR_DATA, 0x0F},
	{DISPCTRL_WR_DATA, 0x00},	
	
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_PANEL_CON},
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_WR_DATA, 0x2C},
	{DISPCTRL_WR_DATA, 0x2C},
	{DISPCTRL_WR_DATA, 0x07},
	{DISPCTRL_WR_DATA, 0x07},
	{DISPCTRL_WR_DATA, 0x5F},
	{DISPCTRL_WR_DATA, 0xB3},
	{DISPCTRL_WR_DATA, 0x6D},
	{DISPCTRL_WR_DATA, 0x97},
	{DISPCTRL_WR_DATA, 0x1D},
	{DISPCTRL_WR_DATA, 0x3A},
	{DISPCTRL_WR_DATA, 0x0F},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x00},	
	
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_DISPCTL}, // Display condition set 
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x05},
	{DISPCTRL_WR_DATA, 0x1C},
	{DISPCTRL_WR_DATA, 0x10},
	{DISPCTRL_WR_DATA, 0x10},

	{DISPCTRL_WR_CMND, S6E63M0X_CMD_IF_CTL}, // Display condition set 
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x00},		
	
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_SRC_CTL}, // Etc condition set 
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x8E},
	{DISPCTRL_WR_DATA, 0x0F},
	
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_PENTILE_1},
	{DISPCTRL_WR_DATA, 0x6c},

	{DISPCTRL_WR_CMND, S6E63M0X_CMD_GAMMA_DELTA_Y_RED}, 
	{DISPCTRL_WR_DATA, 0x2C}, // 1
	{DISPCTRL_WR_DATA, 0x12}, // 2
	{DISPCTRL_WR_DATA, 0x0C}, // 3
	{DISPCTRL_WR_DATA, 0x0A}, // 4
	{DISPCTRL_WR_DATA, 0x10}, // 5
	{DISPCTRL_WR_DATA, 0x0E}, // 6
	{DISPCTRL_WR_DATA, 0x17}, // 7
	{DISPCTRL_WR_DATA, 0x13}, // 8
	{DISPCTRL_WR_DATA, 0x1F}, // 9
	{DISPCTRL_WR_DATA, 0x1A}, // 10
	{DISPCTRL_WR_DATA, 0x2A}, // 11
	{DISPCTRL_WR_DATA, 0x24}, // 12
	{DISPCTRL_WR_DATA, 0x1F}, // 13
	{DISPCTRL_WR_DATA, 0x1B}, // 14
	{DISPCTRL_WR_DATA, 0x1A}, // 15
	{DISPCTRL_WR_DATA, 0x17}, // 16
	{DISPCTRL_WR_DATA, 0x2B}, // 17
	{DISPCTRL_WR_DATA, 0x26}, // 18
	{DISPCTRL_WR_DATA, 0x22}, // 19
	{DISPCTRL_WR_DATA, 0x20}, // 20
	{DISPCTRL_WR_DATA, 0x3A}, // 21
	{DISPCTRL_WR_DATA, 0x34}, // 22
	{DISPCTRL_WR_DATA, 0x30}, // 23
	{DISPCTRL_WR_DATA, 0x2C}, // 24
	{DISPCTRL_WR_DATA, 0x29}, // 25
	{DISPCTRL_WR_DATA, 0x26}, // 26
	{DISPCTRL_WR_DATA, 0x25}, // 27
	{DISPCTRL_WR_DATA, 0x23}, // 28
	{DISPCTRL_WR_DATA, 0x21}, // 29
	{DISPCTRL_WR_DATA, 0x20}, // 30
	{DISPCTRL_WR_DATA, 0x1E}, // 31
	{DISPCTRL_WR_DATA, 0x1E}, // 32

	{DISPCTRL_WR_CMND, S6E63M0X_CMD_GAMMA_DELTA_X_RED}, 
	{DISPCTRL_WR_DATA, 0x00}, // 1
	{DISPCTRL_WR_DATA, 0x00}, // 2
	{DISPCTRL_WR_DATA, 0x11}, // 3
	{DISPCTRL_WR_DATA, 0x22}, // 4
	{DISPCTRL_WR_DATA, 0x33}, // 5
	{DISPCTRL_WR_DATA, 0x44}, // 6
	{DISPCTRL_WR_DATA, 0x44}, // 7
	{DISPCTRL_WR_DATA, 0x44}, // 8
	{DISPCTRL_WR_DATA, 0x55}, // 9
	{DISPCTRL_WR_DATA, 0x55}, // 10
	{DISPCTRL_WR_DATA, 0x66}, // 11
	{DISPCTRL_WR_DATA, 0x66}, // 12
	{DISPCTRL_WR_DATA, 0x66}, // 13
	{DISPCTRL_WR_DATA, 0x66}, // 14
	{DISPCTRL_WR_DATA, 0x66}, // 15
	{DISPCTRL_WR_DATA, 0x66}, // 16	
	
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_GAMMA_DELTA_Y_GREEN}, 
	{DISPCTRL_WR_DATA, 0x2C}, // 1
	{DISPCTRL_WR_DATA, 0x12}, // 2
	{DISPCTRL_WR_DATA, 0x0C}, // 3
	{DISPCTRL_WR_DATA, 0x0A}, // 4
	{DISPCTRL_WR_DATA, 0x10}, // 5
	{DISPCTRL_WR_DATA, 0x0E}, // 6
	{DISPCTRL_WR_DATA, 0x17}, // 7
	{DISPCTRL_WR_DATA, 0x13}, // 8
	{DISPCTRL_WR_DATA, 0x1F}, // 9
	{DISPCTRL_WR_DATA, 0x1A}, // 10
	{DISPCTRL_WR_DATA, 0x2A}, // 11
	{DISPCTRL_WR_DATA, 0x24}, // 12
	{DISPCTRL_WR_DATA, 0x1F}, // 13
	{DISPCTRL_WR_DATA, 0x1B}, // 14
	{DISPCTRL_WR_DATA, 0x1A}, // 15
	{DISPCTRL_WR_DATA, 0x17}, // 16
	{DISPCTRL_WR_DATA, 0x2B}, // 17
	{DISPCTRL_WR_DATA, 0x26}, // 18
	{DISPCTRL_WR_DATA, 0x22}, // 19
	{DISPCTRL_WR_DATA, 0x20}, // 20
	{DISPCTRL_WR_DATA, 0x3A}, // 21
	{DISPCTRL_WR_DATA, 0x34}, // 22
	{DISPCTRL_WR_DATA, 0x30}, // 23
	{DISPCTRL_WR_DATA, 0x2C}, // 24
	{DISPCTRL_WR_DATA, 0x29}, // 25
	{DISPCTRL_WR_DATA, 0x26}, // 26
	{DISPCTRL_WR_DATA, 0x25}, // 27
	{DISPCTRL_WR_DATA, 0x23}, // 28
	{DISPCTRL_WR_DATA, 0x21}, // 29
	{DISPCTRL_WR_DATA, 0x20}, // 30
	{DISPCTRL_WR_DATA, 0x1E}, // 31
	{DISPCTRL_WR_DATA, 0x1E}, // 32

	{DISPCTRL_WR_CMND, S6E63M0X_CMD_GAMMA_DELTA_X_GREEN}, 
	{DISPCTRL_WR_DATA, 0x00}, // 1
	{DISPCTRL_WR_DATA, 0x00}, // 2
	{DISPCTRL_WR_DATA, 0x11}, // 3
	{DISPCTRL_WR_DATA, 0x22}, // 4
	{DISPCTRL_WR_DATA, 0x33}, // 5
	{DISPCTRL_WR_DATA, 0x44}, // 6
	{DISPCTRL_WR_DATA, 0x44}, // 7
	{DISPCTRL_WR_DATA, 0x44}, // 8
	{DISPCTRL_WR_DATA, 0x55}, // 9
	{DISPCTRL_WR_DATA, 0x55}, // 10
	{DISPCTRL_WR_DATA, 0x66}, // 11
	{DISPCTRL_WR_DATA, 0x66}, // 12
	{DISPCTRL_WR_DATA, 0x66}, // 13
	{DISPCTRL_WR_DATA, 0x66}, // 14
	{DISPCTRL_WR_DATA, 0x66}, // 15
	{DISPCTRL_WR_DATA, 0x66}, // 16

	{DISPCTRL_WR_CMND, S6E63M0X_CMD_GAMMA_DELTA_Y_BLUE}, 
	{DISPCTRL_WR_DATA, 0x2C}, // 1
	{DISPCTRL_WR_DATA, 0x12}, // 2
	{DISPCTRL_WR_DATA, 0x0C}, // 3
	{DISPCTRL_WR_DATA, 0x0A}, // 4
	{DISPCTRL_WR_DATA, 0x10}, // 5
	{DISPCTRL_WR_DATA, 0x0E}, // 6
	{DISPCTRL_WR_DATA, 0x17}, // 7
	{DISPCTRL_WR_DATA, 0x13}, // 8
	{DISPCTRL_WR_DATA, 0x1F}, // 9
	{DISPCTRL_WR_DATA, 0x1A}, // 10
	{DISPCTRL_WR_DATA, 0x2A}, // 11
	{DISPCTRL_WR_DATA, 0x24}, // 12
	{DISPCTRL_WR_DATA, 0x1F}, // 13
	{DISPCTRL_WR_DATA, 0x1B}, // 14
	{DISPCTRL_WR_DATA, 0x1A}, // 15
	{DISPCTRL_WR_DATA, 0x17}, // 16
	{DISPCTRL_WR_DATA, 0x2B}, // 17
	{DISPCTRL_WR_DATA, 0x26}, // 18
	{DISPCTRL_WR_DATA, 0x22}, // 19
	{DISPCTRL_WR_DATA, 0x20}, // 20
	{DISPCTRL_WR_DATA, 0x3A}, // 21
	{DISPCTRL_WR_DATA, 0x34}, // 22
	{DISPCTRL_WR_DATA, 0x30}, // 23
	{DISPCTRL_WR_DATA, 0x2C}, // 24
	{DISPCTRL_WR_DATA, 0x29}, // 25
	{DISPCTRL_WR_DATA, 0x26}, // 26
	{DISPCTRL_WR_DATA, 0x25}, // 27
	{DISPCTRL_WR_DATA, 0x23}, // 28
	{DISPCTRL_WR_DATA, 0x21}, // 29
	{DISPCTRL_WR_DATA, 0x20}, // 30
	{DISPCTRL_WR_DATA, 0x1E}, // 31
	{DISPCTRL_WR_DATA, 0x1E}, // 32

	{DISPCTRL_WR_CMND, S6E63M0X_CMD_GAMMA_DELTA_X_BLUE}, 
	{DISPCTRL_WR_DATA, 0x00}, // 1
	{DISPCTRL_WR_DATA, 0x00}, // 2
	{DISPCTRL_WR_DATA, 0x11}, // 3
	{DISPCTRL_WR_DATA, 0x22}, // 4
	{DISPCTRL_WR_DATA, 0x33}, // 5
	{DISPCTRL_WR_DATA, 0x44}, // 6
	{DISPCTRL_WR_DATA, 0x44}, // 7
	{DISPCTRL_WR_DATA, 0x44}, // 8
	{DISPCTRL_WR_DATA, 0x55}, // 9
	{DISPCTRL_WR_DATA, 0x55}, // 10
	{DISPCTRL_WR_DATA, 0x66}, // 11
	{DISPCTRL_WR_DATA, 0x66}, // 12
	{DISPCTRL_WR_DATA, 0x66}, // 13
	{DISPCTRL_WR_DATA, 0x66}, // 14
	{DISPCTRL_WR_DATA, 0x66}, // 15
	{DISPCTRL_WR_DATA, 0x66}, // 16

	{DISPCTRL_WR_CMND, S6E63M0X_CMD_GAMMA_CTL}, //gamma_300cd
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x31},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x4F},
	{DISPCTRL_WR_DATA, 0x14},
	{DISPCTRL_WR_DATA, 0x6E},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xA3},
	{DISPCTRL_WR_DATA, 0xC0},
	{DISPCTRL_WR_DATA, 0x92},
	{DISPCTRL_WR_DATA, 0xA4},
	{DISPCTRL_WR_DATA, 0xBA},
	{DISPCTRL_WR_DATA, 0x93},
	{DISPCTRL_WR_DATA, 0xBD},
	{DISPCTRL_WR_DATA, 0xC8},
	{DISPCTRL_WR_DATA, 0xAF},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xB0},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xA2},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xD1},
	
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_GAMMA_CTL},
	{DISPCTRL_WR_DATA, 0x03},	

	{DISPCTRL_WR_CMND, S6E63M0X_CMD_TEMP_SWIRE}, //Smart Dynamic ELVSS Set
	{DISPCTRL_WR_DATA, 0x16},
	{DISPCTRL_WR_DATA, 0x16},
	{DISPCTRL_WR_DATA, 0x16},
	{DISPCTRL_WR_DATA, 0x16},
	
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_ELVSS_CON},
	{DISPCTRL_WR_DATA, 0x0B}, // 0A - OFF; 0B - ON
	{DISPCTRL_SLEEP_MS, 120}, // wait 120 ms	
		
	{DISPCTRL_LIST_END, 0}
};

DISPCTRL_REC_T acl_on_seq[] =
{
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_ACL_CTL}, //ACL on
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_LIST_END, 0}
};

DISPCTRL_REC_T acl_off_seq[] =
{
	{DISPCTRL_WR_CMND, S6E63M0X_CMD_ACL_CTL}, //ACL off
	{DISPCTRL_WR_DATA, 0x0},
	{DISPCTRL_LIST_END, 0}
};

/*
typedef struct {
	Int32(*init_smart_dimming) (u8* lcdIDs, u8* mtpData);
	Int32(*get_brightness) (struct backlight_device *bd);
	Int32(*set_brightness) (struct backlight_device *bd);
	Int32(*set_acl) (struct s6e63m0_dsi_lcd *lcd);
	Int32(*set_elvss) (struct s6e63m0_dsi_lcd *lcd);
} PANEL_ADD_ON_T;
*/

void s6e63m0x3_winset(char *msgData, DISPDRV_WIN_t *p_win)
{
	return;
}

__initdata struct lcd_config s6e63m0x3_cfg = {
	.name = "S6E63M0X3",
	.mode_supp = LCD_VID_ONLY,
	.phy_timing = &s6e63m0x3_timing[0],
	.max_lanes = 2,
	.max_hs_bps = 350000000,
	.max_lp_bps = 9500000,
	.phys_width = 55,
	.phys_height = 99,
	.init_cmd_seq = NULL,
	.init_vid_seq = &s6e63m0x3_init_panel_vid[0],
	.slp_in_seq = &s6e63m0x3_slp_in[0],
	.slp_out_seq = &s6e63m0x3_slp_out[0],
	.scrn_on_seq = &s6e63m0x3_scrn_on[0],
	.scrn_off_seq = &s6e63m0x3_scrn_off[0],
	.id_seq = NULL,
	.verify_id = false,	
	.updt_win_fn = s6e63m0x3_winset,
	.updt_win_seq_len = 0,
	.vid_cmnds = false,
	.vburst = true,
	.cont_clk = true,	
	.hs = 10,
	.hbp = 16,
	.hfp = 16,
	.vs = 2,
	.vbp = 3,
	.vfp = 28,
};

#endif /* __BCM_LCD_S6E63M0X_H */

