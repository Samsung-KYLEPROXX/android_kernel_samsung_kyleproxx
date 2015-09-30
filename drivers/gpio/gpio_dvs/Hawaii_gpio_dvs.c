/*
 * Samsung Mobile VE Group.
 *
 * drivers/gpio/gpio_dvs/hawaii_gpio_dvs.c - Read GPIO info. from BCM2166x(HAWAII)
 * 
 * Copyright (C) 2013, Samsung Electronics.
 *
 * This program is free software. You can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation
 */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/cpu.h>
#include <linux/cpuset.h>
#include <linux/cgroup.h>
#include <linux/device.h>
#include <linux/io.h>
#include <asm/setup.h>
#include <asm/gpio.h>
#include <mach/memory.h>
#include <mach/rdb/brcm_rdb_util.h>
#include <mach/rdb/brcm_rdb_padctrlreg.h>
#include <mach/rdb/brcm_rdb_gpio.h>
#include <mach/rdb/brcm_rdb_sysmap.h>
//#include <mach/gpio.h>

/********************* Fixed Code Area !***************************/
#include <linux/secgpio_dvs.h>
#include <linux/platform_device.h>
/****************************************************************/

#define GPIO_BASE_ADDR_HAWAII 			( PAD_CTRL_BASE_ADDR )	//0x35004800
#define GPIO_GPCTRx_BASE_ADDR 			( GPIO2_BASE_ADDR )	//0x35003000
#define GPIO_GPCTR(n) GPIO_GPCTR ## n


enum {
	TYPE1=0,
	TYPE2,
	BCM_REG_TYPE_MAX
} BCM_REG_TYPE;

#define GPIO_FUNC_CHECK_TYPE1_BIT_MASK 0x00000700	/* 0100 : IN/OUT */
#define GPIO_FUNC_CHECK_TYPE2_BIT_MASK 0x00000F00	/* 0000 : IN/OUT */
#define GPIO_PD_BIT_MASK 0x00000040	/* 4 : PD */
#define GPIO_PU_BIT_MASK 0x00000020	/* 2 : PU */
#define GPIO_NP_BIT_MASK 0x00000060	/* 6 : NP */
#define GPIO_IN_OUT_CEHCK_BIT_MASK 0x00000001	/* 1 : INPUT , 0 : OUTPUT */
#define GPIO_LOWHIGH_BIT_MASK 0x00000001	/* 1 : HIGH , 0 : LOW */

/********************* Fixed Code Area !***************************/
#define GET_RESULT_GPIO(a, b, c)	\
	(((a)<<10 & 0xFC00) |((b)<<4 & 0x03F0) | ((c) & 0x000F))

#define GET_GPIO_IO(value)	\
	(unsigned char)((0xFC00 & (value)) >> 10)
#define GET_GPIO_PUPD(value)	\
	(unsigned char)((0x03F0 & (value)) >> 4)
#define GET_GPIO_LH(value)	\
	(unsigned char)(0x000F & (value))
/****************************************************************/

/****************************************************************/
/* Define value in accordance with
	the specification of each BB vendor. */
#define AP_GPIO_COUNT	117
/****************************************************************/

struct __gpioInitStatusTest_t {
	unsigned long regvalue;
	unsigned long gpiodirection;
	char state;
};
#define gpioInitStatusTest_t struct __gpioInitStatusTest_t

static unsigned char analyze_gpio_pupd(gpioInitStatusTest_t *gpiostatus);
static unsigned char analyze_gpio_lowhigh(gpioInitStatusTest_t *gpiostatus);
static uint16_t analyze_gpio_property(
	gpioInitStatusTest_t *gpiostatus, unsigned char type);
static void parse_gpio_status(unsigned char phonestate);
 
static gpioInitStatusTest_t check_gpiostate[AP_GPIO_COUNT];


/****************************************************************/
/* Pre-defined variables. (DO NOT CHANGE THIS!!) */
static uint16_t checkgpiomap_result[GDVS_PHONE_STATUS_MAX][AP_GPIO_COUNT];
static struct gpiomap_result_t gpiomap_result = {
	.init = checkgpiomap_result[PHONE_INIT],
	.sleep = checkgpiomap_result[PHONE_SLEEP]
};

#ifdef SECGPIO_SLEEP_DEBUGGING
static struct sleepdebug_gpiotable sleepdebug_table;
#endif
/****************************************************************/

unsigned char analyze_gpio_pupd(gpioInitStatusTest_t *gpiostatus)
{
	/* 000000:NP, 000001:PD, 000010:PU, 111111:error */
	unsigned char result = 0;

	if((gpiostatus->regvalue&GPIO_NP_BIT_MASK)==0x00)
		result = 0x0;	 /* NP */
	else if((gpiostatus->regvalue&GPIO_PD_BIT_MASK)==0x40)
		result = 0x01;	 /* PD */
	else if((gpiostatus->regvalue&GPIO_PU_BIT_MASK)==0x20)
		result = 0x02;	 /* PU */
	else
		result = 0x3F;	/* error */

	return result;
}

unsigned char analyze_gpio_lowhigh(gpioInitStatusTest_t *gpiostatus)
{
	unsigned char result = 0;	/* 1:HIGH, 0:LOW */

	if((gpiostatus->state&GPIO_LOWHIGH_BIT_MASK)==0x1)
		result = 0x01;	/* HIGH */
	else
		result = 0x0;	/* LOW */

	return result;
}

