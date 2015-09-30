/*******************************************************************************
* Copyright 2011 Broadcom Corporation.	All rights reserved.
*
* @file drivers/video/broadcom/dispdrv_s6e63m0x_dsi.c
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
#ifdef __KERNEL__

#include <linux/string.h>
#include <linux/broadcom/mobcom_types.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <plat/osabstract/osinterrupt.h>
#include <plat/osabstract/ostask.h>
#include <plat/csl/csl_dma_vc4lite.h>
#include <plat/dma_drv.h>
#include <plat/pi_mgr.h>
#include <mach/io_map.h>
#include <mach/memory.h>
#include <asm/io.h>
#include <video/kona_fb_boot.h>	   /* dispdrv init interface */
#include "display_drv.h"	   /* dispdrv interface	*/
#include <plat/csl/csl_lcd.h>	   /* dispdrv CSL Common Interface */
#include <plat/csl/csl_dsi.h>	   /* DSI CSL API */
#include "dispdrv_mipi_dcs.h"	   /* MIPI DCS */
#include "dispdrv_mipi_dsi.h"	   /* MIPI DSI */

#include "dispdrv_common.h"	   /* dispdrv common */
#include "lcd_clock.h"
#include <plat/csl/csl_tectl_vc4lite.h>
#include <linux/regulator/consumer.h>

#else

#include <stdio.h>
#include <string.h>
#include "dbg.h"
#include "mobcom_types.h"
#include "chip_version.h"
#include "gpio.h"
#include "platform_config.h"
#include "irqctrl.h"
#include "osinterrupt.h"
#include "ostask.h"
#include "dbg.h"
#include "logapi.h"
#include "dma_drv.h"
#include "display_drv.h"	   /* dispdrv interface	*/
#include "csl_lcd.h"		   /* dispdrv CSL Common Interface */
#include "csl_dsi.h"		   /* DSI CSL API */
#include "dispdrv_mipi_dcs.h"	   /* MIPI DCS */
#include "dispdrv_mipi_dsi.h"	   /* MIPI DSI */
#endif

#include "lcd_S6E63M0X.h"

#define S6E63M0X_VC		(0)
#define S6E63M0X_CMND_IS_LP TRUE /* display init comm LP or HS mode */


#if 1
#define S6E63M0X_LOG(id, fmt, args...) printk(KERN_ERR fmt, ##args)
#else
#define S6E63M0X_LOG(id, fmt, args...)
#endif

#define TE_SCAN_LINE 800

typedef struct {
	CSL_LCD_HANDLE clientH;
	CSL_LCD_HANDLE dsiCmVcHandle;
	DISP_DRV_STATE drvState;
	DISP_PWR_STATE pwrState;
	UInt32 busNo;
	UInt32 teIn;
	UInt32 teOut;
	Boolean isTE;
	DISPDRV_WIN_t win_dim;
	DISPDRV_WIN_t win_cur;
	struct pi_mgr_dfs_node	*dfs_node;
	/* --- */
	Boolean boot_mode;
	UInt32 rst_bridge_pwr_down;
	UInt32 rst_bridge_reset;
	UInt32 rst_panel_reset;
	CSL_DSI_CM_VC_t		*cmnd_mode;
	CSL_DSI_CFG_t		*dsi_cfg;
	DISPDRV_INFO_T		*disp_info;
} S6E63M0X_PANEL_t;

#if 0
/* LOCAL FUNCTIONs */
static void S6E63M0X_WrCmndP0(
	DISPDRV_HANDLE_T drvH,
	UInt32 reg);
#endif

/* DRV INTERFACE FUNCTIONs */
static Int32 S6E63M0X_Init(
	struct dispdrv_init_parms	*parms,
	DISPDRV_HANDLE_T		*handle);

static Int32 S6E63M0X_Exit(DISPDRV_HANDLE_T drvH);

static Int32 S6E63M0X_Open(DISPDRV_HANDLE_T drvH);

static Int32 S6E63M0X_Close(DISPDRV_HANDLE_T drvH);

static void S6E63M0X_ExecCmndList(
	DISPDRV_HANDLE_T     drvH,
	pDISPCTRL_REC_T cmnd_lst);

static Int32 S6E63M0X_GetDispDrvFeatures(
	DISPDRV_HANDLE_T drvH,
	const char			**driver_name,
	UInt32				*version_major,
	UInt32				*version_minor,
	DISPDRV_SUPPORT_FEATURES_T	*flags);

const DISPDRV_INFO_T *S6E63M0X_GetDispDrvData(DISPDRV_HANDLE_T drvH);

static Int32 S6E63M0X_Start(
	DISPDRV_HANDLE_T drvH,
	struct pi_mgr_dfs_node *dfs_node);

static Int32 S6E63M0X_Stop(
	DISPDRV_HANDLE_T drvH,
	struct pi_mgr_dfs_node *dfs_node);

static Int32 S6E63M0X_PowerControl(
	DISPDRV_HANDLE_T drvH,
	DISPLAY_POWER_STATE_T state);

static Int32 S6E63M0X_Atomic_Update(
	DISPDRV_HANDLE_T drvH,
	void			*buff,
	DISPDRV_WIN_t		*p_win);

static Int32 S6E63M0X_Update(
	 DISPDRV_HANDLE_T drvH,
	 void			*buff,
	 DISPDRV_WIN_t		*p_win,
	 DISPDRV_CB_T apiCb);

static Int32 S6E63M0X_WinReset(DISPDRV_HANDLE_T drvH);

static int S6E63M0X_DCS_Read(DISPDRV_HANDLE_T drvH, UInt8 reg, UInt8 *rxBuff, UInt8 buffLen);

static int S6E63M0X_SetMaxRtnPktSize(DISPDRV_HANDLE_T drvH, UInt8 size);

static DISPDRV_T S6E63M0X_Drv = {
	&S6E63M0X_Init,		/* init	*/
	&S6E63M0X_Exit,		/* exit	*/
	&S6E63M0X_GetDispDrvFeatures,/* info	*/
	&S6E63M0X_Open,		/* open	*/
	&S6E63M0X_Close,		/* close */
	&S6E63M0X_GetDispDrvData,	/* get_info */
	&S6E63M0X_Start,		/* start */
	&S6E63M0X_Stop,		/* stop	*/
	&S6E63M0X_PowerControl,	/* power_control */
	&S6E63M0X_Atomic_Update,	/* update_no_os	*/
	&S6E63M0X_Update,		/* update */
	NULL,				/* set_brightness */
	&S6E63M0X_WinReset,		/* reset_win */
};

