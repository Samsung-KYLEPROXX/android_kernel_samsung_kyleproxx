/****************************************************************************/
/*     Copyright 2009-2012  Broadcom Corporation.  All rights reserved.     */
/*     Unless you and Broadcom execute a separate written software license  */
/*	   agreement governing                                              */
/*     use of this software, this software is licensed to you under the     */
/*	   terms of the GNU General Public License version 2 (the GPL),     */
/*     available at                                                         */
/*                                                                          */
/*          http:*www.broadcom.com/licenses/GPLv2.php                      */
/*                                                                          */
/*     with the following added to such license:                            */
/*                                                                          */
/*     As a special exception, the copyright holders of this software give  */
/*     you permission to link this software with independent modules, and   */
/*     to copy and distribute the resulting executable under terms of your  */
/*     choice, provided that you also meet, for each linked independent     */
/*     module, the terms and conditions of the license of that module.      */
/*     An independent module is a module which is not derived from this     */
/*     software.  The special exception does not apply to any modifications */
/*     of the software.                                                     */
/*                                                                          */
/*     Notwithstanding the above, under no circumstances may you combine    */
/*     this software in any way with any other Broadcom software provided   */
/*     under a license other than the GPL, without Broadcom's express prior */
/*     written consent.                                                     */
/*                                                                          */
/****************************************************************************/
/**
*
*  @file   chal_caph_audioh_nvin.c
*
*  @brief  chal layer driver for caph audioh device driver
*
****************************************************************************/

#include "chal_caph.h"
#include "chal_caph_audioh.h"
#include "chal_caph_audioh_int.h"
#include <mach/rdb/brcm_rdb_audioh.h>
#include <mach/rdb/brcm_rdb_util.h>

/****************************************************************************
*                        G L O B A L   S E C T I O N
*****************************************************************************/

/****************************************************************************
* global variable definitions
*****************************************************************************/

/****************************************************************************
*                         L O C A L   S E C T I O N
*****************************************************************************/

/****************************************************************************
* local macro declarations
*****************************************************************************/

/****************************************************************************
* local typedef declarations
*****************************************************************************/

/****************************************************************************
* local variable definitions
*****************************************************************************/

/****************************************************************************
* local function declarations
*****************************************************************************/

/******************************************************************************
* local function definitions
*******************************************************************************/

/*============================================================================
*
* Function Name: chal_audio_nvinpath_digi_mic_enable(CHAL_HANDLE handle,
* cUInt16 enable)
*
* Description:  Enable the microphone on voice out path
*
* Parameters:   handle      : the voice input path handle.
*                 enable      : enable : bit0 - digital microphone 3
*                             : enable : bit1 - digital microphone 4
* Return:       None.
*
*============================================================================*/
cVoid chal_audio_nvinpath_digi_mic_enable(CHAL_HANDLE handle, cUInt16 enable)
{
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;
	cUInt32 reg_val = 0x0;

	reg_val = BRCM_READ_REG(base, AUDIOH_ADC_CTL);
	/* reg_val &= ~(AUDIOH_ADC_CTL_DMIC3_EN_MASK |
	 AUDIOH_ADC_CTL_DMIC4_EN_MASK); */

	if (enable & CHAL_AUDIO_CHANNEL_LEFT)
		reg_val |= AUDIOH_ADC_CTL_DMIC3_EN_MASK;

	if (enable & CHAL_AUDIO_CHANNEL_RIGHT)
		reg_val |= AUDIOH_ADC_CTL_DMIC4_EN_MASK;

	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_ADC_CTL, reg_val);

	return;

}

/*============================================================================
*
* Function Name: chal_audio_nvinpath_digi_mic_disable(CHAL_HANDLE handle,
* cUInt16 disable)
*
* Description:  Disable the microphone on voice out path
*
* Parameters:   handle      : the voice input path handle.
*                 disable      : disable : bit0 - digital microphone 1
*                             : disable : bit1 - digital microphone 2
* Return:       None.
*
*============================================================================*/
cVoid chal_audio_nvinpath_digi_mic_disable(CHAL_HANDLE handle, cUInt16 disable)
{
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;
	cUInt32 reg_val = 0x0;

	reg_val = BRCM_READ_REG(base, AUDIOH_ADC_CTL);

	if (disable & CHAL_AUDIO_CHANNEL_LEFT)
		reg_val &= ~(AUDIOH_ADC_CTL_DMIC3_EN_MASK);

	if (disable & CHAL_AUDIO_CHANNEL_RIGHT)
		reg_val &= ~(AUDIOH_ADC_CTL_DMIC4_EN_MASK);

	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_ADC_CTL, reg_val);

	return;

}

