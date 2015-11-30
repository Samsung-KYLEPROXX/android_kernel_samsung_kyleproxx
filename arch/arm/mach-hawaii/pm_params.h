/*****************************************************************************
*
* Power Manager config parameters for Rhea platform
*
* Copyright 2010 Broadcom Corporation.  All rights reserved.
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

#ifndef __PM_PARAMS_H__
#define __PM_PARAMS_H__

#include <plat/pwr_mgr.h>

#define ARRAY_LIST(...) {__VA_ARGS__}
#define SR_VLT_LUT_SIZE 16

#define VLT_ID_OFF		0x0
#define VLT_ID_RETN		0x1
#define VLT_ID_WAKEUP		0x2
#define VLT_ID_A9_SYSPLL_WFI	0x7
#define VLT_ID_A9_ECO		0x8
#define VLT_ID_OTHER_ECO	0x9
#define VLT_ID_A9_NORMAL	0xA
#define VLT_ID_OTHER_NORMAL	0xB
#define VLT_ID_A9_TURBO		0xC
#define VLT_ID_OTHER_TURBO	0xD
#define VLT_ID_A9_SUPER_TURBO		0xE
#define VLT_ID_OTHER_SUPER_TURBO	0xF

#define ACTIVE_VOLTAGE_OFFSET	7

#define A9_FREQ_NORMAL_DIV	4
#define A9_FREQ_TURBO_DIV	3

#define PROC_CCU_FREQ_ID_XTAL		0
#define PROC_CCU_FREQ_ID_52MHZ		1
#define PROC_CCU_FREQ_ID_156MHZ		2
#define PROC_CCU_FREQ_ID_ECO		4
#define PROC_CCU_FREQ_ID_NRML		6
#define PROC_CCU_FREQ_ID_TURBO		6
#define PROC_CCU_FREQ_ID_SUPER_TURBO	7

#define CPU_FREQ_ID_SYSPLL_WFI		PROC_CCU_FREQ_ID_ECO

#define MM_CCU_FREQ_ID_ECO		2
#define MM_CCU_FREQ_ID_NRML		4
#define MM_CCU_FREQ_ID_TURBO		5
#define MM_CCU_FREQ_ID_SUPER_TURBO	6

#define HUB_CCU_FREQ_ID_ECO	2
#define HUB_CCU_FREQ_ID_NRML	2

#define AON_CCU_FREQ_ID_ECO	1
#define AON_CCU_FREQ_ID_NRML	2

#define KPM_CCU_FREQ_ID_ECO	2
#define KPM_CCU_FREQ_ID_NRML	3

#define KPS_CCU_FREQ_ID_ECO	1
#define KPS_CCU_FREQ_ID_NRML	3

#define VLT_NORMAL_PERI		VLT_ID_OTHER_ECO
#define VLT_HIGH_PERI		VLT_ID_OTHER_TURBO

#define VLT_NORMAL_PERI_MDM	VLT_ID_OTHER_ECO
#define VLT_HIGH_PERI_MDM	VLT_ID_OTHER_NORMAL

#if (CPU_FREQ_ID_SYSPLL_WFI < PROC_CCU_FREQ_ID_ECO)
#define INIT_A9_VLT_TABLE(WFI, ECO, NM, TURBO, STURBO) \
				[VLT_ID_A9_SYSPLL_WFI] = WFI,\
				[VLT_ID_A9_ECO] = ECO, \
				[VLT_ID_A9_NORMAL] = NM, \
				[VLT_ID_A9_TURBO] = TURBO, \
				[VLT_ID_A9_SUPER_TURBO] = STURBO
#else
#define INIT_A9_VLT_TABLE(WFI, ECO, NM, TURBO, STURBO) \
				[VLT_ID_A9_SYSPLL_WFI] = ECO,\
				[VLT_ID_A9_ECO] = ECO, \
				[VLT_ID_A9_NORMAL] = NM, \
				[VLT_ID_A9_TURBO] = TURBO, \
				[VLT_ID_A9_SUPER_TURBO] = STURBO
#endif

#define INIT_OTHER_VLT_TABLE(ECO, NM, TURBO, STURBO) \
				[VLT_ID_OTHER_ECO] = ECO, \
				[VLT_ID_OTHER_NORMAL] = NM, \
				[VLT_ID_OTHER_TURBO] = TURBO, \
				[VLT_ID_OTHER_SUPER_TURBO] = STURBO

#define INIT_LPM_VLT_IDS(off, ret, wakeup) \
				[VLT_ID_OFF]	= off, \
				[VLT_ID_RETN]	= ret, \
				[VLT_ID_WAKEUP]	= wakeup

#define INIT_UNUSED_VLT_IDS(init_val) \
				[0x3] =		init_val, \
				[0x4] =		init_val, \
				[0x5] =		init_val, \
				[0x6] =		init_val,

#ifdef CONFIG_CPU_SYSPLL_WFI_CSTATE
#if (CPU_FREQ_ID_SYSPLL_WFI == PROC_CCU_FREQ_ID_XTAL)
#define PROC_CCU_FREQ_VOLT_TBL	\
		ARRAY_LIST(VLT_ID_A9_SYSPLL_WFI, VLT_ID_A9_ECO,\
			VLT_ID_A9_ECO, VLT_ID_A9_ECO, VLT_ID_A9_ECO,\
			VLT_ID_A9_ECO, VLT_ID_A9_TURBO, VLT_ID_A9_SUPER_TURBO)
#elif (CPU_FREQ_ID_SYSPLL_WFI == PROC_CCU_FREQ_ID_52MHZ)
#define PROC_CCU_FREQ_VOLT_TBL	\
		ARRAY_LIST(VLT_ID_A9_ECO, VLT_ID_A9_SYSPLL_WFI,\
			VLT_ID_A9_ECO, VLT_ID_A9_ECO, VLT_ID_A9_ECO,\
			VLT_ID_A9_ECO, VLT_ID_A9_TURBO, VLT_ID_A9_SUPER_TURBO)
#elif (CPU_FREQ_ID_SYSPLL_WFI == PROC_CCU_FREQ_ID_156MHZ)
#define PROC_CCU_FREQ_VOLT_TBL	\
		ARRAY_LIST(VLT_ID_A9_ECO, VLT_ID_A9_ECO,\
			VLT_ID_A9_SYSPLL_WFI, VLT_ID_A9_ECO, VLT_ID_A9_ECO,\
			VLT_ID_A9_ECO, VLT_ID_A9_TURBO, VLT_ID_A9_SUPER_TURBO)
#elif (CPU_FREQ_ID_SYSPLL_WFI == PROC_CCU_FREQ_ID_ECO)
#define PROC_CCU_FREQ_VOLT_TBL	\
		ARRAY_LIST(VLT_ID_A9_ECO, VLT_ID_A9_ECO,\
			VLT_ID_A9_ECO, VLT_ID_A9_ECO, VLT_ID_A9_ECO,\
			VLT_ID_A9_ECO, VLT_ID_A9_TURBO, VLT_ID_A9_SUPER_TURBO)
#endif /* CPU_FREQ_ID_SYSPLL_WFI */
#else /* !defined(CONFIG_CPU_SYSPLL_WFI_CSTATE) */
#define PROC_CCU_FREQ_VOLT_TBL	\
		ARRAY_LIST(VLT_ID_A9_ECO, VLT_ID_A9_ECO,\
			VLT_ID_A9_ECO, VLT_ID_A9_ECO, VLT_ID_A9_ECO,\
			VLT_ID_A9_ECO, VLT_ID_A9_TURBO, VLT_ID_A9_SUPER_TURBO)
