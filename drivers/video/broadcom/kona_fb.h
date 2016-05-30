#ifndef __KONA_FB_H__
#define __KONA_FB_H__


#ifdef KONA_FB_DEBUG
#define konafb_debug(fmt, arg...)	\
	printk(KERN_INFO"%s:%d " fmt, __func__, __LINE__, ##arg)
#else
#define konafb_debug(fmt, arg...)	\
	do {	} while (0)
#endif /* KONA_FB_DEBUG */


#define konafb_info(fmt, arg...)	\
	printk(KERN_INFO"%s:%d " fmt, __func__, __LINE__, ##arg)

#define konafb_error(fmt, arg...)	\
	printk(KERN_ERR"%s:%d " fmt, __func__, __LINE__, ##arg)

#define konafb_warning(fmt, arg...)	\
	printk(KERN_WARNING"%s:%d " fmt, __func__, __LINE__, ##arg)

#define konafb_alert(fmt, arg...)	\
	printk(KERN_ALERT"%s:%d " fmt, __func__, __LINE__, ##arg)


extern void *DISP_DRV_GetFuncTable(void);
#ifdef CONFIG_FB_BRCM_CP_CRASH_DUMP_IMAGE_SUPPORT
extern int crash_dump_ui_on;
extern unsigned ramdump_enable;
#endif

#ifdef CONFIG_LCD_SC7798_CS02_SUPPORT
#include "lcd/hx8369_cs02.h"
#endif
#ifdef CONFIG_LCD_SC7798_CS02_SUPPORT
#include "lcd/sc7798_cs02.h"
#endif
#ifdef CONFIG_LCD_HX8369_SUPPORT
#include "lcd/hx8369.h"
#endif
#ifdef CONFIG_LCD_NT35510_SUPPORT
#include "lcd/nt35510.h"
#include "lcd/ili9806.h"
#endif
#ifdef CONFIG_LCD_S6E63M0X_SUPPORT
#include "lcd/s6e63m0x3.h"
#endif
#ifdef CONFIG_LCD_SC7798_I9060_SUPPORT
#include "lcd/sc7798_I9060.h"
#endif

#ifdef CONFIG_FB_BRCM_LCD_EXIST_CHECK
int lcd_exist = 1; //  1 means LCD exist
EXPORT_SYMBOL(lcd_exist);
#endif
static struct lcd_config *cfgs[] __initdata = {
#ifdef CONFIG_LCD_HX8369_SUPPORT	
	&hx8369_cfg,
#endif	
#ifdef CONFIG_LCD_SC7798_CS02_SUPPORT	
	&hx8369_cfg,
#endif
#ifdef CONFIG_LCD_SC7798_CS02_SUPPORT	
	&sc7798_cfg,
#endif
#ifdef CONFIG_LCD_SC7798_I9060_SUPPORT
	&sc7798_cfg,
#endif

#ifdef CONFIG_LCD_NT35510_SUPPORT	
	&nt35510_cfg,
	&ili9806_cfg,
#endif		
#ifdef CONFIG_LCD_S6E63M0X_SUPPORT	
	&s6e63m0x3_cfg,
#endif
};

static struct lcd_config * __init get_dispdrv_cfg(const char *name)
{
	int i;
	void *ret = NULL;
	i = sizeof(cfgs) / sizeof(struct lcd_config *);
	while (i--) {
		if (!strcmp(name, cfgs[i]->name)) {
			ret = cfgs[i];
			pr_err("Found a match for %s\n", cfgs[i]->name);
			break;
		}
	}
	return ret;
}

#endif /* __KONA_FB_H__ */
