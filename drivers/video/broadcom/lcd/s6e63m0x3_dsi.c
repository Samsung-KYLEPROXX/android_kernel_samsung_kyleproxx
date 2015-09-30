 /*
 * S6E63M0 AMOLED LCD panel driver.
 *
 * Author: InKi Dae  <inki.dae@samsung.com>
 * Modified by JuneSok Lee <junesok.lee@samsung.com>
 *
 * Derived from drivers/video/broadcom/lcd/smart_dimming_s6e63m0_dsi.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

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
#include <video/kona_fb.h>
#include <linux/broadcom/mobcom_types.h>
#include "dispdrv_common.h"

#include "s6e63m0x3_dsi_param.h"
#include "smart_mtp_s6e63m0_dsi.h"

#undef dev_dbg
#define dev_dbg dev_info

#define MAX_BRIGHTNESS		255
#define DEFAULT_BRIGHTNESS		190
#define DEFAULT_GAMMA_LEVEL	17 /*190cd*/

#define ELVSS_SET_START_IDX 2
#define ELVSS_SET_END_IDX 5

#define ID_VALUE_M2			0xA4
#define ID_VALUE_SM2			0xB4
#define ID_VALUE_SM2_1			0xB6

//#define ESD_OPERATION
//#define ESD_TEST
#ifdef ESD_OPERATION
#define ESD_PORT_NUM 88
#endif

extern char *get_seq(DISPCTRL_REC_T *rec);
extern void panel_write(UInt8 *buff);
extern void panel_read(UInt8 reg, UInt8 *rxBuff, UInt8 buffLen);

extern struct device *lcd_dev;

struct s6e63m0_dsi_lcd {
	struct device	*dev;
	struct mutex	lock;
	unsigned int	current_brightness;
	unsigned int	current_gamma;
	unsigned int	bl;	
	unsigned int	acl_enable;
	unsigned int	cur_acl;	
	bool			panel_awake;			
	u8			elvss_pulse;
	const u8		*elvss_offsets;
	u8*			lcd_id;	
	enum elvss_brightness		elvss_brightness;
	struct backlight_device		*bd;	
	u8			restore_brightness_level;
#ifdef SMART_DIMMING
	u8*			mtpData;
	struct str_smart_dim		smart;
#endif
#ifdef ESD_OPERATION
	unsigned int	lcd_connected;
	unsigned int	esd_enable;
	unsigned int	esd_port;
	struct workqueue_struct	*esd_workqueue;
	struct work_struct	esd_work;
	bool	esd_processing; 
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	earlysuspend;
#endif
#ifdef ESD_TEST
        struct timer_list               esd_test_timer;
#endif
};

struct s6e63m0_dsi_lcd *lcd = NULL;
u8 gPanelID[PANEL_ID_MAX], gMtpData[SEC_DSI_MTP_DATA_LEN];


void panel_read_id(void)
{
	int nMaxReadByte = 16, dataSize = SEC_DSI_MTP_DATA_LEN;
	u8 *pPanelID = gPanelID;
	u8 *pMtpData = gMtpData;	
	
	pr_info("%s\n", __func__);	

	/* Read panel ID*/
	panel_read(0xDA, pPanelID, 1); 
	panel_read(0xDB, pPanelID+1, 1);
	panel_read(0xDC, pPanelID+2, 1);	

	/* Read mtpdata*/
	pr_info("%s:  Write direct_access00_seq\n", __func__);
	panel_write(&direct_access00_seq[0]);
	panel_read(0xD3, pMtpData, nMaxReadByte);
	pr_info("%s:  Write direct_access01_seq\n", __func__);
	panel_write(&direct_access10_seq[0]);		
	panel_read(0xD3, pMtpData+nMaxReadByte, dataSize-nMaxReadByte);
#if 0
	{
		int n = 0;		
		for(n = 0; n < dataSize ; n++)
			printk("[DISPDRV] mtpData[%d] = 0x%02X\n", n, pMtpData[n]);		
	}
#endif
}