#endif /* CONFIG_CPU_SYSPLL_WFI_CSTATE */

#define PROC_CCU_FREQ_VOLT_TBL_SZ	8

#define MM_CCU_FREQ_VOLT_TBL	\
		ARRAY_LIST(VLT_ID_OTHER_ECO, VLT_ID_OTHER_ECO,\
			VLT_ID_OTHER_ECO, VLT_ID_OTHER_ECO,\
			VLT_ID_OTHER_NORMAL, VLT_ID_OTHER_TURBO,\
			 VLT_ID_OTHER_SUPER_TURBO)
#define MM_CCU_FREQ_VOLT_TBL_SZ		7

#define HUB_CCU_FREQ_VOLT_TBL	\
		ARRAY_LIST(VLT_ID_OTHER_ECO, VLT_ID_OTHER_ECO,\
			VLT_ID_OTHER_ECO, VLT_ID_OTHER_ECO,\
			VLT_ID_OTHER_ECO, VLT_ID_OTHER_ECO, VLT_ID_OTHER_ECO)
#define HUB_CCU_FREQ_VOLT_TBL_SZ	7

/*AON is on fixed voltage domain. Voltage ids does not really matter*/
#define AON_CCU_FREQ_VOLT_TBL	\
		ARRAY_LIST(VLT_ID_OTHER_ECO, VLT_ID_OTHER_ECO,\
			VLT_ID_OTHER_NORMAL, VLT_ID_OTHER_NORMAL,\
			VLT_ID_OTHER_NORMAL)
