/* linux/drivers/video/samsung/mdnie.c
 *
 * Register interface file for Samsung mDNIe driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
 
#include <mach/gpio.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/backlight.h>
#include <linux/platform_device.h>
#include "mdnie.h"
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/delay.h>
#include <linux/lcd.h>
#include <linux/rtc.h>
 
#include "mdnie_table.h"
#include <linux/power_supply.h>

#define TRUE 1
#define FALSE 0 

#define MAX_BRIGHTNESS_LEVEL		255
#define MID_BRIGHTNESS_LEVEL		150
#define LOW_BRIGHTNESS_LEVEL		30
#define DIM_BRIGHTNESS_LEVEL		20

#define MDNIE_SUSPEND 0
#define MDNIE_RESUME 1
 
#define SCENARIO_IS_COLOR(scenario)	\
 	((scenario >= COLOR_TONE_1) && (scenario < COLOR_TONE_MAX))
 
#if defined(CONFIG_TDMB) || defined(CONFIG_TARGET_LOCALE_NTT)
#define SCENARIO_IS_DMB(scenario)	\
 	((scenario >= DMB_NORMAL_MODE) && (scenario < DMB_MODE_MAX))
#define SCENARIO_IS_VALID(scenario)	\
 	((SCENARIO_IS_COLOR(scenario)) || SCENARIO_IS_DMB(scenario) || \
 	(scenario < SCENARIO_MAX))
#else
#define SCENARIO_IS_VALID(scenario)	\
 	((SCENARIO_IS_COLOR(scenario)) || (scenario < SCENARIO_MAX))
#endif
 
static char tuning_file_name[50];
 
struct class *mdnie_class;
 
struct mdnie_info *g_mdnie;
 
int cabc_status;
EXPORT_SYMBOL(cabc_status);
 
#ifdef CONFIG_MACH_P4NOTE
static struct mdnie_backlight_value b_value;
#endif
 
struct sysfs_debug_info {
 	u8 enable;
 	pid_t pid;
 	char comm[TASK_COMM_LEN];
 	char time[128];
};
 
static struct sysfs_debug_info negative[5];
static u8 negative_idx;

//typedef unsigned char const UInt8;
 
extern int panel_write(const unsigned char *buff);

int mdnie_status = MDNIE_RESUME;

int mdnie_send_sequence(struct mdnie_info *mdnie, const unsigned char *seq)
{
 	int ret = 0, i = 0;
 	const unsigned short *wbuf;
  	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
 	if (IS_ERR_OR_NULL(seq)) {
 		dev_err(mdnie->dev, "mdnie sequence is null\n");
 		return -EPERM;
 	}
 
      if(MDNIE_SUSPEND == mdnie_status)
            return -EPERM;
 
 	mutex_lock(&mdnie->dev_lock);

   printk(" %s write sequence START\n", __func__);

   ret = panel_write(seq);

   printk(" %s write sequence ret =  %d END\n", __func__, ret);
 
 	mutex_unlock(&mdnie->dev_lock);
 
 	return ret;
 }

 static int ldi_cabc_on(void)
{
   int ret = 0;
 	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
   ret = panel_write(cabc_on00);
   ret = panel_write(cabc_on01);
   ret = panel_write(cabc_on02);
   ret = panel_write(cabc_on03);
   ret = panel_write(cabc_ui);
   
   msleep(3);

   return ret;
}

static int ldi_cabc_off(void)
{
   int ret = 0;
 	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
   ret = panel_write(cabc_off);

   msleep(3);

   return ret;
}

void backlight_cabc_on(void)
{
 	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
   int ret;
   ret =  ldi_cabc_on();
}

void backlight_cabc_off(void)
{
 	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
   int ret;
   ret =  ldi_cabc_off();
}

void set_cabc_value(struct mdnie_info *mdnie, u8 force)
{
   int ret;
 	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
   printk("%s mdnie->cabc = %d \n",__func__, mdnie->cabc);

   if( CABC_ON == mdnie->cabc)
     ret =  ldi_cabc_on();
   else
     ret =  ldi_cabc_off();

}
 
void set_mdnie_value(struct mdnie_info *mdnie, u8 force)
{
 	u8 idx;
  	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
 	if ((!mdnie->enable) && (!force)) {
 		dev_err(mdnie->dev, "mdnie states is off\n");
 		return;
 	}
 
     printk("%s mdnie->mode = %d  mdnie->scenario = %d, mdnie->negative = %d \n",__func__, mdnie->mode ,mdnie->scenario, mdnie->negative);
     mutex_lock(&mdnie->lock);

      if(NEGATIVE_ON == mdnie->negative){
          mdnie_send_sequence(mdnie, negative_tuning); 
          mutex_unlock(&mdnie->lock);
          return;
      }
      
     switch (mdnie->scenario) {

	  case UI_MODE:
             mdnie_send_sequence(mdnie, ui_tuning); 
		break;

	  case VIDEO_MODE:

             if(OUTDOOR_ON == mdnie->outdoor)
                  mdnie_send_sequence(mdnie, video_outdoor_tuning); 
             else
                  mdnie_send_sequence(mdnie, video_tuning); 

		break;

        case VIDEO_WARM_MODE:

              if(OUTDOOR_ON == mdnie->outdoor)
                  mdnie_send_sequence(mdnie, video_warm_outdoor_tuning); 
             else
                  mdnie_send_sequence(mdnie, video_warm_tuning); 

		break;

        case VIDEO_COLD_MODE:

             if(OUTDOOR_ON == mdnie->outdoor)
                  mdnie_send_sequence(mdnie, video_cold_outdoor_tuning); 
             else
                  mdnie_send_sequence(mdnie, video_cold_tuning); 

		break;

	  case CAMERA_MODE:

             if(OUTDOOR_ON == mdnie->outdoor)
                 mdnie_send_sequence(mdnie, camera_outdoor_tuning); 
             else
                 mdnie_send_sequence(mdnie, camera_tuning); 

		break;

      	  case GALLERY_MODE:
             mdnie_send_sequence(mdnie, gallery_tuning); 
		break;

	  default:

		break;
     }
      
 exit:
 	mutex_unlock(&mdnie->lock);
 
 	return;
 }
 
static ssize_t mode_show(struct device *dev,
 		struct device_attribute *attr, char *buf)
{
 	struct mdnie_info *mdnie = dev_get_drvdata(dev);
  	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
 	return sprintf(buf, "%d\n", mdnie->mode);
}
 
static ssize_t mode_store(struct device *dev,
 		struct device_attribute *attr, const char *buf, size_t count)
{
 	struct mdnie_info *mdnie = dev_get_drvdata(dev);
 	unsigned int value;
 	int ret;
  	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
 	ret = strict_strtoul(buf, 0, (unsigned long *)&value);
 
 	dev_info(dev, "%s :: value=%d\n", __func__, value);
 
 	if (value >= MODE_MAX) {
 		value = STANDARD;
 		return -EINVAL;
 	}
 
 	mutex_lock(&mdnie->lock);
 	mdnie->mode = value;
 	mutex_unlock(&mdnie->lock);
 
 	set_mdnie_value(mdnie, 0);
 
 	return count;
}
 
 
static ssize_t scenario_show(struct device *dev,
 		struct device_attribute *attr, char *buf)
{
 	struct mdnie_info *mdnie = dev_get_drvdata(dev);
  	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
 	return sprintf(buf, "%d\n", mdnie->scenario);
}
 
static ssize_t scenario_store(struct device *dev,
 		struct device_attribute *attr, const char *buf, size_t count)
{
 	struct mdnie_info *mdnie = dev_get_drvdata(dev);
 	unsigned int value;
 	int ret;
  	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
 	ret = strict_strtoul(buf, 0, (unsigned long *)&value);
 
 	dev_info(dev, "%s :: value=%d\n", __func__, value);
 
 	if (!SCENARIO_IS_VALID(value))
 		value = UI_MODE;
 
 	mutex_lock(&mdnie->lock);
 	mdnie->scenario = value;
 	mutex_unlock(&mdnie->lock);
 
 	set_mdnie_value(mdnie, 0);
 
 	return count;
}
 
 
static ssize_t outdoor_show(struct device *dev,
 		struct device_attribute *attr, char *buf)
{
 	struct mdnie_info *mdnie = dev_get_drvdata(dev);
  	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
 	return sprintf(buf, "%d\n", mdnie->outdoor);
}
 
static ssize_t outdoor_store(struct device *dev,
 		struct device_attribute *attr, const char *buf, size_t count)
{
 	struct mdnie_info *mdnie = dev_get_drvdata(dev);
 	unsigned int value;
 	int ret;
  	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
 	ret = strict_strtoul(buf, 0, (unsigned long *)&value);
 
 	dev_info(dev, "%s :: value=%d\n", __func__, value);
 
 	if (value >= OUTDOOR_MAX)
 		value = OUTDOOR_OFF;
 
 	value = (value) ? OUTDOOR_ON : OUTDOOR_OFF;
 
 	mutex_lock(&mdnie->lock);
 	mdnie->outdoor = value;
 	mutex_unlock(&mdnie->lock);
 
 	set_mdnie_value(mdnie, 0);
 
 	return count;
}
 
 

static ssize_t cabc_show(struct device *dev,
 		struct device_attribute *attr, char *buf)
{
 	struct mdnie_info *mdnie = dev_get_drvdata(dev);
  	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
 	return sprintf(buf, "%d\n", mdnie->cabc);
}
 
static ssize_t cabc_store(struct device *dev,
 		struct device_attribute *attr, const char *buf, size_t count)
{
 	struct mdnie_info *mdnie = dev_get_drvdata(dev);
 	unsigned int value;
 	int ret;
  	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
 	ret = strict_strtoul(buf, 0, (unsigned long *)&value);
 
 	dev_info(dev, "%s :: value=%d\n", __func__, value);
 
 	if (value >= CABC_MAX)
 		value = CABC_OFF;
 
 	value = (value) ? CABC_ON : CABC_OFF;
 
 	mutex_lock(&mdnie->lock);
 	mdnie->cabc = value;
        cabc_status = value;
 	mutex_unlock(&mdnie->lock);
 
      //backlight_update_CABC();
 
 	set_cabc_value(mdnie, 0);
 	//if ((mdnie->enable) && (mdnie->bd_enable))
 		//update_brightness(mdnie);
 
 	return count;
}
 
 
static ssize_t tunning_show(struct device *dev,
 		struct device_attribute *attr, char *buf)
{
 	char temp[128];
 	printk("[MDNIE] %s : %d\n", __func__, __LINE__); 
 	sprintf(temp, "%s\n", tuning_file_name);
 	strcat(buf, temp);
 
 	return strlen(buf);
}
 
static ssize_t tunning_store(struct device *dev,
 		struct device_attribute *attr, const char *buf, size_t count)
{
 	struct mdnie_info *mdnie = dev_get_drvdata(dev);
 	printk("[MDNIE] %s : %d\n", __func__, __LINE__); 
 	if (!strncmp(buf, "0", 1)) {
 		mdnie->tunning = FALSE;
 		dev_info(dev, "%s :: tunning is disabled.\n", __func__);
 	} else if (!strncmp(buf, "1", 1)) {
 		mdnie->tunning = TRUE;
 		dev_info(dev, "%s :: tunning is enabled.\n", __func__);
 	} else {
 		if (!mdnie->tunning)
 			return count;
 		memset(tuning_file_name, 0, sizeof(tuning_file_name));
 		strcpy(tuning_file_name, "/sdcard/mdnie/");
             /* strcpy(tuning_file_name, "/mnt/extSdCard/mdnie/"); */
 		strncat(tuning_file_name, buf, count-1);
 
 		mdnie_txtbuf_to_parsing(tuning_file_name);
 
 		dev_info(dev, "%s :: %s\n", __func__, tuning_file_name);
 	}
 
 	return count;
}
 