/*============================================================================
*
* Function Name: UInt8 chal_audio_nvinpath_digi_mic_enable_read(
*                                      CHAL_HANDLE handle)
*
* Description:  Get the digital microphone enable status
*
* Parameters:   handle      : the voice input path handle.
* Return:       status        : bit0 - digital microphone 3
*                             : bit1 - digital microphone 4
*============================================================================*/
UInt8 chal_audio_nvinpath_digi_mic_enable_read(CHAL_HANDLE handle)
{
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;
	cUInt32 reg_val = 0x0;

	reg_val = BRCM_READ_REG(base, AUDIOH_ADC_CTL);

	reg_val &= AUDIOH_ADC_CTL_DMIC3_EN_MASK | AUDIOH_ADC_CTL_DMIC4_EN_MASK;
	reg_val >>= AUDIOH_ADC_CTL_DMIC3_EN_SHIFT;

	return (UInt8) reg_val;
}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_int_enable(CHAL_HANDLE handle,
*                            cUInt16 thr_int_enable, cUInt16 err_int_enable )
*
* Description:  Enable interrupt on voice out path
*
* Parameters:   handle      : the voice input pathhandle.
*                 enable      : enable flag
*
* Return:       None.
*
*============================================================================*/
cVoid chal_audio_nvinpath_int_enable(CHAL_HANDLE handle, cUInt16 thr_int_enable,
				     cUInt16 err_int_enable)
{
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;
	cUInt32 reg_val;

	reg_val = BRCM_READ_REG(base, AUDIOH_AUDIO_INTC);

	if (thr_int_enable == CHAL_AUDIO_ENABLE) {
		reg_val |= AUDIOH_AUDIO_INTC_NVINR_INTEN_MASK;
		reg_val |= AUDIOH_AUDIO_INTC_NVINL_INTEN_MASK;
	} else {
		reg_val &= ~AUDIOH_AUDIO_INTC_NVINR_INTEN_MASK;
		reg_val &= ~AUDIOH_AUDIO_INTC_NVINL_INTEN_MASK;
	}

	if (err_int_enable == CHAL_AUDIO_ENABLE) {
		reg_val |= AUDIOH_AUDIO_INTC_NVIN_FIFOR_ERRINT_EN_MASK;
		reg_val |= AUDIOH_AUDIO_INTC_NVIN_FIFOL_ERRINT_EN_MASK;
	} else {
		reg_val &= ~AUDIOH_AUDIO_INTC_NVIN_FIFOR_ERRINT_EN_MASK;
		reg_val &= ~AUDIOH_AUDIO_INTC_NVIN_FIFOL_ERRINT_EN_MASK;
	}

	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_AUDIO_INTC, reg_val);

	return;
}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_dma_enable(CHAL_HANDLE handle,
*                            Boolean dma_enable)
*
* Description:  Enable DAM on voice out path
*
* Parameters:   handle      : the voice input path handle.
*                 enable      : enable flag
*
* Return:       None.
*
*============================================================================*/
cVoid chal_audio_nvinpath_dma_enable(CHAL_HANDLE handle, Boolean dma_enable)
{
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;
	cUInt32 reg_val;

	reg_val = BRCM_READ_REG(base, AUDIOH_DMA_CTL);

	if (dma_enable == CHAL_AUDIO_ENABLE) {
		reg_val |= AUDIOH_DMA_CTL_NVINR_DMA_EN_MASK;
		reg_val |= AUDIOH_DMA_CTL_NVINL_DMA_EN_MASK;
	} else {
		reg_val &= ~AUDIOH_DMA_CTL_NVINR_DMA_EN_MASK;
		reg_val &= ~AUDIOH_DMA_CTL_NVINL_DMA_EN_MASK;
	}

	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_DMA_CTL, reg_val);

	return;
}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_get_dma_port_addr(CHAL_HANDLE handle,
*		UInt32 *dma_addr)
*
* Description:  Get the DMA port address on noise vocie in path
*
* Parameters:   handle     - audio chal handle.
*               *dma_addr  - the buffer to save dma port address.
* Return:       None.
*
*============================================================================*/

