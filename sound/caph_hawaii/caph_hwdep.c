/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
/*****************************************************************************
Copyright 2010-2011 Broadcom Corporation.  All rights reserved.
Unless you and Broadcom execute a separate written software license agreement
governing use of this software, this software is licensed to you under the
terms of the GNU General Public License version 2, available at
http://www.gnu.org/copyleft/gpl.html (the "GPL").
Notwithstanding the above, under no circumstances may you combine this software
in any way with any other Broadcom software provided under a license other than
the GPL, without Broadcom's express prior written consent.
*****************************************************************************/
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm_params.h>
#include <sound/pcm.h>
#include <sound/rawmidi.h>
#include <sound/initval.h>
#include <sound/hwdep.h>
#include "mobcom_types.h"
#include "osdal_os.h"
#include "resultcode.h"
#include "audio_consts.h"
#include "audio_trace.h"
#include "bcm_fuse_sysparm_CIB.h"
#include "csl_caph.h"
#include "audio_vdriver.h"
#include "audio_controller.h"
#include "audio_ddriver.h"
#include "audio_caph.h"
#include "caph_common.h"
#include "csl_voip.h"
#include "csl_dsp.h"
#include "osdw_dsp_drv.h"
#include "csl_audio_render.h"
#include "bcm_audio.h"
#include "auddrv_audlog.h"
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/broadcom/bcm_major.h>
#include <caph_common.h>
/* Local defines */
#define VOIP_FRAMES_IN_BUFFER 1
enum __voip_hwdep_status_t {
	voip_hwdep_status_none,
	voip_hwdep_status_opened,
	voip_hwdep_status_started,
	voip_hwdep_status_stopped,
	voip_hwdep_status_released,
};
struct __dl_frame {
	u32 timestamp;
	char data[VOIP_MAX_FRAME_LEN];
};
struct __audio_voip_driver {
	u32 voip_data_dl_rd_index;
	u32 voip_data_dl_wr_index;
	u32 voip_data_ul_rd_index;
	u32 voip_data_ul_wr_index;
	u8 *voip_data_ul_buf_ptr;
	u8 *voip_data_dl_buf_ptr;
	u32 dl_timestamp;
	AUDIO_DRIVER_HANDLE_t drv_handle;
};
#define	audio_voip_driver_t	struct __audio_voip_driver
struct __bcm_caph_hwdep_voip {
	int status;
	int frames_available_to_read;
	int frames_available_to_write;
	int writecount;
	wait_queue_head_t sleep;
	audio_voip_driver_t *buffer_handle;
	AUDIO_SOURCE_Enum_t mic;
	AUDIO_SINK_Enum_t spk;
	u32 codec_type_ul;
	u32 codec_type_dl;
	u8 voip_type;
	u32 frame_size_ul;
	u32 frame_size_dl;
	u32 buffer_size_ul;
	u32 buffer_size_dl;
	int dlstarted;
	int ulstarted;
};
#define	bcm_caph_hwdep_voip_t struct __bcm_caph_hwdep_voip
static const u16 svoipframelen[] = { 320, 158, 36, 164, 640, 68 };
static u16 svoipamrsilenceframe[1] = { 0x000f };
static u32 voipinstcnt;
static struct semaphore sVoipAction; /* critical region protection */
static Boolean isvolte = FALSE;
bcm_caph_voip_log_t voip_log;
/* local functions */
static void HWDEP_VOIP_DumpUL_CB(void *pPrivate, u8 * pSrc, u32 nSize);
static void HWDEP_VOIP_FillDL_CB(
		void *pPrivate,
		u8 *pDst,
		u32 nSize,
		u32 *timestamp);
static void FillSilenceFrame(u32 codec_type, u32 frame_size, u8 *pDst);
static void voip_log_capture(CAPTURE_POINT_t log_pint, unsigned char *buf,
					int size);
