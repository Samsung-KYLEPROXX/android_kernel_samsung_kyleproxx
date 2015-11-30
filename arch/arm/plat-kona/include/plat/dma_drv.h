/*******************************************************************************
* Copyright 2011 Broadcom Corporation.  All rights reserved.
*
*             @file     arch/arm/plat-kona/include/plat/dma_drv.h
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

/**
*
*   @file   dma_drv.h
*
*   @brief  DMA device driver defines and prototypes.
*
****************************************************************************/
/**
*
* @defgroup DMAGroup Direct Memory Access
* @ingroup CSLGroup
* @brief This group defines the APIs for DMA driver

Click here to navigate back to the Chip Support Library Overview page: \ref CSLOverview. \n
*****************************************************************************/
#ifndef _DMA_DRV_H_
#define _DMA_DRV_H_

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @addtogroup DMAGroup 
 * @{
 */

/**
*
*  DMA driver status definition
*
*****************************************************************************/
#define   DMADRV_STATUS_t              DMADRV_STATUS
	typedef enum {
		DMADRV_STATUS_CLOSED,
		DMADRV_STATUS_OPEN,
		DMADRV_STATUS_OK,
		DMADRV_STATUS_FAIL
	} DMADRV_STATUS_t;

/**
*
*  DMA driver callback status definition
*
*****************************************************************************/
#define   DMADRV_CALLBACK_STATUS_t     DMADRV_CALLBACK_STATUS
	typedef enum {
		DMADRV_CALLBACK_OK = 0,
		DMADRV_CALLBACK_FAIL
	} DMADRV_CALLBACK_STATUS_t;

/**
*
*  DMA driver channel descriptor definition
*
*****************************************************************************/
	typedef struct {
		UInt32 src;
		UInt32 dest;
		UInt32 next;
		UInt32 control;
		UInt32 size;
		UInt32 owner;
	} Dma_Chan_Desc;

/**
*
*  DMA driver data buffer feature definition
*
*****************************************************************************/
	typedef struct {
		UInt32 srcAddr;
		UInt32 destAddr;
		UInt32 length;
		UInt32 bRepeat;
		UInt32 interrupt;
	} Dma_Buffer;

/**
*
*  DMA driver data buffer definition
*
*****************************************************************************/
	typedef struct {
		Dma_Buffer buffers[1];
	} Dma_Buffer_List;

/**
*
*  DMA driver data buffer list definition
*
*****************************************************************************/
	typedef struct {
		UInt32 numBuffer;
		Dma_Buffer_List *pBufList;
	} Dma_Data;

/**
*
*  DMA data transfer width definition
*
*****************************************************************************/
	typedef enum {
		DMA_DATA_SIZE_8BIT = 0x00,
		DMA_DATA_SIZE_16BIT = 0x01,
		DMA_DATA_SIZE_32BIT = 0x02
	} DMA_DWIDTH;

/**
*
*  DMA data transfer type definition
*
*****************************************************************************/
	typedef enum {
		DMA_FCTRL_MEM_TO_MEM = 0,
		DMA_FCTRL_MEM_TO_PERI = 1,
		DMA_FCTRL_PERI_TO_MEM = 2,
		DMA_FCTRL_SRCPERI_TO_DESTPERI = 3,
		DMA_FCTRL_SRCPERI_TO_DESTPERI_CTRL_DESTPERI = 4,
		DMA_FCTRL_MEM_TO_PERI_CTRL_PERI = 5,
		DMA_FCTRL_PERI_TO_MEM_CTRL_PERI = 6,
		DMA_FCTRL_SRCPERI_TO_DESTPERI_CTRL_SRCPERI = 7
	} DMA_CHAN_TYPE;

/**
*
*  DMA burst length definition
*
*****************************************************************************/
#if (defined (_HERA_) || defined (_RHEA_) || defined (_SAMOA_))
	typedef enum {
		DMA_BURST_LEN_1 = 0x00,	///<
		DMA_BURST_LEN_2 = 0x01,	///<
		DMA_BURST_LEN_3 = 0x02,	///<
		DMA_BURST_LEN_4 = 0x03,	///<
		DMA_BURST_LEN_5 = 0x04,	///<
		DMA_BURST_LEN_6 = 0x05,	///<
		DMA_BURST_LEN_7 = 0x06,	///<
		DMA_BURST_LEN_8 = 0x07,	///<
		DMA_BURST_LEN_9 = 0x08,	///<
		DMA_BURST_LEN_10 = 0x09,	///<
		DMA_BURST_LEN_11 = 0x0A,	///<
		DMA_BURST_LEN_12 = 0x0B,	///<
		DMA_BURST_LEN_13 = 0x0C,	///<
		DMA_BURST_LEN_14 = 0x0D,	///<
		DMA_BURST_LEN_15 = 0x0E,	///<
		DMA_BURST_LEN_16 = 0x0F	///<
	} DMADRV_BLENGTH;

	typedef enum {
		DMA_BURST_SIZE_1 = 0x00,
		DMA_BURST_SIZE_2 = 0x01,
		DMA_BURST_SIZE_4 = 0x02,
		DMA_BURST_SIZE_8 = 0x03,
		DMA_BURST_SIZE_16 = 0x04,
		DMA_BURST_SIZE_32 = 0x05,
		DMA_BURST_SIZE_64 = 0x06,
		DMA_BURST_SIZE_128 = 0x07
	} DMA_BSIZE;

#else
/**
*
*  DMA burst size definition
*
*****************************************************************************/
	typedef enum {
		DMA_BURST_SIZE_1 = 0x00,
		DMA_BURST_SIZE_4 = 0x01,
		DMA_BURST_SIZE_8 = 0x02,
		DMA_BURST_SIZE_16 = 0x03,
		DMA_BURST_SIZE_32 = 0x04,
		DMA_BURST_SIZE_64 = 0x05,
		DMA_BURST_SIZE_128 = 0x06,
		DMA_BURST_SIZE_256 = 0x07
	} DMA_BSIZE;
#endif

/**
*
*  DMA alignment definition
*
*****************************************************************************/
	typedef enum {
		DMA_ALIGNMENT_8 = 8,
		DMA_ALIGNMENT_16 = 16,
		DMA_ALIGNMENT_32 = 32
	} DMA_ALIGN;

/**
*
*  DMA data transfer incremnet definition
*
*****************************************************************************/
	typedef enum {
		DMA_INC_MODE_NONE = 0,
		DMA_INC_MODE_SRC,
		DMA_INC_MODE_DST,
		DMA_INC_MODE_BOTH,
	} DMA_INC_MODE;

/**
*
*  DMA driver client type definition
*
*****************************************************************************/
#if (defined (_HERA_) || defined (_RHEA_) || defined (_SAMOA_))
	typedef enum {
		DMA_CLIENT_EP_INVALID = 0xff,
		DMA_CLIENT_EP_UARTB_A = 8,
		DMA_CLIENT_EP_UARTB_B = 9,
		DMA_CLIENT_EP_UARTB2_A = 10,
		DMA_CLIENT_EP_UARTB2_B = 11,
		DMA_CLIENT_EP_UARTB3_A = 12,
		DMA_CLIENT_EP_UARTB3_B = 13,
		DMA_CLIENT_EP_SSP_0A_RX0 = 16,
		DMA_CLIENT_EP_SSP_0B_TX0 = 17,
		DMA_CLIENT_EP_SSP_0C_RX1 = 18,
		DMA_CLIENT_EP_SSP_0D_TX1 = 19,
		DMA_CLIENT_EP_SSP_1A_RX0 = 20,
		DMA_CLIENT_EP_SSP_1B_TX0 = 21,
		DMA_CLIENT_EP_SSP_1C_RX1 = 22,
		DMA_CLIENT_EP_SSP_1D_TX1 = 23,
		DMA_CLIENT_EP_HSIA = 32,
		DMA_CLIENT_EP_HSIB = 33,
		DMA_CLIENT_EP_HSIC = 34,
		DMA_CLIENT_EP_HSID = 35,
		DMA_CLIENT_EP_EANC = 40,
		DMA_CLIENT_EP_STEREO = 41,
		DMA_CLIENT_EP_NVIN = 42,
		DMA_CLIENT_EP_VIN = 43,
		DMA_CLIENT_EP_VIBRA = 44,
		DMA_CLIENT_EP_IHF_0 = 45,
		DMA_CLIENT_EP_VOUT = 46,
		DMA_CLIENT_EP_SLIMA = 47,
		DMA_CLIENT_EP_SLIMB = 48,
		DMA_CLIENT_EP_SLIMC = 49,
		DMA_CLIENT_EP_SLIMD = 50,
		DMA_CLIENT_EP_SIM_A = 51,
		DMA_CLIENT_EP_SIM_B = 52,
		DMA_CLIENT_EP_SIM2_A = 53,
		DMA_CLIENT_EP_SIM2_B = 54,
		DMA_CLIENT_EP_IHF_1 = 55,
#if defined (_RHEA_)
		DMA_CLIENT_EP_SSP_3A_RX0 = 56,
		DMA_CLIENT_EP_SSP_3B_TX0 = 57,
		DMA_CLIENT_EP_SSP_3C_RX1 = 58,
		DMA_CLIENT_EP_SSP_3D_TX1 = 59,
#else
		DMA_CLIENT_EP_SSP_2A_RX0 = 56,
		DMA_CLIENT_EP_SSP_2B_TX0 = 57,
		DMA_CLIENT_EP_SSP_2C_RX1 = 58,
		DMA_CLIENT_EP_SSP_2D_TX1 = 59,
#endif
		DMA_CLIENT_EP_SPUM_SecureA = 65,
		DMA_CLIENT_EP_SPUM_SecureB = 66,
		DMA_CLIENT_EP_SPUM_OpenA = 67,
		DMA_CLIENT_EP_SPUM_OpenB = 68,
		DMA_CLIENT_MEMORY = 69,
#if defined (_RHEA_)
		DMA_CLIENT_EP_SSP_4A_RX0 = 76,
		DMA_CLIENT_EP_SSP_4B_TX0 = 77,
		DMA_CLIENT_EP_SSP_4C_RX1 = 78,
		DMA_CLIENT_EP_SSP_4D_TX1 = 79,
#endif
		DMA_CLIENT_TOTAL
	} DMA_CLIENT;
#else
	typedef enum {
		DMA_CLIENT_BULK_CRYPT_OUT = 0,
		DMA_CLIENT_CAM = 1,
		DMA_CLIENT_I2S_TX = 2,
		DMA_CLIENT_I2S_RX = 3,
		DMA_CLIENT_SIM_RX = 4,
		DMA_CLIENT_SIM_TX = 4,
		DMA_CLIENT_CRC = 5,
		DMA_CLIENT_SPI_RX = 6,
		DMA_CLIENT_SPI_TX = 7,
		DMA_CLIENT_UARTA_RX = 8,
		DMA_CLIENT_UARTA_TX = 9,
		DMA_CLIENT_UARTB_RX = 10,
		DMA_CLIENT_UARTB_TX = 11,
		DMA_CLIENT_DES_IN = 12,
		DMA_CLIENT_DES_OUT = 13,
		DMA_CLIENT_USB_RX = 14,
		DMA_CLIENT_USB_TX = 15,
		DMA_CLIENT_UARTC_RX = 16,
		DMA_CLIENT_UARTC_TX = 17,
		DMA_CLIENT_BULK_CRYPT_IN = 18,
		DMA_CLIENT_LCD = 19,
		DMA_CLIENT_MSPRO = 20,
		DMA_CLIENT_DSI_CM = 21,
		DMA_CLIENT_DSI_VM = 22,
		DMA_CLIENT_TVENC1 = 23,
		DMA_CLIENT_TVENC2 = 24,
#if defined (_ATHENA_)
		DMA_CLIENT_AUDIO_IN_FIFO = 25,
		DMA_CLIENT_AUDIO_OUT_FIFO = 26,
		DMA_CLIENT_POLYRING_OUT_FIFO = 27,
		DMA_CLIENT_AUDIO_WB_MIXERTAP = 28,
		DMA_CLIENT_MEMORY = 29,
#else
		DMA_CLIENT_MEMORY = 25,
#endif
		DMA_CLIENT_TOTAL
	} DMA_CLIENT;
#endif

/**
*
*  DMA driver channel definition
*
*****************************************************************************/
	typedef enum {
		DMA_CHANNEL_INVALID = 0xFF,
		DMA_CHANNEL_0 = 0,
		DMA_CHANNEL_1 = 1,
		DMA_CHANNEL_2 = 2,
		DMA_CHANNEL_3 = 3,
#if !defined (_SAMOA_)
		DMA_CHANNEL_4 = 4,
		DMA_CHANNEL_5 = 5,
		DMA_CHANNEL_6 = 6,
		DMA_CHANNEL_7 = 7,
#if defined (_ATHENA_)
		DMA_CHANNEL_8 = 8,	//used for DMA_CLIENT_AUDIO_OUT_FIFO
		DMA_CHANNEL_9 = 9,	//used for DMA_CLIENT_POLYRING_OUT_FIFO
		DMA_CHANNEL_10 = 10,	//used for DMA_CLIENT_AUDIO_WB_MIXERTAP
		DMA_CHANNEL_11 = 11,	//used for DMA_CLIENT_AUDIO_IN_FIFO
#endif
#endif
		TOTAL_DMA_CHANNELS
	} DMA_CHANNEL;

/**
*
*  DMA driver callback function definition
*
*****************************************************************************/
#define   DMADRV_CALLBACK_t            DmaDrv_Callback
	typedef void (*DMADRV_CALLBACK_t) (DMADRV_CALLBACK_STATUS_t Err);

/**
*
*  DMA driver channel info structure definition
*
*****************************************************************************/
	typedef struct {
		DMA_CLIENT srcID;
		DMA_CLIENT dstID;
		DMA_CHAN_TYPE type;
		DMA_ALIGN alignment;
		DMA_BSIZE srcBstSize;
		DMA_BSIZE dstBstSize;
		DMA_DWIDTH srcDataWidth;
		DMA_DWIDTH dstDataWidth;
		UInt32 priority;
		UInt32 chanNumber;
		UInt32 dmaCfgReg;
		UInt32 incMode;
		DmaDrv_Callback xferCompleteCb;
		UInt32 prot;
		UInt32 dstMaster;
		UInt32 srcMaster;
		UInt32 dstIncrement;
		UInt32 srcIncrement;
#if (defined (_HERA_) || defined (_RHEA_) || defined (_SAMOA_))
		DMADRV_BLENGTH srcBstLength;
		DMADRV_BLENGTH dstBstLength;
#endif
		Boolean freeChan;
		Boolean bCircular;
	} Dma_Chan_Info, *pChanInfo;

/**
*
*  DMA driver LLI structure definition
*
*****************************************************************************/
	typedef void *DMADRV_LLI_T;

/**
*
*  This function initialize dma driver
*
*  @return	    DMA driver return status
*
*****************************************************************************/
	DMADRV_STATUS DMADRV_Init(void);

/**
*
*  This function deinitialize dma driver
*
*  @return	    DMA driver return status
*
*****************************************************************************/
	DMADRV_STATUS DMADRV_DeInit(void);

/**
*
*  This function allocates dma channel
*
*  @param		srcID (in) source identification
*  @param       dstID (in) destination identification
*  @param       chanID (in) buffer to store channel number
*
*  @return	    DMA driver return status
*
*****************************************************************************/
	DMADRV_STATUS DMADRV_Obtain_Channel(DMA_CLIENT srcID,
					    DMA_CLIENT dstID,
					    DMA_CHANNEL * chanID);

/**
*
*  This function release dma channel
*
*  @param		chanID (in) channel identification
*
*  @return	    DMA driver return status
*
*****************************************************************************/
	DMADRV_STATUS DMADRV_Release_Channel(DMA_CHANNEL chanID);

/**
*
*  This function configure dma channel
*
*  @param       chanID       (in) channel number
*  @param       pChanInfo    (in) pointer to dma channe info structure
*
*  @return	    DMA driver return status
*
*****************************************************************************/
	DMADRV_STATUS DMADRV_Config_Channel(DMA_CHANNEL chanID,
					    Dma_Chan_Info * pChanInfo);

/**
*
*  This function bind data buffer for the DMA channel
*
*  @param		chanID    (in) channel to bind data 
*  @param       pData     (in) pointer to dma channel data buffer 
*
*  @return	    DMA driver return status
*
*****************************************************************************/
	DMADRV_STATUS DMADRV_Bind_Data(DMA_CHANNEL chanID, Dma_Data * pData);

/**
*
*  This function start dma channel transfer
*
*  @param		chanID (in) channel identification
*
*  @return	    DMA driver return status
*
*****************************************************************************/
	DMADRV_STATUS DMADRV_Start_Transfer(DMA_CHANNEL chanID);

/**
*
*  This function bind data buffer for the DMA channel
*
*  @param		chanID    (in) channel to bind data 
*  @param       pData     (in) pointer to dma channel data buffer 
*  @param       pLLI      (in) buffer to store returned LLI table 
*                              identification info
*
*  @return	    DMA driver return status
*
*****************************************************************************/
	DMADRV_STATUS DMADRV_Bind_Data_Ex(DMA_CHANNEL chanID,
					  Dma_Data * pData,
					  DMADRV_LLI_T * pLLI);

/**
*
*  This function start dma channel transfer
*
*  @param		chanID (in) channel identification
*  @param       pLLI   (in) one of the LLI tables needs to be used for DMA 
*                           transfer 
*
*  @return	    DMA driver return status
*
*****************************************************************************/
	DMADRV_STATUS DMADRV_Start_Transfer_Ex(DMA_CHANNEL chanID,
					       DMADRV_LLI_T pLLI);

/**
*
*  This function stop dma channel trnasfer
*
*  @param		chanID (in) channel identification
*
*  @return	    DMA driver return status
*
*****************************************************************************/
	DMADRV_STATUS DMADRV_Stop_Transfer(DMA_CHANNEL chanID);

/**
*
*  This function stop dma channel trnasfer and lose all data in FIFO
*
*  @param		chanID (in) channel identification
*
*  @return	    DMA driver return status
*
*****************************************************************************/
	DMADRV_STATUS DMADRV_Force_Shutdown_Channel(DMA_CHANNEL chanID);

/**
*
*  This function register hisr for client usage
*
*  @param		client (in) client identification
*  @param       hisr   (in) registered hisr
*
*  @return	    void
*
*****************************************************************************/
	void DMADRV_Register_HISR(DMA_CLIENT client, void *hisr);

/**
*
*  This function unregister hisr from client usage
*
*  @param		client (in) client identification
*
*  @return	    void
*
*****************************************************************************/
	void DMADRV_UnRegister_HISR(DMA_CLIENT client);

/**
*
*  This function get hisr for client usage
*
*  @param		client (in) client identification
*
*  @return	    hisr   (out) return registered client's hisr
*
*****************************************************************************/
	void *DMADRV_Get_HISR(DMA_CLIENT client);

/**
*
*  This function get DMA driver version number
*
*  @return	    driver version number
*
*****************************************************************************/
	UInt32 DMADRV_Get_Version(void);

/** @} */

#ifdef __cplusplus
}
#endif
#endif				/* _DMA_DRV_H_ */