void panel_initialize(char *init_seq)
{
	int nMaxReadByte = 16, dataSize = 21;
	u8 *pPanelID = gPanelID;
	u8 *pMtpData = gMtpData;	
	
	pr_info("%s\n", __func__);	

	panel_write(init_seq);	

	//panel_read_id();
}

static ssize_t show_lcd_info(struct device *dev, struct device_attribute *attr, char *buf)
{
	char temp[20];
	sprintf(temp, "SDC_AMS397GE84\n");
	strcat(buf, temp);
	return strlen(buf);
}

static DEVICE_ATTR(lcd_type, 0444, show_lcd_info, NULL);


#ifdef ESD_OPERATION

#ifdef ESD_TEST
static void esd_test_timer_func(unsigned long data)
{
	pr_info("%s\n", __func__);

	if (list_empty(&(lcd->esd_work.entry))) {
		disable_irq_nosync(gpio_to_irq(lcd->esd_port));
		queue_work(lcd->esd_workqueue, &(lcd->esd_work));
		pr_info("%s invoked\n", __func__);
	}

	mod_timer(&lcd->esd_test_timer,  jiffies + (30*HZ));
}
#endif

void s6e63m0_esd_recovery(struct s6e63m0_dsi_lcd *lcd)
{
	pr_info("%s\n", __func__);	
	
	/* To Do : LCD Reset & Initialize */
}
static void esd_work_func(struct work_struct *work)
{
	struct s6e63m0_dsi_lcd *lcd = container_of(work, struct s6e63m0_dsi_lcd, esd_work);
	
	pr_info(" %s \n", __func__);
       
	if (lcd->esd_enable && !lcd->esd_processing/* && !battpwroff_charging*/) {
		pr_info("ESD PORT =[%d]\n", gpio_get_value(lcd->esd_port));
		
		lcd->esd_processing = true; 
		
		mutex_lock(&lcd->lock);		
		s6e63m0_esd_recovery(lcd);
		mutex_unlock(&lcd->lock);			
		
		msleep(100);
		
    lcd->esd_processing = false; 
		
		/* low is normal. On PBA esd_port coule be HIGH */
		if (gpio_get_value(lcd->esd_port)) 
			pr_info(" %s esd_work_func re-armed\n", __func__);

#ifdef ESD_TEST			
		enable_irq(gpio_to_irq(lcd->esd_port));			
#endif
	}
}

static irqreturn_t esd_interrupt_handler(int irq, void *data)
{
	struct s6e63m0_dsi_lcd *lcd = data;
	
	dev_dbg(lcd->dev,"lcd->esd_enable :%d\n", lcd->esd_enable);
	
	if (lcd->esd_enable && !lcd->esd_processing/* && !battpwroff_charging*/) {
		if (list_empty(&(lcd->esd_work.entry)))
			queue_work(lcd->esd_workqueue, &(lcd->esd_work));
		else
			dev_dbg(lcd->dev,"esd_work_func is not empty\n" );
	}
	
	return IRQ_HANDLED;
}
#endif	/*ESD_OPERATION*/