#if 1
static struct DSI_COUNTER phy_timing[] = {
	/* LP Data Symbol Rate Calc - MUST BE FIRST RECORD */
	{"ESC2LP_RATIO", DSI_C_TIME_ESC2LPDT, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0x0000003F, 1, 1, 0},
	/* SPEC:  min =  100[us] + 0[UI] */
	/* SET:   min = 1000[us] + 0[UI]                             <= */
	{"HS_INIT", DSI_C_TIME_HS, 0,
		0, 100000, 0, 0, 0, 0, 0, 0, 0x00FFFFFF, 0, 0, 0},
	/* SPEC:  min = 1[ms] + 0[UI] */
	/* SET:   min = 1[ms] + 0[UI] */
	{"HS_WAKEUP", DSI_C_TIME_HS, 0,
		0, 1000000, 0, 0, 0, 0, 0, 0, 0x00FFFFFF, 0, 0, 0},
	/* SPEC:  min = 1[ms] + 0[UI] */
	/* SET:   min = 1[ms] + 0[UI] */
	{"LP_WAKEUP", DSI_C_TIME_ESC, 0,
		0, 1000000, 0, 0, 0, 0, 0, 0, 0x00FFFFFF, 1, 1, 0},
	/* SPEC:  min = 0[ns] +  8[UI] */
	/* SET:   min = 0[ns] + 12[UI]                               <= */
	{"HS_CLK_PRE", DSI_C_TIME_HS, 0,
		0, 0, 12, 0, 0, 0, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min = 38[ns] + 0[UI]   max= 95[ns] + 0[UI] */
	/* SET:   min = 48[ns] + 0[UI]   max= 95[ns] + 0[UI]         <= */
	{"HS_CLK_PREPARE", DSI_C_TIME_HS, DSI_C_HAS_MAX,
		0, 48, 0, 0, 0, 95, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min = 262[ns] + 0[UI] */
	/* SET:   min = 262[ns] + 0[UI]                              <= */
	{"HS_CLK_ZERO", DSI_C_TIME_HS, 0,
		0, 320, 0, 0, 0, 0, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min =  60[ns] + 52[UI] */
	/* SET:   min =  70[ns] + 52[UI]                             <= */
	{"HS_CLK_POST", DSI_C_TIME_HS, 0,
		0, 70, 52, 0, 0, 0, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min =  60[ns] + 0[UI] */
	/* SET:   min =  70[ns] + 0[UI]                              <= */
	{"HS_CLK_TRAIL", DSI_C_TIME_HS, 0,
		0, 70, 0, 0, 0, 0, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min =  50[ns] + 0[UI] */
	/* SET:   min =  60[ns] + 0[UI]                              <= */
	{"HS_LPX", DSI_C_TIME_HS, 0,
		0, 60, 0, 0, 0, 75, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min = 40[ns] + 4[UI]      max= 85[ns] + 6[UI] */
	/* SET:   min = 50[ns] + 4[UI]      max= 85[ns] + 6[UI]      <= */
	{"HS_PRE", DSI_C_TIME_HS, DSI_C_HAS_MAX,
		0, 50, 4, 0, 0, 85, 6, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min = 105[ns] + 6[UI] */
	/* SET:   min = 105[ns] + 6[UI]                              <= */
	{"HS_ZERO", DSI_C_TIME_HS, 0,
		0, 105, 6, 0, 0, 0, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min = max(0[ns]+32[UI],60[ns]+16[UI])  n=4 */
	/* SET:   min = max(0[ns]+32[UI],60[ns]+16[UI])  n=4 */
	{"HS_TRAIL", DSI_C_TIME_HS, DSI_C_MIN_MAX_OF_2,
		0, 60, 4, 60, 16, 0, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min = 100[ns] + 0[UI] */
	/* SET:   min = 110[ns] + 0[UI]                              <= */
	{"HS_EXIT", DSI_C_TIME_HS, 0,
		0, 110, 0, 0, 0, 0, 0, 0, 0x000001FF, 0, 0, 0},
	/* min = 50[ns] + 0[UI] */
	/* LP esc counters are speced in LP LPX units.
	   LP_LPX is calculated by chal_dsi_set_timing
	   and equals LP data clock */
	{"LPX", DSI_C_TIME_ESC, 0,
		1, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1, 0},
	/* min = 4*[Tlpx]  max = 4[Tlpx], set to 4 */
	{"LP-TA-GO", DSI_C_TIME_ESC, 0,
		4, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1, 0},
	/* min = 1*[Tlpx]  max = 2[Tlpx], set to 2 */
	{"LP-TA-SURE", DSI_C_TIME_ESC, 0,
		2, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1, 0},
	/* min = 5*[Tlpx]  max = 5[Tlpx], set to 5 */
	{"LP-TA-GET", DSI_C_TIME_ESC, 0,
		5, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1, 0},
};
#endif
/* DISP DRV API	- Display Info */
static DISPDRV_INFO_T S6E63M0X_Info = {
	DISPLAY_TYPE_LCD_STD,		/* type	*/

	480,				/* width */
	800,				/* height */

	DISPDRV_FB_FORMAT_RGB888_U,	/* input_format	*/
	DISPLAY_BUS_DSI,		/* bus_type */
	4,				/* Bpp */
	54,				/* phys_width */
	95,				/* phys_height */
	32150,				/* pixclock */
};

/* DSI Command Mode VC Configuration */
CSL_DSI_CM_VC_t S6E63M0X_VCCmCfg = {
	S6E63M0X_VC,			/* VC */
	DSI_DT_LG_DCS_WR,		/* dsiCmnd */
	MIPI_DCS_WRITE_MEMORY_START,	/* dcsCmndStart	*/
	MIPI_DCS_WRITE_MEMORY_CONTINUE,	/* dcsCmndContinue */
	FALSE,				/* isLP	*/
	LCD_IF_CM_I_RGB888U,		/* cm_in */
	LCD_IF_CM_O_RGB888,		/* cm_out */
	/* TE configuration */
	{
		DSI_TE_NONE, /* DSI Te Input Type */
	},
};

/* DSI BUS CONFIGURATION */
CSL_DSI_CFG_t S6E63M0X_dsiCfg = {
	0,		/* bus */
	2,		/* dlCount */
	phy_timing,	/* DSI_DPHY_SPEC_T */
	/* ESC CLK Config */
	{500, 5},	/* escClk   500|312 500[MHz], DIV by 5 = 100[MHz] */

	/* HS CLK Config, RHEA VCO range 600-2400 */
	{900, 3},	/* hsBitClk PLL 1050[MHz], DIV by 3 = 300[Mbps]	*/

	/* LP Speed */
	5,		/* lpBitRate_Mbps, Max 10[Mbps]	*/

	TRUE,		/* enaContClock	*/
	TRUE,		/* enaRxCrc */
	TRUE,		/* enaRxEcc */
	TRUE,		/* enaHsTxEotPkt */
	FALSE,		/* enaLpTxEotPkt */
	FALSE,		/* enaLpRxEotPkt */
	0,		/* dispEngine */
	0,		/* pixTxporter */
};



static S6E63M0X_PANEL_t panel[1];
static struct regulator *key_led = NULL;
static struct regulator *disp_3v0 = NULL;

/*###########################################################################*/

static void S6E63M0X_panel_on(S6E63M0X_PANEL_t *pPanel)
{
	S6E63M0X_ExecCmndList(pPanel, panel_on_seq);
	S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] %s\n", __func__);
	return;
}

static void S6E63M0X_panel_off(S6E63M0X_PANEL_t *pPanel)
{
	S6E63M0X_ExecCmndList(pPanel, panel_off_seq);
	S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] %s\n", __func__);
	return;
}

static void S6E63M0X_panel_sleep_in(S6E63M0X_PANEL_t *pPanel)
{
	S6E63M0X_ExecCmndList(pPanel, sleep_in_seq);
	S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] %s\n", __func__);
	return;
}

static void S6E63M0X_panel_sleep_out(S6E63M0X_PANEL_t *pPanel)
{
	S6E63M0X_ExecCmndList(pPanel, sleep_out_seq);
	S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] sleep out%s\n", __func__);
	return;
}

static UInt8 panelID[3], mtpData[21];

/*
	S6E63M0X_read_panel_info()
	: this function should be called after sending "error_check_seq(0xD5h)"
*/
static void S6E63M0X_read_panel_info(S6E63M0X_PANEL_t *pPanel)
{
	int ret;
	
	S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] Read panelID");		
	ret = S6E63M0X_DCS_Read(pPanel, 0xDA, panelID, 1); //for reading panel ID
	ret = S6E63M0X_DCS_Read(pPanel, 0xDB, panelID+1, 1); //for reading panel ID
	ret = S6E63M0X_DCS_Read(pPanel, 0xDC, panelID+2, 1); //for reading panel ID	
	
	S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] Read mtpData");		
	ret = S6E63M0X_DCS_Read(pPanel, 0xD3, mtpData, 21); //for reading MTP with SEC_DSI_MTP_DATA_LEN length
	
	return;
}

static UInt32 panel_init_cnt = 0;

static void S6E63M0X_panel_init(S6E63M0X_PANEL_t *pPanel)
{
	panel_init_cnt++;
	//current_gamma = GAMMA_INDEX_NOT_SET;
	//current_bl = DEFAULT_GAMMA_LEVEL;
	//elvss_brightness = elvss_not_set;

	S6E63M0X_LOG(LCD_DBG_INIT_ID,"[DISPDRV] %s: start with panel init: \n\r",__func__);
	S6E63M0X_ExecCmndList(pPanel, l2_mtp_enable_seq);
	//S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s: start with sleep_out: \n\r",__func__);
	//pr_err("[DISPDRV] %s: start sleep out: \n\r",__func__);
	//S6E63M0X_panel_sleep_out(pPanel);
	//S6E63M0X_ExecCmndList(pPanel, acl_on_seq);
	S6E63M0X_ExecCmndList(pPanel, error_check_seq);

	S6E63M0X_read_panel_info(pPanel);

	S6E63M0X_ExecCmndList(pPanel, panel_conditon_seq);
	S6E63M0X_ExecCmndList(pPanel, disp_conditon_seq);
	
	S6E63M0X_ExecCmndList(pPanel, etc_condition_seq);
	
	S6E63M0X_ExecCmndList(pPanel, gamma_300CD_seq);
	S6E63M0X_ExecCmndList(pPanel, gamma_update_seq);
		
	//S6E63M0X_ExecCmndList(pPanel, acl_40P_seq);
	S6E63M0X_ExecCmndList(pPanel, elvss_16_seq);
	S6E63M0X_ExecCmndList(pPanel, elvss_on_seq);
	
	S6E63M0X_ExecCmndList(pPanel, sleep_out_seq);	
	//S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s: complete elvss setting: %d\n\r", __func__);
	S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] End of %s: count: %d\n\r", __func__, panel_init_cnt);
	//pr_err("[DISPDRV] %s: count: %d\n\r", __func__, panel_init_cnt);
	return;
}

/*
 *
 *   Function Name: S6E63M0X_TeOn
 *
 *   Description:   Configure TE Input Pin & Route it to DSI Controller Input
 *
 */
static int S6E63M0X_TeOn(S6E63M0X_PANEL_t	*pPanel)
{
	Int32 res	= 0;

	TECTL_CFG_t teCfg;

	teCfg.te_mode	  = TE_VC4L_MODE_VSYNC;
	teCfg.sync_pol	  = TE_VC4L_ACT_POL_LO;
	teCfg.vsync_width = 0;
	teCfg.hsync_line  = 0;

	res = CSL_TECTL_VC4L_OpenInput(pPanel->teIn, pPanel->teOut, &teCfg);

	return res;
}

/*
 *
 *   Function Name: S6E63M0X_TeOff
 *
 *   Description:   'Release' TE Input Pin Used
 *
 */
static int S6E63M0X_TeOff(S6E63M0X_PANEL_t *pPanel)
{
	Int32  res = 0;

	res = CSL_TECTL_VC4L_CloseInput(pPanel->teIn);

	return res;
}

/*
 *
 *   Function Name: S6E63M0X_DCS_ReadReg
 *
 *   Description:   DSI Read Reg
 *
 */
static int S6E63M0X_DCS_Read(DISPDRV_HANDLE_T drvH, UInt8 reg, UInt8 *rxBuff, UInt8 buffLen)
{
	S6E63M0X_PANEL_t		*pPanel	= (S6E63M0X_PANEL_t *)drvH;
	CSL_DSI_CMND_t msg;
	CSL_DSI_REPLY_t rxMsg;      /* DSI RX message */
	UInt8 txData[2];  /* DCS Rd Command */
	Int32 res = 0;
	CSL_LCD_RES_T cslRes;

	msg.dsiCmnd    = DSI_DT_SH_DCS_RD_P0;
	msg.msg        = &txData[0];
	msg.msgLen     = 2;
	msg.vc         = S6E63M0X_VC;
	msg.isLP       = S6E63M0X_CMND_IS_LP;
	//msg.isLong     = FALSE;
	if(buffLen > 2) //matching with datasheet
		msg.isLong     = TRUE;
	else
		msg.isLong     = FALSE;
	msg.endWithBta = TRUE;

	rxMsg.pReadReply = (UInt8 *)rxBuff;
	msg.reply        = (CSL_DSI_REPLY_t *)&rxMsg;

	txData[0] = reg;
	txData[1] = 0x0;
	S6E63M0X_SetMaxRtnPktSize(pPanel, buffLen);
	//S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s: sizeof(rxBuff): %d]\n\r", __func__, buffLen);
	cslRes = CSL_DSI_SendPacket(pPanel->clientH, &msg, FALSE);
	if ((cslRes != CSL_LCD_OK) ||
		((rxMsg.type & DSI_RX_TYPE_READ_REPLY) == 0)) {

		S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s:	ERR"
			"Reading From Reg[0x%08X]\n\r", __func__, reg);
		res = -1;
	} else {
		S6E63M0X_LOG(LCD_DBG_INIT_ID, "Command: 0x%02X\n",reg);
		UInt8 i;
		for(i=0; i<buffLen; i++)
			S6E63M0X_LOG(LCD_DBG_INIT_ID, "Parameter[%d]: [0x%02X]\n", i, rxBuff[i]);
		res = rxMsg.readReplySize;
	}

	return res;
}

/*
 *
 *   Function Name: S6E63M0X_SetMaxRtnPktSize
 *
 *   Description:   Set Window
 *
 */
static int S6E63M0X_SetMaxRtnPktSize(DISPDRV_HANDLE_T drvH, UInt8 size)
{
	S6E63M0X_PANEL_t *pPanel = (S6E63M0X_PANEL_t *) drvH;
	CSL_DSI_CMND_t msg;
	CSL_DSI_REPLY_t rxMsg;      /* DSI RX message */
	UInt8 txData[2];  /* DCS Rd Command */
	Int32 res = 0;
	CSL_LCD_RES_T cslRes;

	txData[0]=size;
	txData[1]=0x0;
	msg.dsiCmnd    = DSI_DT_SH_MAX_RET_PKT_SIZE;
	msg.msg        = &txData[0];
	msg.msgLen     = 2;
	msg.vc         = S6E63M0X_VC;
	msg.isLP       = S6E63M0X_CMND_IS_LP;
	msg.isLong     = FALSE;
	msg.endWithBta = FALSE;

	S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] %s: disCmnd[0x%02X]\n",
				__func__, DSI_DT_SH_MAX_RET_PKT_SIZE);
	cslRes = CSL_DSI_SendPacket(pPanel->clientH, &msg, FALSE);
	if (cslRes != CSL_LCD_OK) {

		S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s:	ERR"
			"Setting Max. Return Packet Size [0x%02X]\n\r",
			__func__, DSI_DT_SH_MAX_RET_PKT_SIZE);
		res = -1;
	}
	return res;
}