uint32_t guULCount = 0, guDLCount = 0;
static void FillSilenceFrame(u32 codec_type, u32 frame_size, u8 *pDst)
{
	CSL_VOIP_Buffer_t tmpBuf;
	memset(&tmpBuf, 0, sizeof(CSL_VOIP_Buffer_t));
	if (codec_type == VoIP_Codec_AMR475)
		tmpBuf.voip_frame.frame_amr[0] = svoipamrsilenceframe[0];
	else if (codec_type == VOIP_Codec_AMR_WB_7K)
		tmpBuf.voip_frame.frame_amr_wb.frame_type = 0x7;
	else if (codec_type == VoIP_Codec_FR) {
		tmpBuf.voip_frame.frame_fr[0] = 1;
		tmpBuf.voip_frame.frame_fr[1] = 0;
		tmpBuf.voip_frame.frame_fr[2] = 0;
	} else if (codec_type == VOIP_Codec_G711_U) {
		tmpBuf.voip_frame.frame_g711[0].frame_type = 1;
		tmpBuf.voip_frame.frame_g711[1].frame_type = 1;
	}
	memcpy(pDst, &(tmpBuf.voip_frame), frame_size);
}
static void HWDEP_VOIP_DumpUL_CB(void *pPrivate, u8 * pSrc, u32 nSize)
{
	bcm_caph_hwdep_voip_t *pVoIP;
	pVoIP = (bcm_caph_hwdep_voip_t *) pPrivate;
	/* DEBUG("HWDEP_VOIP_DumpUL_CB nSize %d pVoIP 0x%x\n",
	 * nSize,pVoIP);
	 */
	guULCount++;
	if (0 == (guULCount%500))
		aTrace(LOG_ALSA_INTERFACE, "DumpUL_CB ulstarted=%d dlstarted="
		"%d, voipinstcnt=%d frames_available_to_read=%d, "
		"frames_available_to_write=%d",
		pVoIP->ulstarted, pVoIP->dlstarted, voipinstcnt,
		pVoIP->frames_available_to_read,
		pVoIP->frames_available_to_write);
	if (pVoIP->ulstarted == 0)
		return;

/*	aTrace(LOG_ALSA_INTERFACE, "DumpUL_CB buffer_handle=0x%x"
							   "voip_data_ul_buf_ptr=0x%x,"
							   "voip_data_ul_wr_index %d\n",
				pVoIP->buffer_handle,	
				pVoIP->buffer_handle->voip_data_ul_buf_ptr,
				pVoIP->buffer_handle->voip_data_ul_wr_index);*/

	
	if (pVoIP->buffer_handle) {
		if (pVoIP->buffer_handle->voip_data_ul_buf_ptr) {


			if (pVoIP->buffer_handle->voip_data_ul_buf_ptr + 
				pVoIP->buffer_handle->voip_data_ul_wr_index == 0x10)
				CAPH_ASSERT(0);	
			
			memcpy(pVoIP->buffer_handle->voip_data_ul_buf_ptr +
			       pVoIP->buffer_handle->voip_data_ul_wr_index,
			       pSrc, nSize);
			pVoIP->frames_available_to_read++;
			if (pVoIP->frames_available_to_read > 1)
				aTrace(LOG_ALSA_INTERFACE, "more than 1 frame"
				" available frame_size %d, readcount %d\n",
				pVoIP->frame_size_ul,
				pVoIP->frames_available_to_read);
#ifdef CONFIG_VOIP_BUFFER_INCREASE
			pVoIP->buffer_handle->voip_data_ul_wr_index += nSize;
			if (pVoIP->buffer_handle->voip_data_ul_wr_index >=
			    pVoIP->buffer_size_ul) {
				pVoIP->buffer_handle->voip_data_ul_wr_index -=
				    pVoIP->buffer_size_ul;
			}
#endif
		}
	}
	wake_up(&pVoIP->sleep);
}
static void HWDEP_VOIP_FillDL_CB(
		void *pPrivate,
		u8 *pDst,
		u32 nSize,
		u32 *timestamp)
{
	bcm_caph_hwdep_voip_t *pVoIP;
	pVoIP = (bcm_caph_hwdep_voip_t *) pPrivate;
	guDLCount++;
	if (0 == (guDLCount%500))
		aTrace(LOG_ALSA_INTERFACE, "FillDL_CB ulstarted=%d"
		"dlstarted=%d, voipinstcnt=%d frames_available_to_read=%d, "
		"frames_available_to_write=%d",
		pVoIP->ulstarted, pVoIP->dlstarted, voipinstcnt,
		pVoIP->frames_available_to_read,
		pVoIP->frames_available_to_write);
	if (pVoIP->dlstarted == 0) {
		FillSilenceFrame(pVoIP->codec_type_dl, nSize, pDst);
		return;
	}
	if (pVoIP->buffer_handle->voip_data_dl_buf_ptr) {
		/*DEBUG("HWDEP_VOIP_FillDL_CB pVoIP->"
		 *"frames_available_to_write %d\n", pVoIP->
		 * frames_available_to_write);
		 */
		if (pVoIP->frames_available_to_write == 0) {
			/* fill with silent data based on the frame type  */
			FillSilenceFrame(pVoIP->codec_type_dl, nSize, pDst);
			aTrace(LOG_ALSA_INTERFACE, "under run frame_size %d,"
				"writecount %d\n",
				pVoIP->frame_size_dl, pVoIP->writecount);
		} else {
		if (isvolte)
			*timestamp = pVoIP->buffer_handle->dl_timestamp;
		else
			*timestamp = 0;
			memcpy(pDst,
			       pVoIP->buffer_handle->voip_data_dl_buf_ptr +
			       pVoIP->buffer_handle->voip_data_dl_rd_index,
			       nSize);
			pVoIP->frames_available_to_write--;
			if (!isvolte)
				pVoIP->writecount++;
#ifdef CONFIG_VOIP_BUFFER_INCREASE
			pVoIP->buffer_handle->voip_data_dl_rd_index += nSize;
			if (pVoIP->buffer_handle->voip_data_dl_rd_index >=
			    pVoIP->buffer_size_dl) {
				pVoIP->buffer_handle->voip_data_dl_rd_index -=
				    pVoIP->buffer_size_dl;
			}
#endif
		}
	}
	wake_up(&pVoIP->sleep);
}
#if 1
static int voip_open(struct inode *voipinode , struct file *filp)
{
	voipdev *pvoipchrdevpvtdata;
	bcm_caph_hwdep_voip_t *pvoip;
	int rc;
	aTrace(LOG_ALSA_INTERFACE, "voip_open\n");
	pvoipchrdevpvtdata =
		container_of(voipinode->i_cdev, voipdev, voipcdev);
	if (!pvoipchrdevpvtdata)
		return;
	pvoip = pvoipchrdevpvtdata->pvoip;
	rc = down_interruptible(&sVoipAction);
	if (rc)
		return rc;
	filp->private_data = pvoipchrdevpvtdata;
	aTrace(LOG_ALSA_INTERFACE, "filp->private_data = 0x%x, filp = 0x%x\n",
		(unsigned int)filp->private_data, (unsigned int)filp);
	up(&sVoipAction);
	return 0;
}
static int voip_release(struct inode *voipinode , struct file *filp)
{
	voipdev *pvoipchrdevpvtdata = (voipdev *)filp->private_data;
	bcm_caph_hwdep_voip_t *pvoip;
	int rc;
	aTrace(LOG_ALSA_INTERFACE , "voip_release\n");
	if (!pvoipchrdevpvtdata)
		return;
	pvoip = pvoipchrdevpvtdata->pvoip;
	rc = down_interruptible(&sVoipAction);
	if (rc)
		return rc;
	if (voipinstcnt == 0 && pvoip != NULL) {
		kfree(pvoip);
		pvoipchrdevpvtdata->pvoip = NULL;
		aTrace(LOG_ALSA_INTERFACE, "freeing pvoip\n");
	}
	up(&sVoipAction);
	return 0;
}
static ssize_t voip_read(struct file *filep, char __user *buf,
					size_t count, loff_t *pos)
{
	bcm_caph_hwdep_voip_t *pvoip;
	voipdev *voipchrdevpvtdata;
	long ret = 0;
	voipchrdevpvtdata = (voipdev *)filep->private_data;
	pvoip = (bcm_caph_hwdep_voip_t *)voipchrdevpvtdata->pvoip;
	if (!pvoip) {
		aError("!pvoip return 0");
		return 0;
		}
	 /* DEBUG("voip_read count %ld\n",count); */

/*   aTrace(LOG_ALSA_INTERFACE,"voip_read: buffer_handle=0x%x,"
   						    "voip_data_ul_buf_ptr =0x%x\n", 
			   pvoip->buffer_handle, 
			   pvoip->buffer_handle->voip_data_ul_buf_ptr);*/
	 
	if ((pvoip->status == voip_hwdep_status_started)
	     && (pvoip->frames_available_to_read > 0)) {
		if (pvoip->frames_available_to_read) {
			if (pvoip->buffer_handle->voip_data_ul_buf_ptr) {
				ret =
				    copy_to_user(buf,
						 pvoip->buffer_handle->
						 voip_data_ul_buf_ptr +
						 pvoip->buffer_handle->
						 voip_data_ul_rd_index,
						 pvoip->frame_size_ul);
				voip_log_capture(AUD_LOG_VOCODER_UL,
						pvoip->buffer_handle->
						voip_data_ul_buf_ptr +
						pvoip->buffer_handle->
						voip_data_ul_rd_index,
						pvoip->frame_size_ul);
				pvoip->frames_available_to_read--;
#ifdef CONFIG_VOIP_BUFFER_INCREASE
				pvoip->buffer_handle->voip_data_ul_rd_index +=
				    pvoip->frame_size_ul;
				if (pvoip->buffer_handle->
				    voip_data_ul_rd_index >=
				    pvoip->buffer_size_ul) {
					pvoip->buffer_handle->
					    voip_data_ul_rd_index -=
					    pvoip->buffer_size_ul;
				}
#endif
				ret = pvoip->frame_size_ul;
			}
		}
	} else
		ret = 0;
	return ret;
}
static ssize_t voip_write(struct file *filep, const char __user *buf,
	size_t count, loff_t *pos)
{
	bcm_caph_hwdep_voip_t *pvoip;
	voipdev *voipchrdevpvtdata;
	long ret = 0;
	struct __dl_frame *frame_dl_data = (struct __dl_frame *)buf;
	voipchrdevpvtdata = (voipdev *)filep->private_data;
	pvoip = (bcm_caph_hwdep_voip_t *)voipchrdevpvtdata->pvoip;
	if (!pvoip) {
		aError("voip_write returns error as !pvoip");
		return count;
		}
	/* DEBUG("voip_write pvoip->frame_size %d,pvoip->"
	 * "writecount %d\n",pvoip->frame_size,pvoip->writecount);
	 */
	if ((pvoip->status == voip_hwdep_status_started)
	    && (pvoip->buffer_handle)) {
		if (pvoip->buffer_handle->voip_data_dl_buf_ptr) {
			if (isvolte) {
				pvoip->buffer_handle->dl_timestamp =
						frame_dl_data->timestamp;
				ret =
			    copy_from_user(
					pvoip->buffer_handle->
					voip_data_dl_buf_ptr +
					pvoip->buffer_handle->
					voip_data_dl_wr_index,
					(struct __dl_frame *)
					frame_dl_data->data,
					pvoip->frame_size_dl);
				voip_log_capture(AUD_LOG_VOCODER_DL,
					pvoip->buffer_handle->
					voip_data_dl_buf_ptr +
					pvoip->buffer_handle->
					voip_data_dl_wr_index,
					pvoip->frame_size_dl);
				/* send the DL frame to DSP . In case of VoLTE,
				whenever the application sends the data,
				need to send it to DSP.No need to wait
				for DSP callback. */
				VOLTE_ProcessDLData();
			} else {
				ret =
					copy_from_user(pvoip->buffer_handle->
					voip_data_dl_buf_ptr +
					pvoip->buffer_handle->
					voip_data_dl_wr_index,
					buf,
					pvoip->frame_size_dl);
				voip_log_capture(AUD_LOG_VOCODER_DL,
					pvoip->buffer_handle->
					voip_data_dl_buf_ptr +
					pvoip->buffer_handle->
					voip_data_dl_wr_index,
					pvoip->frame_size_dl);
			}
			pvoip->frames_available_to_write++;
			if (!isvolte)
				pvoip->writecount--;
#ifdef CONFIG_VOIP_BUFFER_INCREASE
			pvoip->buffer_handle->voip_data_dl_wr_index +=
			    pvoip->frame_size_dl;
			if (pvoip->buffer_handle->voip_data_dl_wr_index >=
			    pvoip->buffer_size_dl) {
				pvoip->buffer_handle->voip_data_dl_wr_index -=
				    pvoip->buffer_size_dl;
			}
#endif
		}
		ret = pvoip->frame_size_dl;
	} else
		ret = 0;
	return ret;
}
static long voip_ioctl(struct file *hw, unsigned int cmd, unsigned long arg)
{
	bcm_caph_hwdep_voip_t *pvoip;
	voipdev *pvoipchrdevpvtdata;
	long ret = 0;
	Boolean enable = FALSE;
	Int32 size = 0;
	int data;
	voip_codec_type_data_t val;
	static UserCtrl_data_t *dataptr;
	brcm_alsa_chip_t *pchip = NULL;
	struct treq_sysparm_t *eq;
	struct snd_card *pcard = NULL;
	int rc;
	pvoipchrdevpvtdata = (voipdev *)hw->private_data;
	pvoip = (bcm_caph_hwdep_voip_t *)pvoipchrdevpvtdata->pvoip;
	pcard =  (struct snd_card *)pvoipchrdevpvtdata->card;
	pchip = (brcm_alsa_chip_t *)pcard->private_data;
	aTrace(LOG_ALSA_INTERFACE, "ALSA-CAPH hwdep_ioctl cmd=%08X\n", cmd);
	switch (cmd) {
	case VoIP_Ioctl_GetVersion:
		/* ret = put_user(BrcmAACEncVersion, (int __user *)arg); */
		break;
	case VoIP_Ioctl_Start:
		get_user(data, (int __user *)arg);
		aTrace(LOG_ALSA_INTERFACE, "VoIP_Ioctl_Start type=%d (2==UL)"
			"voipinstcnt=%u\n", data, voipinstcnt);
		rc = down_interruptible(&sVoipAction);
		if (rc)		
			return rc;
		
		aTrace(LOG_ALSA_INTERFACE,"VoIP_Ioctl_Start: acquired sVoipAction\n");
		if (voipinstcnt == 0) { /* start VoIP only once */
			BRCM_AUDIO_Param_RateChange_t param_rate_change;
			BRCM_AUDIO_Param_Open_t param_open;
			BRCM_AUDIO_Param_Prepare_t parm_prepare;
			BRCM_AUDIO_Param_Start_t param_start;
			voipinstcnt++;
			aTrace(LOG_ALSA_INTERFACE,"voipinstcnt=%d\n", voipinstcnt);
#if 0
			hw->private_data =
			    kzalloc(sizeof(bcm_caph_hwdep_voip_t), GFP_KERNEL);
			CAPH_ASSERT(hw->private_data);
			pvoip = (bcm_caph_hwdep_voip_t *) hw->private_data;
#endif
			pvoipchrdevpvtdata->pvoip =
			    kzalloc(sizeof(bcm_caph_hwdep_voip_t), GFP_KERNEL);
			pvoip = pvoipchrdevpvtdata->pvoip;
			CAPH_ASSERT(pvoip);
			init_waitqueue_head(&pvoip->sleep);
			pvoip->buffer_handle =
			    (audio_voip_driver_t *)
			    kzalloc(sizeof(audio_voip_driver_t), GFP_KERNEL);

			aTrace(LOG_ALSA_INTERFACE,"pvoip->buffer_handle=0x%x",pvoip->buffer_handle);
			
			if (pvoip->buffer_handle)
				memset((u8 *) pvoip->buffer_handle, 0,
				       sizeof(audio_voip_driver_t));
			else {
				pvoip->buffer_handle = NULL;
				voipinstcnt--;
				up(&sVoipAction);
				aError("voip_ioctl failed with ENOMEM 1");
				return -ENOMEM;
			}
/*ul data*/
			if (pchip->voip_data.codec_type_ul == VoIP_Codec_None)
			{
				if (pchip->voip_data.codec_type_dl != VoIP_Codec_None)
					pchip->voip_data.codec_type_ul = pchip->voip_data.codec_type_dl;
				else
					pchip->voip_data.codec_type_ul = VoIP_Codec_PCM_16K; // by default

				aTrace(LOG_ALSA_INTERFACE, "VoIP_Ioctl_Start: VoIPCmd forced UL codec [0x%x]\n",
					pchip->voip_data.codec_type_ul);
			}
			pvoip->codec_type_ul = pchip->voip_data.codec_type_ul;
			pvoip->frame_size_ul = svoipframelen[pvoip->
								codec_type_ul - 1];
			pvoip->buffer_size_ul = pvoip->frame_size_ul *
							VOIP_FRAMES_IN_BUFFER;
			pvoip->buffer_handle->voip_data_ul_buf_ptr =
			kzalloc(pvoip->buffer_size_ul, GFP_KERNEL);

			aTrace(LOG_ALSA_INTERFACE, "VoIP_Ioctl_Start: voip_data_ul_buf_ptr=0x%x,"
				"buffer_size_ul=%d\n",
			pvoip->buffer_handle->voip_data_ul_buf_ptr,
			pvoip->buffer_size_ul);
			
			if (pvoip->buffer_handle->voip_data_ul_buf_ptr)
				memset(pvoip->buffer_handle->
				       voip_data_ul_buf_ptr, 0,
				       pvoip->buffer_size_ul);
			else {
				if (pvoip->buffer_handle->
					voip_data_dl_buf_ptr) {
					kfree(pvoip->buffer_handle->
						voip_data_dl_buf_ptr);
					pvoip->buffer_handle->
						voip_data_dl_buf_ptr = NULL;
				}
				pvoip->buffer_handle->voip_data_ul_buf_ptr =
									NULL;
				kfree(pvoip->buffer_handle);
				pvoip->buffer_handle = NULL;
				voipinstcnt--;
				up(&sVoipAction);
				aError("voip_ioctl failed with ENOMEM 2");
				return -ENOMEM;
			}
/*dl data*/
			if (pchip->voip_data.codec_type_dl == VoIP_Codec_None)
			{
				if (pchip->voip_data.codec_type_ul != VoIP_Codec_None)
					pchip->voip_data.codec_type_dl = pchip->voip_data.codec_type_ul;
				else
					pchip->voip_data.codec_type_dl = VoIP_Codec_PCM_16K; // by default
				aTrace(LOG_ALSA_INTERFACE, "VoIP_Ioctl_Start: VoIPCmd forced DL codec [0x%x]\n",
					pchip->voip_data.codec_type_dl);
			}
			pvoip->codec_type_dl = pchip->voip_data.codec_type_dl;
			pvoip->frame_size_dl = svoipframelen[pvoip->
								codec_type_dl - 1];
			pvoip->buffer_size_dl = pvoip->frame_size_dl *
							VOIP_FRAMES_IN_BUFFER;
			pvoip->buffer_handle->voip_data_dl_buf_ptr =
				kzalloc(pvoip->buffer_size_dl, GFP_KERNEL);
			if (pvoip->buffer_handle->voip_data_dl_buf_ptr) {
				memset(pvoip->buffer_handle->
				       voip_data_dl_buf_ptr, 0,
				       pvoip->buffer_size_dl);
			} else {
				if (pvoip->buffer_handle->
					voip_data_ul_buf_ptr) {
					kfree(pvoip->buffer_handle->
						voip_data_ul_buf_ptr);
					pvoip->buffer_handle->
						voip_data_ul_buf_ptr = NULL;
				}
				pvoip->buffer_handle->voip_data_dl_buf_ptr =
									NULL;
				kfree(pvoip->buffer_handle);
				pvoip->buffer_handle = NULL;
				voipinstcnt--;
				up(&sVoipAction);
				aError("voip_ioctl failed with ENOMEM 3");
				return -ENOMEM;
			}
			param_open.drv_handle = NULL;
			param_open.drv_type = AUDIO_DRIVER_VOIP;
			AUDIO_Ctrl_Trigger(ACTION_AUD_OpenVoIP,
				&param_open, NULL, 1);
			pvoip->buffer_handle->drv_handle =
				param_open.drv_handle;
			if (!pvoip->buffer_handle->drv_handle) {
				kfree(pvoip->buffer_handle->
				      voip_data_dl_buf_ptr);
				pvoip->buffer_handle->
				      voip_data_dl_buf_ptr = NULL;
				kfree(pvoip->buffer_handle->
				      voip_data_ul_buf_ptr);
				pvoip->buffer_handle->
				      voip_data_ul_buf_ptr = NULL;
				kfree(pvoip->buffer_handle);
				pvoip->buffer_handle = NULL;
				voipinstcnt--;
				up(&sVoipAction);
				aError("voip_ioctl failed with ENOMEM 4");
				return -ENOMEM;
			}
			/* set UL callback */
			parm_prepare.drv_handle =
				pvoip->buffer_handle->drv_handle;
			parm_prepare.cbParams.voipULCallback =
				HWDEP_VOIP_DumpUL_CB;
			parm_prepare.cbParams.pPrivateData = (void *)pvoip;
			AUDIO_Ctrl_Trigger(
				ACTION_AUD_SET_VOIP_UL_CB,
				&parm_prepare, NULL, 0);
			/* set DL callback */
			parm_prepare.drv_handle =
				pvoip->buffer_handle->drv_handle;
			parm_prepare.cbParams.voipDLCallback =
				HWDEP_VOIP_FillDL_CB;
			parm_prepare.cbParams.pPrivateData = (void *)pvoip;
			AUDIO_Ctrl_Trigger(
				ACTION_AUD_SET_VOIP_DL_CB,
				&parm_prepare, NULL, 0);
			/* VoIP is always 16K.
			No need to set the codec type here*/
			if (isvolte) {
				if (((data == VoIP_UL) &&
					((pvoip->codec_type_ul == VoIP_Codec_PCM_16K) ||
					(pvoip->codec_type_ul == VOIP_Codec_AMR_WB_7K)))
				|| ((data == VoIP_DL) &&
					((pvoip->codec_type_dl == VoIP_Codec_PCM_16K) ||
					(pvoip->codec_type_dl == VOIP_Codec_AMR_WB_7K)))) {
					/* VOIP_PCM_16K or
					VOIP_AMR_WB_MODE_7k */
				param_rate_change.codecID = 0x0A;
			} else
				param_rate_change.codecID = 0x06;
			AUDIO_Ctrl_Trigger(ACTION_AUD_RateChange,
					&param_rate_change, NULL, 0);
			}
			param_start.drv_handle =
				pvoip->buffer_handle->drv_handle;
			param_start.data = (void *)&pchip->voip_data;
			AUDIO_Ctrl_Trigger(ACTION_AUD_StartVoIP,
				&param_start, NULL, 0);
			pvoip->writecount = VOIP_FRAMES_IN_BUFFER;
			pvoip->status = voip_hwdep_status_started;
			aTrace(LOG_ALSA_INTERFACE, "Status made to voip_hwdep_status_started");
		} else {
			voipinstcnt++;
			/*Limit Voip instance to two*/
			if (voipinstcnt > 2)
				voipinstcnt = 2;
			aTrace(LOG_ALSA_INTERFACE,
					"VoIP_Ioctl_Start -> just increment "
					"the count, voip already started voipinstcnt=%d\n", voipinstcnt);
		}
		if (pvoip != NULL) {
			if (data == VoIP_UL)
				pvoip->ulstarted = 1;
			else
				pvoip->dlstarted = 1;
		}
		up(&sVoipAction);
		aTrace(LOG_ALSA_INTERFACE, "VoIP_Ioctl_Start END voipinstcnt=%d\n",voipinstcnt);
		break;
	case VoIP_Ioctl_Stop:
		get_user(data, (int __user *)arg);
		aTrace(LOG_ALSA_INTERFACE, "VoIP_Ioctl_Stop type=%d (2==UL) "
			"voipinstcnt=%u\n", data, voipinstcnt);
		rc = down_interruptible(&sVoipAction);
		if (rc)		
			return rc;
		aTrace(LOG_ALSA_INTERFACE, "VoIP_Ioctl_Stop acquired sVoipAction");
		if (data == VoIP_UL)
			pvoip->ulstarted = 0;
		else
			pvoip->dlstarted = 0;
		if (voipinstcnt >= 2)
		{
			voipinstcnt--;
			aTrace(LOG_ALSA_INTERFACE, "VoIP_Ioctl_Stop--1--voipinstcnt=%d\n", voipinstcnt);
		}	
		else if ((voipinstcnt < 2) ||
				(pvoip->ulstarted == 0 &&
					pvoip->dlstarted == 0)) {
			BRCM_AUDIO_Param_Stop_t param_stop;
			BRCM_AUDIO_Param_Close_t param_close;
			voipinstcnt = 0;
			aTrace(LOG_ALSA_INTERFACE, "VoIP_Ioctl_Stop--2--voipinstcnt=%d\n", voipinstcnt);
			if (pvoip->buffer_handle)
			param_stop.drv_handle =
				pvoip->buffer_handle->drv_handle;
			else
				param_stop.drv_handle = NULL;
			AUDIO_Ctrl_Trigger(ACTION_AUD_StopVoIP,
				&param_stop, NULL, 0);
			if (pvoip->buffer_handle)
			param_close.drv_handle =
				pvoip->buffer_handle->drv_handle;
			else
				param_close.drv_handle = NULL;
			AUDIO_Ctrl_Trigger(ACTION_AUD_CloseVoIP,
				&param_close, NULL, 1);
			if (pvoip->buffer_handle) {
			kfree(pvoip->buffer_handle->voip_data_dl_buf_ptr);
			pvoip->buffer_handle->voip_data_dl_buf_ptr = NULL;
			kfree(pvoip->buffer_handle->voip_data_ul_buf_ptr);
			pvoip->buffer_handle->voip_data_ul_buf_ptr = NULL;
			kfree(pvoip->buffer_handle);
			pvoip->buffer_handle = NULL;
			}
			pvoip->status = voip_hwdep_status_stopped;
			wake_up(&pvoip->sleep);
		}
		up(&sVoipAction);
		break;
	case VoIP_Ioctl_SetSource:
		aTrace(LOG_ALSA_INTERFACE ,
				" Warning: VoIP_Ioctl_SetSource"
				"is depreciated , please"
				"use mixer control VC-SEL instead\n");
		break;
	case VoIP_Ioctl_SetSink:
		aTrace(LOG_ALSA_INTERFACE ,
				" Warning: VoIP_Ioctl_SetSink"
				"is depreciated, please"
				"use mixer control VC-SEL instead\n");
		break;
	case VoIP_Ioctl_SetCodecType:
		aTrace(LOG_ALSA_INTERFACE, "VoIP_Ioctl_SetCodecType\n");
		copy_from_user(&val, (int __user *)arg,
				   sizeof(voip_codec_type_data_t));
		if (val.ul_dl_type == VoIP_UL) {
			pchip->voip_data.codec_type_ul = val.codec_type;
			aTrace(LOG_ALSA_INTERFACE,
				" VoIP_Ioctl_SetCodecType (UL) codec_type %ld\n",
				pchip->voip_data.codec_type_ul);
		/*pvoip = (bcm_caph_hwdep_voip_t *) hw->private_data;*/
			if (!pvoip)
				break;
			pvoip->codec_type_ul = pchip->voip_data.codec_type_ul;
		} else {
			pchip->voip_data.codec_type_dl = val.codec_type;
			aTrace(LOG_ALSA_INTERFACE,
				" VoIP_Ioctl_SetCodecType (DL) codec_type %ld\n",
				pchip->voip_data.codec_type_dl);
		/*pvoip = (bcm_caph_hwdep_voip_t *) hw->private_data;*/
			if (!pvoip)
				break;
			pvoip->codec_type_dl = pchip->voip_data.codec_type_dl;
		}
		/*Check whether in a VoLTE call*/
		/*If no, do nothing.*/
		/*If yes, do the NB<->WB switching*/
		if (isvolte) {
			BRCM_AUDIO_Param_RateChange_t param_rate_change;
			if (pvoip->ulstarted == 0 && pvoip->dlstarted == 0)
				break;
			if (!(pvoip->buffer_handle))
				break;
			if (!(pvoip->buffer_handle->drv_handle))
				break;
			AUDIO_DRIVER_Ctrl(pvoip->buffer_handle->drv_handle,
				AUDIO_DRIVER_SET_AMR, &pchip->voip_data);
			if (((val.ul_dl_type == VoIP_UL) &&
				((pvoip->codec_type_ul == VoIP_Codec_PCM_16K) ||
				(pvoip->codec_type_ul == VOIP_Codec_AMR_WB_7K)))
			|| ((val.ul_dl_type == VoIP_DL) &&
				((pvoip->codec_type_dl == VoIP_Codec_PCM_16K) ||
				(pvoip->codec_type_dl == VOIP_Codec_AMR_WB_7K))))
				/* VOIP_PCM_16K or VOIP_AMR_WB_MODE_7k */
					param_rate_change.codecID = 0x0A;
			else
					param_rate_change.codecID = 0x06;
			AUDIO_Ctrl_Trigger(ACTION_AUD_RateChange,
					&param_rate_change, NULL, 0);
		}
		break;
	case VoIP_Ioctl_SetBitrate:
		get_user(data, (int __user *)arg);
		pchip->voip_data.bitrate_index = (u32) data;
		aTrace(LOG_ALSA_INTERFACE,
				" VoIP_Ioctl_SetBitrate bitrate_index %ld,\n",
				pchip->voip_data.bitrate_index);
		/*pvoip = (bcm_caph_hwdep_voip_t *) hw->private_data;*/
		if (!pvoip)
			break;
		if (isvolte) {
			if (pvoip->ulstarted == 0 && pvoip->dlstarted == 0)
				break;
			if (!(pvoip->buffer_handle))
				break;
			if (!(pvoip->buffer_handle->drv_handle))
				break;
			AUDIO_DRIVER_Ctrl(pvoip->buffer_handle->drv_handle,
				AUDIO_DRIVER_SET_AMR, &pchip->voip_data);
		}
		break;
	case VoIP_Ioctl_GetSource:
		{
		s32 *psel;
		psel = pchip->streamCtl[CTL_STREAM_PANEL_VOICECALL - 1]
			.iLineSelect;
		data = (int)psel[0];
		put_user(data, (int __user *)arg);
		}
		break;
	case VoIP_Ioctl_GetSink:
		{
		s32 *psel;
		psel = pchip->streamCtl[CTL_STREAM_PANEL_VOICECALL - 1]
			.iLineSelect;
		data = (int)psel[1];
		put_user(data, (int __user *)arg);
		}
		break;
	case VoIP_Ioctl_GetCodecType:
		aTrace(LOG_ALSA_INTERFACE, "voip_ioctl in VoIP_Ioctl_GetCodecType");
		copy_from_user(&val, (int __user *)arg,
				   sizeof(voip_codec_type_data_t));
		if (val.ul_dl_type == VoIP_UL)
			val.codec_type = (int)pchip->voip_data.codec_type_ul;
		else
			val.codec_type = (int)pchip->voip_data.codec_type_dl;
		aTrace(LOG_ALSA_INTERFACE,
			" VoIP_Ioctl_GetCodecType (UL:DL) codec_type %ld:%ld\n",
			pchip->voip_data.codec_type_ul, pchip->voip_data.codec_type_dl);
		copy_to_user((int __user *)arg, &val,
				   sizeof(voip_codec_type_data_t));
		break;
	case VoIP_Ioctl_GetBitrate:
		data = (int)pchip->voip_data.bitrate_index;
		put_user(data, (int __user *)arg);
		break;
	case VoIP_Ioctl_GetMode:
		{
			AudioMode_t mode = AUDCTRL_GetAudioMode();
			put_user((int)mode, (int __user *)arg);
			aTrace(LOG_ALSA_INTERFACE,
					" VoIP_Ioctl_GetMode mode %d,\n",
					mode);
		}
		break;
	case VoIP_Ioctl_SetMode:
		aTrace(LOG_ALSA_INTERFACE ,
				" Warning: VoIP_Ioctl_SetMode"
				"is depreciated, please "
				"use mixer control VC-SEL instead\n");
		break;
	case VoIP_Ioctl_SetVoLTEDTX:
		get_user(data, (int __user *)arg);
		pchip->voip_data.isDTXEnabled = (u8) data;
		aTrace(LOG_ALSA_INTERFACE, " VoIP_Ioctl_SetVoLTEDTX %d,\n",
				pchip->voip_data.isDTXEnabled);
		/*pvoip = (bcm_caph_hwdep_voip_t *) hw->private_data;*/
		if (!pvoip)
			break;
		if (!(pvoip->buffer_handle))
			break;
		if (!(pvoip->buffer_handle->drv_handle))
			break;
		AUDIO_DRIVER_Ctrl(pvoip->buffer_handle->drv_handle,
				AUDIO_DRIVER_SET_DTX, &pchip->voip_data);
		break;
	case VoIP_Ioctl_SetVoLTEFlag:
		get_user(data, (int __user *)arg);
		pchip->voip_data.isVoLTE = (u8) data;
		aTrace(LOG_ALSA_INTERFACE, " VoIP_Ioctl_SetFlag isVoLTE %d,\n",
				pchip->voip_data.isVoLTE);
		isvolte = pchip->voip_data.isVoLTE;
		break;
	case VoIP_Ioctl_GetVoLTEFlag:
		data = (int)pchip->voip_data.isVoLTE;
		put_user(data, (int __user *)arg);
		break;
	case DSPCtrl_Ioctl_SPCtrl:
		if (dataptr == NULL)
			dataptr = kzalloc(sizeof(UserCtrl_data_t), GFP_KERNEL);
		else
			memset(dataptr, 0, sizeof(UserCtrl_data_t));
		if (!dataptr)
			return -ENOMEM;
		ret =
		    copy_from_user(dataptr, (int __user *)arg,
				   sizeof(UserCtrl_data_t));
		enable = (Boolean) dataptr->data[0];
		size = dataptr->data[1];
		ret =
		    AUDDRV_User_CtrlDSP(AUDDRV_USER_SP_CTRL, enable, size,
					(void *)&(dataptr->data[2]));
		if (!enable) {
			kfree(dataptr);
			dataptr = NULL;
		}
		break;
	case DSPCtrl_Ioctl_SPSetVar:
		/*
		 * Will move this part later after we separate the voip and
		 * dspctrl connections.
		 */
		if (dataptr == NULL)
			dataptr = kzalloc(sizeof(UserCtrl_data_t), GFP_KERNEL);
		else
			memset(dataptr, 0, sizeof(UserCtrl_data_t));
		if (!dataptr)
			return -ENOMEM;
		ret =
		    copy_from_user(dataptr, (int __user *)arg,
				   sizeof(UserCtrl_data_t));
		size = dataptr->data[0];
		ret =
		    AUDDRV_User_CtrlDSP(AUDDRV_USER_SP_VAR, enable, size,
					(void *)&(dataptr->data[2]));
		break;
	case DSPCtrl_Ioctl_SPQuery:
		if (dataptr == NULL)
			dataptr = kzalloc(sizeof(UserCtrl_data_t), GFP_KERNEL);
		else
			memset(dataptr, 0, sizeof(UserCtrl_data_t));
		if (!dataptr)
			return -ENOMEM;
		ret =
		    AUDDRV_User_CtrlDSP(AUDDRV_USER_SP_QUERY, enable, size,
					(void *)dataptr);
		if (ret < 0)
			return ret;
		if (copy_to_user
		    ((int __user *)arg, dataptr, sizeof(UserCtrl_data_t)))
			return -EFAULT;
		break;
	case DSPCtrl_Ioctl_EQCtrl:
		if (dataptr == NULL)
			dataptr = kzalloc(sizeof(UserCtrl_data_t), GFP_KERNEL);
		else
			memset(dataptr, 0, sizeof(UserCtrl_data_t));
		if (!dataptr)
			return -ENOMEM;
		if (copy_from_user
		    (dataptr, (int __user *)arg, sizeof(UserCtrl_data_t)))
			return -EFAULT;
		enable = (Boolean) dataptr->data[0];
		ret =
		    AUDDRV_User_CtrlDSP(AUDDRV_USER_EQ_CTRL, enable, size,
					(void *)&(dataptr->data[2]));
		if (!enable) {
			kfree(dataptr);
			dataptr = NULL;
		}
		break;
	case Ctrl_Ioctl_SWEQParm:
		aTrace(LOG_ALSA_INTERFACE,
			"ALSA-CAPH hwdep_ioctl Ctrl_Ioctl_SWEQParm");
		eq = kzalloc(sizeof(*eq), GFP_KERNEL);
		if (eq == NULL) {
			aError("treq_sysparm_t mem alloc failed");
			return -ENOMEM;
		}
		/* get the sysparm from driver
		 SW EQ is only for music playback for now*/
		if (copy_from_user(eq, (int __user *)arg,
			sizeof(struct treq_sysparm_t))) {
			if (eq != NULL) {
				kfree(eq);
				eq = NULL;
			}
			return -EFAULT;
		}
		ret = AUDDRV_Get_TrEqParm((void *)eq,
			sizeof(*eq), AUDIO_APP_MUSIC,
			(eq->data)[TREQ_DATA_SIZE-1]);
		if (!ret) {
			if (copy_to_user((void __user *)arg, eq,
			sizeof(*eq))) {
				if (eq != NULL) {
					kfree(eq);
					eq = NULL;
				}
				return -EFAULT;
			}
		}
		if (eq != NULL) {
			kfree(eq);
			eq = NULL;
		}
		break;
	default:
		ret = -ENOTTY;
		break;
	}
	return ret;
}
static ssize_t voip_mmap(struct file *filep, struct vm_area_struct *vma)
{
	return 0;
}
static unsigned int voip_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	bcm_caph_hwdep_voip_t *pvoip;
	voipdev *pvoipchrdevpvtdata;
	pvoipchrdevpvtdata = (voipdev *)file->private_data;
	pvoip = (bcm_caph_hwdep_voip_t *)pvoipchrdevpvtdata->pvoip;
	if (!pvoip) {
		aError("returning from poll\n");
		return 0;
	}
	poll_wait(file, &pvoip->sleep, wait);
	if (pvoip->status == voip_hwdep_status_started) {
		if (pvoip->frames_available_to_read > 0)
			mask = POLLIN | POLLRDNORM;
		if (pvoip->writecount > 0)	/* buffer available to write */
			mask |= POLLOUT | POLLWRNORM;
	}
	/* DEBUG("voip_poll mask %ld\n",mask); */
	return mask;
}
static struct class *voipclass;
/* File operations for audio logging */
static const struct file_operations voip_fops = {
	.owner = THIS_MODULE,
	.open = voip_open,
	.read = voip_read,
	.write = voip_write,
	.release = voip_release,
	.unlocked_ioctl = voip_ioctl,
	.mmap = voip_mmap,
	.poll = voip_poll,
#if 0
	.dsp_status = hwdep_dsp_status,
	.poll = hwdep_poll
#endif
};
int voipdevicecreate(voipdev *pvoipchrdevpvtdata)
{
	int err = 0;
	aError("M:Inside voipdevicecreate\n");
	err = register_chrdev_region(MKDEV(BCM_VOIP_CHRDEV_MAJOR, 0),
		1, "bcm_voip_chrdev");
	if (err < 0) {
		aError("error create hwdep device\n");
		return err;
	}
	cdev_init(&pvoipchrdevpvtdata->voipcdev, &voip_fops);
	pvoipchrdevpvtdata->voipcdev.owner = THIS_MODULE;
	pvoipchrdevpvtdata->voipcdev.ops = &voip_fops;
	if (cdev_add(&pvoipchrdevpvtdata->voipcdev,
			MKDEV(BCM_VOIP_CHRDEV_MAJOR, 0), 1)) {
		unregister_chrdev_region(MKDEV(BCM_VOIP_CHRDEV_MAJOR, 0), 1);
		return 1;
	}
	voipclass = class_create(THIS_MODULE, "bcm_voip_chrdev");
	if (IS_ERR(voipclass)) {
		cdev_del(&pvoipchrdevpvtdata->voipcdev);
		unregister_chrdev_region(MKDEV(BCM_VOIP_CHRDEV_MAJOR, 0), 1);
		return PTR_ERR(voipclass);
	}
	device_create(voipclass, NULL, MKDEV(BCM_VOIP_CHRDEV_MAJOR, 0), NULL,
		"bcm_voip_chrdev");
	sema_init(&sVoipAction, 1);
	return err;
}
int voipdevicedestroy(voipdev *pvoipchrdevpvtdata)
{
	device_destroy(voipclass, MKDEV(BCM_VOIP_CHRDEV_MAJOR, 0));
	class_destroy(voipclass);
	cdev_del(&pvoipchrdevpvtdata->voipcdev);
	unregister_chrdev_region(MKDEV(BCM_VOIP_CHRDEV_MAJOR, 0), 1);
	return 0;
}
#endif
void voip_log_capture(CAPTURE_POINT_t log_point, unsigned char *buf,
					int size) {
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	if (voip_log.voip_log_on <= 0)
		return;
	if (log_point == AUD_LOG_VOCODER_UL) {
		substream = &voip_log.substream_ul;
		runtime = &voip_log.runtime_ul;
		if (mutex_lock_interruptible(&voip_log.voip_ul_mutex))
			return;
	} else {
		substream = &voip_log.substream_dl;
		runtime = &voip_log.runtime_dl;
		if (mutex_lock_interruptible(&voip_log.voip_dl_mutex))
			return;
	}
	runtime->rate = AUDIO_SAMPLING_RATE_16000;
	runtime->sample_bits = 16;
	runtime->channels = AUDIO_CHANNEL_MONO;
	runtime->dma_area = buf;
	runtime->dma_bytes = 2*size;
	substream->runtime = runtime;
	if (log_point == AUD_LOG_VOCODER_UL)
		mutex_unlock(&voip_log.voip_ul_mutex);
	else
		mutex_unlock(&voip_log.voip_dl_mutex);
	logmsg_ready(substream, log_point);
}