static int get_gamma_value_from_bl(int brightness)
{
	int backlightlevel;

	/* brightness setting from platform is from 0 to 255
	 * But in this driver, brightness is
	  only supported from 0 to 24 */

	switch (brightness) {
	case 0 ... 29:
		backlightlevel = GAMMA_30CD;
		break;
	case 30 ... 39:
		backlightlevel = GAMMA_40CD;
		break;
	case 40 ... 49:
		backlightlevel = GAMMA_50CD;
		break;
	case 50 ... 59:
		backlightlevel = GAMMA_60CD;
		break;
	case 60 ... 69:
		backlightlevel = GAMMA_70CD;
		break;
	case 70 ... 79:
		backlightlevel = GAMMA_80CD;
		break;
	case 80 ... 89:
		backlightlevel = GAMMA_90CD;
		break;
	case 90 ... 99:
		backlightlevel = GAMMA_100CD;
		break;
	case 100 ... 109:
		backlightlevel = GAMMA_110CD;
		break;
	case 110 ... 119:
		backlightlevel = GAMMA_120CD;
		break;
	case 120 ... 129:
		backlightlevel = GAMMA_130CD;
		break;
	case 130 ... 139:
		backlightlevel = GAMMA_140CD;
		break;
	case 140 ... 149:
		backlightlevel = GAMMA_150CD;
		break;
	case 150 ... 159:
		backlightlevel = GAMMA_160CD;
		break;
	case 160 ... 169:
		backlightlevel = GAMMA_170CD;
		break;
	case 170 ... 179:
		backlightlevel = GAMMA_180CD;
		break;
	case 180 ... 189:
		backlightlevel = GAMMA_190CD;
		break;
	case 190 ... 199:
		backlightlevel = GAMMA_200CD;
		break;
	case 200 ... 209:
		backlightlevel = GAMMA_210CD;
		break;
	case 210 ... 219:
		backlightlevel = GAMMA_220CD;
		break;
	case 220 ... 229:
		backlightlevel = GAMMA_230CD;
		break;
	case 230 ... 245:
		backlightlevel = GAMMA_240CD;
		break;
	case 246 ... 254:
		backlightlevel = GAMMA_250CD;
		break;
	case 255:
		backlightlevel = GAMMA_300CD;
		break;

	default:
		backlightlevel = DEFAULT_GAMMA_LEVEL;
		break;
        }
	return backlightlevel;

}

static int s6e63m0_set_gamma(struct s6e63m0_dsi_lcd *lcd)
{
	int ret = 0;

	dev_dbg(lcd->dev,"%s : bl=[%d]\n", __func__, lcd->bl);
	
#if 1	
	{
		int j;
		printk("gamma_sequence : ");
		for (j = 0; j < GAMMA_PARAM_LEN ; j++)
			printk("[0x%02x], ", gamma_table_sm2[lcd->bl][j+3]);
	}
#endif	

	panel_write(gamma_table_sm2[lcd->bl]);
	panel_write(gamma_update_seq);
	
	return ret;
}

static int s6e63m0_set_acl(struct s6e63m0_dsi_lcd *lcd)
{
	int ret = 0;

	if (lcd->acl_enable) {
		dev_dbg(lcd->dev,"current lcd-> bl [%d]\n",lcd->bl);
		switch (lcd->bl) {
		case 0 ... 2: /* 30cd ~ 60cd */
			if (lcd->cur_acl != 0) {
				panel_write(SEQ_ACL_NULL_DSI);
				dev_dbg(lcd->dev, "ACL_cutoff_set Percentage : off!!\n");
				lcd->cur_acl = 0;
			}
			break;
		case 3 ... 24: /* 70cd ~ 250 */
			if (lcd->cur_acl != 40) {
				panel_write(SEQ_ACL_40P_DSI);				
				dev_dbg(lcd->dev, "ACL_cutoff_set Percentage : 40!!\n");
				lcd->cur_acl = 40;
			}
			break;
			
		default:
			if (lcd->cur_acl != 40) {
				panel_write(SEQ_ACL_40P_DSI);						
				dev_dbg(lcd->dev, "ACL_cutoff_set Percentage : 40!!\n");
				lcd->cur_acl = 40;
			}
				dev_dbg(lcd->dev, " cur_acl=%d\n", lcd->cur_acl);
			break;
		}
	} else {
			panel_write(SEQ_ACL_NULL_DSI);						
			lcd->cur_acl = 0;
			dev_dbg(lcd->dev, "ACL_cutoff_set Percentage : off!!\n");
	}

	if (ret) {
		dev_err(lcd->dev, "failed to initialize ldi.\n");
		return -EIO;
	}

	return ret;
}