/*
 *
 *   Function Name: S6E63M0X_WinReset
 *
 *   Description:   Reset windowing to full screen size.
 *		    Typically, only used in boot code environment
 */
Int32 S6E63M0X_WinReset(DISPDRV_HANDLE_T drvH)
{
	return 0;
}

/*
 *
 *   Function Name: S6E63M0X_reset
 *
 *   Description:   (De)Assert display reset
 *
 */
static void S6E63M0X_reset(DISPDRV_HANDLE_T drvH, Boolean on)
{
	S6E63M0X_PANEL_t *pPanel = (S6E63M0X_PANEL_t *) drvH;

	int reset_active = 0;

	gpio_request(pPanel->rst_panel_reset, "LCD_RST1");
	if (!on) {
		S6E63M0X_LOG(LCD_DBG_ERR_ID, "Resetting the panel, gpio = %d\n",
			pPanel->rst_panel_reset);
		gpio_direction_output(pPanel->rst_panel_reset, !reset_active);
		udelay(5);
		gpio_set_value_cansleep(pPanel->rst_panel_reset, reset_active);
		mdelay(15); //Spec= 10us
		gpio_set_value_cansleep(pPanel->rst_panel_reset, !reset_active);
		msleep(200); //Spec= 5ms

	

	} else {
		S6E63M0X_LOG(LCD_DBG_ERR_ID, "Powering off the panel, gpio = %d\n",pPanel->rst_panel_reset);
		gpio_direction_output(pPanel->rst_panel_reset, !reset_active);
		udelay(5);
		gpio_set_value_cansleep(pPanel->rst_panel_reset, reset_active);
		msleep(20); //Spec= 5ms

	}
}

