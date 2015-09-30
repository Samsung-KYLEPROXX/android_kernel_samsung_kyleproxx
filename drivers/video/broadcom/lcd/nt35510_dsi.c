/*******************************************************************************
* Copyright 2011 Broadcom Corporation.	All rights reserved.
*
* @file drivers/video/broadcom/lcd/nt35510.h
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

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/lcd.h>
#include <linux/backlight.h>
#include <linux/mutex.h>
#include <linux/fb.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <video/kona_fb.h>
#include <linux/broadcom/mobcom_types.h>
#include "dispdrv_common.h"

#undef dev_dbg
#define dev_dbg dev_info

#define ESD_OPERATION
#define DURATION_TIME 3000 /* 3000ms */
#define BOOT_WAIT_TIME 1000*600 /* 600sec */

static u8 mauc0_cmd[] = {
/* 0: Length	1: Command	2~: Parameters */
	6, 0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00,
	0x00 /* The Last 0x00 : Sequence End Mark */
};		
		
static u8 mauc1_cmd[] = {
/* 0: Length	1: Command	2~: Parameters */
	6, 0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01,
	0x00 /* The Last 0x00 : Sequence End Mark */	
};
		
extern char *get_seq(DISPCTRL_REC_T *rec);
extern void panel_write(UInt8 *buff);
extern void panel_read(UInt8 reg, UInt8 *rxBuff, UInt8 buffLen);
extern void kona_fb_esd_hw_reset(void);
extern int kona_fb_obtain_lock(void);
extern int kona_fb_release_lock(void);

extern struct device *lcd_dev;
#ifdef CONFIG_FB_BRCM_LCD_EXIST_CHECK
extern int lcd_exist;
#endif

struct nt35510_dsi_lcd {
	struct device	*dev;
	struct mutex	lock;
	u8*		lcd_id;	
#ifdef ESD_OPERATION
	unsigned int	esd_enable;
	struct delayed_work	esd_work;
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	earlysuspend;
#endif
};

struct nt35510_dsi_lcd *lcd = NULL;
u8 gPanelID[3];

int panel_read_id(void)
{
	int nMaxReadByte = 16;
	u8 *pPanelID = gPanelID;
	
	pr_info("%s\n", __func__);	

	/* To Do : Read Panel ID */
	return 0;
}

void panel_initialize(char *init_seq)
{
	int nMaxReadByte = 16;
	u8 *pPanelID = gPanelID;
	
	pr_info("%s\n", __func__);	

	panel_write(init_seq);	

	panel_read_id();
}

