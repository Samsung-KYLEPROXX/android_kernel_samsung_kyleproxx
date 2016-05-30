/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fc8150_ppi.c

 Description : fc8150 host interface

*******************************************************************************/
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <asm/delay.h>


#include "fci_types.h"
#include "fc8150_regs.h"
#include "fci_oal.h"

#define BBM_BASE_ADDR       0x8C000000
#define BBM_MAP_SIZE		0x80000

#define PPI_BMODE           0x00
#define PPI_WMODE           0x10
#define PPI_LMODE           0x20
#define PPI_READ            0x40
#define PPI_WRITE           0x00
#define PPI_AINC            0x80


#ifndef __iomem
#define __iomem
#endif



#if 0
#define FC8150_PPI_REG_OUT(x)      		writew(x, ebi2_data_ptr)
#define FC8150_PPI_REG_IN()      		readw(ebi2_data_ptr)

static void __iomem 	*ebi2_data_ptr=0;
int	ppi_clk_state = BBM_NOK;

#else // PPI definition


void __iomem *ebi2_data_ptr=0;
void __iomem *ebi2_virt_ptr=0;
int	ppi_clk_state = BBM_NOK;
static struct clk *isdbt_clk=NULL;


static void FC8150_PPI_REG_OUT(u8 x)
{
	unsigned short val = 0;
	if((ppi_clk_state == BBM_OK) )
	{
		if (ebi2_data_ptr)
		{
			val = x&0x00FF;
			writew(val, ebi2_data_ptr);
		}
		else
		{
			PRINTF(0, "(%s)ebi2_data_ptr is NULL \n", __func__);
		}
	}
	else if(ppi_clk_state == BBM_NOK)
	{
		PRINTF(0, "(%s)EBI2 Clock State is NOK\n", __func__);		
	}
	

}

static u8 FC8150_PPI_REG_IN(void)
{
	unsigned short readVal = 0;
	static u8 addr_flag = 0;
	
	if( (ppi_clk_state == BBM_OK))
	{

		if (ebi2_data_ptr)
		{
			readVal = readw(ebi2_data_ptr);	
			addr_flag = 1- addr_flag;
		}
		else
		{
			PRINTF(0, "(SJINU %s)ebi2_data_ptr is NULL \n", __func__);
		}
	}
	else if(ppi_clk_state == BBM_NOK)
	{
			PRINTF(0, "(SJINU %s)EBI2 Clock State is NOK\n", __func__);		
	}	

	return (u8)(readVal&0xFF);
	
}

#endif

int fc8150_ppi_init(HANDLE hDevice, u16 param1, u16 param2)
{
	unsigned long clkrate;
	int ret = 0;
	int round_rate = 0;

	OAL_CREATE_SEMAPHORE();

	 if(request_mem_region(BBM_BASE_ADDR, BBM_MAP_SIZE, "isdbt") != NULL)
	 {
//			ebi2_data_ptr = ioremap(BBM_BASE_ADDR, BBM_MAP_SIZE) ;
			ebi2_data_ptr = ioremap_nocache(BBM_BASE_ADDR, 0x100) ;

			ebi2_virt_ptr = phys_to_virt(BBM_BASE_ADDR) ;

#if 0  // The routine of Clock Setting is moved to fc8150.c

			if(isdbt_clk == NULL)
			{
			isdbt_clk = clk_get(NULL, "ebi2_clk");
			}
			if (IS_ERR(isdbt_clk))
			{
				return PTR_ERR(isdbt_clk);
			}
			else
			{
				
				clkrate = clk_get_rate(isdbt_clk);

				round_rate = clk_round_rate(isdbt_clk, 40000000);
				
				clk_enable(isdbt_clk);

				ret = clk_set_rate(isdbt_clk, round_rate);
				
				if (ret)
				{
					PRINTF(0, "(%s) CAN'T set the rate of EBI2 CLOCK\n", __func__);	
					// return BBM_NOK;
				}

				clk_put(isdbt_clk);

			}
#endif
			ppi_clk_state = BBM_OK;
	 }
	 else
	 {
		 PRINTF(0, "(%s) Response for EBI2 DATA PTR MEM region is NOK\n", __func__);	 
	 }
	return BBM_OK;
}