/*
 *
 *   Function Name: LCD_DRV_S6E63M0X_GetDrvInfo
 *
 *   Description:   Get Driver Funtion Table
 *
 */
DISPDRV_T *DISP_DRV_S6E63M0X_GetFuncTable(void)
{
	return &S6E63M0X_Drv;
}

/*
 *
 *   Function Name: S6E63M0X_Init
 *
 *   Description:   Setup / Verify display driver init interface
 *
 */
Int32 S6E63M0X_Init(
	struct dispdrv_init_parms	*parms,
	DISPDRV_HANDLE_T		*handle)
{
	Int32 res = 0;
	S6E63M0X_PANEL_t	*pPanel;

	pPanel = &panel[0];

	if (pPanel->drvState ==	DRV_STATE_OFF)	{

		S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s: Bus %d\n",
			__func__, parms->w0.bits.bus_type);
		S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s: BootMode %d\n",
			__func__, parms->w0.bits.boot_mode);
		S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s: BusNo %d\n",
			__func__, parms->w0.bits.bus_no);
		S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s: col_mode_i %d\n",
			__func__, parms->w0.bits.col_mode_i);
		S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s: col_mode_o %d\n",
			__func__, parms->w0.bits.col_mode_o);
		S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s: te_input %d\n",
			__func__, parms->w0.bits.te_input);

		S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s: API Rev %d\n",
			__func__, parms->w1.bits.api_rev);
		S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s: Rst 0 %d\n",
			__func__, parms->w1.bits.lcd_rst0);
		S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s: Rst 1 %d\n",
			__func__, parms->w1.bits.lcd_rst1);
		S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s: Rst 2 %d\n",
			__func__, parms->w1.bits.lcd_rst2);

		if ((u8)parms->w1.bits.api_rev != KONA_LCD_BOOT_API_REV) {
			S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] %s:"
				"Boot Init API Rev Mismatch(%d.%d vs %d.%d)\n",
				__func__,
				(parms->w1.bits.api_rev	& 0xF0)	>> 8,
				(parms->w1.bits.api_rev	& 0x0F),
				(KONA_LCD_BOOT_API_REV	& 0xF0)	>> 8,
				(KONA_LCD_BOOT_API_REV	& 0x0F));
			return -1;
		}

		pPanel->boot_mode = parms->w0.bits.boot_mode;

		pPanel->cmnd_mode = &S6E63M0X_VCCmCfg;
		pPanel->dsi_cfg	  = &S6E63M0X_dsiCfg;
		pPanel->disp_info = &S6E63M0X_Info;

		pPanel->busNo =	dispdrv2busNo(parms->w0.bits.bus_no);

		/* check for valid DSI bus no */
		if (pPanel->busNo & 0xFFFFFFFE)	{
			S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s:	Invlid DSI "
				"BusNo[%lu]\n",	__func__, pPanel->busNo);
			return -1;
		}

		pPanel->cmnd_mode->cm_in  =
			dispdrv2cmIn(parms->w0.bits.col_mode_i);
		pPanel->cmnd_mode->cm_out =
			dispdrv2cmOut(parms->w0.bits.col_mode_o);

		/* as of now, only 565 */
		switch (pPanel->cmnd_mode->cm_in) {
		case LCD_IF_CM_I_RGB565P:
			pPanel->disp_info->input_format
				= DISPDRV_FB_FORMAT_RGB565;
			pPanel->disp_info->Bpp = 2;
			break;
		case LCD_IF_CM_I_RGB888U:
			pPanel->disp_info->input_format
				= DISPDRV_FB_FORMAT_RGB888_U;
			pPanel->disp_info->Bpp = 4;
			break;
		default:
			S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s:	Unsupported"
				" Color Mode\n", __func__);
			return -1;
		}

		/* get reset pin */
		pPanel->rst_panel_reset = parms->w1.bits.lcd_rst0;

		pPanel->isTE = pPanel->cmnd_mode->teCfg.teInType != DSI_TE_NONE;

		/* get TE pin configuration */
		pPanel->teIn  =	dispdrv2busTE(parms->w0.bits.te_input);
		pPanel->teOut =	pPanel->busNo
			? TE_VC4L_OUT_DSI1_TE0 : TE_VC4L_OUT_DSI0_TE0;

		pPanel->drvState = DRV_STATE_INIT;
		pPanel->pwrState = STATE_PWR_OFF;

		*handle	= (DISPDRV_HANDLE_T)pPanel;

		S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s: OK\n", __func__);
	} else {
		S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] %s: Not in OFF state\n",
			__func__);
		res = -1;
	}

	return res;
}