cVoid chal_audio_nvinpath_get_dma_port_addr(CHAL_HANDLE handle,
					    UInt32 *dma_addr)
{
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;

	*dma_addr = (UInt32) (base) + AUDIOH_NVIN_FIFOR_DATA0_OFFSET;

	return;
}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_get_dma_port_addr2(
* CHAL_HANDLE handle, UInt32 *dma_addr)
*
* Description:  Get the DMA port address 2  on noise vocie in path
*
* Parameters:   handle     - audio chal handle.
*		*dma_addr  - the buffer to save dma port address.
* Return:       None.
*
*============================================================================*/

cVoid chal_audio_nvinpath_get_dma_port_addr2(CHAL_HANDLE handle,
					     UInt32 *dma_addr)
{
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;

	*dma_addr = (UInt32) (base) + AUDIOH_NVIN_FIFOL_DATA0_OFFSET;

	return;
}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_set_bits_per_sample(
*	CHAL_HANDLE handle, cUInt16 bits)
*
* Description:  Set the sample bits on voice input path
*
* Parameters:   handle  the voice input path handle.
*                 bits - sample bits value.
* Return:       None.
*
*============================================================================*/

cVoid chal_audio_nvinpath_set_bits_per_sample(CHAL_HANDLE handle, UInt16 bits)
{
	cUInt32 reg_val;
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;

	reg_val = BRCM_READ_REG(base, AUDIOH_NVIN_FIFO_CTRL);

	if (bits >= 24)
		reg_val |= AUDIOH_NVIN_FIFO_CTRL_NVIN_FIFO_24BIT_MASK;
	else
		reg_val &= ~AUDIOH_NVIN_FIFO_CTRL_NVIN_FIFO_24BIT_MASK;

	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_NVIN_FIFO_CTRL, reg_val);

	return;
}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_set_bits_per_sample(
*	CHAL_HANDLE handle, Boolean mode)
*
* Description:  Set the mode on voice input path
*
* Parameters:   handle  the earpiece path handle.
*                 mode - true : stereo, false : mono.
* Return:       None.
*
*============================================================================*/

cVoid chal_audio_nvinpath_set_mono_stereo(CHAL_HANDLE handle, Boolean mode)
{
	cUInt32 reg_val;
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;

	reg_val = BRCM_READ_REG(base, AUDIOH_NVIN_FIFO_CTRL);

	if (mode == CHAL_AUDIO_ENABLE)
		reg_val |= AUDIOH_NVIN_FIFO_CTRL_NVIN_FIFO_MONO_MASK;
	else	/* stereo */
		reg_val &= ~AUDIOH_NVIN_FIFO_CTRL_NVIN_FIFO_MONO_MASK;

	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_NVIN_FIFO_CTRL, reg_val);

	return;
}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_set_pack(CHAL_HANDLE handle,
* Boolean pack)
*
* Description:  Set the sample pack/upack mode on voice input path
*
* Parameters:   handle  the voice input path handle.
*                 pack - true : pack, false : unpack.
* Return:       None.
*
*============================================================================*/

cVoid chal_audio_nvinpath_set_pack(CHAL_HANDLE handle, Boolean pack)
{
	cUInt32 reg_val;
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;

	reg_val = BRCM_READ_REG(base, AUDIOH_NVIN_FIFO_CTRL);

	if (pack == CHAL_AUDIO_ENABLE)
		reg_val |= AUDIOH_NVIN_FIFO_CTRL_NVIN_FIFO_PACK_MASK;
	else
		reg_val &= ~AUDIOH_NVIN_FIFO_CTRL_NVIN_FIFO_PACK_MASK;

	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_NVIN_FIFO_CTRL, reg_val);

	return;
}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_select_sidetone (CHAL_HANDLE handle,
*                    Boolean read_sidetone)
*
* Description:  Select sidetone or not
*
* Parameters:   handle  the voice input path handle.
*                 read_sidetone - selcetd flag for left and right channels
* Return:       None.
*
*============================================================================*/