#define AON_CCU_FREQ_VOLT_TBL_SZ	5

#define KPS_CCU_FREQ_VOLT_TBL	\
		ARRAY_LIST(VLT_ID_OTHER_ECO, VLT_ID_OTHER_ECO,\
			VLT_ID_OTHER_ECO, VLT_ID_OTHER_NORMAL,\
			VLT_ID_OTHER_NORMAL, VLT_ID_OTHER_NORMAL)
#define KPS_CCU_FREQ_VOLT_TBL_SZ	6

#define KPM_CCU_FREQ_VOLT_TBL	\
		ARRAY_LIST(VLT_ID_OTHER_ECO, VLT_ID_OTHER_ECO,\
			VLT_ID_OTHER_ECO, VLT_ID_OTHER_NORMAL,\
			VLT_ID_OTHER_NORMAL, VLT_ID_OTHER_NORMAL,\
			VLT_ID_OTHER_NORMAL, VLT_ID_OTHER_NORMAL)
#define KPM_CCU_FREQ_VOLT_TBL_SZ	8

#define BMDM_CCU_FREQ_VOLT_TBL	\
		ARRAY_LIST(VLT_ID_OTHER_ECO, VLT_ID_OTHER_ECO,\
			VLT_ID_OTHER_ECO, VLT_ID_OTHER_ECO,\
			VLT_ID_OTHER_ECO, VLT_ID_OTHER_NORMAL,\
			VLT_ID_OTHER_TURBO, VLT_ID_OTHER_TURBO)
/* As per RDB there are only 6 valid freq_ids for bmdm. this count
   is used only to initialize voltage table from AP */
#define BMDM_CCU_FREQ_VOLT_TBL_SZ	8

#define DSP_CCU_FREQ_VOLT_TBL	\
		ARRAY_LIST(VLT_ID_OTHER_ECO, VLT_ID_OTHER_ECO,\
			VLT_ID_OTHER_ECO, VLT_ID_OTHER_NORMAL,\
			VLT_ID_OTHER_TURBO, VLT_ID_OTHER_TURBO,\
			VLT_ID_OTHER_TURBO, VLT_ID_OTHER_TURBO)
/* As per RDB there are only 5 valid freq_ids for dsp. this count
   is used only to initialize voltage table from AP */
#define DSP_CCU_FREQ_VOLT_TBL_SZ	8

#define PROC_CCU_FREQ_POLICY_TBL	\
		ARRAY_LIST(PROC_CCU_FREQ_ID_ECO, PROC_CCU_FREQ_ID_ECO,\
			PROC_CCU_FREQ_ID_ECO, PROC_CCU_FREQ_ID_SUPER_TURBO)