static ssize_t negative_show(struct device *dev,
 		struct device_attribute *attr, char *buf)
{
 	struct mdnie_info *mdnie = dev_get_drvdata(dev);
 	char *pos = buf;
 	u32 i;
 	printk("[MDNIE] %s : %d\n", __func__, __LINE__); 
 	pos += sprintf(pos, "%d\n", mdnie->negative);
 
 	for (i = 0; i < 5; i++) {
 		if (negative[i].enable) {
 			pos += sprintf(pos, "pid=%d, ", negative[i].pid);
 			pos += sprintf(pos, "%s, ", negative[i].comm);
 			pos += sprintf(pos, "%s\n", negative[i].time);
 		}
 	}
 
 	return pos - buf;
}
 
static ssize_t negative_store(struct device *dev,
 		struct device_attribute *attr, const char *buf, size_t count)
{
 	struct mdnie_info *mdnie = dev_get_drvdata(dev);
 	unsigned int value;
 	int ret, i;
 	struct timespec ts;
 	struct rtc_time tm;
  	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
 	ret = strict_strtoul(buf, 0, (unsigned long *)&value);
 	dev_info(dev, "%s :: value=%d, by %s\n", __func__, value, current->comm);
 
      printk("%s ret = %d\n", __func__, ret);
 