cVoid chal_audio_nvinpath_select_sidetone(CHAL_HANDLE handle,
					  cUInt16 read_sidetone)
{
	cUInt32 reg_val;
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;

	/* Read the current contents */
	reg_val = BRCM_READ_REG(base, AUDIOH_NVIN_FIFO_CTRL);

	/* Clear the paths that were enabled before */
	reg_val &= ~AUDIOH_NVIN_FIFO_CTRL_SIDETONE_SEL_L_MASK;
	reg_val &= ~AUDIOH_NVIN_FIFO_CTRL_SIDETONE_SEL_R_MASK;

	if (read_sidetone & CHAL_AUDIO_CHANNEL_LEFT) {
		/* Enable Left Channel to read side tone data */
		reg_val |= AUDIOH_NVIN_FIFO_CTRL_SIDETONE_SEL_L_MASK;
	}

	if (read_sidetone & CHAL_AUDIO_CHANNEL_RIGHT) {
		/* Enable Right Channel to read side tone data */
		reg_val |= AUDIOH_NVIN_FIFO_CTRL_SIDETONE_SEL_R_MASK;
	}

	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_NVIN_FIFO_CTRL, reg_val);

	return;
}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_clr_fifo(CHAL_HANDLE handle)
*
* Description:  Clear the FIFO on voice input path
*
* Parameters:   handle  the voice input path handle.
*
* Return:       None.
*
*============================================================================*/

cVoid chal_audio_nvinpath_clr_fifo(CHAL_HANDLE handle)
{
	cUInt32 reg_val;
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;

	reg_val = BRCM_READ_REG(base, AUDIOH_NVIN_FIFO_CTRL);
	reg_val |= AUDIOH_NVIN_FIFO_CTRL_NVIN_FIFO_CLEAR_MASK;
	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_NVIN_FIFO_CTRL, reg_val);

	reg_val &= ~AUDIOH_NVIN_FIFO_CTRL_NVIN_FIFO_CLEAR_MASK;
	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_NVIN_FIFO_CTRL, reg_val);

	return;
}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_set_fifo_thres(CHAL_HANDLE handle,
*                        cUInt16 thres, cUInt16 thres_2)
*
* Description:  Set the threshold for FIFO on voice input path
*
* Parameters:   handle  the vocie input path handle.
*
*                 thres   - threshold 1
*                 thres_2 - threshold 2
* Return:       None.
*
*============================================================================*/

cVoid chal_audio_nvinpath_set_fifo_thres(CHAL_HANDLE handle, cUInt16 thres,
					 cUInt16 thres_2)
{
	cUInt32 reg_val;
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;

	reg_val = BRCM_READ_REG(base, AUDIOH_NVIN_FIFO_CTRL);
	reg_val &= ~(AUDIOH_NVIN_FIFO_CTRL_NVIN_FIFO_THRES_MASK);
	reg_val &= ~(AUDIOH_NVIN_FIFO_CTRL_NVIN_FIFO_THRES2_MASK);
	reg_val |= thres;
	reg_val |= thres_2 << AUDIOH_NVIN_FIFO_CTRL_NVIN_FIFO_THRES2_SHIFT;

	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_NVIN_FIFO_CTRL, reg_val);

	return;
}

/*============================================================================
*
* Function Name: cUInt32 chal_audio_nvinpath_read_fifo_status(
* CHAL_HANDLE handle)
*
* Description:  Read the FIFO status on voice input path
*
* Parameters:   handle  the voice input path handle.
*
* Return:       the status.
*
*============================================================================*/

cUInt32 chal_audio_nvinpath_read_fifo_status(CHAL_HANDLE handle)
{
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;
	cUInt32 status = 0;

	/* Read NVIN FIFO status */
	status = BRCM_READ_REG(base, AUDIOH_NVIN_FIFOR_STATUS);

	/* Add NVIN FIFO interrupt status */
	status |= chal_audio_nvinpath_read_int_status(handle);

	return status;
}

/*============================================================================
*
* Function Name: cUInt32 chal_audio_nvinpath_read_fifo_status(
* CHAL_HANDLE handle)
*
* Description:  Read the FIFO status on voice input path
*
* Parameters:   handle  the voice input path handle.
*
* Return:       the status.
*
*============================================================================*/

