/*****************************************************************************
* Copyright 2012 Broadcom Corporation.  All rights reserved.
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a
* license other than the GPL, without Broadcom's express prior written
* consent.
*****************************************************************************/
#include <linux/version.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <asm/mach/arch.h>
#include <mach/io_map.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <mach/irqs.h>
#include <mach/memory.h>
#include<mach/clock.h>
#include<plat/pi_mgr.h>
#include<plat/clock.h>
#include<mach/pi_mgr.h>
#include<mach/pwr_mgr.h>
#include<plat/pwr_mgr.h>
#include <mach/rdb/brcm_rdb_chipreg.h>
#include <mach/rdb/brcm_rdb_bmdm_pwrmgr.h>
#include <plat/kona_cpufreq_drv.h>
#include <plat/kona_reset_reason.h>
#ifdef CONFIG_DEBUG_FS
#include <mach/rdb/brcm_rdb_padctrlreg.h>
#endif
#include "pm_params.h"
#include "sequencer_ucode.h"
#include <mach/avs.h>
#ifdef CONFIG_DELAYED_PM_INIT
#include <mach/pm.h>
#include <plat/kona_pm.h>
#endif
#include <mach/rdb/brcm_rdb_root_clk_mgr_reg.h>


#define VLT_LUT_SIZE	16

static int delayed_init_complete;

#ifdef CONFIG_DELAYED_PM_INIT
struct pm_late_init {
	int dummy;
};

#define __param_check_pm_late_init(name, p, type) \
	static inline struct type *__check_##name(void) { return (p); }

#define param_check_pm_late_init(name, p) \
	__param_check_pm_late_init(name, p, pm_late_init)

static int param_set_pm_late_init(const char *val,
		const struct kernel_param *kp);
static struct kernel_param_ops param_ops_pm_late_init = {
	.set = param_set_pm_late_init,
};
static struct pm_late_init pm_late_init;
module_param_named(pm_late_init, pm_late_init, pm_late_init,
				S_IWUSR | S_IWGRP);
#endif