#define ANALYZE_GPIO(i, TYPE)	\
	{ checkgpiomap_result[phonestate][i] =	\
		analyze_gpio_property(&check_gpiostate[i], TYPE); }

uint16_t analyze_gpio_property(
	gpioInitStatusTest_t *gpiostatus, unsigned char type)
{
	/* result : 16 bit ( FUNC/GPIO 6bit + PUPD 6bit + LowHigh 4bit) */
	uint16_t entire_result = 0;
	unsigned char temp_io = 0, temp_pupd = 0, temp_lh = 0;

	if (type == TYPE1) {
		/* It's GPIO!! */
		if ((gpiostatus->regvalue &
				GPIO_FUNC_CHECK_TYPE1_BIT_MASK) == 0x400) {
			/* 1: Input , 0: Output */
			if ((gpiostatus->gpiodirection &
					GPIO_IN_OUT_CEHCK_BIT_MASK) == 0x1)
				temp_io = 0x01;	/* GPIO_IN */
			else
				temp_io = 0x02;	/* GPIO_OUT */
		}
		/* It's FUNC!! */
			else
			temp_io = 0x0;		/* FUNC */
	} else if (type == TYPE2) {
		/* It's GPIO!! */
		if ((gpiostatus->regvalue &
				GPIO_FUNC_CHECK_TYPE2_BIT_MASK) == 0x0) {
			/* 1: Input , 0: Output */
			if ((gpiostatus->gpiodirection &
					GPIO_IN_OUT_CEHCK_BIT_MASK) == 0x1)
				temp_io = 0x01;	/* GPIO_IN */
			else
				temp_io = 0x02;	/* GPIO_OUT */
		}
		/* It's FUNC!! */
		else
			temp_io = 0x0;		/* FUNC */
	}

	/* 6 bit (000000:NP, 000001:PD, 000010:PU, 111111:error) */
	temp_pupd = analyze_gpio_pupd(gpiostatus);
	/* 4 bit (0:LOW, 1:HIGH) */
	temp_lh = analyze_gpio_lowhigh(gpiostatus);

	entire_result = GET_RESULT_GPIO(temp_io, temp_pupd, temp_lh);

	return entire_result;	
 }


void parse_gpio_status(unsigned char phonestate)
{
	int i=0;

	/* This procedure was inevitable
		because the way of GPIO property of BCOM wasn't sequential. */
	for (i = 0; i < AP_GPIO_COUNT; i++) {
		if (i <= 29)		/* #1. 000 - GPIO */
			ANALYZE_GPIO(i, TYPE2)
		else if (i <= 31)	/* #2. 100 - GPIO */
			ANALYZE_GPIO(i, TYPE1)
		else if (i <= 34)	/* #3. 000 - GPIO */
			 ANALYZE_GPIO(i, TYPE2)
		else if (i <= 36)	/* #4. 100 - GPIO */
			 ANALYZE_GPIO(i, TYPE1)
		else if (i <= 37)	/* #5. 000 - GPIO */
			 ANALYZE_GPIO(i, TYPE2)
		else if (i <= 124)	/* #6. 100 - GPIO */
			 ANALYZE_GPIO(i, TYPE1)
	}

}


#define READ_REGVALUE(x, y)	\
	check_gpiostate[x].regvalue = BRCM_READ_REG(base_addr,  y)
#define READ_GPIODIRECTION(x, y)	\
	check_gpiostate[x].gpiodirection = BRCM_READ_REG(base_addr,  y)


