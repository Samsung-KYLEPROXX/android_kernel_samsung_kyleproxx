/************************************************************************/
/*                                                                      */
/*  Copyright 2012  Broadcom Corporation                                */
/*                                                                      */
/* Unless you and Broadcom execute a separate written software license  */
/* agreement governing use of this software, this software is licensed  */
/* to you under the terms of the GNU General Public License version 2   */
/* (the GPL), available at						*/
/*                                                                      */
/*          http://www.broadcom.com/licenses/GPLv2.php                  */
/*                                                                      */
/*  with the following added to such license:                           */
/*                                                                      */
/*  As a special exception, the copyright holders of this software give */
/*  you permission to link this software with independent modules, and  */
/*  to copy and distribute the resulting executable under terms of your */
/*  choice, provided that you also meet, for each linked independent    */
/*  module, the terms and conditions of the license of that module. An  */
/*  independent module is a module which is not derived from this       */
/*  software.  The special   exception does not apply to any            */
/*  modifications of the software.					*/
/*									*/
/*  Notwithstanding the above, under no circumstances may you combine	*/
/*  this software in any way with any other Broadcom software provided	*/
/*  under a license other than the GPL, without Broadcom's express	*/
/*  prior written consent.						*/
/*									*/
/************************************************************************/

#include <linux/version.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#if defined(CONFIG_RT8969) || defined(CONFIG_RT8973)
#include <linux/mfd/bcmpmu.h>
#include <linux/power_supply.h>
#include <linux/mfd/rt8973.h>
#include <mach/dev-muic_rt8973.h>
#endif

#ifdef CONFIG_USB_SWITCH_TSU6111
#include <linux/mfd/bcmpmu.h>
#include <linux/power_supply.h>
#include <linux/i2c/tsu6111.h>
#include <mach/dev-muic_tsu6111.h>
#endif

extern unsigned int system_rev;

#ifdef CONFIG_USB_SWITCH_TSU6111
int use_muic_tsu6111;
#endif

#ifdef CONFIG_RT8973
int use_muic_rt8973;
#endif

void check_musb_chip(void)
{
	pr_info("%s\n", __func__);
	printk(KERN_INFO"previously system_rev Revision: %u\n",	system_rev);

#if defined(CONFIG_MACH_HAWAII_SS_KYLEVE_REV00)	\
	|| defined(CONFIG_MACH_HAWAII_SS_JBTLP_REV00)	\
	|| defined(CONFIG_MACH_HAWAII_SS_KYLEVESS_REV00)	\
	|| defined(CONFIG_MACH_HAWAII_SS_HEAT3G_REV00)
	system_rev = 0x0;
#endif

#if defined (CONFIG_MACH_HAWAII_SS_LOGANDS_REV01) || defined(CONFIG_MACH_HAWAII_SS_HEAT3G_REV00)\
  || defined(CONFIG_MACH_HAWAII_SS_HEAT3G_REV01) || defined(CONFIG_MACH_HAWAII_SS_HEATNFC3G_REV00) || defined(CONFIG_MACH_HAWAII_SS_VIVALTONFC3G_REV00) || defined(CONFIG_MACH_HAWAII_SS_VIVALTODS5M_REV00)
	system_rev = 0x1;
	pr_info(" DS01/Heat00 system_rev Revision: %u\n", system_rev);
#endif

#if defined(CONFIG_MACH_HAWAII_SS_LOGANDS_REV00)
	system_rev = 0x0;
	pr_info(" DS00 system_rev Revision: %u\n", system_rev);
#endif

	pr_info(" final system_rev Revision: %u\n", system_rev);
	if (system_rev == 1) {
#if defined(CONFIG_MACH_HAWAII_SS_LOGANDS) || defined(CONFIG_MACH_HAWAII_SS_HEAT3G) \
	|| defined(CONFIG_MACH_HAWAII_SS_HEATNFC3G) || defined(CONFIG_MACH_HAWAII_SS_VIVALTONFC3G) || defined(CONFIG_MACH_HAWAII_SS_VIVALTODS5M)
#ifdef CONFIG_RT8973
		pr_info(" LoganDS01 RT8973 MUIC\n");
		use_muic_rt8973 = 1;
#endif
#endif
	} else if (system_rev == 0) {
#if defined(CONFIG_MACH_HAWAII_SS_LOGANDS) \
	|| defined(CONFIG_MACH_HAWAII_SS_HEAT3G_REV00)
#ifdef CONFIG_USB_SWITCH_TSU6111
		pr_info(" LoganDS00 TSU6111 MUIC\n");
		use_muic_tsu6111 = 1;
#endif
#endif

#if defined(CONFIG_MACH_HAWAII_SS_KYLEVE)	\
	|| defined(CONFIG_MACH_HAWAII_SS_JBTLP)	\
	|| defined(CONFIG_MACH_HAWAII_SS_KYLEVESS)	\
	|| defined(CONFIG_MACH_HAWAII_SS_HEAT3G_REV00) \
	|| defined(CONFIG_MACH_HAWAII_SS_KYLEPRO_REV00) \
	|| defined(CONFIG_MACH_HAWAII_SS_KYLEPRODS_REV00) 
#ifdef CONFIG_RT8973
		pr_info(" KyleVE00/JBTLP00/KyleVESS00/HEAT RT8973  MUIC\n");
		use_muic_rt8973 = 1;
#endif
#endif
	} else {
		pr_info(" not 00 nor 01 Please Check this line\n");
#ifdef CONFIG_RT8973
		use_muic_rt8973 = 1;
#endif
	}
}