#ifdef CONFIG_DEBUG_FS
const char *__event2str[] = {
	__stringify(LCDTE_EVENT),
	__stringify(SSP2SYN_EVENT),
	__stringify(SSP2DI_EVENT),
	__stringify(SSP2CK_EVENT),
	__stringify(SSP1SYN_EVENT),
	__stringify(SSP1DI_EVENT),
	__stringify(SSP1CK_EVENT),
	__stringify(SSP0SYN_EVENT),
	__stringify(SSP0DI_EVENT),
	__stringify(SSP0CK_EVENT),
	__stringify(DIGCLKREQ_EVENT),
	__stringify(ANA_SYS_REQ_EVENT),
	__stringify(SYSCLKREQ_EVENT),
	__stringify(UBRX_EVENT),
	__stringify(UBCTSN_EVENT),
	__stringify(UB2RX_EVENT),
	__stringify(UB2CTSN_EVENT),
	__stringify(SIMDET_EVENT),
	__stringify(SIM2DET_EVENT),
	__stringify(MMC0D3_EVENT),
	__stringify(MMC0D1_EVENT),
	__stringify(MMC1D3_EVENT),
	__stringify(MMC1D1_EVENT),
	__stringify(SDDAT3_EVENT),
	__stringify(SDDAT1_EVENT),
	__stringify(SLB1CLK_EVENT),
	__stringify(SLB1DAT_EVENT),
	__stringify(SWCLKTCK_EVENT),
	__stringify(SWDIOTMS_EVENT),
	__stringify(KEY_R0_EVENT),
	__stringify(KEY_R1_EVENT),
	__stringify(KEY_R2_EVENT),
	__stringify(KEY_R3_EVENT),
	__stringify(KEY_R4_EVENT),
	__stringify(KEY_R5_EVENT),
	__stringify(KEY_R6_EVENT),
	__stringify(KEY_R7_EVENT),
	NULL,
	NULL,
	__stringify(MISC_WKP_EVENT),
	__stringify(BATRM_EVENT),
	__stringify(USBDP_EVENT),
	__stringify(USBDN_EVENT),
	__stringify(RXD_EVENT),
	__stringify(GPIO29_A_EVENT),
	__stringify(GPIO32_A_EVENT),
	__stringify(GPIO33_A_EVENT),
	__stringify(GPIO43_A_EVENT),
	__stringify(GPIO44_A_EVENT),
	__stringify(GPIO45_A_EVENT),
	__stringify(GPIO46_A_EVENT),
	__stringify(GPIO47_A_EVENT),
	__stringify(GPIO48_A_EVENT),
	__stringify(GPIO71_A_EVENT),
	__stringify(GPIO72_A_EVENT),
	__stringify(GPIO73_A_EVENT),
	__stringify(GPIO74_A_EVENT),
	__stringify(GPIO95_A_EVENT),
	__stringify(GPIO96_A_EVENT),
	__stringify(GPIO99_A_EVENT),
	__stringify(GPIO100_A_EVENT),
	__stringify(GPIO111_A_EVENT),
	__stringify(GPIO18_A_EVENT),
	__stringify(GPIO19_A_EVENT),
	__stringify(GPIO20_A_EVENT),
	__stringify(GPIO89_A_EVENT),
	__stringify(GPIO90_A_EVENT),
	__stringify(GPIO91_A_EVENT),
	__stringify(GPIO92_A_EVENT),
	__stringify(GPIO93_A_EVENT),
	__stringify(GPIO18_B_EVENT),
	__stringify(GPIO19_B_EVENT),
	__stringify(GPIO20_B_EVENT),
	__stringify(GPIO89_B_EVENT),
	__stringify(GPIO90_B_EVENT),
	__stringify(GPIO91_B_EVENT),
	__stringify(GPIO92_B_EVENT),
	__stringify(GPIO93_B_EVENT),
	__stringify(GPIO29_B_EVENT),
	__stringify(GPIO32_B_EVENT),
	__stringify(GPIO33_B_EVENT),
	__stringify(GPIO43_B_EVENT),
	__stringify(GPIO44_B_EVENT),
	__stringify(GPIO45_B_EVENT),
	__stringify(GPIO46_B_EVENT),
	__stringify(GPIO47_B_EVENT),
	__stringify(GPIO48_B_EVENT),
	__stringify(GPIO71_B_EVENT),
	__stringify(GPIO72_B_EVENT),
	__stringify(GPIO73_B_EVENT),
	__stringify(GPIO74_B_EVENT),
	__stringify(GPIO95_B_EVENT),
	__stringify(GPIO96_B_EVENT),
	__stringify(GPIO99_B_EVENT),
	__stringify(GPIO100_B_EVENT),
	__stringify(GPIO111_B_EVENT),
	__stringify(COMMON_TIMER_0_EVENT),
	__stringify(COMMON_TIMER_1_EVENT),
	__stringify(COMMON_TIMER_2_EVENT),
	__stringify(COMMON_TIMER_3_EVENT),
	__stringify(COMMON_TIMER_4_EVENT),
	__stringify(COMMON_INT_TO_AC_EVENT),
	__stringify(TZCFG_INT_TO_AC_EVENT),
	__stringify(DMA_REQUEST_EVENT),
	__stringify(MODEM1_EVENT),
	__stringify(MODEM2_EVENT),
	__stringify(SD1DAT1DAT3_EVENT),
	__stringify(BRIDGE_TO_AC_EVENT),
	__stringify(BRIDGE_TO_MODEM_EVENT),
	__stringify(VREQ_NONZERO_PI_MODEM_EVENT),
	NULL,
	__stringify(USBOTG_EVENT),
	__stringify(GPIO_EXP_IRQ_EVENT),
	__stringify(DBR_IRQ_EVENT),
	__stringify(ACI_EVENT),
	__stringify(PHY_RESUME_EVENT),
	__stringify(MODEMBUS_ACTIVE_EVENT),
	__stringify(SOFTWARE_0_EVENT),
	__stringify(SOFTWARE_1_EVENT),
	__stringify(SOFTWARE_2_EVENT),
};

#endif