cUInt32 chal_audio_nvinpath_read_fifo_status2(CHAL_HANDLE handle)
{
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;
	cUInt32 status = 0;

	/* Read NVIN FIFO status */
	status = BRCM_READ_REG(base, AUDIOH_NVIN_FIFOL_STATUS);

	/* Add NVIN FIFO interrupt status */
	status |= chal_audio_nvinpath_read_int_status(handle);

	return status;
}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_read_fifo(CHAL_HANDLE handle,
*                        cUInt32 *src, cUInt32 length)
*
* Description:  Read the data from FIFO on voice input path
*
* Parameters:   handle  the voice input path handle.
*
*                 *src    - the data buffer address
*                 length  - the data buffer length
*                 ign_udf  - keep reading the fifo even on uderflow
*
* Return:       None.
*
*============================================================================*/

cUInt32 chal_audio_nvinpath_read_fifo(CHAL_HANDLE handle, cUInt32 *src,
				      cUInt32 length, Boolean ign_udf)
{
	UInt32 n;
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;

	if (ign_udf == CHAL_AUDIO_STOP_ON_FIFO_UNDERFLOW) {
		for (n = 0; n < length; n++) {
			/* Check if the FIFO is getting underflow or not, */
			/* if ign_udf flag is not set */
			if (chal_audio_nvinpath_read_fifo_status(handle) &
			    CHAL_AUDIO_FIFO_STATUS_NEAR_EMPTY) {
				break;
			}
			*src++ = BRCM_READ_REG(base, AUDIOH_NVIN_FIFOR_DATA0);
		}
	} else {
		for (n = 0; n < length; n++)
			*src++ = BRCM_READ_REG(base, AUDIOH_NVIN_FIFOR_DATA0);
	}
	return n;

}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_read_fifo2(CHAL_HANDLE handle,
*                        cUInt32 *src, cUInt32 length)
*
* Description:  Read the data from FIFO on voice input path
*
* Parameters:   handle  the voice input path handle.
*
*                 *src    - the data buffer address
*                 length  - the data buffer length
*                 ign_udf  - keep reading the fifo even on uderflow
*
* Return:       None.
*
*============================================================================*/

cUInt32 chal_audio_nvinpath_read_fifo2(CHAL_HANDLE handle, cUInt32 *src,
				       cUInt32 length, Boolean ign_udf)
{
	UInt32 n;
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;

	if (ign_udf == CHAL_AUDIO_STOP_ON_FIFO_UNDERFLOW) {
		for (n = 0; n < length; n++) {
			/* Check if the FIFO is getting underflow or not,*/
			/* if ign_udf flag is not set */
			if (chal_audio_nvinpath_read_fifo_status(handle) &
			    CHAL_AUDIO_FIFO_STATUS_NEAR_EMPTY) {
				break;
			}
			*src++ = BRCM_READ_REG(base, AUDIOH_NVIN_FIFOL_DATA0);
		}
	} else {
		for (n = 0; n < length; n++)
			*src++ = BRCM_READ_REG(base, AUDIOH_NVIN_FIFOL_DATA0);
	}
	return n;

}

/*============================================================================
*
* Function: cUInt32 chal_audio_nvinpath_read_int_status(CHAL_HANDLE handle)
*
* Description:  Read the interrupt status for the Voice In path
*
* Parameters:   handle      - audio chal handle.
*
* Return:       None.
*
*============================================================================*/

