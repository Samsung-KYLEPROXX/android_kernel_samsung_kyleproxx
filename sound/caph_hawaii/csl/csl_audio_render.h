/******************************************************************************/
/* Copyright 2009, 2010 Broadcom Corporation.  All rights reserved.           */
/*     Unless you and Broadcom execute a separate written software license    */
/*     agreement governing use of this software, this software is licensed to */
/*     you under the terms of the GNU General Public License version 2        */
/*    (the GPL), available at                                                 */
/*                                                                            */
/*          http://www.broadcom.com/licenses/GPLv2.php                        */
/*                                                                            */
/*     with the following added to such license:                              */
/*                                                                            */
/*     As a special exception, the copyright holders of this software give    */
/*     you permission to link this software with independent modules, and to  */
/*     copy and distribute the resulting executable under terms of your       */
/*     choice, provided that you also meet, for each linked independent       */
/*     module, the terms and conditions of the license of that module.        */
/*     An independent module is a module which is not derived from this       */
/*     software.  The special exception does not apply to any modifications   */
/*     of the software.                                                       */
/*                                                                            */
/*     Notwithstanding the above, under no circumstances may you combine this */
/*     software in any way with any other Broadcom software provided under a  */
/*     license other than the GPL, without Broadcom's express prior written   */
/*     consent.                                                               */
/******************************************************************************/
/**
*
*   @file   csl_audio_render.h
*
*   @brief  This file contains the definition for render CSL driver layer
*
****************************************************************************/

#ifndef _CSL_AUDIO_RENDER_
#define _CSL_AUDIO_RENDER_

#include <linux/spinlock.h>

enum _DYNDMA_STATE {
	DYNDMA_DUMMY,
	DYNDMA_NORMAL,
	DYNDMA_TRIGGER,
	DYNDMA_LOW_DONE,
	DYNDMA_LOW_RDY,
};
#define DYNDMA_STATE enum __DYNDMA_STATE
#define DYNDMA_COPY_SIZE 0x2000 /*in bytes*/
extern struct completion completeDynDma;

typedef void (*CSL_AUDRENDER_CB)(UInt32 streamID,
			UInt32 buffer_status);

struct _CSL_CAPH_Render_Drv_t {
	UInt32 streamID;
	CSL_CAPH_DEVICE_e source;
	CSL_CAPH_DEVICE_e sink;
	CSL_CAPH_PathID pathID;
	CSL_AUDRENDER_CB dmaCB;
	CSL_CAPH_DMA_CHNL_e dmaCH;
	UInt8 *ringBuffer;
	UInt32 numBlocks;
	UInt32 blockSize;
	UInt32 readyBlockStatus;
	spinlock_t readyStatusLock;
	spinlock_t configLock;
	UInt32 blockIndex;
	int numBlocks2;
	int readyBlockIndex;
	atomic_t dmaState;
	atomic_t availBytes;
	int first;
};
#define CSL_CAPH_Render_Drv_t struct _CSL_CAPH_Render_Drv_t
/**
*
*  @brief  initialize the audio render block
*
*  @param  source (in) The source of the audio stream.
*  @param  sink (in) The sink of the audio stream.
*
*  @return UInt32 the obtained stream id.
*****************************************************************************/
UInt32 csl_audio_render_init(CSL_CAPH_DEVICE_e source, CSL_CAPH_DEVICE_e sink);

/**
*
*  @brief  deinitialize the audio render
*
*  @param  streamID (in) Render stream ID
*
*  @return Result_t status
*****************************************************************************/
Result_t csl_audio_render_deinit(UInt32 streamID);

/**
*
*  @brief  configure a audio render for processing audio stream
*
*  @param   sampleRate (in) Sampling rate for this stream
*  @param   numChannels (in) number of channels
*  @param   bitsPerSample (in) bits/sample
*  @param   ringBuffer (in) start address of ring buffer
*  @param   numBlocks (in) number of blocks in ring buffer
*  @param   blockSize (in) size per each block
*  @param   csl_audio_render_cb (in) cb registered by render driver
*  @param   streamID (in) stream id of this stream
*  @param   mixMode (in) arm2sp mix mode
*
*  @return Result_t status
*****************************************************************************/
Result_t csl_audio_render_configure(AUDIO_SAMPLING_RATE_t sampleRate,
				    AUDIO_NUM_OF_CHANNEL_t numChannels,
				    AUDIO_BITS_PER_SAMPLE_t bitsPerSample,
				    UInt8 *ringBuffer,
				    UInt32 numBlocks,
				    UInt32 blockSize,
				    CSL_AUDRENDER_CB csl_audio_render_cb,
				    UInt32 streamID,
					int mixMode);

/**
*
*  @brief  start the stream for audio render
*
*  @param   streamID  (in) render audio stream id
*
*  @return Result_t status
*****************************************************************************/
Result_t csl_audio_render_start(UInt32 streamID);

/**
*
*  @brief  stop the stream for audio render
*
*  @param   streamID  (in) render audio stream id
*
*  @return Result_t status
*****************************************************************************/
Result_t csl_audio_render_stop(UInt32 streamID);

/**
*
*  @brief  pause the stream for audio render
*
*  @param   streamID  (in) render audio stream id
*
*  @return Result_t status
*****************************************************************************/
Result_t csl_audio_render_pause(UInt32 streamID);

/**
*
*  @brief  resume the stream for audio render
*
*  @param   streamID  (in) render audio stream id
*
*  @return Result_t status
*****************************************************************************/
Result_t csl_audio_render_resume(UInt32 streamID);
/****************************************************************************
*
*  Function Name: csl_audio_render_buffer_ready
*
*  Description: set the SW_READY to aadmac when a new buffer is ready
*
****************************************************************************/
Result_t csl_audio_render_buffer_ready(UInt32 streamID, int size, int offset);
/**
*
*  @brief  get current position for this stream
*
*  @param   streamID  (in) render audio stream id
*
*  @return UInt16 currmemptr in SR
*****************************************************************************/
UInt16 csl_audio_render_get_current_position(UInt32 streamID);

/**
*
*  @brief  get current dma processing buffer for this stream
*
*  @param   streamID  (in) render audio stream id
*
*  @return UInt16 current buffer being processed by dma: 1-low 2-hi 0-none
*****************************************************************************/
UInt16 csl_audio_render_get_current_buffer(UInt32 streamID);

CSL_CAPH_Render_Drv_t *GetRenderDriverByType(UInt32 streamID);
void csl_audio_render_set_dma_size(int rate);
int csl_audio_render_get_num_blocks(void);
int csl_audio_render_get_dma_size(void);
#endif /* _CSL_AUDIO_RENDER_ */