/*
 *
 *   Function Name: S6E63M0X_Exit
 *
 *   Description:
 *
 */
Int32 S6E63M0X_Exit(DISPDRV_HANDLE_T drvH)
{
	S6E63M0X_PANEL_t *pPanel;

	pPanel = (S6E63M0X_PANEL_t *) drvH;
	pPanel->drvState = DRV_STATE_OFF;
	return 0;
}


/*
 *
 *  Function Name: S6E63M0X_Open
 *
 *  Description:   disp bus ON
 *
 */



Int32 S6E63M0X_Open(DISPDRV_HANDLE_T drvH)
{
	Int32 res = 0;
	S6E63M0X_PANEL_t	*pPanel;
	

	pPanel = (S6E63M0X_PANEL_t *) drvH;

	S6E63M0X_LOG(LCD_DBG_ERR_ID, "S6E63M0X_Open\n");

	///Temporary solution only, need to move the new driver
	
	if(key_led==NULL)
	{

		//key_led=regulator_get(NULL,"gpldo3_uc");
		key_led=regulator_get(NULL,"camldo2");

		if (!key_led || IS_ERR(key_led)) {
			//pr_err("No Regulator available for gpldo3\n");
			pr_err("No Regulator available for camldo2\n");
		}
		else
		{
			//regulator_set_voltage(key_led,3300000,3300000);
			regulator_set_voltage(key_led,3000000,3000000);
		}
	}
#if 0
	{
		disp_3v0=regulator_get(NULL,"camldo2");

		if (IS_ERR(disp_3v0)) {
			pr_err("No Regulator available for camldo2\n");			
		}
		else
		{
			regulator_set_voltage(disp_3v0,3000000,3000000);
			regulator_enable(disp_3v0);
			S6E63M0X_LOG(LCD_DBG_ERR_ID, "S6E63M0X powered on, 3.0V\n");
		}
	}	
#endif

	////////
	if (pPanel->drvState !=	DRV_STATE_INIT)	{
		S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] %s: ERROR State != Init\n",
			__func__);
		return -1;
	}

	if (brcm_enable_dsi_pll_clocks(pPanel->busNo,
		pPanel->dsi_cfg->hsBitClk.clkIn_MHz * 1000000,
		pPanel->dsi_cfg->hsBitClk.clkInDiv,
		pPanel->dsi_cfg->escClk.clkIn_MHz   * 1000000
		/ pPanel->dsi_cfg->escClk.clkInDiv)) {

		S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] %s:ERROR to enable the "
			"clock\n", __func__);
	}

	if (pPanel->isTE && S6E63M0X_TeOn(pPanel) == -1) {
		S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] %s:"
			"Failed To Configure TE Input\n", __func__);
		goto err_te_on;
	}

	if (CSL_DSI_Init(pPanel->dsi_cfg) != CSL_LCD_OK) {
		S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] %s:ERROR, DSI CSL Init "
			"Failed\n", __func__);
		goto err_dsi_init;
	}

	if (CSL_DSI_OpenClient(pPanel->busNo, &pPanel->clientH)	!= CSL_LCD_OK) {
		S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] %s:ERROR, "
			"CSL_DSI_OpenClient Failed\n", __func__);
		goto err_dsi_open_cl;
	}

	if (CSL_DSI_OpenCmVc(pPanel->clientH,
		pPanel->cmnd_mode, &pPanel->dsiCmVcHandle) != CSL_LCD_OK) {

		S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] %s:CSL_DSI_OpenCmVc "
			"Failed\n", __func__);
		goto err_dsi_open_cm;
	}

	if (!pPanel->dsi_cfg->dispEngine && pPanel->dsi_cfg->pixTxporter) {
		S6E63M0X_LOG(LCD_DBG_ERR_ID, "Cannot transfer pixels via MMDMA"
		" to Display Engine 0. Default to Display Engine 1\n");
		pPanel->dsi_cfg->dispEngine = 1;
	}

	if (pPanel->dsi_cfg->dispEngine && pPanel->dsi_cfg->pixTxporter) {
		if (csl_dma_vc4lite_init() != DMA_VC4LITE_STATUS_SUCCESS) {
			S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] %s: "
				"csl_dma_vc4lite_init Failed\n", __func__);
			goto err_dma_init;
		}
	}
	S6E63M0X_reset(drvH, FALSE);

	pPanel->win_dim.l = 0;
	pPanel->win_dim.r = pPanel->disp_info->width-1;
	pPanel->win_dim.t = 0;
	pPanel->win_dim.b = pPanel->disp_info->height-1;
	pPanel->win_dim.w = pPanel->disp_info->width;
	pPanel->win_dim.h = pPanel->disp_info->height;

	pPanel->drvState = DRV_STATE_OPEN;

	S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s:	OK video mode LDI.\n\r", __func__);
	pr_err("[DISPDRV] %s: OK video mode LDI.\n\r", __func__);
	return res;