static int s6e63m0_set_elvss(struct s6e63m0_dsi_lcd *lcd)
{
	u8 elvss_val, i = 0;
	int ret = 0;

	enum elvss_brightness elvss_setting;

	if (lcd->current_gamma < 110)
		elvss_setting = elvss_30cd_to_100cd;
	else if (lcd->current_gamma < 170)
		elvss_setting = elvss_110cd_to_160cd;
	else if (lcd->current_gamma < 210)
		elvss_setting = elvss_170cd_to_200cd;
	else
		elvss_setting = elvss_210cd_to_300cd;

	if (elvss_setting != lcd->elvss_brightness) {

		if (lcd->elvss_offsets)
			elvss_val = lcd->elvss_pulse +
					lcd->elvss_offsets[elvss_setting];
		else
			elvss_val = 0x16;

		if (elvss_val > 0x1F)
			elvss_val = 0x1F;	/* max for STOD13CM */

		dev_dbg(lcd->dev, "ELVSS setting to %x\n", elvss_val);
		lcd->elvss_brightness = elvss_setting;
		
		for (i=ELVSS_SET_START_IDX;	i <= ELVSS_SET_END_IDX; i++)
			elvss_sequence[i] = elvss_val;		

		{
			char *buff = elvss_sequence;
			int len = *buff++;
			while(len--)
				printk("elvss seq: [0x%02X]\n", *buff++);
		}
		{
			char *buff = elvss_on;
			int len = *buff++;
			while(len--)
				printk("elvss on: [0x%02X]\n", *buff++);			
		}

		panel_write(elvss_sequence);
		panel_write(elvss_on);

	}
	return ret;
}

static int s6e63m0_dsi_display_sleep(struct s6e63m0_dsi_lcd *lcd)
{
	int ret = 0;

	if (lcd->panel_awake) {
		
		/* To Do : Update SEQ with DCS_CMD_SEQ_ENTER_SLEEP */
		
		if (!ret)
			lcd->panel_awake = false;		
	}
	
	dev_dbg(lcd->dev, "%s: display sleep & panel=%d\n", __func__, lcd->panel_awake);
		
	return ret;
}

static int s6e63m0_dsi_display_exit_sleep(struct s6e63m0_dsi_lcd *lcd)
{
	int ret = 0;

	if (!lcd->panel_awake) {
		
		/* To Do : Update SEQ with DCS_CMD_SEQ_EXIT_SLEEP */
		
		if (!ret)
			lcd->panel_awake = true;
	}
	
	dev_dbg(lcd->dev, "%s: S6E63M0 display exit sleep & panel=%d\n", __func__, lcd->panel_awake);
		
	return ret;
}

static int s6e63m0_dsi_update_brightness(struct s6e63m0_dsi_lcd *lcd, int brightness)
{
	int ret = 0;
	int gamma = 0;

	/* Defensive - to prevent table overrun. */
	if (brightness > MAX_BRIGHTNESS)
		brightness = MAX_BRIGHTNESS;

	lcd->bl = get_gamma_value_from_bl(brightness);
	gamma = candela_table[lcd->bl];

	if (gamma != lcd->current_gamma || lcd->restore_brightness_level) {
		
		lcd->current_gamma = gamma;
		
		s6e63m0_set_gamma(lcd);
		s6e63m0_set_acl(lcd);
		s6e63m0_set_elvss(lcd);

		dev_dbg(lcd->dev, "Update Brightness: brightness=%d, gamma=%d\n", brightness, gamma);
	}
	else
	{
		dev_dbg(lcd->dev, "Skip Brightness: brightness=%d, new gamma=%d, cur_gamma=%d\n", brightness, gamma, lcd->current_gamma);		
	}

	return ret;
}

static int s6e63m0_dsi_get_brightness(struct backlight_device *bd)
{
	dev_dbg(&bd->dev, "lcd get brightness returns %d\n", bd->props.brightness);
	return bd->props.brightness;
}

static int s6e63m0_dsi_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	int brightness = bd->props.brightness;

	struct s6e63m0_dsi_lcd *lcd = bl_get_data(bd);
	
	/*Protection code for  power on /off test */
	if(lcd->dev <= 0)
		return ret;

	if(!lcd->panel_awake){
		dev_dbg(lcd->dev,"panel is off. bl=[%d], brightness=[%d]\n",lcd->bl,brightness);	
		return ret;
	}

	if ((brightness < 0) ||	(brightness > bd->props.max_brightness)) {
		dev_err(&bd->dev, "lcd brightness should be 0 to %d.\n",
			bd->props.max_brightness);
		return -EINVAL;
	}
	
	dev_dbg(lcd->dev,"%s : bl=[%d], brightness=[%d]\n", __func__, lcd->bl, brightness);	
	

	mutex_lock(&lcd->lock);
	if ((brightness == 0) && (lcd->current_brightness != 0)) {
		ret = s6e63m0_dsi_display_sleep(lcd);
	}

	if ((brightness != 0) && (lcd->current_brightness == 0)) {
		ret = s6e63m0_dsi_display_exit_sleep(lcd);
	}

	if (!ret)
		ret = s6e63m0_dsi_update_brightness(lcd, brightness);

	if (ret) {
		dev_info(&bd->dev, "lcd brightness setting failed.\n");
		ret = 0;
	}
	mutex_unlock(&lcd->lock);		

	lcd->current_brightness = brightness;

	return ret;
}