/****************************************************************/
/* Define this function in accordance with the specification of each BB vendor */
static void check_gpio_status(unsigned char phonestate)
{
	unsigned long base_addr;
	int i;

	pr_info("[GPIO_DVS][%s] ++\n", __func__);
	
	base_addr=HW_IO_PHYS_TO_VIRT(GPIO_BASE_ADDR_HAWAII);

	/* Had to do this way. Because the addresses were not sequential. */
	READ_REGVALUE(0, PADCTRLREG_GPIO00);
	READ_REGVALUE(1, PADCTRLREG_GPIO01);
	READ_REGVALUE(2, PADCTRLREG_GPIO02);
	READ_REGVALUE(3, PADCTRLREG_GPIO03);
	READ_REGVALUE(4, PADCTRLREG_GPIO04);
	READ_REGVALUE(5, PADCTRLREG_GPIO05);
	READ_REGVALUE(6, PADCTRLREG_GPIO06);
	READ_REGVALUE(7, PADCTRLREG_GPIO07);
	READ_REGVALUE(8, PADCTRLREG_GPIO08);
	READ_REGVALUE(9, PADCTRLREG_GPIO09);

	READ_REGVALUE(10, PADCTRLREG_GPIO10);
	READ_REGVALUE(11, PADCTRLREG_GPIO11);
	READ_REGVALUE(12, PADCTRLREG_GPIO12);
	READ_REGVALUE(13, PADCTRLREG_GPIO13);
	READ_REGVALUE(14, PADCTRLREG_GPIO14);
	READ_REGVALUE(15, PADCTRLREG_GPIO15);
	READ_REGVALUE(16, PADCTRLREG_GPIO16);
	READ_REGVALUE(17, PADCTRLREG_GPIO17);
	READ_REGVALUE(18, PADCTRLREG_GPIO18);
	READ_REGVALUE(19, PADCTRLREG_GPIO19);

	READ_REGVALUE(20, PADCTRLREG_GPIO20);
	READ_REGVALUE(21, PADCTRLREG_GPIO21);
	READ_REGVALUE(22, PADCTRLREG_GPIO22);
	READ_REGVALUE(23, PADCTRLREG_GPIO23);
	READ_REGVALUE(24, PADCTRLREG_GPIO24);
	READ_REGVALUE(25, PADCTRLREG_GPIO25);
	READ_REGVALUE(26, PADCTRLREG_GPIO26);
	READ_REGVALUE(27, PADCTRLREG_GPIO27);
	READ_REGVALUE(28, PADCTRLREG_GPIO28);
	READ_REGVALUE(29, PADCTRLREG_PMUINT);

	READ_REGVALUE(30, PADCTRLREG_BATRM);
	READ_REGVALUE(31, PADCTRLREG_STAT1);
	READ_REGVALUE(32, PADCTRLREG_GPIO32);
	READ_REGVALUE(33, PADCTRLREG_GPIO33);
	READ_REGVALUE(34, PADCTRLREG_GPIO34);
	READ_REGVALUE(35, PADCTRLREG_STAT2);
	READ_REGVALUE(36, PADCTRLREG_ADCSYN);
	READ_REGVALUE(37, PADCTRLREG_DSI0TE);
	READ_REGVALUE(38, PADCTRLREG_LCDCS0);
	READ_REGVALUE(39, PADCTRLREG_LCDSCL);

	READ_REGVALUE(40, PADCTRLREG_LCDSDA);
	READ_REGVALUE(41, PADCTRLREG_LCDRES);
	READ_REGVALUE(42, PADCTRLREG_LCDTE);
	READ_REGVALUE(43, PADCTRLREG_CAMCS0);
	READ_REGVALUE(44, PADCTRLREG_CAMCS1);
	READ_REGVALUE(45, PADCTRLREG_UBRX);
	READ_REGVALUE(46, PADCTRLREG_UBTX);
	READ_REGVALUE(47, PADCTRLREG_UBRTSN);
	READ_REGVALUE(48, PADCTRLREG_UBCTSN);
	READ_REGVALUE(49, PADCTRLREG_PMBSCCLK);

	READ_REGVALUE(50, PADCTRLREG_PMBSCDAT);
	READ_REGVALUE(51, PADCTRLREG_BSC1CLK);
	READ_REGVALUE(52, PADCTRLREG_BSC1DAT);
	READ_REGVALUE(53, PADCTRLREG_SIMRST);
	READ_REGVALUE(54, PADCTRLREG_SIMDAT);
	READ_REGVALUE(55, PADCTRLREG_SIMCLK);
	READ_REGVALUE(56, PADCTRLREG_SIMDET);
	READ_REGVALUE(57, PADCTRLREG_MMC0CK);
	READ_REGVALUE(58, PADCTRLREG_MMC0CMD);
	READ_REGVALUE(59, PADCTRLREG_MMC0RST);

	READ_REGVALUE(60, PADCTRLREG_MMC0DAT7);
	READ_REGVALUE(61, PADCTRLREG_MMC0DAT6);
	READ_REGVALUE(62, PADCTRLREG_MMC0DAT5);
	READ_REGVALUE(63, PADCTRLREG_MMC0DAT4);
	READ_REGVALUE(64, PADCTRLREG_MMC0DAT3);
	READ_REGVALUE(65, PADCTRLREG_MMC0DAT2);
	READ_REGVALUE(66, PADCTRLREG_MMC0DAT1);
	READ_REGVALUE(67, PADCTRLREG_MMC0DAT0);
	READ_REGVALUE(68, PADCTRLREG_MMC1CK);
	READ_REGVALUE(69, PADCTRLREG_MMC1CMD);

	READ_REGVALUE(70, PADCTRLREG_MMC1RST);
	READ_REGVALUE(71, PADCTRLREG_MMC1DAT7);
	READ_REGVALUE(72, PADCTRLREG_MMC1DAT6);
	READ_REGVALUE(73, PADCTRLREG_MMC1DAT5);
	READ_REGVALUE(74, PADCTRLREG_MMC1DAT4);
	READ_REGVALUE(75, PADCTRLREG_MMC1DAT3);
	READ_REGVALUE(76, PADCTRLREG_MMC1DAT2);
	READ_REGVALUE(77, PADCTRLREG_MMC1DAT1);
	READ_REGVALUE(78, PADCTRLREG_MMC1DAT0);
	READ_REGVALUE(79, PADCTRLREG_SDCK);

	READ_REGVALUE(80, PADCTRLREG_SDCMD);
	READ_REGVALUE(81, PADCTRLREG_SDDAT3);
	READ_REGVALUE(82, PADCTRLREG_SDDAT2);
	READ_REGVALUE(83, PADCTRLREG_SDDAT1);
	READ_REGVALUE(84, PADCTRLREG_SDDAT0);
	READ_REGVALUE(85, PADCTRLREG_SSPSYN);
	READ_REGVALUE(86, PADCTRLREG_SSPDO);
	READ_REGVALUE(87, PADCTRLREG_SSPCK);
	READ_REGVALUE(88, PADCTRLREG_SSPDI);
	READ_REGVALUE(89, PADCTRLREG_SPI0FSS);

	READ_REGVALUE(90, PADCTRLREG_SPI0CLK);
	READ_REGVALUE(91, PADCTRLREG_SPI0TXD);
	READ_REGVALUE(92, PADCTRLREG_SPI0RXD);
	READ_REGVALUE(93, PADCTRLREG_GPIO93);
	READ_REGVALUE(94, PADCTRLREG_GPIO94);
	READ_REGVALUE(95, PADCTRLREG_DCLK4);
	READ_REGVALUE(96, PADCTRLREG_DCLKREQ4);
	READ_REGVALUE(97, PADCTRLREG_GPS_PABLANK);
	READ_REGVALUE(98, PADCTRLREG_GPS_TMARK);
	READ_REGVALUE(99, PADCTRLREG_GPS_CALREQ);

	READ_REGVALUE(100, PADCTRLREG_GPS_HOSTREQ);

	READ_REGVALUE(101, PADCTRLREG_TRACECLK);
	READ_REGVALUE(102, PADCTRLREG_DCLK1);
	READ_REGVALUE(103, PADCTRLREG_DCLKREQ1);
	READ_REGVALUE(104, PADCTRLREG_MDMGPIO00);
	READ_REGVALUE(105, PADCTRLREG_MDMGPIO01);
	READ_REGVALUE(106, PADCTRLREG_MDMGPIO02);
	READ_REGVALUE(107, PADCTRLREG_MDMGPIO03);
	READ_REGVALUE(108, PADCTRLREG_MDMGPIO04);
	READ_REGVALUE(109, PADCTRLREG_MDMGPIO05);

	READ_REGVALUE(110, PADCTRLREG_MDMGPIO06);
	READ_REGVALUE(111, PADCTRLREG_MDMGPIO07);
	READ_REGVALUE(112, PADCTRLREG_MDMGPIO08);
	READ_REGVALUE(113, PADCTRLREG_ICUSBDP);
	READ_REGVALUE(114, PADCTRLREG_ICUSBDM);
	READ_REGVALUE(115, PADCTRLREG_DMIC0CLK);
	READ_REGVALUE(116, PADCTRLREG_DMIC0DQ);

	/* ======================================================= */
	base_addr = HW_IO_PHYS_TO_VIRT(GPIO_GPCTRx_BASE_ADDR);

	READ_GPIODIRECTION(0, GPIO_GPCTR0);
	READ_GPIODIRECTION(1, GPIO_GPCTR1);
	READ_GPIODIRECTION(2, GPIO_GPCTR2);
	READ_GPIODIRECTION(3, GPIO_GPCTR3);
	READ_GPIODIRECTION(4, GPIO_GPCTR4);
	READ_GPIODIRECTION(5, GPIO_GPCTR5);
	READ_GPIODIRECTION(6, GPIO_GPCTR6);
	READ_GPIODIRECTION(7, GPIO_GPCTR7);
	READ_GPIODIRECTION(8, GPIO_GPCTR8);
	READ_GPIODIRECTION(9, GPIO_GPCTR9);

	READ_GPIODIRECTION(10, GPIO_GPCTR10);
	READ_GPIODIRECTION(11, GPIO_GPCTR11);
	READ_GPIODIRECTION(12, GPIO_GPCTR12);
	READ_GPIODIRECTION(13, GPIO_GPCTR13);
	READ_GPIODIRECTION(14, GPIO_GPCTR14);
	READ_GPIODIRECTION(15, GPIO_GPCTR15);
	READ_GPIODIRECTION(16, GPIO_GPCTR16);
	READ_GPIODIRECTION(17, GPIO_GPCTR17);
	READ_GPIODIRECTION(18, GPIO_GPCTR18);
	READ_GPIODIRECTION(19, GPIO_GPCTR19);

	READ_GPIODIRECTION(20, GPIO_GPCTR20);
	READ_GPIODIRECTION(21, GPIO_GPCTR21);
	READ_GPIODIRECTION(22, GPIO_GPCTR22);
	READ_GPIODIRECTION(23, GPIO_GPCTR23);
	READ_GPIODIRECTION(24, GPIO_GPCTR24);
	READ_GPIODIRECTION(25, GPIO_GPCTR25);
	READ_GPIODIRECTION(26, GPIO_GPCTR26);
	READ_GPIODIRECTION(27, GPIO_GPCTR27);
	READ_GPIODIRECTION(28, GPIO_GPCTR28);
	READ_GPIODIRECTION(29, GPIO_GPCTR29);

	READ_GPIODIRECTION(30, GPIO_GPCTR30);
	READ_GPIODIRECTION(31, GPIO_GPCTR31);
	READ_GPIODIRECTION(32, GPIO_GPCTR32);
	READ_GPIODIRECTION(33, GPIO_GPCTR33);
	READ_GPIODIRECTION(34, GPIO_GPCTR34);
	READ_GPIODIRECTION(35, GPIO_GPCTR35);
	READ_GPIODIRECTION(36, GPIO_GPCTR36);
	READ_GPIODIRECTION(37, GPIO_GPCTR37);
	READ_GPIODIRECTION(38, GPIO_GPCTR38);
	READ_GPIODIRECTION(39, GPIO_GPCTR39);

	READ_GPIODIRECTION(40, GPIO_GPCTR40);
	READ_GPIODIRECTION(41, GPIO_GPCTR41);
	READ_GPIODIRECTION(42, GPIO_GPCTR42);
	READ_GPIODIRECTION(43, GPIO_GPCTR43);
	READ_GPIODIRECTION(44, GPIO_GPCTR44);
	READ_GPIODIRECTION(45, GPIO_GPCTR45);
	READ_GPIODIRECTION(46, GPIO_GPCTR46);
	READ_GPIODIRECTION(47, GPIO_GPCTR47);
	READ_GPIODIRECTION(48, GPIO_GPCTR48);
	READ_GPIODIRECTION(49, GPIO_GPCTR49);

	READ_GPIODIRECTION(50, GPIO_GPCTR50);
	READ_GPIODIRECTION(51, GPIO_GPCTR51);
	READ_GPIODIRECTION(52, GPIO_GPCTR52);
	READ_GPIODIRECTION(53, GPIO_GPCTR53);
	READ_GPIODIRECTION(54, GPIO_GPCTR54);
	READ_GPIODIRECTION(55, GPIO_GPCTR55);
	READ_GPIODIRECTION(56, GPIO_GPCTR56);
	READ_GPIODIRECTION(57, GPIO_GPCTR57);
	READ_GPIODIRECTION(58, GPIO_GPCTR58);
	READ_GPIODIRECTION(59, GPIO_GPCTR59);

	READ_GPIODIRECTION(60, GPIO_GPCTR60);
	READ_GPIODIRECTION(61, GPIO_GPCTR61);
	READ_GPIODIRECTION(62, GPIO_GPCTR62);
	READ_GPIODIRECTION(63, GPIO_GPCTR63);
	READ_GPIODIRECTION(64, GPIO_GPCTR64);
	READ_GPIODIRECTION(65, GPIO_GPCTR65);
	READ_GPIODIRECTION(66, GPIO_GPCTR66);
	READ_GPIODIRECTION(67, GPIO_GPCTR67);
	READ_GPIODIRECTION(68, GPIO_GPCTR68);
	READ_GPIODIRECTION(69, GPIO_GPCTR69);

	READ_GPIODIRECTION(70, GPIO_GPCTR70);
	READ_GPIODIRECTION(71, GPIO_GPCTR71);
	READ_GPIODIRECTION(72, GPIO_GPCTR72);
	READ_GPIODIRECTION(73, GPIO_GPCTR73);
	READ_GPIODIRECTION(74, GPIO_GPCTR74);
	READ_GPIODIRECTION(75, GPIO_GPCTR75);
	READ_GPIODIRECTION(76, GPIO_GPCTR76);
	READ_GPIODIRECTION(77, GPIO_GPCTR77);
	READ_GPIODIRECTION(78, GPIO_GPCTR78);
	READ_GPIODIRECTION(79, GPIO_GPCTR79);

	READ_GPIODIRECTION(80, GPIO_GPCTR80);
	READ_GPIODIRECTION(81, GPIO_GPCTR81);
	READ_GPIODIRECTION(82, GPIO_GPCTR82);
	READ_GPIODIRECTION(83, GPIO_GPCTR83);
	READ_GPIODIRECTION(84, GPIO_GPCTR84);
	READ_GPIODIRECTION(85, GPIO_GPCTR85);
	READ_GPIODIRECTION(86, GPIO_GPCTR86);
	READ_GPIODIRECTION(87, GPIO_GPCTR87);
	READ_GPIODIRECTION(88, GPIO_GPCTR88);
	READ_GPIODIRECTION(89, GPIO_GPCTR89);

	READ_GPIODIRECTION(90, GPIO_GPCTR90);
	READ_GPIODIRECTION(91, GPIO_GPCTR91);
	READ_GPIODIRECTION(92, GPIO_GPCTR92);
	READ_GPIODIRECTION(93, GPIO_GPCTR93);
	READ_GPIODIRECTION(94, GPIO_GPCTR94);
	READ_GPIODIRECTION(95, GPIO_GPCTR95);
	READ_GPIODIRECTION(96, GPIO_GPCTR96);
	READ_GPIODIRECTION(97, GPIO_GPCTR97);
	READ_GPIODIRECTION(98, GPIO_GPCTR98);
	READ_GPIODIRECTION(99, GPIO_GPCTR99);

	READ_GPIODIRECTION(100, GPIO_GPCTR100);

	READ_GPIODIRECTION(101, GPIO_GPCTR101);
	READ_GPIODIRECTION(102, GPIO_GPCTR110);
	READ_GPIODIRECTION(103, GPIO_GPCTR111);
	READ_GPIODIRECTION(104, GPIO_GPCTR112);
	READ_GPIODIRECTION(105, GPIO_GPCTR113);
	READ_GPIODIRECTION(106, GPIO_GPCTR114);
	READ_GPIODIRECTION(107, GPIO_GPCTR115);
	READ_GPIODIRECTION(108, GPIO_GPCTR116);
	READ_GPIODIRECTION(109, GPIO_GPCTR117);

	READ_GPIODIRECTION(110, GPIO_GPCTR118);
	READ_GPIODIRECTION(111, GPIO_GPCTR119);
	READ_GPIODIRECTION(112, GPIO_GPCTR120);
	READ_GPIODIRECTION(113, GPIO_GPCTR121);
	READ_GPIODIRECTION(114, GPIO_GPCTR122);
	READ_GPIODIRECTION(115, GPIO_GPCTR123);
	READ_GPIODIRECTION(116, GPIO_GPCTR124);
	
	for (i = 0; i < AP_GPIO_COUNT; i++)
		check_gpiostate[i].state = gpio_get_value(i);
#if 0
	for (i = 0; i < AP_GPIO_COUNT; i++)
		pr_info("[GPIO_DVS] [%d] REG value = 0x%lx, REG Direction = 0x%lx, REG Logic = 0x%x\n",
			i, check_gpiostate[i].regvalue,
			check_gpiostate[i].gpiodirection,
			check_gpiostate[i].state);
#endif

	parse_gpio_status(phonestate);

#if 0
	if (phonestate == PHONE_INIT) {
		for (i = 0; i < AP_GPIO_COUNT; i++)
			pr_info("[GPIO_DVS] Entire INIT Result[%d] = 0x%x\n",
				i, checkgpiomap_result[PHONE_INIT][i]);
	} else if (phonestate == PHONE_SLEEP) {
		for (i = 0; i < AP_GPIO_COUNT; i++)
			pr_info("[GPIO_DVS] Entire SLEEP Result[%d] = 0x%x\n",
				i, checkgpiomap_result[PHONE_SLEEP][i]);
	}
#endif

	pr_info("[GPIO_DVS][%s] --\n", __func__);

	return;
}
/****************************************************************/

