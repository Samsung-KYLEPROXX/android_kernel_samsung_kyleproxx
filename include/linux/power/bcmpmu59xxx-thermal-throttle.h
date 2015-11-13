/*****************************************************************************
*  Copyright 2001 - 2013 Broadcom Corporation.  All rights reserved.
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
#define MAX_CHARGER_REGISTERS		10
#define THROTTLE_WORK_POLL_TIME		5000 /* 5 Seconds */
#define HYSTERESIS_DEFAULT_TEMP		1 /* 1C */



struct batt_temp_curr_map {
	int temp;
	int curr;
};

struct bcmpmu_throttle_pdata {
	struct batt_temp_curr_map *temp_curr_lut;
	u32 temp_curr_lut_sz;
	u8 temp_adc_channel;
	u8 temp_adc_req_mode;
	u32 *throttle_backup_reg;
	u8 throttle_backup_reg_sz;
	u32 throttle_poll_time;
	u32 hysteresis_temp;
};