static struct backlight_ops s6e63m0_dsi_backlight_ops  = {
	.get_brightness = s6e63m0_dsi_get_brightness,
	.update_status = s6e63m0_dsi_set_brightness,
};

struct backlight_properties s6e63m0_dsi_backlight_props = {
	.brightness = DEFAULT_BRIGHTNESS,
	.max_brightness = MAX_BRIGHTNESS,
	.type = BACKLIGHT_RAW,
};

static ssize_t power_reduce_show(struct device *dev, struct
device_attribute *attr, char *buf)
{
	//struct s6e63m0_dsi_lcd *lcd = dev_get_drvdata(dev);
	char temp[3];
	sprintf(temp, "%d\n", lcd->acl_enable);
	strcpy(buf, temp);
	return strlen(buf);
}
static ssize_t power_reduce_store(struct device *dev, struct
device_attribute *attr, const char *buf, size_t size)
{
	//struct s6e63m0_dsi_lcd *lcd = dev_get_drvdata(dev);
	int value;
	int rc;
	
	/*Protection code for  power on /off test */
	if(lcd->dev <= 0)
		return size;
	
	pr_info(" %s function entered\n", __func__);
  	
	rc = strict_strtoul(buf, (unsigned int) 0, (unsigned long *)&value);
	if (rc < 0)
	{
		dev_info(lcd->dev, "Failed due to acl value=%d\n", value);		
		return rc;
	}
	else{
		dev_info(lcd->dev, "acl_set_store - %d, %d\n", lcd->acl_enable, value);
		if (lcd->acl_enable != value) {
			mutex_lock(&lcd->lock);
			lcd->acl_enable = value;
			if (lcd->panel_awake)			
				s6e63m0_set_acl(lcd);
			mutex_unlock(&lcd->lock);			
		}
		return size;
	}
}
static DEVICE_ATTR(power_reduce, 0664,power_reduce_show, power_reduce_store);