void set_gpio_pupd(int gpionum, unsigned long temp_gpio_reg)
{
	unsigned long base_addr;

	base_addr = HW_IO_PHYS_TO_VIRT(GPIO_BASE_ADDR_HAWAII);

	switch (gpionum) {
	case 0:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO00, temp_gpio_reg);
		break;
	case 1:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO01, temp_gpio_reg);
		break;
	case 2:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO02, temp_gpio_reg);
		break;
	case 3:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO03, temp_gpio_reg);
		break;
	case 4:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO04, temp_gpio_reg);
		break;
	case 5:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO05, temp_gpio_reg);
		break;
	case 6:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO06, temp_gpio_reg);
		break;
	case 7:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO07, temp_gpio_reg);
		break;
	case 8:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO08, temp_gpio_reg);
		break;
	case 9:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO09, temp_gpio_reg);
		break;
	case 10:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO10, temp_gpio_reg);
		break;
	case 11:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO11, temp_gpio_reg);
		break;
	case 12:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO12, temp_gpio_reg);
		break;
	case 13:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO13, temp_gpio_reg);
		break;
	case 14:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO14, temp_gpio_reg);
		break;
	case 15:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO15, temp_gpio_reg);
		break;
	case 16:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO16, temp_gpio_reg);
		break;
	case 17:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO17, temp_gpio_reg);
		break;
	case 18:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO18, temp_gpio_reg);
		break;
	case 19:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO19, temp_gpio_reg);
		break;
	case 20:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO20, temp_gpio_reg);
		break;
	case 21:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO21, temp_gpio_reg);
		break;
	case 22:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO22, temp_gpio_reg);
		break;
	case 23:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO23, temp_gpio_reg);
		break;
	case 24:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO24, temp_gpio_reg);
		break;
	case 25:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO25, temp_gpio_reg);
		break;
	case 26:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO26, temp_gpio_reg);
		break;
	case 27:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO27, temp_gpio_reg);
		break;
	case 28:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO28, temp_gpio_reg);
		break;
	case 29:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_PMUINT, temp_gpio_reg);
		break;
	case 30:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_BATRM, temp_gpio_reg);
		break;
	case 31:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_STAT1, temp_gpio_reg);
		break;
	case 32:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO32, temp_gpio_reg);
		break;
	case 33:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO33, temp_gpio_reg);
		break;
	case 34:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO34, temp_gpio_reg);
		break;
	case 35:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_STAT2, temp_gpio_reg);
		break;
	case 36:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_ADCSYN, temp_gpio_reg);
		break;
	case 37:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_DSI0TE, temp_gpio_reg);
		break;
	case 38:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_LCDCS0, temp_gpio_reg);
		break;
	case 39:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_LCDSCL, temp_gpio_reg);
		break;
	case 40:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_LCDSDA, temp_gpio_reg);
		break;
	case 41:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_LCDRES, temp_gpio_reg);
		break;
	case 42:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_LCDTE, temp_gpio_reg);
		break;
	case 43:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_CAMCS0, temp_gpio_reg);
		break;
	case 44:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_CAMCS1, temp_gpio_reg);
		break;
	case 45:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_UBRX, temp_gpio_reg);
		break;
	case 46:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_UBTX, temp_gpio_reg);
		break;
	case 47:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_UBRTSN, temp_gpio_reg);
		break;
	case 48:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_UBCTSN, temp_gpio_reg);
		break;
	case 49:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_PMBSCCLK, temp_gpio_reg);
		break;
	case 50:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_PMBSCDAT, temp_gpio_reg);
		break;
	case 51:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_BSC1CLK, temp_gpio_reg);
		break;
	case 52:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_BSC1DAT, temp_gpio_reg);
		break;
	case 53:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SIMRST, temp_gpio_reg);
		break;
	case 54:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SIMDAT, temp_gpio_reg);
		break;
	case 55:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SIMCLK, temp_gpio_reg);
		break;
	case 56:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SIMDET, temp_gpio_reg);
		break;
	case 57:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC0CK, temp_gpio_reg);
		break;
	case 58:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC0CMD, temp_gpio_reg);
		break;
	case 59:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC0RST, temp_gpio_reg);
		break;
	case 60:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC0DAT7, temp_gpio_reg);
		break;
	case 61:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC0DAT6, temp_gpio_reg);
		break;
	case 62:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC0DAT5, temp_gpio_reg);
		break;
	case 63:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC0DAT4, temp_gpio_reg);
		break;
	case 64:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC0DAT3, temp_gpio_reg);
		break;
	case 65:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC0DAT2, temp_gpio_reg);
		break;
	case 66:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC0DAT1, temp_gpio_reg);
		break;
	case 67:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC0DAT0, temp_gpio_reg);
		break;
	case 68:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC1CK, temp_gpio_reg);
		break;
	case 69:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC1CMD, temp_gpio_reg);
		break;
	case 70:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC1RST, temp_gpio_reg);
		break;
	case 71:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC1DAT7, temp_gpio_reg);
		break;
	case 72:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC1DAT6, temp_gpio_reg);
		break;
	case 73:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC1DAT5, temp_gpio_reg);
		break;
	case 74:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC1DAT4, temp_gpio_reg);
		break;
	case 75:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC1DAT3, temp_gpio_reg);
		break;
	case 76:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC1DAT2, temp_gpio_reg);
		break;
	case 77:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC1DAT1, temp_gpio_reg);
		break;
	case 78:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MMC1DAT0, temp_gpio_reg);
		break;
	case 79:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SDCK, temp_gpio_reg);
		break;
	case 80:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SDCMD, temp_gpio_reg);
		break;
	case 81:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SDDAT3, temp_gpio_reg);
		break;
	case 82:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SDDAT2, temp_gpio_reg);
		break;
	case 83:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SDDAT1, temp_gpio_reg);
		break;
	case 84:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SDDAT0, temp_gpio_reg);
		break;
	case 85:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SSPSYN, temp_gpio_reg);
		break;
	case 86:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SSPDO, temp_gpio_reg);
		break;
	case 87:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SSPCK, temp_gpio_reg);
		break;
	case 88:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SSPDI, temp_gpio_reg);
		break;
	case 89:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SPI0FSS, temp_gpio_reg);
		break;
	case 90:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SPI0CLK, temp_gpio_reg);
		break;
	case 91:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SPI0TXD, temp_gpio_reg);
		break;
	case 92:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_SPI0RXD, temp_gpio_reg);
		break;
	case 93:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO93, temp_gpio_reg);
		break;
	case 94:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPIO94, temp_gpio_reg);
		break;
	case 95:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_DCLK4, temp_gpio_reg);
		break;
	case 96:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_DCLKREQ4, temp_gpio_reg);
		break;
	case 97:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPS_PABLANK, temp_gpio_reg);
		break;
	case 98:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPS_TMARK, temp_gpio_reg);
		break;
	case 99:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPS_CALREQ, temp_gpio_reg);
		break;
	case 100:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_GPS_HOSTREQ, temp_gpio_reg);
		break;
	case 101:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_TRACECLK, temp_gpio_reg);
		break;
	case 102:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_DCLK1, temp_gpio_reg);
		break;
	case 103:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_DCLKREQ1, temp_gpio_reg);
		break;
	case 104:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MDMGPIO00, temp_gpio_reg);
		break;
	case 105:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MDMGPIO01, temp_gpio_reg);
		break;
	case 106:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MDMGPIO02, temp_gpio_reg);
		break;
	case 107:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MDMGPIO03, temp_gpio_reg);
		break;
	case 108:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MDMGPIO04, temp_gpio_reg);
		break;
	case 109:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MDMGPIO05, temp_gpio_reg);
		break;
	case 110:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MDMGPIO06, temp_gpio_reg);
		break;
	case 111:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MDMGPIO07, temp_gpio_reg);
		break;
	case 112:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_MDMGPIO08, temp_gpio_reg);
		break;
	case 113:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_ICUSBDP, temp_gpio_reg);
		break;
	case 114:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_ICUSBDM, temp_gpio_reg);
		break;
	case 115:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_DMIC0CLK, temp_gpio_reg);
		break;
	case 116:
		BRCM_WRITE_REG(base_addr, PADCTRLREG_DMIC0DQ, temp_gpio_reg);
		break;

	default:
		break;
	}
}