err_dma_init:
	CSL_DSI_CloseCmVc(pPanel->dsiCmVcHandle);
err_dsi_open_cm:
	CSL_DSI_CloseClient(pPanel->clientH);
err_dsi_open_cl:
	CSL_DSI_Close(pPanel->busNo);
err_dsi_init:
	if (pPanel->isTE)
		S6E63M0X_TeOff(pPanel);
err_te_on:
	return -1;
}

/*
 *
 *  Function Name: S6E63M0X_Close
 *
 *  Description:   disp bus OFF
 *
 */
Int32 S6E63M0X_Close(DISPDRV_HANDLE_T drvH)
{
	Int32 res = 0;
	S6E63M0X_PANEL_t	*pPanel	= (S6E63M0X_PANEL_t *)drvH;

	if (CSL_DSI_CloseCmVc(pPanel->dsiCmVcHandle)) {
		S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s: ERROR,	"
			"Closing Command Mode Handle\n\r", __func__);
		return -1;
	}

	if (CSL_DSI_CloseClient(pPanel->clientH) != CSL_LCD_OK)	{
		S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s:	ERROR, Closing "
			"DSI Client\n",	__func__);
		return -1;
	}

	if (CSL_DSI_Close(pPanel->busNo) != CSL_LCD_OK)	{
		S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s: ERR Closing DSI "
			"Controller\n",	__func__);
		return -1;
	}

	if (pPanel->isTE)
		S6E63M0X_TeOff(pPanel);

	if (brcm_disable_dsi_pll_clocks(pPanel->busNo))	{
		S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s: ERROR to disable	"
			"the pll clock\n", __func__);
		return -1;
	}

	pPanel->pwrState = STATE_PWR_OFF;
	pPanel->drvState = DRV_STATE_INIT;
	S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s:	OK\n\r", __func__);
	

	return res;
}


/*
 *
 * Function Name:  S6E63M0X_WrSendCmnd
 *
 * Parameters:	   msg_buff   =	TX byte buffer
 *		   msg_size   =	size of message to be sent [bytes]
 *
 * Description:	   Send commands with variable no of parms
 *		   Assumption: DISPLAY's Controller Accepts DT Used
 */
static int S6E63M0X_WrSendCmnd(
	DISPDRV_HANDLE_T drvH,
	UInt8			*msg_buff,
	UInt32 msg_size)
{
	S6E63M0X_PANEL_t	*pPanel	= (S6E63M0X_PANEL_t *)drvH;
	CSL_DSI_CMND_t msg;
	int res = 0;

	switch (msg_size) {
	case 0:
		res = -1;
		break;
	case 1:
		msg.dsiCmnd = DSI_DT_SH_DCS_WR_P0;
		break;
	case 2:
		msg.dsiCmnd = DSI_DT_SH_DCS_WR_P1;
		break;
	default:
		if (msg_size <=	CSL_DSI_GetMaxTxMsgSize())
			msg.dsiCmnd = DSI_DT_LG_DCS_WR;
		else
			res = -1;
		break;
	}

	if (res	== 0) {
		msg.msg	       = msg_buff;
		msg.msgLen     = msg_size;
		msg.vc	       = S6E63M0X_VC;
		msg.isLP       = S6E63M0X_CMND_IS_LP;
		msg.isLong     = msg_size > 2;
		msg.endWithBta = FALSE;

		CSL_DSI_SendPacket(pPanel->clientH, &msg, FALSE);
		S6E63M0X_LOG(LCD_DBG_INIT_ID, "[DISPDRV] %s:	DT[0x%02lX] "
			"SIZE[%lu]\n",	__func__, msg.dsiCmnd, msg_size);
	}

	return res;
}


/*
 *
 *   Function Name:   S6E63M0X_ExecCmndList
 *
 *   Description:
 *
 */
static void S6E63M0X_ExecCmndList(
	DISPDRV_HANDLE_T     drvH,
	pDISPCTRL_REC_T cmnd_lst)
{
	#define TX_MSG_MAX 64
	UInt32 i = 0;

	UInt8 tx_buff[TX_MSG_MAX];
	UInt32 tx_size	= 0;

	while (cmnd_lst[i].type	!= DISPCTRL_LIST_END) {
		if (cmnd_lst[i].type ==	DISPCTRL_WR_CMND) {
			tx_buff[tx_size++] = cmnd_lst[i].cmnd;

			while (cmnd_lst[i+1].type == DISPCTRL_WR_DATA) {
				tx_buff[tx_size++] = cmnd_lst[i+1].data;
				i++;
			}
			S6E63M0X_WrSendCmnd(drvH, tx_buff, tx_size);
			tx_size	= 0;
		} else if (cmnd_lst[i].type == DISPCTRL_WR_CMND_DATA) {
			tx_buff[tx_size++] = cmnd_lst[i].cmnd;
			tx_buff[tx_size++] = cmnd_lst[i].data;
			S6E63M0X_WrSendCmnd(drvH, tx_buff, tx_size);
			tx_size	= 0;

		} else if (cmnd_lst[i].type == DISPCTRL_SLEEP_MS) {
			msleep(cmnd_lst[i].data);
		}
		i++;
	}
}



/*
 *
 *   Function Name: S6E63M0X_PowerControl
 *
 *   Description:	  Display Module Control
 *
 */