#define MM_CCU_FREQ_POLICY_TBL	\
		ARRAY_LIST(MM_CCU_FREQ_ID_ECO, MM_CCU_FREQ_ID_ECO,\
			MM_CCU_FREQ_ID_ECO, MM_CCU_FREQ_ID_TURBO)
#define HUB_CCU_FREQ_POLICY_TBL	\
		ARRAY_LIST(HUB_CCU_FREQ_ID_ECO, HUB_CCU_FREQ_ID_ECO,\
			HUB_CCU_FREQ_ID_ECO, HUB_CCU_FREQ_ID_NRML)
#define AON_CCU_FREQ_POLICY_TBL	\
		ARRAY_LIST(AON_CCU_FREQ_ID_ECO, AON_CCU_FREQ_ID_ECO,\
			AON_CCU_FREQ_ID_ECO, AON_CCU_FREQ_ID_NRML)
#define KPM_CCU_FREQ_POLICY_TBL	\
		ARRAY_LIST(KPM_CCU_FREQ_ID_ECO, KPM_CCU_FREQ_ID_ECO,\
			KPM_CCU_FREQ_ID_ECO, KPM_CCU_FREQ_ID_NRML)
#define KPS_CCU_FREQ_POLICY_TBL	\
		ARRAY_LIST(KPS_CCU_FREQ_ID_ECO, KPS_CCU_FREQ_ID_ECO,\
			KPS_CCU_FREQ_ID_ECO, KPS_CCU_FREQ_ID_NRML)



/*PM Eratta ids*/
#define ERRATUM_MM_V3D_TIMEOUT		(1 << 0)
#define ERRATUM_MM_POWER_OK		(1 << 1)
#define ERRATUM_PLL1_8PHASE_OFF		(1 << 2)
#define ERRATUM_MM_FREEZE_VAR500M	(1 << 3)

#ifdef CONFIG_KONA_POWER_MGR
struct pwrmgr_init_param {
	struct i2c_cmd *cmd_buf;
	u32 cmd_buf_size;
	struct v0x_spec_i2c_cmd_ptr *v0xptr[V_SET_MAX];
	u8 *def_vlt_tbl;
	u32 vlt_tbl_size;
	u32 i2c_rd_off;
	int i2c_rd_slv_id_off1;	/*slave id offset -  write reg address */
	int i2c_rd_slv_id_off2;	/*slave id offset - read reg value */
	int i2c_rd_reg_addr_off;
	int i2c_rd_nack_off;
	int i2c_rd_nack_jump_off;
	int i2c_rd_fifo_off;
	u32 i2c_wr_off;
	int i2c_wr_slv_id_off;
	int i2c_wr_reg_addr_off;
	int i2c_wr_val_addr_off;
	u32 i2c_seq_timeout;	/*timeout in ms */
#ifdef CONFIG_KONA_PWRMGR_SWSEQ_FAKE_TRG_ERRATUM
	int pc_toggle_off;
#endif
};

extern struct pwrmgr_init_param pwrmgr_init_param;

#endif	/*CONFIG_KONA_POWER_MGR */

/*This API should be defined in appropriate PMU board file*/
bool is_pm_erratum(u32 erratum);
int __init pm_params_init(void);
int pm_init_pmu_sr_vlt_map_table(u32 silicon_type, int freq_id);
extern int bcmpmu_init_sr_volt(void);

enum {
	DDR_FREQ_400M,
	DDR_FREQ_450M,
	DDR_FREQ_MAX,
};

#define CONFIG_A9_PLL_2GHZ	1
#define CONFIG_A9_PLL_2P4GHZ	2
#define CONFIG_A9_PLL_2P8GHZ	3
/*Wake up PM policy*/
#define PM_WKP          7
int mach_config_a9_pll(int turbo_val, int update_volt_tbl);

#endif	/*__PM_PARAMS_H__*/