#ifdef SECGPIO_SLEEP_DEBUGGING
/****************************************************************/
/* Define this function in accordance with the specification of each BB vendor */
void setgpio_for_sleepdebug(int gpionum, uint16_t  io_pupd_lh)
{
	unsigned char temp_io, temp_pupd, temp_lh;
	int temp_data = io_pupd_lh;
	u32 temp_gpio_reg;

	pr_info("[GPIO_DVS][%s] gpionum=%d, io_pupd_lh=0x%x\n",
		__func__, gpionum, io_pupd_lh);

	temp_io = GET_GPIO_IO(io_pupd_lh);
	temp_pupd = GET_GPIO_PUPD(io_pupd_lh);
	temp_lh = GET_GPIO_LH(io_pupd_lh);

	pr_info("[GPIO_DVS][%s] io=%d, pupd=%d, lh=%d\n",
		__func__, temp_io, temp_pupd, temp_lh);

	/* in case of 'INPUT', set PD/PU */
	if (temp_io == 0x01) {
		/* 0x0:NP, 0x1:PD, 0x2:PU */
		if (temp_pupd == 0x0)
			temp_pupd = 0x00;
		else if (temp_pupd == 0x1)
			temp_pupd = 0x1 << 6;
		else if (temp_pupd == 0x2)
			temp_pupd = 0x1 << 5;

		/* Once after DUT goes into sleep state,
		 * "check_gpiostate" always has values from sleep state*/
		temp_gpio_reg = check_gpiostate[gpionum].regvalue;
		pr_info("[GPIO_DVS][%s] temp_gpio_reg = 0x%x",
			__func__ , temp_gpio_reg);

		temp_gpio_reg = temp_gpio_reg | (0x00000000|(u32)temp_pupd);
		pr_info("[GPIO_DVS][%s] post temp_gpio_reg = 0x%x",
			__func__ , temp_gpio_reg);

		set_gpio_pupd(gpionum, temp_gpio_reg);
	}
	/* in case of 'OUTPUT', set L/H */
	else if (temp_io == 0x02) {
		pr_info("[GPIO_DVS][%s] %d gpio set %d\n",
			__func__, gpionum, temp_lh);
		gpio_set_value(gpionum, temp_lh);
	}

}
/****************************************************************/