Int32 S6E63M0X_PowerControl(
	DISPDRV_HANDLE_T drvH,
	DISPLAY_POWER_STATE_T state)
{
	Int32  res = 0;
	S6E63M0X_PANEL_t   *pPanel =	(S6E63M0X_PANEL_t *)drvH;

	switch (state) {
	case CTRL_PWR_ON:
		switch (pPanel->pwrState) {
		case STATE_PWR_OFF:
			S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV]	%s: current state=STATE_PWR_OFF  " 
						"request CTRL_PWR_ON\n\r", __func__);
			//pr_err("[DISPDRV]	S6E63M0X_PowerControl: current state=STATE_PWR_OFF, request CTRL_PWR_ON\n\r");
			if (key_led &&!( IS_ERR(key_led)))
			{
				printk("enable key_led\n");
				regulator_enable(key_led);	
			}
			usleep_range(1000, 1010);
			if(pPanel->drvState != DRV_STATE_OPEN)
				S6E63M0X_Open(drvH);
			S6E63M0X_panel_init(pPanel);
			
			//OSTASK_Sleep(TICKS_IN_MILLISECONDS(120));
			//S6E63M0X_panel_on(pPanel);
			pPanel->pwrState = STATE_SCREEN_OFF;
			//pPanel->pwrState = STATE_SCREEN_ON;
			break;
		default:
			S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s:	POWER ON "
				"Requested While Not In POWER DOWN State\n\r", __func__);
			break;
		}
		break;

	case CTRL_PWR_OFF:
		if (pPanel->pwrState !=	STATE_PWR_OFF) {
			S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV]	%s: current state=!STATE_PWR_OFF  " 
						"request CTRL_PWR_OFF\n\r", __func__);
			//pr_err("[DISPDRV]	S6E63M0X_PowerControl: current state=!STATE_PWR_OFF, request CTRL_PWR_OFF\n\r", __func__);
			//S6E63M0X_panel_shut_down(pPanel);
			S6E63M0X_panel_sleep_in(pPanel);
			//OSTASK_Sleep(TICKS_IN_MILLISECONDS(120));
			//S6E63M0X_panel_dstb(pPanel);

			memset(&pPanel->win_cur, 0, sizeof(DISPDRV_WIN_t));
			pPanel->pwrState = STATE_PWR_OFF;

			if (key_led &&!( IS_ERR(key_led)))
			{
				printk("disable key_led\n");
				regulator_disable(key_led);	
			}
			

			S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV]	%s: PWR DOWN\n\r",
				__func__);
/*			gpio_direction_input(BACKLIGHT_CTL); */
		}
		break;

	case CTRL_SLEEP_IN:
/* 		gpio_direction_input(BACKLIGHT_CTL); */
		switch (pPanel->pwrState) {
		case STATE_SCREEN_ON:
			S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV]	%s: current state=STATE_SCREEN_ON  "
						"request CTRL_SLEEP_IN\n\r", __func__);
		case STATE_SCREEN_OFF:
			S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV]	%s: current state=STATE_SCREEN_OFF  "
						"request CTRL_SLEEP_IN\n\r", __func__);
			S6E63M0X_panel_sleep_in(pPanel);
			//OSTASK_Sleep(TICKS_IN_MILLISECONDS(120));

			pPanel->pwrState = STATE_SLEEP;
			break;
		default:
			S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s:	"
				"SLEEP Requested, But Not In "
				"DISP ON|OFF State\n\r", __func__);
			break;
		}
		break;

	case CTRL_SLEEP_OUT:
		switch (pPanel->pwrState) {
		case STATE_SLEEP:
			S6E63M0X_panel_sleep_out(pPanel);
			pPanel->pwrState = STATE_SCREEN_OFF;
/*			gpio_direction_output(BACKLIGHT_CTL,1); */
			S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV]	%s: SLEEP-OUT\n\r", __func__);
			//pr_err("[DISPDRV]	%s: SLEEP-OUT from STATE_SLEEP\n\r",__func__);
			break;
		default:
			S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s:	"
				"SLEEP-OUT Req While Not In SLEEP State\n",
				__func__);
			//pr_err("[DISPDRV]	%s: SLEEP-OUT from default\n\r",__func__);
			break;
		}
		break;

	case CTRL_SCREEN_ON:
		switch (pPanel->pwrState) {
		case STATE_SCREEN_OFF:
			S6E63M0X_panel_on(pPanel);

			pPanel->pwrState = STATE_SCREEN_ON;
			S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV]	%s: SCREEN ON\n", __func__);
			//pr_err("[DISPDRV]	%s: SCREEN_ON from STATE_SCREEN_OFF\n\r",__func__);
/*			gpio_direction_output(BACKLIGHT_CTL,1); */
			break;
		default:
			S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s:	"
				"SCREEN ON Req While Not In SCREEN OFF State\n", __func__);
			//pr_err("[DISPDRV]	%s: SCREEN_ON from default\n\r",__func__);
			break;
		}
		break;
	case CTRL_SCREEN_OFF:
/*		gpio_direction_input(BACKLIGHT_CTL); */
		switch (pPanel->pwrState) {
		case STATE_SCREEN_ON:
			S6E63M0X_panel_off(pPanel);

			pPanel->pwrState = STATE_SCREEN_OFF;
			S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV]	%s: SCREEN OFF\n",
				__func__);
			break;
		default:
			S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s:	"
				"SCREEN OFF Req While Not In SCREEN ON State\n",
				__func__);
			break;
		}
		break;
	default:
		S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s: Invalid Power "
			"State[%d] Requested\n", __func__, state);
		res = -1;
		break;
	}
/*	gpio_free(BACKLIGHT_CTL); */
	return res;
}

/*
 *
 *  Function Name: S6E63M0X_Start
 *
 *  Description:
 *
 */
Int32 S6E63M0X_Start(
	DISPDRV_HANDLE_T drvH,
	struct pi_mgr_dfs_node *dfs_node)
{
	S6E63M0X_PANEL_t *pPanel = (S6E63M0X_PANEL_t *)drvH;

	if (brcm_enable_dsi_lcd_clocks(dfs_node, pPanel->busNo,
		S6E63M0X_dsiCfg.hsBitClk.clkIn_MHz * 1000000,
		S6E63M0X_dsiCfg.hsBitClk.clkInDiv,
		S6E63M0X_dsiCfg.escClk.clkIn_MHz * 1000000
		/ S6E63M0X_dsiCfg.escClk.clkInDiv)) {

		S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s: ERROR to enable "
			"the clock\n", __func__);
		return -1;
	}

	return 0;
}

/*
 *
 *   Function Name: S6E63M0X_Stop
 *
 *   Description:
 *
 */
Int32 S6E63M0X_Stop(DISPDRV_HANDLE_T drvH, struct pi_mgr_dfs_node *dfs_node)
{
	S6E63M0X_PANEL_t *pPanel = (S6E63M0X_PANEL_t *)drvH;

	if (brcm_disable_dsi_lcd_clocks(dfs_node, pPanel->busNo)) {
		S6E63M0X_LOG(LCD_DBG_ERR_ID, "[DISPDRV] %s: ERROR to enable "
			"the clock\n", __func__);
	    return -1;
	}
	return 0;
}

/*
 *
 *   Function Name: S6E63M0X_GetInfo
 *
 *   Description:
 *
 */
const DISPDRV_INFO_T *S6E63M0X_GetDispDrvData(DISPDRV_HANDLE_T drvH)
{
	return &S6E63M0X_Info;
}

/*
 *
 *  Function Name: S6E63M0X_GetDispDrvFeatures
 *
 *  Description:
 *
 */