void uas_jig_force_sleep(void)
{
#ifdef CONFIG_USB_SWITCH_TSU6111
	if (use_muic_tsu6111 == 1)
		uas_jig_force_sleep_tsu6111();
#endif

#ifdef CONFIG_RT8973
	if (use_muic_rt8973 == 1)
		uas_jig_force_sleep_rt8973();
#endif
}
EXPORT_SYMBOL(uas_jig_force_sleep);

/* for external charger detection  apart from PMU/BB*/
int bcm_ext_bc_status(void)
{
#ifdef CONFIG_USB_SWITCH_TSU6111
	if (use_muic_tsu6111 == 1)
		return bcm_ext_bc_status_tsu6111();
#endif

#ifdef CONFIG_RT8973
	if (use_muic_rt8973 == 1)
		return bcm_ext_bc_status_rt8973();
#endif

	return 0;
}
EXPORT_SYMBOL(bcm_ext_bc_status);

void musb_vbus_changed(int state)
{
#ifdef CONFIG_USB_SWITCH_TSU6111
	if (use_muic_tsu6111 == 1)
		musb_vbus_changed_tsu6111(state);
#endif

#ifdef CONFIG_RT8973
	if (use_muic_rt8973 == 1)
		musb_vbus_changed_rt8973(state);
#endif
}
EXPORT_SYMBOL(musb_vbus_changed);

unsigned int musb_get_charger_type(void)
{
#ifdef CONFIG_USB_SWITCH_TSU6111
	if (use_muic_tsu6111 == 1)
		return musb_get_charger_type_tsu6111();
#endif

#ifdef CONFIG_RT8973
	if (use_muic_rt8973 == 1)
		return musb_get_charger_type_rt8973();
#endif

	return 0;
}
EXPORT_SYMBOL(musb_get_charger_type);

void __init hawaii_muic_init(void)
{
	pr_info("%s\n", __func__);
	check_musb_chip();
#ifdef CONFIG_USB_SWITCH_TSU6111
	if (use_muic_tsu6111 == 1) {
		if (system_rev == 0) {
			pr_info("LoganDS 00 TSU6111 Please Check this\n");
			/* micro_usb_i2c_devices_info[0].addr = 0x4A >> 1;  */
		}
		/* dev_muic_tsu6111_init(); */
	}
#endif

	return;
}