int fc8150_ppi_byteread(HANDLE hDevice, u16 addr, u8 *data)
{
	u32 length = 1;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_PPI_REG_OUT(addr & 0xff);
	FC8150_PPI_REG_OUT((addr & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(PPI_READ | ((length & 0x0f0000) >> 16));
	FC8150_PPI_REG_OUT((length & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(length & 0xff);

	*data = FC8150_PPI_REG_IN();

	OAL_RELEASE_SEMAPHORE();
	return BBM_OK;
}

int fc8150_ppi_wordread(HANDLE hDevice, u16 addr, u16 *data)
{
	u32 length = 2;
	u8 command = PPI_AINC | PPI_READ | PPI_BMODE;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_PPI_REG_OUT(addr & 0xff);
	FC8150_PPI_REG_OUT((addr & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(command | ((length & 0x0f0000) >> 16));
	FC8150_PPI_REG_OUT((length & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(length & 0xff);

	*data = FC8150_PPI_REG_IN();
	*data |= FC8150_PPI_REG_IN() << 8;
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_ppi_longread(HANDLE hDevice, u16 addr, u32 *data)
{
	u32 length = 4;
	u8 command = PPI_AINC | PPI_READ | PPI_BMODE;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_PPI_REG_OUT(addr & 0xff);
	FC8150_PPI_REG_OUT((addr & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(command | ((length & 0x0f0000) >> 16));
	FC8150_PPI_REG_OUT((length & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(length & 0xff);

	*data = FC8150_PPI_REG_IN();
	*data |= FC8150_PPI_REG_IN() << 8;
	*data |= FC8150_PPI_REG_IN() << 16;
	*data |= FC8150_PPI_REG_IN() << 24;
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_ppi_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int i;
	u8 command = PPI_AINC | PPI_READ | PPI_BMODE;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_PPI_REG_OUT(addr & 0xff);
	FC8150_PPI_REG_OUT((addr & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(command | ((length & 0x0f0000) >> 16));
	FC8150_PPI_REG_OUT((length & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(length & 0xff);

	for (i = 0; i < length; i++)
		data[i] = FC8150_PPI_REG_IN();

	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_ppi_bytewrite(HANDLE hDevice, u16 addr, u8 data)
{
	u32 length = 1;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_PPI_REG_OUT(addr & 0xff);
	FC8150_PPI_REG_OUT((addr & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(PPI_WRITE | ((length & 0x0f0000) >> 16));
	FC8150_PPI_REG_OUT((length & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(length & 0xff);

	FC8150_PPI_REG_OUT(data);
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_ppi_wordwrite(HANDLE hDevice, u16 addr, u16 data)
{
	u32 length = 2;
	u8 command = PPI_AINC | PPI_WRITE | PPI_BMODE;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_PPI_REG_OUT(addr & 0xff);
	FC8150_PPI_REG_OUT((addr & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(command | ((length & 0x0f0000) >> 16));
	FC8150_PPI_REG_OUT((length & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(length & 0xff);

	FC8150_PPI_REG_OUT(data & 0xff);
	FC8150_PPI_REG_OUT((data & 0xff00) >> 8);
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_ppi_longwrite(HANDLE hDevice, u16 addr, u32 data)
{
	u32 length = 4;
	u8 command = PPI_AINC | PPI_WRITE | PPI_BMODE;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_PPI_REG_OUT(addr & 0xff);
	FC8150_PPI_REG_OUT((addr & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(command | ((length & 0x0f0000) >> 16));
	FC8150_PPI_REG_OUT((length & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(length & 0xff);

	FC8150_PPI_REG_OUT(data &  0x000000ff);
	FC8150_PPI_REG_OUT((data & 0x0000ff00) >> 8);
	FC8150_PPI_REG_OUT((data & 0x00ff0000) >> 16);
	FC8150_PPI_REG_OUT((data & 0xff000000) >> 24);
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_ppi_bulkwrite(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int i;
	u8 command = PPI_AINC | PPI_WRITE | PPI_BMODE;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_PPI_REG_OUT(addr & 0xff);
	FC8150_PPI_REG_OUT((addr & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(command | ((length & 0x0f0000) >> 16));
	FC8150_PPI_REG_OUT((length & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(length & 0xff);

	for (i = 0; i < length; i++)
		FC8150_PPI_REG_OUT(data[i]);

	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_ppi_dataread(HANDLE hDevice, u16 addr, u8 *data, u32 length)
{
	int i;
	u8 command = PPI_READ | PPI_BMODE;

	OAL_OBTAIN_SEMAPHORE();
	FC8150_PPI_REG_OUT(addr & 0xff);
	FC8150_PPI_REG_OUT((addr & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(command | ((length & 0x0f0000) >> 16));
	FC8150_PPI_REG_OUT((length & 0xff00) >> 8);
	FC8150_PPI_REG_OUT(length & 0xff);

	for (i = 0; i < length; i++)
		data[i] = FC8150_PPI_REG_IN();

	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

int fc8150_ppi_deinit(HANDLE hDevice)
{
	OAL_DELETE_SEMAPHORE();
	iounmap(ebi2_data_ptr);

	ebi2_data_ptr=0;

#if 0 // The routine of clk_disable is moved to fc8150.c
	clk_disable_unprepare(isdbt_clk);
	clk_put(isdbt_clk);

	isdbt_clk = NULL;
	// should add the routine of disabling clock, by SJINU, SJINU_DEBUG
#endif
	return BBM_OK;
}