Int32 S6E63M0X_GetDispDrvFeatures(
	DISPDRV_HANDLE_T drvH,
	const char			**driver_name,
	UInt32				*version_major,
	UInt32				*version_minor,
	DISPDRV_SUPPORT_FEATURES_T	*flags)
{
	Int32 res = -1;

	if ((NULL != driver_name)   && (NULL !=	version_major) &&
	    (NULL != version_minor) && (NULL !=	flags))	{

		*driver_name   = "S6E63M0X (IN:ARG888 OUT:RGB888)";
		*version_major = 0;
		*version_minor = 15;
		*flags	       = DISPDRV_SUPPORT_NONE;
		res	       = 0;
	}
	return res;
}


/*
 *
 *  Function Name: S6E63M0X_Cb
 *
 *  Description:   CSL callback
 *
 */
static void S6E63M0X_Cb(CSL_LCD_RES_T cslRes, pCSL_LCD_CB_REC pCbRec)
{
	DISPDRV_CB_RES_T apiRes;

	//S6E63M0X_LOG(LCD_DBG_ID, "[DISPDRV]	+%s\r\n", __func__);

	if (pCbRec->dispDrvApiCb != NULL) {
		switch (cslRes)	{
		case CSL_LCD_OK:
			apiRes = DISPDRV_CB_RES_OK;
			break;
		default:
			apiRes = DISPDRV_CB_RES_ERR;
			break;
		}

		((DISPDRV_CB_T)pCbRec->dispDrvApiCb)(apiRes);
	}

	//S6E63M0X_LOG(LCD_DBG_ID, "[DISPDRV] -%s\r\n", __func__);
}


/*
 *
 *  Function Name: S6E63M0X_Update
 *
 *  Description:
 *
 */
Int32 S6E63M0X_Update(
	DISPDRV_HANDLE_T drvH,
	void *buff,
	DISPDRV_WIN_t *p_win,
	DISPDRV_CB_T apiCb)
{
	S6E63M0X_PANEL_t	*pPanel	= (S6E63M0X_PANEL_t *)drvH;
	CSL_LCD_UPD_REQ_T req;
	Int32 res  = 0;
	uint32_t offset;

	//S6E63M0X_LOG(LCD_DBG_ID, "[DISPDRV] +%s\r\n", __func__);
	if (pPanel->pwrState ==	STATE_PWR_OFF) {
		S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s: Skip Due To "
			"Power State\n", __func__);
		return -1;
	}

	if (p_win == NULL)
		p_win =	&pPanel->win_dim;

	/*S6E63M0X_LOG(LCD_DBG_ID, "%s %d %d %d %d\n", __func__, p_win->l, p_win->r, p_win->t,
		p_win->b);*/


	offset = (uint32_t)buff;
	offset += (p_win->t * pPanel->disp_info->width + p_win->l)
			* pPanel->disp_info->Bpp;

	req.buff        = (uint32_t *)offset;
	req.lineLenP	= p_win->w;
	req.lineCount	= p_win->h;
	req.xStrideB	= pPanel->disp_info->width - p_win->w;
	req.buffBpp	= pPanel->disp_info->Bpp;
	req.timeOut_ms = MAX_SCHEDULE_TIMEOUT;

	/*S6E63M0X_LOG(LCD_DBG_ID, "%s: buf=%08x, linelenp = %lu, linecnt =%lu\n",
		__func__, (u32)req.buff, req.lineLenP, req.lineCount);*/

	req.cslLcdCbRec.cslH		= pPanel->clientH;
	req.cslLcdCbRec.dispDrvApiCbRev	= DISP_DRV_CB_API_REV_1_0;
	req.cslLcdCbRec.dispDrvApiCb	= (void	*) apiCb;
	req.cslLcdCbRec.dispDrvApiCbP1	= NULL;

	if (apiCb != NULL)
		req.cslLcdCb = S6E63M0X_Cb;
	else
		req.cslLcdCb = NULL;

	if (CSL_DSI_UpdateVmVc(pPanel->dsiCmVcHandle, &req)
		!= CSL_LCD_OK)	{
		S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s:	ERROR ret by "
			"CSL_DSI_UpdateCmVc\n\r", __func__);
		res = -1;
		S6E63M0X_Cb(res, &req.cslLcdCbRec);
	}

	//S6E63M0X_LOG(LCD_DBG_ID, "[DISPDRV]	-%s\r\n", __func__);

	return res;
}

/*
 *
 *  Function Name: S6E63M0X_Atomic_Update
 *
 *  Description:
 *
 */
Int32 S6E63M0X_Atomic_Update(
	DISPDRV_HANDLE_T drvH,
	void			*buff,
	DISPDRV_WIN_t		*p_win)
{
	S6E63M0X_PANEL_t	*pPanel	= (S6E63M0X_PANEL_t *)drvH;
	CSL_LCD_UPD_REQ_T req;
	Int32 res  = 0;

	//S6E63M0X_LOG(LCD_DBG_ID, "[DISPDRV]	+%s\r\n", __func__);

	if (pPanel->pwrState ==	STATE_PWR_OFF) {
		S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s: Skip Due To "
			"Power State\n", __func__);
		return -1;
	}

	if (p_win == NULL)
		p_win =	&pPanel->win_dim;

	CSL_DSI_Force_Stop(pPanel->dsiCmVcHandle);

	CSL_DSI_Lock(pPanel->dsiCmVcHandle);

	csl_dma_lock();

	req.buff	= buff;
	req.lineLenP	= p_win->w;
	req.lineCount	= p_win->h;
	req.buffBpp	= pPanel->disp_info->Bpp;
	req.timeOut_ms	= 100;
	req.xStrideB	= 0;

	/*S6E63M0X_LOG(LCD_DBG_ID, "%s: buf=%08x, linelenp = %lu, linecnt =%lu\n",
		__func__, (u32)req.buff, req.lineLenP, req.lineCount);*/

	req.cslLcdCbRec.cslH		= pPanel->clientH;
	req.cslLcdCbRec.dispDrvApiCbRev	= DISP_DRV_CB_API_REV_1_0;
	req.cslLcdCbRec.dispDrvApiCb	= NULL;
	req.cslLcdCbRec.dispDrvApiCbP1	= NULL;

	req.cslLcdCb = NULL;

	if (CSL_DSI_UpdateCmVc(pPanel->dsiCmVcHandle, &req, pPanel->isTE)
		!= CSL_LCD_OK)	{
		S6E63M0X_LOG(LCD_DBG_ERR_ID,	"[DISPDRV] %s:	ERROR ret by "
			"CSL_DSI_UpdateCmVc\n\r", __func__);
		res = -1;
	}

	//S6E63M0X_LOG(LCD_DBG_ID, "[DISPDRV]	-%s\r\n", __func__);

	//csl_dma_unlock();

	CSL_DSI_Unlock(pPanel->dsiCmVcHandle);

	return res;
}

