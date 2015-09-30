/*******************************************************************************
* Copyright 2010 Broadcom Corporation.  All rights reserved.
*
* 	@file	include/linux/ktd259b_bl.h
*
* Unless you and Broadcom execute a separate written software license agreement
* governing use of this software, this software is licensed to you under the
* terms of the GNU General Public License version 2, available at
* http://www.gnu.org/copyleft/gpl.html (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a license
* other than the GPL, without Broadcom's express prior written consent.
*******************************************************************************/

/*
 * Generic S2C based backlight driver data
 * - see drivers/video/backlight/ktd259b_bl.c
 */
#ifndef __LINUX_KTD_BL_H
#define __LINUX_KTD_BL_H


#define CABC_FEATURE_ON 0
#define BACKLIGHT_DEBUG 1

struct platform_ktd_backlight_data {
      	unsigned int max_brightness;
     	unsigned int dft_brightness;
     	unsigned int ctrl_pin;
#if CABC_FEATURE_ON     	
      unsigned int auto_brightness;
#endif      
};


#if CABC_FEATURE_ON
struct brt_value brt_table_ktd_cabc[] = {
  { MIN_BRIGHTNESS_VALUE,  32 },  /* Min pulse 32 */
   { 27,  32 },
   { 39,  31 },
   { 51,  29 },  
   { 63,  28 }, 
   { 75,  27 }, 
   { 87,  26 }, 
   { 99,  24 }, 
   { 111,  22 }, 
   { 123,  20 }, 
   { 135,  19 }, 
   { 147,  18 }, 
   { 159,  17 }, 
   { 171,  16 }, 
   { 183,  15 },   
   { 195,  15 },
   { 207,  14 }, 
   { 220,  14 }, 
   { 230,  13 },
   { 235,  13 },
   { 240,  12 },
   { MAX_BRIGHTNESS_VALUE,  12 },

};

#define MAX_BRT_STAGE_KTD_CABC (int)(sizeof(brt_table_ktd_cabc)/sizeof(struct brt_value))

extern int cabc_status;
struct backlight_device g_bl;

#endif


#endif