/****************************************************************/
/* Define this function in accordance with the specification of each BB vendor */
static void undo_sleepgpio(void)
{
	int i;
	int gpio_num;

	pr_info("[GPIO_DVS][%s] ++\n", __func__);

	for (i = 0; i < sleepdebug_table.gpio_count; i++) {
		gpio_num = sleepdebug_table.gpioinfo[i].gpio_num;
		/* 
		 * << Caution >> 
		 * If it's necessary, 
		 * change the following function to another appropriate one 
		 * or delete it 
		 */
		setgpio_for_sleepdebug(gpio_num, gpiomap_result.sleep[gpio_num]);
	}

	pr_info("[GPIO_DVS][%s] --\n", __func__);
	return;
}
/****************************************************************/
#endif

/********************* Fixed Code Area !***************************/
#ifdef SECGPIO_SLEEP_DEBUGGING
static void set_sleepgpio(void)
{
	int i;
	int gpio_num;
	uint16_t set_data;

	pr_info("[GPIO_DVS][%s] ++, cnt=%d\n",
		__func__, sleepdebug_table.gpio_count);

	for (i = 0; i < sleepdebug_table.gpio_count; i++) {
		pr_info("[GPIO_DVS][%d] gpio_num(%d), io(%d), pupd(%d), lh(%d)\n",
			i, sleepdebug_table.gpioinfo[i].gpio_num,
			sleepdebug_table.gpioinfo[i].io,
			sleepdebug_table.gpioinfo[i].pupd,
			sleepdebug_table.gpioinfo[i].lh);

		gpio_num = sleepdebug_table.gpioinfo[i].gpio_num;

		// to prevent a human error caused by "don't care" value 
		if( sleepdebug_table.gpioinfo[i].io == 1)		/* IN */
			sleepdebug_table.gpioinfo[i].lh =
				GET_GPIO_LH(gpiomap_result.sleep[gpio_num]);
		else if( sleepdebug_table.gpioinfo[i].io == 2)		/* OUT */
			sleepdebug_table.gpioinfo[i].pupd =
				GET_GPIO_PUPD(gpiomap_result.sleep[gpio_num]);

		set_data = GET_RESULT_GPIO(
			sleepdebug_table.gpioinfo[i].io,
			sleepdebug_table.gpioinfo[i].pupd,
			sleepdebug_table.gpioinfo[i].lh);

		setgpio_for_sleepdebug(gpio_num, set_data);
	}

	pr_info("[GPIO_DVS][%s] --\n", __func__);
	return;
}
#endif

static struct gpio_dvs_t gpio_dvs = {
	.result = &gpiomap_result,
	.count = AP_GPIO_COUNT,
	.check_init = false,
	.check_sleep = false,
	.check_gpio_status = check_gpio_status,
#ifdef SECGPIO_SLEEP_DEBUGGING
	.sdebugtable = &sleepdebug_table,
	.set_sleepgpio = set_sleepgpio,
	.undo_sleepgpio = undo_sleepgpio,
#endif
};

static struct platform_device secgpio_dvs_device = {
	.name	= "secgpio_dvs",
	.id		= -1,
	.dev.platform_data = &gpio_dvs,
};

static struct platform_device *secgpio_dvs_devices[] __initdata = {
	&secgpio_dvs_device,
};

static int __init secgpio_dvs_device_init(void)
{
	return platform_add_devices(
		secgpio_dvs_devices, ARRAY_SIZE(secgpio_dvs_devices));
}
arch_initcall(secgpio_dvs_device_init);
/****************************************************************/