static ssize_t show_lcd_info(struct device *dev, struct device_attribute *attr, char *buf)
{
	char temp[20];
	sprintf(temp, "INH_55BCC0\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0444, show_lcd_info, NULL);

#ifdef ESD_OPERATION
static int nt35510_panel_check(void)
{
	int ret = 0;
	u8 rdnumed, wonder[3], rdidic[3];
	u8 exp_rdidic[3] = {0x55, 0x10, 0x05};
	
	//pr_info(" %s \n", __func__);	
	
	kona_fb_obtain_lock();
	
	/*Read Number of Errors on DSI*/
	panel_read(0x05, &rdnumed, 1);
	//pr_info("read_data(0x05) = 0x%02X \n", rdnumed);
	if (rdnumed != 0x00)
	{
		pr_err("%s : error in read_data(0x05) = 0x%02X \n", __func__, rdnumed);
		ret = -1;
		goto esd_detection;
	}
		
	/*Read ID for IC Vender Code*/
	panel_write(mauc1_cmd);
	panel_read(0xC5, rdidic, 1);	
	//pr_info("read_data(0xC5) = 0x%02X : page 1\n", rdidic[0]);	
	if (exp_rdidic[0] != rdidic[0])
	{
		pr_err("%s : error in read_data(0xC5) = 0x%02X : page 1\n", __func__, rdidic[0]);
		ret = -1;
		goto esd_detection;
	}

	/*for check switching page*/
	panel_write(mauc0_cmd);
	panel_read(0xC5, wonder, 1);	
	//pr_info("read_data(0xC5) = 0x%02X : page 0\n", wonder[0]);	
	if (wonder[0] == rdidic[0])
	{
		pr_err("%s : error in read_data(0xC5) = 0x%02X : page 0\n", __func__, wonder[0]);
		ret = -1;
		goto esd_detection;
	}
		
esd_detection:
	kona_fb_release_lock();

	//pr_info(" %s : finished\n", __func__);
	return ret;
}

static void nt35510_esd_recovery(void)
{
	//pr_info("%s\n", __func__);	
	
	kona_fb_esd_hw_reset();
}

static void esd_work_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
		
	pr_info(" lcd %s \n", __func__);

	{
	mutex_lock(&lcd->lock);		       

	if (lcd->esd_enable /* && !battpwroff_charging*/) {
		
		while(nt35510_panel_check())
		{
				nt35510_esd_recovery();
				msleep(100);
		}
		
		schedule_delayed_work(dwork, msecs_to_jiffies(DURATION_TIME));		
	}
	}
	mutex_unlock(&lcd->lock);	
}
#endif	/*ESD_OPERATION*/

#ifdef CONFIG_HAS_EARLYSUSPEND
static void nt35510_dsi_early_suspend(struct early_suspend *earlysuspend)
{
  struct nt35510_dsi_lcd *lcd = container_of(earlysuspend, struct nt35510_dsi_lcd, earlysuspend);

#ifdef ESD_OPERATION
#ifdef CONFIG_FB_BRCM_LCD_EXIST_CHECK
	if (lcd_exist) 
#endif
	{
	lcd->esd_enable = 0;
	cancel_delayed_work_sync(&(lcd->esd_work));
	dev_dbg(lcd->dev,"disable esd operation\n");
	}
#endif		  
}

static void nt35510_dsi_late_resume(struct early_suspend *earlysuspend)
{
  struct nt35510_dsi_lcd *lcd = container_of(earlysuspend, struct nt35510_dsi_lcd, earlysuspend);

#ifdef ESD_OPERATION
#ifdef CONFIG_FB_BRCM_LCD_EXIST_CHECK
	if (lcd_exist) 
#endif
	{
	lcd->esd_enable = 1;
	schedule_delayed_work(&(lcd->esd_work), msecs_to_jiffies(DURATION_TIME));	
	dev_dbg(lcd->dev, "enable esd operation : %d\n", lcd->esd_enable);
	}
#endif	
}
#endif

static int nt35510_panel_probe(struct platform_device *pdev)
{
	int ret = 0;

	lcd = kzalloc(sizeof(struct nt35510_dsi_lcd), GFP_KERNEL);
	if (!lcd)
		return -ENOMEM;

	lcd->dev = &pdev->dev;
	lcd->lcd_id = gPanelID;

	pr_info("%s function entered\n", __func__);			
	
	platform_set_drvdata(pdev, lcd);	

	mutex_init(&lcd->lock);	
	
#ifdef CONFIG_LCD_CLASS_DEVICE
	ret = device_create_file(lcd_dev, &dev_attr_lcd_type);
	if (ret < 0)
		printk("Failed to add lcd_type sysfs entries, %d\n",	__LINE__);		
#endif		
	
#ifdef ESD_OPERATION
#ifdef CONFIG_FB_BRCM_LCD_EXIST_CHECK
	if (lcd_exist) 
#endif
	{
	lcd->esd_enable = 1;	
	INIT_DELAYED_WORK(&(lcd->esd_work), esd_work_func);
	schedule_delayed_work(&(lcd->esd_work), msecs_to_jiffies(BOOT_WAIT_TIME));	
	}
#endif	

#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd->earlysuspend.level   = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	lcd->earlysuspend.suspend = nt35510_dsi_early_suspend;
	lcd->earlysuspend.resume  = nt35510_dsi_late_resume;
	register_early_suspend(&lcd->earlysuspend);
#endif	

	return ret;
}

static int nt35510_panel_remove(struct platform_device *pdev)
{
	int ret = 0;
		
	struct nt35510_dsi_lcd *lcd = platform_get_drvdata(pdev);

	dev_dbg(lcd->dev, "%s function entered\n", __func__);
	
#ifdef ESD_OPERATION	
	cancel_delayed_work_sync(&(lcd->esd_work));	
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
  unregister_early_suspend(&lcd->earlysuspend);
#endif

	kfree(lcd);	

	return ret;
}

static struct platform_driver nt35510_panel_driver = {
	.driver		= {
		.name	= "nt35510",
		.owner	= THIS_MODULE,
	},
	.probe		= nt35510_panel_probe,
	.remove		= nt35510_panel_remove,
};
static int __init nt35510_panel_init(void)
{
	return platform_driver_register(&nt35510_panel_driver);
}

static void __exit nt35510_panel_exit(void)
{
	platform_driver_unregister(&nt35510_panel_driver);
}

late_initcall_sync(nt35510_panel_init);
module_exit(nt35510_panel_exit);

MODULE_DESCRIPTION("nt35510 panel control driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:nt35510");