#ifdef SMART_DIMMING
static void init_smart_dimming_table_22(struct s6e63m0_dsi_lcd *lcd)
{
	unsigned int i, j;
	u8 gamma_22[SEC_DSI_MTP_DATA_LEN] = {0,};

	for (i = 0; i < MAX_GAMMA_VALUE; i++) {
		calc_gamma_table_22(&lcd->smart, candela_table[i], gamma_22);
		
		/* Update gamma table with the calibration values.
		   It should skip 2 fields to update since gamma parmeter starts from the 3rd field. */
		for (j = 0; j < GAMMA_PARAM_LEN; j++)
			gamma_table_sm2[i][j+3] = (gamma_22[j]);
	}
	
#if 1
	printk("++++++++++++++++++ !SMART DIMMING RESULT! +++++++++++++++++++\n");
	for (i = 0; i < MAX_GAMMA_VALUE; i++) {
		printk("SmartDimming Gamma Result=[%3d] : ",candela_table[i]);
		for (j = 0; j < GAMMA_PARAM_LEN; j++)
			printk("[0x%02x], ", gamma_table_sm2[i][j+3]);
		printk("\n");
	}
	printk("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");	
#endif
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void s6e63m0_dsi_early_suspend(struct early_suspend *earlysuspend)
{
  int ret;
  struct s6e63m0_dsi_lcd *lcd = container_of(earlysuspend, struct s6e63m0_dsi_lcd, earlysuspend);

  pr_info(" %s function entered\n", __func__);

#ifdef ESD_OPERATION
	if (lcd->esd_enable) {
		lcd->esd_enable = 0;
		irq_set_irq_type(gpio_to_irq(lcd->esd_port), IRQF_TRIGGER_NONE);
		disable_irq_nosync(gpio_to_irq(lcd->esd_port));

		if (!list_empty(&(lcd->esd_work.entry))) {
			cancel_work_sync(&(lcd->esd_work));
			dev_dbg(lcd->dev,"cancel esd works\n");
		}

		dev_dbg(lcd->dev,"disable esd operation\n", lcd->esd_enable);
	}
#endif		  

	if (lcd->panel_awake) {
			lcd->panel_awake = false;		
			dev_dbg(lcd->dev, "panel goes to sleep\n");	
	}	
}

static void s6e63m0_dsi_late_resume(struct early_suspend *earlysuspend)
{
  struct s6e63m0_dsi_lcd *lcd = container_of(earlysuspend, struct s6e63m0_dsi_lcd, earlysuspend);

  pr_info(" %s function entered\n", __func__);

	if (!lcd->panel_awake) {
			lcd->panel_awake = true;
			dev_dbg(lcd->dev, "panel is ready\n");			
	}

	mutex_lock(&lcd->lock);	
	lcd->restore_brightness_level	= 1;
	s6e63m0_dsi_update_brightness(lcd, lcd->bd->props.brightness);	
	lcd->restore_brightness_level	= 0;
	mutex_unlock(&lcd->lock);			
	
#ifdef ESD_OPERATION
	if (lcd->lcd_connected) {
		enable_irq(gpio_to_irq(lcd->esd_port));
		irq_set_irq_type(gpio_to_irq(lcd->esd_port), IRQF_TRIGGER_RISING);
		lcd->esd_enable = 1;
		dev_dbg(lcd->dev, "change lcd->esd_enable :%d\n",lcd->esd_enable);
	}
#endif	
}
#endif

/*
 [ panelID ]
 0xDA = [fe], 0xDB = [b6], 0xDC = [19]

*/
static int s6e63m0x_panel_probe(struct platform_device *pdev)
{
	int ret = 0;

	lcd = kzalloc(sizeof(struct s6e63m0_dsi_lcd), GFP_KERNEL);
	if (!lcd)
		return -ENOMEM;

	lcd->dev = &pdev->dev;
	lcd->bl = DEFAULT_GAMMA_LEVEL;
	lcd->acl_enable = true;
	lcd->cur_acl = 0;
	lcd->lcd_id = gPanelID;
	lcd->mtpData = gMtpData;
	lcd->current_gamma = GAMMA_INDEX_NOT_SET;
	lcd->current_brightness	= 255;
	lcd->elvss_brightness = elvss_not_set;	
	lcd->panel_awake = true;	

	dev_info(lcd->dev, "%s function entered\n", __func__);			
	
	{
		int n = 0;

		dev_info(lcd->dev, "panelID : [0x%02X], [0x%02X], [0x%02X]\n", gPanelID[0], gPanelID[1], gPanelID[2]);		
		
		for(n = 0; n < SEC_DSI_MTP_DATA_LEN ; n++)
			printk("mtpData[%d] = 0x%02X\n", n, gMtpData[n]);		
	}	
	
	platform_set_drvdata(pdev, lcd);	

	mutex_init(&lcd->lock);

	lcd->bd = backlight_device_register("panel",
					lcd->dev,
					lcd,
					&s6e63m0_dsi_backlight_ops,
					&s6e63m0_dsi_backlight_props);
	if (IS_ERR(lcd->bd)) {
		ret =  PTR_ERR(lcd->bd);
		kfree(lcd);
		return ret;
	}
	lcd->restore_brightness_level	= 0;
	
	ret = device_create_file(lcd_dev, &dev_attr_power_reduce);
	if (ret < 0)
		printk("Failed to add power_reduce sysfs entries, %d\n", __LINE__);	
	
#ifdef CONFIG_LCD_CLASS_DEVICE
	ret = device_create_file(lcd_dev, &dev_attr_lcd_type);
	if (ret < 0)
		printk("Failed to add lcd_type sysfs entries, %d\n",	__LINE__);		
#endif			
	
#ifdef SMART_DIMMING
	memcpy(lcd->smart.panelid, lcd->lcd_id, 3);
	init_table_info_22(&lcd->smart);
	calc_voltage_table(&lcd->smart, lcd->mtpData);
	init_smart_dimming_table_22(lcd);
#endif

	switch (lcd->lcd_id[1]) {

		case ID_VALUE_M2:
			dev_info(lcd->dev, "Panel is AMS397GE MIPI M2\n");
			lcd->elvss_pulse = lcd->lcd_id[2];
			lcd->elvss_offsets = stod13cm_elvss_offsets;
			break;

		case ID_VALUE_SM2:
		case ID_VALUE_SM2_1:
			dev_info(lcd->dev, "Panel is AMS397GE MIPI SM2\n");
			lcd->elvss_pulse = lcd->lcd_id[2];
			lcd->elvss_offsets = stod13cm_elvss_offsets;
			break;

		default:
			dev_err(lcd->dev, "panel type not recognised (panel_id = %x, %x, %x)\n", lcd->lcd_id[0], lcd->lcd_id[1], lcd->lcd_id[2]);
			lcd->elvss_pulse = 0x16;
			lcd->elvss_offsets = stod13cm_elvss_offsets;				
			break;
	}
	
#ifdef ESD_OPERATION
	lcd->esd_workqueue = create_singlethread_workqueue("esd_workqueue");
	if (!lcd->esd_workqueue) {
		dev_info(lcd->dev, "esd_workqueue create fail\n");
		return 0;
	}
	
	INIT_WORK(&(lcd->esd_work), esd_work_func);
	
	lcd->esd_port = ESD_PORT_NUM;
	
	if (request_threaded_irq(gpio_to_irq(lcd->esd_port), NULL,
			esd_interrupt_handler, IRQF_TRIGGER_RISING, "esd_interrupt", lcd)) {
			dev_info(lcd->dev, "esd irq request fail\n");
			free_irq(gpio_to_irq(lcd->esd_port), NULL);
			lcd->lcd_connected = 0;
	}
	    
#ifdef ESD_TEST
	setup_timer(&lcd->esd_test_timer, esd_test_timer_func, 0);
	mod_timer(&lcd->esd_test_timer,  jiffies + (30*HZ));
#endif

	lcd->esd_processing = false; 
	lcd->lcd_connected = 1;	
	lcd->esd_enable = 1;		
#endif	
	
#ifdef CONFIG_HAS_EARLYSUSPEND
  lcd->earlysuspend.level   = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
  lcd->earlysuspend.suspend = s6e63m0_dsi_early_suspend;
  lcd->earlysuspend.resume  = s6e63m0_dsi_late_resume;
  register_early_suspend(&lcd->earlysuspend);
#endif	

	return 0;
}

static int s6e63m0x_panel_remove(struct platform_device *pdev)
{
	struct s6e63m0_dsi_lcd *lcd = platform_get_drvdata(pdev);

	dev_dbg(lcd->dev, "%s function entered\n", __func__);

#ifdef CONFIG_HAS_EARLYSUSPEND
  unregister_early_suspend(&lcd->earlysuspend);
#endif

	kfree(lcd);

	return 0;
}

static struct platform_driver s6e63m0x_panel_driver = {
	.driver		= {
		.name	= "S6E63M0",
		.owner	= THIS_MODULE,
	},
	.probe		= s6e63m0x_panel_probe,
	.remove		= s6e63m0x_panel_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend        = s6e63m0x_panel_suspend,
	.resume         = s6e63m0x_panel_resume,
#endif
};
static int __init s6e63m0x_panel_init(void)
{
	return platform_driver_register(&s6e63m0x_panel_driver);
}

static void __exit s6e63m0x_panel_exit(void)
{
	platform_driver_unregister(&s6e63m0x_panel_driver);
}

late_initcall_sync(s6e63m0x_panel_init);
module_exit(s6e63m0x_panel_exit);

MODULE_DESCRIPTION("s6e63m0x panel control driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:s6e63m0x");