cUInt32 chal_audio_nvinpath_read_int_status(CHAL_HANDLE handle)
{
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;
	cUInt32 status = 0;
	cUInt32 reg_val = 0;

	/* Read FIFO interrupt status of all paths */
	reg_val = BRCM_READ_REG(base, AUDIOH_AUDIO_INT_STATUS);

	/* Check for NVIN path FIFO error interrupt */
	if (reg_val & AUDIOH_AUDIO_INT_STATUS_NVIN_FIFOR_ERR_MASK)
		status |= CHAL_AUDIO_FIFO_STATUS_ERR_INT;

	if (reg_val & AUDIOH_AUDIO_INT_STATUS_NVIN_FIFOL_ERR_MASK)
		status |= CHAL_AUDIO_FIFO_STATUS_ERR_INT;

	/* Check for NVIN path FIFO threshold interrupt */
	if (reg_val & AUDIOH_AUDIO_INT_STATUS_NVINR_INT_MASK)
		status |= CHAL_AUDIO_FIFO_STATUS_THR_INT;

	if (reg_val & AUDIOH_AUDIO_INT_STATUS_NVINL_INT_MASK)
		status |= CHAL_AUDIO_FIFO_STATUS_THR_INT;

	return status;
}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_int_clear(CHAL_HANDLE handle,
* cUInt16 thr_int_enable, cUInt16 err_int_enable )
*
* Description:  Clear interrupt on Noise Microphone path
*
* Parameters:   handle      : the Noise Microphone input pathhandle.
*                      thr_int      : Clear FIFO threshold interrupt
*                      err_int      : Clear FIFO Error interrupt
*
* Return:       None.
*
*============================================================================*/
cVoid chal_audio_nvinpath_int_clear(CHAL_HANDLE handle, Boolean thr_int,
				    Boolean err_int)
{
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;
	cUInt32 reg_val = 0;

	if (thr_int == TRUE) {
		reg_val |= AUDIOH_AUDIO_INT_STATUS_NVINR_INT_MASK;
		reg_val |= AUDIOH_AUDIO_INT_STATUS_NVINL_INT_MASK;
	}

	if (err_int == TRUE) {
		reg_val |= AUDIOH_AUDIO_INT_STATUS_NVIN_FIFOR_ERR_MASK;
		reg_val |= AUDIOH_AUDIO_INT_STATUS_NVIN_FIFOL_ERR_MASK;
	}

	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_AUDIO_INT_STATUS, reg_val);

	return;
}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_set_cic_scale(CHAL_HANDLE handle,
*  UInt32 dmic3_coarse_scale, UInt32 dmic3_fine_scale,
*  UInt32 dmic4_coarse_scale, UInt32 dmic4_fine_scale)
*
* Description:  Set the CIC coarse/fine scale for the Digital MIC 3 & 4
*
* Parameters:   handle  the noise voice input path handle.
*
* Return:       None.
*
*============================================================================*/

cVoid chal_audio_nvinpath_set_cic_scale(CHAL_HANDLE handle,
					UInt32 dmic3_coarse_scale,
					UInt32 dmic3_fine_scale,
					UInt32 dmic4_coarse_scale,
					UInt32 dmic4_fine_scale)
{
	UInt32 dmic3_scale = dmic3_coarse_scale | dmic3_fine_scale;
	UInt32 dmic4_scale = dmic4_coarse_scale | dmic4_fine_scale;
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;

	dmic3_scale &=
	    (AUDIOH_NVIN_FILTER_CTRL_DMIC3_CIC_BIT_SEL_MASK |
	     AUDIOH_NVIN_FILTER_CTRL_DMIC3_CIC_FINE_SCL_MASK);
	dmic4_scale <<= (AUDIOH_NVIN_FILTER_CTRL_DMIC4_CIC_FINE_SCL_SHIFT);
	dmic4_scale &=
	    (AUDIOH_NVIN_FILTER_CTRL_DMIC4_CIC_BIT_SEL_MASK |
	     AUDIOH_NVIN_FILTER_CTRL_DMIC4_CIC_FINE_SCL_MASK);

	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_NVIN_FILTER_CTRL,
		       (dmic4_scale | dmic3_scale));

	return;
}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_set_each_cic_scale(
*                              CHAL_HANDLE handle,
*                              CAPH_AUDIOH_MIC_GAIN_e micGainSelect,
*                              UInt32 gain)
*
* Description:  Set the each CIC coarse/fine scale for the Digital MIC 3 & 4
* Parameters:   handle  the voice input path handle.
*               micGainSelect  the exact mic gain
*               gain  the gain
*
* Return:       None.
*
*============================================================================*/

