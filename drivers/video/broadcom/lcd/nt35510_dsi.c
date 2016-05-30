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
#define ILI_ESD_OPERATION
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
extern int panel_read(UInt8 reg, UInt8 *rxBuff, UInt8 buffLen);
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
int multi_lcd_num = 2;
int panel1_id_checked;
int panel2_id_checked;
int id1, id2, id3=0;
int backlight_check = 0;
EXPORT_SYMBOL(backlight_check);

int panel_read_id(void)
{
	int ret=0;
	multi_lcd_num = multi_lcd_num -1;
	/* Read panel ID*/
	kona_fb_obtain_lock();
	
	if(!panel1_id_checked){
	ret=panel_read(0xDA, gPanelID, 1);	
	if(ret<0)
		goto read_error;	
	id1= gPanelID[0];
	
	ret=panel_read(0xDB, gPanelID, 1);	
	if(ret<0)
		goto read_error;
	id2= gPanelID[0];
	
	ret=panel_read(0xDC, gPanelID, 1);
	if(ret<0)
		goto read_error;
	id3= gPanelID[0];	
	}

	printk("[LCD] id1 = %x, id2 = %x, id3 = %x,\n", id1, id2, id3);	

	if(!panel1_id_checked){
		if((id1==0x55)&&(id2==0xBC)&&(id3==0xD1)){ // DTC
			ret=LCD_PANEL_ID_ONE;
			printk("[LCD] DTC panel, ret=%d\n", ret);
		}
		else{
			ret= -1;
		}
		panel1_id_checked=1;
		goto read_error;
	}

	if(!panel2_id_checked){
		if((id1==0x55)&&(id2==0xBC)&&(id3==0xC0)){ // BOE
			ret=LCD_PANEL_ID_TWO;
			printk("[LCD] BOE panel, ret=%d\n", ret);			
		}
		else{
			ret= -1;
		}
		panel2_id_checked=1;
		goto read_error;
	}

	read_error:
	kona_fb_release_lock();	
	if((multi_lcd_num==0)&&(ret<0))
		ret=LCD_PANEL_NOT_CONNECTION;
	printk("[LCD] %s : ret=%d\n", __func__, ret);
	backlight_check = ret;
	return ret;	
}
EXPORT_SYMBOL(panel_read_id);

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
	char temp[20] = { 0 };
	
	if(id3==0xD1)
	sprintf(temp, "INH_55BCD1\n");
	else if(id3==0xC0)	
	sprintf(temp, "INH_55BCC0\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0444, show_lcd_info, NULL);

#ifdef ILI_ESD_OPERATION
static int first_esd_enter_09h=1;
static u8 iliteck_ref_09h[5];
static int first_esd_enter_BCh=1;
static u8 iliteck_ref_BCh[21];
static int ili9806_esd_reset = 0;

int nt35510_esd_reset_get(void)
{
    pr_info("%s %d \n", __func__, ili9806_esd_reset);

    return ili9806_esd_reset;
}
void nt35510_esd_reset_set(int val)
{
    ili9806_esd_reset = val;
    pr_info("%s %d \n", __func__, ili9806_esd_reset);
}
#endif

#ifdef ESD_OPERATION
static int nt35510_panel_check(void)
{
	int ret, read_check = 0;
	u8 rdnumed, wonder[3], rdidic[3];
	u8 exp_rdidic[3] = {0x55, 0x10, 0x05};
#ifdef ILI_ESD_OPERATION
    u8 iliteck_09h[5], iliteck_BCh[21];
	u8 bch_cmd[] = {21,0xBC,0x1,0x12,0x61,0xff,0x10,0x10,0xb,0x13,0x32,0x73,0xff,0xff,0xe,0xe,0x0,0x3,0x66,0x63,0x1,0x0,0x0};
#endif
	
	//pr_info(" %s \n", __func__);	
	
	kona_fb_obtain_lock();
	if(backlight_check==2){	
		
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
}
#ifdef ILI_ESD_OPERATION
    else if(backlight_check==1){
        pr_info(" %s read 09h and BCh \n", __func__);
		
        //09 Address
        read_check=panel_read(0x09, iliteck_09h, 5);
        if(first_esd_enter_09h){
        iliteck_ref_09h[0]=iliteck_09h[1];
        iliteck_ref_09h[1]=iliteck_09h[2];
        iliteck_ref_09h[2]=iliteck_09h[3];
        iliteck_ref_09h[3]=iliteck_09h[4];      
        first_esd_enter_09h=0;
		}
		
        if( (read_check<0)||
            ((iliteck_ref_09h[0] != iliteck_09h[1])||
             (iliteck_ref_09h[1] != iliteck_09h[2])||
             (iliteck_ref_09h[2] != iliteck_09h[3])||
             (iliteck_ref_09h[3] != iliteck_09h[4]))
          ){
            pr_info("%s read_data(0x09) fail = 0x%x,0x%x,0x%x,0x%x,0x%x, %d \n", __func__,
                iliteck_09h[0], iliteck_09h[1], iliteck_09h[2], iliteck_09h[3], iliteck_09h[4], read_check);   
		ret = -1;
		goto esd_detection;
     }
		
        //BC Address
        read_check=panel_read(0xBC, iliteck_BCh, 21);
        if(first_esd_enter_BCh){
            iliteck_ref_BCh[0]=iliteck_BCh[1];
            iliteck_ref_BCh[1]=iliteck_BCh[2];
            iliteck_ref_BCh[2]=iliteck_BCh[3];
            iliteck_ref_BCh[3]=iliteck_BCh[4];
            iliteck_ref_BCh[4]=iliteck_BCh[5];
            iliteck_ref_BCh[5]=iliteck_BCh[6];
            iliteck_ref_BCh[6]=iliteck_BCh[7];
            iliteck_ref_BCh[7]=iliteck_BCh[8];
            iliteck_ref_BCh[8]=iliteck_BCh[9];
            iliteck_ref_BCh[9]=iliteck_BCh[10];
            iliteck_ref_BCh[10]=iliteck_BCh[11];
            iliteck_ref_BCh[11]=iliteck_BCh[12];
            iliteck_ref_BCh[12]=iliteck_BCh[13];
            iliteck_ref_BCh[13]=iliteck_BCh[14];
            iliteck_ref_BCh[14]=iliteck_BCh[15];
            iliteck_ref_BCh[15]=iliteck_BCh[16];
            iliteck_ref_BCh[16]=iliteck_BCh[17];
            iliteck_ref_BCh[17]=iliteck_BCh[18];
            iliteck_ref_BCh[18]=iliteck_BCh[19];
            iliteck_ref_BCh[19]=iliteck_BCh[20];

            first_esd_enter_BCh=0;
		}
		
        if((read_check<0)||
            (iliteck_ref_BCh[0] != iliteck_BCh[1])||
            (iliteck_ref_BCh[1] != iliteck_BCh[2])||
            (iliteck_ref_BCh[2] != iliteck_BCh[3])||
            (iliteck_ref_BCh[3] != iliteck_BCh[4])||
            (iliteck_ref_BCh[4] != iliteck_BCh[5])||
            (iliteck_ref_BCh[5] != iliteck_BCh[6])||
            (iliteck_ref_BCh[6] != iliteck_BCh[7])||
            (iliteck_ref_BCh[7] != iliteck_BCh[8])||
            (iliteck_ref_BCh[8] != iliteck_BCh[9])||
            (iliteck_ref_BCh[9] != iliteck_BCh[10])||
            (iliteck_ref_BCh[10] != iliteck_BCh[11])||
            (iliteck_ref_BCh[11] != iliteck_BCh[12])||
            (iliteck_ref_BCh[12] != iliteck_BCh[13])||
            (iliteck_ref_BCh[13] != iliteck_BCh[14])||
            (iliteck_ref_BCh[14] != iliteck_BCh[15])||
            (iliteck_ref_BCh[15] != iliteck_BCh[16])||
            (iliteck_ref_BCh[16] != iliteck_BCh[17])||
            (iliteck_ref_BCh[17] != iliteck_BCh[18])||
            (iliteck_ref_BCh[18] != iliteck_BCh[19])||
            (iliteck_ref_BCh[19] != iliteck_BCh[20])
            ){

                pr_info("%s read_data(0xBC) fail = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", __func__,
                    iliteck_BCh[0], iliteck_BCh[1],iliteck_BCh[2], iliteck_BCh[3],iliteck_BCh[4], iliteck_BCh[5],iliteck_BCh[6], iliteck_BCh[7]);    
                
                pr_info("%s read_data(0xBC) fail = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", __func__,
                    iliteck_BCh[8],iliteck_BCh[9], iliteck_BCh[10],iliteck_BCh[11], iliteck_BCh[12],iliteck_BCh[13], iliteck_BCh[14]);    

                pr_info("%s read_data(0xBC) fail = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x, %d\n", __func__,
                    iliteck_BCh[15], iliteck_BCh[16],iliteck_BCh[17], iliteck_BCh[18],iliteck_BCh[19], iliteck_BCh[20], read_check);   

                panel_write(bch_cmd);
                msleep(10);

			ret = -1;
			goto esd_detection;
		}		
#endif
	ret = 0;
}		
esd_detection:
	kona_fb_release_lock();

	//pr_info(" %s : finished\n", __func__);
	return ret;
}

static void nt35510_esd_recovery(void)
{
#ifdef ILI_ESD_OPERATION
    nt35510_esd_reset_set(1);
#endif

	//pr_info("%s\n", __func__);	
	
	kona_fb_esd_hw_reset();

#ifdef ILI_ESD_OPERATION
    nt35510_esd_reset_set(0);
#endif
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