 	if (ret < 0)
 		return ret;
 	else {
 		if (mdnie->negative == value)
 			return count;
 
 		if (value >= NEGATIVE_MAX)
 			value = NEGATIVE_OFF;
 
 		value = (value) ? NEGATIVE_ON : NEGATIVE_OFF;
 
 		mutex_lock(&mdnie->lock);
 		mdnie->negative = value;
 		if (value) {
 			getnstimeofday(&ts);
 			rtc_time_to_tm(ts.tv_sec, &tm);
 			negative[negative_idx].enable = value;
 			negative[negative_idx].pid = current->pid;
 			strcpy(negative[negative_idx].comm, current->comm);
 			sprintf(negative[negative_idx].time, "%d-%02d-%02d %02d:%02d:%02d",
 				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
 			negative_idx++;
 			negative_idx %= 5;
 		}else{

                   for(i=0; i < 5; i++)
                   {
                        negative[i].enable = NEGATIVE_OFF;
                   }
 		}
 		mutex_unlock(&mdnie->lock);
 
 		set_mdnie_value(mdnie, 0);
 	}
 	return count;
}
 
static struct device_attribute mdnie_attributes[] = {
 	__ATTR(mode, 0664, mode_show, mode_store),
 	__ATTR(scenario, 0664, scenario_show, scenario_store),
 	__ATTR(outdoor, 0664, outdoor_show, outdoor_store),
 	__ATTR(cabc, 0664, cabc_show, cabc_store),
 	__ATTR(tunning, 0664, tunning_show, tunning_store),
 	__ATTR(negative, 0664, negative_show, negative_store),
 	__ATTR_NULL,
};
 
#ifdef CONFIG_PM
#if defined(CONFIG_HAS_EARLYSUSPEND)
void mdnie_early_suspend(struct early_suspend *h)
{
 
      mdnie_status = MDNIE_SUSPEND;
 	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
 
 	return ;
}
 
void mdnie_late_resume(struct early_suspend *h)
{
 	u32 i, conversion;
 	struct mdnie_info *mdnie = container_of(h, struct mdnie_info, early_suspend);

 	printk("[MDNIE] %s : %d\n", __func__, __LINE__);

      mdnie_status = MDNIE_RESUME;

 	for (i = 0; i < 5; i++) {
 		if (negative[i].enable){
 			dev_info(mdnie->dev, "pid=%d, %s, %s\n", negative[i].pid, negative[i].comm, negative[i].time);
                   mdnie->negative = NEGATIVE_ON;
 	}
 	}

      //if(NEGATIVE_ON == mdnie->negative)
      //   msleep(110);

      //set_cabc_value(mdnie, 0);
      set_mdnie_value(mdnie, 0);

      //if(NEGATIVE_ON == mdnie->negative)
        msleep(10);

 	return ;
}
#endif
#endif
 
static int mdnie_probe(struct platform_device *pdev)
{
 	struct mdnie_info *mdnie;
 	int ret = 0;


 	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
 	mdnie_class = class_create(THIS_MODULE, dev_name(&pdev->dev));
 	if (IS_ERR_OR_NULL(mdnie_class)) {
 		pr_err("failed to create mdnie class\n");
 		ret = -EINVAL;
 		goto error0;
 	}
 
 	mdnie_class->dev_attrs = mdnie_attributes;
 
 	mdnie = kzalloc(sizeof(struct mdnie_info), GFP_KERNEL);
 	if (!mdnie) {
 		pr_err("failed to allocate mdnie\n");
 		ret = -ENOMEM;
 		goto error1;
 	}
 
 	mdnie->dev = device_create(mdnie_class, &pdev->dev, 0, &mdnie, "mdnie");
 	if (IS_ERR_OR_NULL(mdnie->dev)) {
 		pr_err("failed to create mdnie device\n");
 		ret = -EINVAL;
 		goto error2;
 	}
 
 
 	mdnie->scenario = UI_MODE;
 	mdnie->mode = STANDARD;
 	mdnie->tone = TONE_NORMAL;
 	mdnie->outdoor = OUTDOOR_OFF;
 	mdnie->cabc = CABC_OFF;
 
 	mdnie->enable = TRUE;
 	mdnie->tunning = FALSE;
 	mdnie->negative = NEGATIVE_OFF;
 
 	mutex_init(&mdnie->lock);
 	mutex_init(&mdnie->dev_lock);
 
 	platform_set_drvdata(pdev, mdnie);
 	dev_set_drvdata(mdnie->dev, mdnie);
 
#ifdef CONFIG_HAS_WAKELOCK
#ifdef CONFIG_HAS_EARLYSUSPEND
 	mdnie->early_suspend.suspend = mdnie_early_suspend;
 	mdnie->early_suspend.resume = mdnie_late_resume;
 	mdnie->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN+2; //EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
 	register_early_suspend(&mdnie->early_suspend);
#endif
#endif
 
  	g_mdnie = mdnie;
 
 	set_mdnie_value(mdnie, 0);
 
 	dev_info(mdnie->dev, "registered successfully\n");
 
 	return 0;
 
 error2:
 	kfree(mdnie);
 error1:
 	class_destroy(mdnie_class);
 error0:
 	return ret; 
}
 
static int mdnie_remove(struct platform_device *pdev)
{
 	struct mdnie_info *mdnie = dev_get_drvdata(&pdev->dev);
 
 	class_destroy(mdnie_class);
 	kfree(mdnie);
 
 	return 0;
}
 
static void mdnie_shutdown(struct platform_device *pdev)
{
	printk("[mdnie] %s\n", __func__);
}
 
 
static struct platform_driver mdnie_driver = {
 	.driver		= {
 		.name	= "mdnie",
 		.owner	= THIS_MODULE,
 	},
 	.probe		= mdnie_probe,
 	.remove		= mdnie_remove,
 #ifndef CONFIG_HAS_EARLYSUSPEND
 	.suspend	= mdnie_suspend,
 	.resume		= mdnie_resume,
 #endif
 	.shutdown	= mdnie_shutdown,
};
 
static int __init mdnie_init(void)
{
 	printk("[MDNIE] %s : %d\n", __func__, __LINE__);
 	return platform_driver_register(&mdnie_driver);
}
late_initcall_sync(mdnie_init);
 
static void __exit mdnie_exit(void)
{
 	platform_driver_unregister(&mdnie_driver);
}
module_exit(mdnie_exit);
 
MODULE_DESCRIPTION("mDNIe Driver");
MODULE_LICENSE("GPL");