cVoid chal_audio_nvinpath_set_each_cic_scale(CHAL_HANDLE handle,
					     CAPH_AUDIOH_MIC_GAIN_e
					     micGainSelect, UInt32 gain)
{
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;
	UInt32 value = 0;
	/* Read VIN path FIFO status */
	value = BRCM_READ_REG(base, AUDIOH_NVIN_FILTER_CTRL);
	switch (micGainSelect) {
	case CAPH_AUDIOH_MIC3_COARSE_GAIN:
		value &= ~(AUDIOH_NVIN_FILTER_CTRL_DMIC3_CIC_BIT_SEL_MASK);
		gain <<= (AUDIOH_NVIN_FILTER_CTRL_DMIC3_CIC_BIT_SEL_SHIFT);
		value |= gain;
		break;

	case CAPH_AUDIOH_MIC3_FINE_GAIN:
		value &= ~(AUDIOH_NVIN_FILTER_CTRL_DMIC3_CIC_FINE_SCL_MASK);
		gain <<= (AUDIOH_NVIN_FILTER_CTRL_DMIC3_CIC_FINE_SCL_SHIFT);
		value |= gain;
		break;

	case CAPH_AUDIOH_MIC4_COARSE_GAIN:
		value &= ~(AUDIOH_NVIN_FILTER_CTRL_DMIC4_CIC_BIT_SEL_MASK);
		gain <<= (AUDIOH_NVIN_FILTER_CTRL_DMIC4_CIC_BIT_SEL_SHIFT);
		value |= gain;
		break;

	case CAPH_AUDIOH_MIC4_FINE_GAIN:
		value &= ~(AUDIOH_NVIN_FILTER_CTRL_DMIC4_CIC_FINE_SCL_MASK);
		gain <<= (AUDIOH_NVIN_FILTER_CTRL_DMIC4_CIC_FINE_SCL_SHIFT);
		value |= gain;
		break;

	default:
		/* xassert(0, micGainSelect); */
		break;
	}

	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_NVIN_FILTER_CTRL, value);

	return;
}

/*============================================================================
*
* Function Name: chal_audio_nvinpath_set_digimic_clkdelay(CHAL_HANDLE handle,
* cUInt16 enable)
*
* Description:  Set the delay for sampling the DIGITAL MIC3,4 signals
*
* Parameters:   handle      : the voice input path handle.
*               delay        : delay in 5.95 usec max possible is 41.6 usec
*
* Return:       None.
*
*============================================================================*/
cVoid chal_audio_nvinpath_set_digimic_clkdelay(CHAL_HANDLE handle,
					       cUInt16 delay)
{
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;
	cUInt32 reg_val;

	reg_val = BRCM_READ_REG(base, AUDIOH_ADC_CTL);
	reg_val &= ~(AUDIOH_ADC_CTL_DMIC_CLK_DELAY2_MASK);

	delay <<= AUDIOH_ADC_CTL_DMIC_CLK_DELAY2_SHIFT;

	if (delay > AUDIOH_ADC_CTL_DMIC_CLK_DELAY2_MASK)
		delay = AUDIOH_ADC_CTL_DMIC_CLK_DELAY2_MASK;

	reg_val |= delay;

	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_ADC_CTL, reg_val);

	return;

}

/*============================================================================
*
* Function Name: cVoid chal_audio_nvinpath_set_filter(CHAL_HANDLE handle,
* cUInt16 filter)
*
* Description:  set Filter type for the nvin path
*
* Parameters:   handle    - audio chal handle.
*                 filter    -  0: Linear Phase, 1 Minimum Phase
* Return:       None.
*
*============================================================================*/
cVoid chal_audio_nvinpath_set_filter(CHAL_HANDLE handle, cUInt16 filter)
{
	cUInt32 base = ((ChalAudioCtrlBlk_t *) handle)->audioh_base;
	cUInt32 reg_val;

	reg_val = BRCM_READ_REG(base, AUDIOH_MIN_PHASE);
	reg_val &=
	    ~(AUDIOH_MIN_PHASE_DMIC3_MIN_PHASE_MASK |
	      AUDIOH_MIN_PHASE_DMIC4_MIN_PHASE_MASK);

	if (filter & CHAL_AUDIO_MINIMUM_PHASE_FILTER)
		reg_val |= AUDIOH_MIN_PHASE_DMIC3_MIN_PHASE_MASK;

	if (filter & CHAL_AUDIO_MINIMUM_PHASE_FILTER_L)
		reg_val |= AUDIOH_MIN_PHASE_DMIC4_MIN_PHASE_MASK;

	/* Set the required setting */
	BRCM_WRITE_REG(base, AUDIOH_MIN_PHASE, reg_val);

	return;
}