struct event_table {
	u32 event_id;
	u32 trig_type;
	u32 policy_modem;
	u32 policy_arm;
	u32 policy_arm_sub;
	u32 policy_aon;
	u32 policy_hub;
	u32 policy_mm;
};
static const struct event_table __event_table[] = {
	{
		.event_id	= SOFTWARE_0_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},

	{
		.event_id	= SOFTWARE_1_EVENT,
		.trig_type	= PM_TRIG_NONE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_OFF,
		.policy_arm_sub	= PM_RET,
		.policy_aon	= PM_RET,
		.policy_hub	= PM_RET,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= SOFTWARE_2_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_ECO,
		.policy_aon	= PM_ECO,
		.policy_hub	= PM_ECO,
		.policy_mm	= PM_OFF,
	},

	{
		.event_id	= VREQ_NONZERO_PI_MODEM_EVENT,
		.trig_type	= PM_TRIG_POS_EDGE,
		.policy_modem	= PM_DFS,
		.policy_arm	= PM_OFF,
		.policy_arm_sub	= PM_RET,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= COMMON_INT_TO_AC_EVENT,
		.trig_type	= PM_TRIG_POS_EDGE,
		.policy_modem 	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= COMMON_TIMER_0_EVENT,
		.trig_type	= PM_TRIG_POS_EDGE,
		.policy_modem 	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= COMMON_TIMER_1_EVENT,
		.trig_type	= PM_TRIG_POS_EDGE,
		.policy_modem 	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id       = COMMON_TIMER_2_EVENT,
		.trig_type      = PM_TRIG_POS_EDGE,
		.policy_modem   = PM_RET,
		.policy_arm     = PM_DFS,
		.policy_arm_sub = PM_DFS,
		.policy_aon     = PM_DFS,
		.policy_hub     = PM_DFS,
		.policy_mm      = PM_OFF,
	},

	{
		.event_id	= UBRX_EVENT,
		.trig_type	= PM_TRIG_NEG_EDGE,
		.policy_modem 	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= UB2RX_EVENT,
		.trig_type	= PM_TRIG_NEG_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},

	{
		.event_id	= SIMDET_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= SIM2DET_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= KEY_R0_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= KEY_R1_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= KEY_R2_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= KEY_R3_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= KEY_R4_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= KEY_R5_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= KEY_R6_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= KEY_R7_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},

#ifndef CONFIG_MACH_HAWAII_SS_COMMON
	/* battery removal event for SIM protection */
	{
		.event_id	= BATRM_EVENT,
		.trig_type	= PM_TRIG_NEG_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
#endif /* CONFIG_MACH_HAWAII_SS_COMMON */

	{
		.event_id	= GPIO29_A_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= GPIO71_A_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= GPIO74_A_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
		{
		.event_id	= GPIO111_A_EVENT,
		.trig_type	= PM_TRIG_BOTH_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},

	{
		.event_id	= MMC1D1_EVENT,
		.trig_type	= PM_TRIG_NEG_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= DBR_IRQ_EVENT,
		.trig_type	= PM_TRIG_POS_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
	{
		.event_id	= ACI_EVENT,
		.trig_type	= PM_TRIG_POS_EDGE,
		.policy_modem	= PM_RET,
		.policy_arm	= PM_DFS,
		.policy_arm_sub	= PM_DFS,
		.policy_aon	= PM_DFS,
		.policy_hub	= PM_DFS,
		.policy_mm	= PM_OFF,
	},
};

static struct i2c_cmd i2c_dummy_seq_cmd[] = {
	{REG_ADDR, 0},		/*0 - NOP*/
	{JUMP_VOLTAGE, 0},	/*1 - jump based on voltage request*/
	{REG_ADDR, 0},		/*2 - NOP   - other (all)*/
	{REG_ADDR, 0},		/*3 - NOP*/
	{REG_ADDR, 0},		/*4 - NOP*/
	{REG_ADDR, 0},		/*5 - NOP*/
	{REG_ADDR, 0},		/*6 - NOP*/
	{REG_ADDR, 0},		/*7 - NOP*/
	{REG_ADDR, 0},		/*8 - NOP*/
	{REG_ADDR, 0},		/*9 - NOP*/
	{REG_ADDR, 0},		/*10 - NOP*/
	{REG_ADDR, 0},		/*11 - NOP*/
	{REG_ADDR, 0},		/*12 - NOP*/
	{REG_ADDR, 0},		/*13 - NOP*/
	{REG_ADDR, 0},		/*14 - NOP*/
	{REG_ADDR, 0},		/*15 - NOP*/
	{REG_ADDR, 0},		/*16 - NOP*/
	{END, 0},			/*17 - End*/
	{SET_PC_PINS, SET_PC_PIN_CMD(PC1)}, /*18 - wake up (other)*/
	{END, 0},			/*19 - End*/
	{REG_ADDR, 0},		//20 - NOP
	{SET_PC_PINS, CLEAR_PC_PIN_CMD(PC1) |\
		CLEAR_PC_PIN_CMD(PC0)}, /*21 - retention, zero (other)*/
	{END, 0},			/*22 - End*/
	{REG_ADDR, 0},		/*23 - NOP*/
	{SET_PC_PINS, SET_PC_PIN_CMD(PC2)}, /*24 - wake up(A9)*/
	{WAIT_TIMER, SR_VLT_SOFT_START_DELAY}, /* Wait for voltage change */
	{END, 0},			/*26 - End*/
	{REG_ADDR, 0},		/*27 - NOP*/
	{SET_PC_PINS, CLEAR_PC_PIN_CMD(PC2) |\
		CLEAR_PC_PIN_CMD(PC0)}, /*28 - retention, zero (A9)*/
	{END, 0},			/*28 - End*/
	{REG_ADDR, 0},		/*29 - NOP*/
	{REG_ADDR, 0},		/*30 - NOP*/
	{REG_ADDR, 0},		/*31 - NOP*/
	{REG_ADDR, 0},		/*32 - NOP*/
	{REG_ADDR, 0},		/*33 - NOP*/
	{REG_ADDR, 0},		/*34 - NOP*/
	{REG_ADDR, 0},		/*35 - NOP*/
	{REG_ADDR, 0},		/*36 - NOP*/
	{REG_ADDR, 0},		/*37 - NOP*/
	{REG_ADDR, 0},		/*38 - NOP*/
	{REG_ADDR, 0},		/*39 - NOP*/
	{REG_ADDR, 0},		/*40 - NOP*/
	{REG_ADDR, 0},		/*41 - NOP*/
	{REG_ADDR, 0},		/*42 - NOP*/
	{REG_ADDR, 0},		/*43 - NOP*/
	{REG_ADDR, 0},		/*44 - NOP*/
	{REG_ADDR, 0},		/*45 - NOP*/
	{REG_ADDR, 0},		/*46 - NOP*/
	{REG_ADDR, 0},		/*47 - NOP*/
	{REG_ADDR, 0},		/*48 - NOP*/
	{REG_ADDR, 0},		/*49 - NOP*/
	{REG_ADDR, 0},		/*50 - NOP*/
	{REG_ADDR, 0},		/*51 - NOP*/
	{REG_ADDR, 0},		/*52 - NOP*/
	{REG_ADDR, 0},		/*53 - NOP*/
	{REG_ADDR, 0},		/*54 -NOP*/
	{REG_ADDR, 0},		/*55 - NOP*/
	{REG_ADDR, 0},		/*56 -NOP*/
	{REG_ADDR, 0},		/*57 - NOP*/
	{REG_ADDR, 0},		/*58 -NOP*/
	{REG_ADDR, 0},		/*59 -NOP*/
	{REG_ADDR, 0},		/*60 - NOP*/
	{REG_ADDR, 0},		/*61 - NOP*/
	{REG_ADDR, 0},		/*62 - NOP*/
	{END, 0},		/*63 -END*/

};
/**
 * Offsets in the dummy Sequencer code
 */
#define DUMMY_SEQ_VO0_HW_SEQ_START_OFF	2
#define DUMMY_SEQ_VO0_SET1_OFFSET	21
#define DUMMY_SEQ_VO0_SET2_OFFSET	18

#define DUMMY_SEQ_VO1_HW_SEQ_START_OFF	2
#define DUMMY_SEQ_VO1_SET1_OFFSET	28
#define DUMMY_SEQ_VO1_ZERO_PTR_OFFSET	28
#define DUMMY_SEQ_VO1_SET2_OFFSET	24

#define HAWAII_EVENT_POLICY_OFFSET	{ \
	[LCDTE_EVENT]		= 0x0, \
	[SSP2SYN_EVENT]		= 0x4, \
	[SSP2DI_EVENT]		= 0x4, \
	[SSP2CK_EVENT]		= 0x4, \
	[SSP1SYN_EVENT]		= 0x10, \
	[SSP1DI_EVENT]		= 0x10, \
	[SSP1CK_EVENT]		= 0x10, \
	[SSP0SYN_EVENT]		= 0x1C, \
	[SSP0DI_EVENT]		= 0x1C, \
	[SSP0CK_EVENT]		= 0x1C, \
	[DIGCLKREQ_EVENT]	= 0x28, \
	[ANA_SYS_REQ_EVENT]	= 0x28, \
	[SYSCLKREQ_EVENT]	= 0x28, \
	[UBRX_EVENT]		= 0x34, \
	[UBCTSN_EVENT]		= 0x34, \
	[UB2RX_EVENT]		= 0x3C, \
	[UB2CTSN_EVENT]		= 0x3C, \
	[SIMDET_EVENT]		= 0x44, \
	[SIM2DET_EVENT]		= 0x44, \
	[MMC0D3_EVENT]		= 0x4C, \
	[MMC0D1_EVENT]		= 0x4C, \
	[MMC1D3_EVENT]		= 0x4C, \
	[MMC1D1_EVENT]		= 0x4C, \
	[SDDAT3_EVENT]		= 0x5C, \
	[SDDAT1_EVENT]		= 0x5C, \
	[SLB1CLK_EVENT]		= 0x64, \
	[SLB1DAT_EVENT]		= 0x64, \
	[SWCLKTCK_EVENT]	= 0x6C, \
	[SWDIOTMS_EVENT]	= 0x6C, \
	[KEY_R0_EVENT]		= 0x74, \
	[KEY_R1_EVENT]		= 0x74, \
	[KEY_R2_EVENT]		= 0x74, \
	[KEY_R3_EVENT]		= 0x74, \
	[KEY_R4_EVENT]		= 0x74, \
	[KEY_R5_EVENT]		= 0x74, \
	[KEY_R6_EVENT]		= 0x74, \
	[KEY_R7_EVENT]		= 0x74, \
	[DUMMY_EVENT1]		= INVALID_EVENT_OFFSET, \
	[DUMMY_EVENT2]		= INVALID_EVENT_OFFSET, \
	[MISC_WKP_EVENT]	= 0x94, \
	[BATRM_EVENT]		= 0xA0, \
	[USBDP_EVENT]		= 0xA4, \
	[USBDN_EVENT]		= 0xA4, \
	[RXD_EVENT]		= 0xAC, \
	[GPIO29_A_EVENT]	= 0xB0, \
	[GPIO32_A_EVENT]	= 0xB0, \
	[GPIO33_A_EVENT]	= 0xB0, \
	[GPIO43_A_EVENT]	= 0xB0, \
	[GPIO44_A_EVENT]	= 0xB0, \
	[GPIO45_A_EVENT]	= 0xB0, \
	[GPIO46_A_EVENT]	= 0xB0, \
	[GPIO47_A_EVENT]	= 0xB0, \
	[GPIO48_A_EVENT]	= 0xB0, \
	[GPIO71_A_EVENT]	= 0xB0, \
	[GPIO72_A_EVENT]	= 0xB0, \
	[GPIO73_A_EVENT]	= 0xB0, \
	[GPIO74_A_EVENT]	= 0xB0, \
	[GPIO95_A_EVENT]	= 0xB0, \
	[GPIO96_A_EVENT]	= 0xB0, \
	[GPIO99_A_EVENT]	= 0xB0, \
	[GPIO100_A_EVENT]	= 0xB0, \
	[GPIO111_A_EVENT]	= 0xB0, \
	[GPIO18_A_EVENT]	= 0xB0, \
	[GPIO19_A_EVENT]	= 0xB0, \
	[GPIO20_A_EVENT]	= 0xB0, \
	[GPIO89_A_EVENT]	= 0xB0, \
	[GPIO90_A_EVENT]	= 0xB0, \
	[GPIO91_A_EVENT]	= 0xB0, \
	[GPIO92_A_EVENT]	= 0xB0, \
	[GPIO93_A_EVENT]	= 0xB0, \
	[GPIO18_B_EVENT]	= 0x118, \
	[GPIO19_B_EVENT]	= 0x118, \
	[GPIO20_B_EVENT]	= 0x118, \
	[GPIO89_B_EVENT]	= 0x118, \
	[GPIO90_B_EVENT]	= 0x118, \
	[GPIO91_B_EVENT]	= 0x118, \
	[GPIO92_B_EVENT]	= 0x118, \
	[GPIO93_B_EVENT]	= 0x118, \
	[GPIO29_B_EVENT]	= 0x118, \
	[GPIO32_B_EVENT]	= 0x118, \
	[GPIO33_B_EVENT]	= 0x118, \
	[GPIO43_B_EVENT]	= 0x118, \
	[GPIO44_B_EVENT]	= 0x118, \
	[GPIO45_B_EVENT]	= 0x118, \
	[GPIO46_B_EVENT]	= 0x118, \
	[GPIO47_B_EVENT]	= 0x118, \
	[GPIO48_B_EVENT]	= 0x118, \
	[GPIO71_B_EVENT]	= 0x118, \
	[GPIO72_B_EVENT]	= 0x118, \
	[GPIO73_B_EVENT]	= 0x118, \
	[GPIO74_B_EVENT]	= 0x118, \
	[GPIO95_B_EVENT]	= 0x118, \
	[GPIO96_B_EVENT]	= 0x118, \
	[GPIO99_B_EVENT]	= 0x118, \
	[GPIO100_B_EVENT]	= 0x118, \
	[GPIO111_B_EVENT]	= 0x118, \
	[COMMON_TIMER_0_EVENT]	= 0x180, \
	[COMMON_TIMER_1_EVENT]	= 0x184, \
	[COMMON_TIMER_2_EVENT]	= 0x188, \
	[COMMON_TIMER_3_EVENT]	= 0x18C, \
	[COMMON_TIMER_4_EVENT]	= 0x190, \
	[COMMON_INT_TO_AC_EVENT] = 0x194, \
	[TZCFG_INT_TO_AC_EVENT]	= 0x198, \
	[DMA_REQUEST_EVENT]	= 0x19C, \
	[MODEM1_EVENT]		= 0x1A0, \
	[MODEM2_EVENT]		= 0x1A4, \
	[SD1DAT1DAT3_EVENT]	= 0x1A8, \
	[BRIDGE_TO_AC_EVENT]	= 0x1AC, \
	[BRIDGE_TO_MODEM_EVENT]	= 0x1B0, \
	[VREQ_NONZERO_PI_MODEM_EVENT]	= 0x1B4, \
	[DUMMY_EVENT3]		= INVALID_EVENT_OFFSET, \
	[USBOTG_EVENT]		= 0x1BC, \
	[GPIO_EXP_IRQ_EVENT]	= 0x1C0, \
	[DBR_IRQ_EVENT]		= 0x1C4, \
	[ACI_EVENT]		= 0x1C8, \
	[PHY_RESUME_EVENT]	= 0x1CC, \
	[MODEMBUS_ACTIVE_EVENT]	= 0x1D0, \
	[SOFTWARE_0_EVENT]	= 0x1D4, \
	[SOFTWARE_1_EVENT]	= 0x1D8, \
	[SOFTWARE_2_EVENT]	= 0x1DC, \
}

struct pwr_mgr_info __pwr_mgr_info = {
	.num_pi = PI_MGR_PI_ID_MAX,
	.base_addr = KONA_PWRMGR_VA,
	.flags = PM_PMU_I2C | I2C_SIMULATE_BURST_MODE,
	.pwrmgr_intr = BCM_INT_ID_PWR_MGR,
	.event_policy_offset = HAWAII_EVENT_POLICY_OFFSET,
};

static int __init hawaii_pwr_mgr_init(void)
{
	struct pm_policy_cfg cfg;
#ifdef CONFIG_KONA_AVS
	int update_vlt_tbl = 0;
#else
	int update_vlt_tbl = 1;
#endif
	int i;
	u32 reg_val;
	int insurance = 1000;
	struct pi *pi;
	struct v0x_spec_i2c_cmd_ptr dummy_seq_v0_ptr = {
		.other_ptr = DUMMY_SEQ_VO0_HW_SEQ_START_OFF,
		.set2_val = VLT_ID_WAKEUP,
		.set2_ptr = DUMMY_SEQ_VO0_SET2_OFFSET,
		.set1_val = VLT_ID_RETN,
		.set1_ptr = DUMMY_SEQ_VO0_SET1_OFFSET,
		.zerov_ptr = DUMMY_SEQ_VO0_SET1_OFFSET,
	};

	struct v0x_spec_i2c_cmd_ptr dummy_seq_v1_ptr = {
		.other_ptr = DUMMY_SEQ_VO1_HW_SEQ_START_OFF,
		.set2_val = VLT_ID_WAKEUP,
		.set2_ptr = DUMMY_SEQ_VO1_SET2_OFFSET,
		.set1_val = VLT_ID_RETN,
		.set1_ptr = DUMMY_SEQ_VO1_SET1_OFFSET,
		.zerov_ptr = DUMMY_SEQ_VO1_ZERO_PTR_OFFSET,
	};

	cfg.ac = 1;
	cfg.atl = 0;
	pm_params_init();
	/**
	 * Load the dummy sequencer during power manager initialization
	 * Actual sequencer will be loaded during pmu i2c driver
	 * initialisation
	 */
	__pwr_mgr_info.i2c_cmds = i2c_dummy_seq_cmd;
	__pwr_mgr_info.num_i2c_cmds = ARRAY_SIZE(i2c_dummy_seq_cmd);
	__pwr_mgr_info.i2c_cmd_ptr[VOLT0] = &dummy_seq_v0_ptr;
	__pwr_mgr_info.i2c_cmd_ptr[VOLT1] = &dummy_seq_v1_ptr;

	__pwr_mgr_info.i2c_var_data = pwrmgr_init_param.def_vlt_tbl;
	__pwr_mgr_info.num_i2c_var_data = pwrmgr_init_param.vlt_tbl_size;

	pwr_mgr_init(&__pwr_mgr_info);

	hawaii_pi_mgr_init();

#ifdef CONFIG_MM_POWER_OK_ERRATUM
/* it was observed that if MM CCU is switched to and
	from shutdown state, it would break the DDR self refresh.
	Recommended workaround is to set the POWER_OK_MASK bit to 0 */
	if (is_pm_erratum(ERRATUM_MM_POWER_OK))
		pwr_mgr_ignore_power_ok_signal(false);
#endif

#if defined(CONFIG_PLL1_8PHASE_OFF_ERRATUM) ||\
		defined(CONFIG_MM_FREEZE_VAR500M_ERRATUM)
	writel(0x00A5A501, KONA_ROOT_CLK_VA +
			ROOT_CLK_MGR_REG_WR_ACCESS_OFFSET);
	if (is_pm_erratum(ERRATUM_PLL1_8PHASE_OFF)) {
		reg_val = readl(KONA_ROOT_CLK_VA
				+ ROOT_CLK_MGR_REG_PLL1CTRL0_OFFSET);
		reg_val |= ROOT_CLK_MGR_REG_PLL1CTRL0_PLL1_8PHASE_EN_MASK;
		/*Enable 8ph bit in pll 1*/
		do {
			writel(reg_val, KONA_ROOT_CLK_VA
				+ ROOT_CLK_MGR_REG_PLL1CTRL0_OFFSET);
			insurance--;
		} while (!(readl(KONA_ROOT_CLK_VA +
			ROOT_CLK_MGR_REG_PLL1CTRL0_OFFSET) &
			ROOT_CLK_MGR_REG_PLL1CTRL0_PLL1_8PHASE_EN_MASK) &&
			insurance);
		BUG_ON(insurance == 0);
	}
	if (is_pm_erratum(ERRATUM_MM_FREEZE_VAR500M))
		var500m_clk_en_override(true);

	writel(0, KONA_ROOT_CLK_VA + ROOT_CLK_MGR_REG_WR_ACCESS_OFFSET);
#endif

	/*MM override is not set by default */
	pwr_mgr_pi_set_wakeup_override(PI_MGR_PI_ID_MM, false /*clear */);

	/*Done in 3 steps to skip DUMMY_EVENTs */
	pwr_mgr_event_clear_events(LCDTE_EVENT, KEY_R7_EVENT);
	pwr_mgr_event_clear_events(MISC_WKP_EVENT, VREQ_NONZERO_PI_MODEM_EVENT);
	pwr_mgr_event_clear_events(USBOTG_EVENT, EVENT_ID_ALL);

	pwr_mgr_event_set(SOFTWARE_2_EVENT, 1);
	pwr_mgr_event_set(SOFTWARE_0_EVENT, 1);

	pwr_mgr_set_pc_sw_override(PC0, false, 0);
	pwr_mgr_set_pc_clkreq_override(PC0, true, 1);

	/* make sure that PC1, PC2 software override
	 * is cleared so that sequencer can toggle these
	 * pins during retention/off/wakeup
	 */
	pwr_mgr_set_pc_sw_override(PC1, false, 0);
	pwr_mgr_set_pc_sw_override(PC2, false, 0);
	pwr_mgr_set_pc_sw_override(PC3, false, 0);
	pwr_mgr_set_pc_clkreq_override(PC1, false, 0);
	pwr_mgr_set_pc_clkreq_override(PC2, false, 0);
	pwr_mgr_set_pc_clkreq_override(PC3, false, 0);

	pwr_mgr_pm_i2c_enable(true);
	/*Init event table */
	for (i = 0; i < ARRAY_SIZE(__event_table); i++) {
		pwr_mgr_event_trg_enable(__event_table[i].event_id,
					 __event_table[i].trig_type);

		cfg.policy = __event_table[i].policy_modem;
		pwr_mgr_event_set_pi_policy(__event_table[i].event_id,
					    PI_MGR_PI_ID_MODEM, &cfg);

		cfg.policy = __event_table[i].policy_arm;
		pwr_mgr_event_set_pi_policy(__event_table[i].event_id,
					    PI_MGR_PI_ID_ARM_CORE, &cfg);

		cfg.policy = __event_table[i].policy_arm_sub;
		pwr_mgr_event_set_pi_policy(__event_table[i].event_id,
					    PI_MGR_PI_ID_ARM_SUB_SYSTEM, &cfg);

		cfg.policy = __event_table[i].policy_aon;
		pwr_mgr_event_set_pi_policy(__event_table[i].event_id,
					    PI_MGR_PI_ID_HUB_AON, &cfg);

		cfg.policy = __event_table[i].policy_hub;
		pwr_mgr_event_set_pi_policy(__event_table[i].event_id,
					    PI_MGR_PI_ID_HUB_SWITCHABLE, &cfg);

		cfg.policy = __event_table[i].policy_mm;
		pwr_mgr_event_set_pi_policy(__event_table[i].event_id,
					    PI_MGR_PI_ID_MM, &cfg);
	}
	/*Init all PIs */

	for (i = 0; i < PI_MGR_PI_ID_MODEM; i++) {
		pi = pi_mgr_get(i);
		BUG_ON(pi == NULL);
		pi_init(pi);
	}

	/*init clks */
	__clock_init();

#ifdef CONFIG_PWRMGR_1P2GHZ_OPS_SET_SELECT
	mach_config_a9_pll(CONFIG_A9_PLL_2P4GHZ, update_vlt_tbl);
#else
	mach_config_a9_pll(CONFIG_A9_PLL_2GHZ, update_vlt_tbl);
#endif

	return 0;
}

early_initcall(hawaii_pwr_mgr_init);

#ifdef CONFIG_DEBUG_FS

void pwr_mgr_mach_debug_fs_init(int type, int db_mux, int mux_param,
		u32 dbg_bit_sel)
{
	if (db_mux == 0)	/*GPIO ? */
		mux_param = (type == 0) ? DBG_BUS_PM_DBG_BUS_SEL
				: DBG_BUS_BMDB_DBG_BUS_SEL;

	debug_bus_mux_sel(db_mux, mux_param, dbg_bit_sel);
}

#endif /*CONFIG_DEBUG_FS */


int hawaii_pwr_mgr_delayed_init(void)
{
	int i;
	struct pi *pi;

	/*All the initializations are done. Clear override bit here so that
	 * appropriate policies take effect*/
	for (i = 0; i < PI_MGR_PI_ID_MODEM; i++) {
		pi = pi_mgr_get(i);
		BUG_ON(pi == NULL);
		pi_init_state(pi);
	}

#ifdef CONFIG_IGNORE_DAP_POWERUP_REQ
	pwr_mgr_ignore_dap_powerup_request(true);
	pwr_mgr_ignore_mdm_dap_powerup_req(true);
#endif
	delayed_init_complete = 1;
	return 0;
}

#ifdef CONFIG_DELAYED_PM_INIT
static int param_set_pm_late_init(const char *val,
			const struct kernel_param *kp)
{
	int ret = -1;
	int pm_delayed_init = 0;

	pr_info("%s\n", __func__);
	if (delayed_init_complete)
		return 0;
	if (!val)
		return -EINVAL;

	/* coverity[secure_coding] */
	ret = sscanf(val, "%d", &pm_delayed_init);
	pr_info("%s, pm_delayed_init:%d\n", __func__, pm_delayed_init);
	if (pm_delayed_init == 1)
		hawaii_pwr_mgr_delayed_init();
/* For sequencer to run with the update voltage table. During boot MM will be
   running at wakeup policy at 1.3V(set by loader), at turbo OPP.
   There will be no sequencer at that point. After that voltage tables will be
   setup, sequencer will be enabled. Once boot completes MM will be at DFS.
   But it will still be running at turbo, so new voltage req won't go to PMU.
   Therefore for the voltage to get updated we limit the OPP to normal and
   set it back to turbo. Turbo voltage will change from 1.3 to 1.2 or less */

	pi_mgr_set_dfs_opp_limit(PI_MGR_PI_ID_MM, PI_OPP_ECONOMY,
					PI_OPP_NORMAL);
#ifdef CONFIG_PI_MGR_MM_STURBO_ENABLE
	pi_mgr_set_dfs_opp_limit(PI_MGR_PI_ID_MM, PI_OPP_ECONOMY,
					PI_OPP_SUPER_TURBO);
#else
	pi_mgr_set_dfs_opp_limit(PI_MGR_PI_ID_MM, PI_OPP_ECONOMY,
					PI_OPP_TURBO);
#endif

	kona_pm_disable_idle_state(CSTATE_ALL, 0);
	return 0;
}
#endif

int __init hawaii_pwr_mgr_late_init(void)
{
#ifdef CONFIG_DELAYED_PM_INIT
	if (is_charging_state()) {
		pr_info("%s: power off charging, complete int here\n",
						__func__);
		hawaii_pwr_mgr_delayed_init();
	} else
		kona_pm_disable_idle_state(CSTATE_ALL, 1);
#else
	hawaii_pwr_mgr_delayed_init();
#endif
#ifdef CONFIG_DEBUG_FS
	return pwr_mgr_debug_init(KONA_BMDM_PWRMGR_VA);
#endif
	return 0;
}

late_initcall(hawaii_pwr_mgr_late_init);

/**
 * Initialize the real sequencer
 * On return PWRMGR I2C block will be
 * enabled
 */

int __devinit mach_init_sequencer(void)
{

#ifndef CONFIG_PWRMGR_DUMMY_SEQUENCER
	pr_info("%s\n", __func__);

	__pwr_mgr_info.i2c_rd_off = pwrmgr_init_param.i2c_rd_off;
	__pwr_mgr_info.i2c_rd_slv_id_off1 =
	    pwrmgr_init_param.i2c_rd_slv_id_off1;
	__pwr_mgr_info.i2c_rd_slv_id_off2 =
	    pwrmgr_init_param.i2c_rd_slv_id_off2;
	__pwr_mgr_info.i2c_rd_reg_addr_off =
	    pwrmgr_init_param.i2c_rd_reg_addr_off;
	__pwr_mgr_info.i2c_wr_off = pwrmgr_init_param.i2c_wr_off;
	__pwr_mgr_info.i2c_wr_slv_id_off =
	    pwrmgr_init_param.i2c_wr_slv_id_off;
	__pwr_mgr_info.i2c_wr_reg_addr_off =
	    pwrmgr_init_param.i2c_wr_reg_addr_off;
	__pwr_mgr_info.i2c_wr_val_addr_off =
	    pwrmgr_init_param.i2c_wr_val_addr_off;
	__pwr_mgr_info.i2c_seq_timeout = pwrmgr_init_param.i2c_seq_timeout;
#ifdef CONFIG_KONA_PWRMGR_SWSEQ_FAKE_TRG_ERRATUM
	__pwr_mgr_info.pc_toggle_off = pwrmgr_init_param.pc_toggle_off;
#endif

	__pwr_mgr_info.i2c_cmds = pwrmgr_init_param.cmd_buf;
	__pwr_mgr_info.num_i2c_cmds = pwrmgr_init_param.cmd_buf_size;
	__pwr_mgr_info.i2c_cmd_ptr[VOLT0] = pwrmgr_init_param.v0xptr[0];
	__pwr_mgr_info.i2c_cmd_ptr[VOLT1] = pwrmgr_init_param.v0xptr[1];
	pwr_mgr_init_sequencer(&__pwr_mgr_info);

	/* pwr_mgr_init_sequencer function will disable the sequencer
	   re-enable the power manager i2c sequencer */
	pwr_mgr_pm_i2c_enable(true);
#endif /*CONFIG_PWRMGR_DUMMY_SEQUENCER*/
	return 0;
}
EXPORT_SYMBOL(mach_init_sequencer);
