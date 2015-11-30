/*
 * linux/drivers/video/backlight/pwm_bl.c
 *
 * simple PWM based backlight control, board code has to setup
 * 1) pin configuration so PWM waveforms can output
 * 2) platform_data being correctly configured
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/pwm/pwm.h>
#include <linux/pwm_backlight.h>
#include <linux/slab.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <mach/pinmux.h>
#include <linux/gpio.h>

struct pwm_bl_data {
	struct pwm_device	*pwm;
	struct device		*dev;
	unsigned int		period;
	unsigned int		lth_brightness;
	int			(*notify)(struct device *,
					  int brightness);
	void			(*notify_after)(struct device *,
					int brightness);
	int			(*check_fb)(struct device *, struct fb_info *);
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend bd_early_suspend;
#endif
	struct delayed_work bl_delay_on_work;
};

static int backlight_mode=1;
#define DIMMING_VALUE		10
#define MAX_BRIGHTNESS_VALUE	255
#define MIN_BRIGHTNESS_VALUE	10
#define BACKLIGHT_SUSPEND 0
#define BACKLIGHT_RESUME 1
/*
During soft reset, the PWM registers are reset but the pad
control registers are not.
So for a short duration (till loader sets 0x404 to PWM control
register) the PWM output remains high.

During hard reset this is not seen as both the pad control and
the PWM registers are in reset state.

Hence setting the function of pad cntrl register at 0x3500489C from
PWM2 to GPIO24 on soft reset.
*/

//static int pwm_pin = -1;
//static int pwm_pin_reboot_func = -1;
static int pwm_pin = PN_GPIO24;
static int pwm_pin_reboot_func = PF_GPIO24;
extern int backlight_check;

struct brt_value{
	int level;				// Platform setting values
	int tune_level;			// Chip Setting values
};

struct brt_value brt_table[] = {
   { 10,  7 }, 	
   { 20,  10 }, 
   { 30,  16 },
   { 40,  22 }, 
   { 50,  28 },
   { 60,  35 },
   { 70,  40 },
   { 80,  45 }, 
   { 90,  50 }, 
   { 100,  55 },   
   { 110,  60 }, 
   { 120,  65 },
   { 130,  72 },
   { 140,  78 },
   { 150,  85 },
   { 160,  92 },/* default 160:99 -> 119 */
   { 165,  98 },
   { 170,  104 },
   { 175,  110 },
   { 180,  115 },
   { 185,  120 }, 
   { 190,  125 },
   { 195,  130 },
   { 200,  135 },
   { 205,  140 },
   { 210,  145 },
   { 215,  150 },
   { 220,  155 },
   { 225,  160 },
   { 230,  165 },
   { 235,  170 },
   { 240,  175 },
   { 245,  181 },
   { 250,  187 },
   { 255,  193 }, /* MAX 190 -> 225 */
};

struct brt_value brt_table_ilit9806[] = {
	{ 10,  16 },  
	{ 20,  22 }, 
	{ 30,  28 },
	{ 40,  35 }, 
	{ 50,  40 },
	{ 60,  45 },
	{ 70,  50 },
	{ 80,  55 }, 
	{ 90,  60 }, 
	{ 100,	65 },	
	{ 110,	72 }, 
	{ 120,	78 },
	{ 130,	85 },
	{ 140,	92 },
	{ 150,	98 },
	{ 160,	104 },/* default 160:99 -> 119 */
	{ 165,	110 },
	{ 170,	115 },
	{ 175,	120 },
	{ 180,	125 },
	{ 185,	130 }, 
	{ 190,	135 },
	{ 195,	140 },
	{ 200,	145 },
	{ 205,	150 },
	{ 210,	155 },
	{ 215,	160 },
	{ 220,	165 },
	{ 225,	170 },
	{ 230,	175 },
	{ 235,	181 },
	{ 240,	187 },
	{ 245,	193 },
	{ 250,	199 },
	{ 255,	205 }, /* MAX 190 -> 225 */

};

#define MAX_BRT_STAGE (int)(sizeof(brt_table)/sizeof(struct brt_value))
#define MAX_BRT_STAGE_ILI9806 (int)(sizeof(brt_table_ilit9806)/sizeof(struct brt_value))

static int pwm_backlight_update_status(struct backlight_device *bl)
{
	struct pwm_bl_data *pb = dev_get_drvdata(&bl->dev);
	int user_intensity = bl->props.brightness;
	int brightness = bl->props.brightness;
	int max = bl->props.max_brightness;
	int i;
	
	printk("[BACKLIGHT] %s : %d\n", __func__, __LINE__);

	if (bl->props.power != FB_BLANK_UNBLANK)
		brightness = 0;

	if (bl->props.fb_blank != FB_BLANK_UNBLANK)
		brightness = 0;

	if(backlight_mode != BACKLIGHT_RESUME)
		return 0;

	if(user_intensity > 0) {
		if(user_intensity < MIN_BRIGHTNESS_VALUE) {
			brightness = DIMMING_VALUE; //DIMMING
		} else if (user_intensity == MAX_BRIGHTNESS_VALUE) {
			if(backlight_check==1)
				brightness = brt_table_ilit9806[MAX_BRT_STAGE_ILI9806-1].tune_level;
			else				
			brightness = brt_table[MAX_BRT_STAGE-1].tune_level;

		} else {
			if(backlight_check==1){
				printk("ILI9806_BACKLIGHT\n");
				for(i = 0; i < MAX_BRT_STAGE_ILI9806; i++) {
					if(user_intensity <= brt_table_ilit9806[i].level ) {
						brightness = brt_table_ilit9806[i].tune_level;
						break;
					}					
				}
			}	
			else{
				printk("NT35510_BACKLIGHT\n");				
			for(i = 0; i < MAX_BRT_STAGE; i++) {
				if(user_intensity <= brt_table[i].level ) {
					brightness = brt_table[i].tune_level;
					break;
				}
			}

			}
		}
	}

	printk("[BACKLIGHT] brightness : %d\n", brightness);

	if (pb->notify)
		brightness = pb->notify(pb->dev, brightness);

	if (brightness == 0) {
		pwm_config(pb->pwm, 0, pb->period);
		pwm_disable(pb->pwm);
	} else {
		brightness = pb->lth_brightness +
			(brightness * (pb->period - pb->lth_brightness) / max);
		pwm_config(pb->pwm, brightness, pb->period);
		pwm_enable(pb->pwm);
	}

	if (pb->notify_after)
		pb->notify_after(pb->dev, brightness);

	return 0;
}

static int pwm_backlight_get_brightness(struct backlight_device *bl)
{
	return bl->props.brightness;
}

static int pwm_backlight_check_fb(struct backlight_device *bl,
				  struct fb_info *info)
{
	struct pwm_bl_data *pb = dev_get_drvdata(&bl->dev);

	return !pb->check_fb || pb->check_fb(pb->dev, info);
}

static void bl_delay_on_func(struct work_struct *work)
{
	struct pwm_bl_data *pb =
		container_of(work, struct pwm_bl_data, bl_delay_on_work.work);
	struct platform_device *pdev =
		container_of(pb->dev, struct platform_device, dev);
	struct backlight_device *bl = dev_get_drvdata(&pdev->dev);

	pr_info("bl_delay_on_func update brightness\r\n");
	backlight_update_status(bl);
}

static const struct backlight_ops pwm_backlight_ops = {
	.update_status	= pwm_backlight_update_status,
	.get_brightness	= pwm_backlight_get_brightness,
	.check_fb	= pwm_backlight_check_fb,
};
#ifdef CONFIG_HAS_EARLYSUSPEND
static void backlight_driver_early_suspend(struct early_suspend *h)
{
	struct pwm_bl_data *pb = container_of(h, struct pwm_bl_data, bd_early_suspend);
	struct platform_device *pdev = container_of(pb->dev, struct platform_device, dev);
	struct backlight_device *bl = dev_get_drvdata(&pdev->dev);
	printk("[BACKLIGHT] %s : %d\n", __func__, __LINE__);

    backlight_mode=BACKLIGHT_SUSPEND;	
	if( bl->props.brightness) {
		pwm_config(pb->pwm, 0, pb->period);
		pwm_disable(pb->pwm);
	}
}

static void backlight_driver_late_resume(struct early_suspend *h)
{
	struct pwm_bl_data *pb = container_of(h, struct pwm_bl_data, bd_early_suspend);
	struct platform_device *pdev = container_of(pb->dev, struct platform_device, dev);
	struct backlight_device *bl = dev_get_drvdata(&pdev->dev);
	int brightness = bl->props.brightness;
	printk("[BACKLIGHT] %s : %d\n", __func__, __LINE__);

    backlight_mode=BACKLIGHT_RESUME;
    pwm_backlight_update_status(bl);	
}
#endif

static int pwm_backlight_probe(struct platform_device *pdev)
{
	struct backlight_properties props;
	struct platform_pwm_backlight_data *data = NULL;
	struct backlight_device *bl;
	struct pwm_bl_data *pb;
	const char *pwm_request_label = NULL;
	int ret;
	int bl_delay_on = 0;
	printk("[BACKLIGHT] %s : %d\n", __func__, __LINE__);
	if (pdev->dev.platform_data)
		data = pdev->dev.platform_data;

	else if (pdev->dev.of_node) {
		u32 val;
		data = kzalloc(sizeof(struct platform_pwm_backlight_data),
				GFP_KERNEL);
		if (!data)
			return -ENOMEM;

		if (of_property_read_u32(pdev->dev.of_node, "pwm-id", &val)) {
			ret = -EINVAL;
			goto err_read;
		}
		data->pwm_id = val;

		if (of_property_read_u32(pdev->dev.of_node,
				"max-brightness", &val)) {
			ret = -EINVAL;
			goto err_read;
		}
		data->max_brightness = val;

		if (of_property_read_u32(pdev->dev.of_node,
				"dft-brightness", &val)) {
			ret = -EINVAL;
			goto err_read;
		}
		data->dft_brightness = val;

		if (of_property_read_u32(pdev->dev.of_node,
				"polarity", &val)) {
			ret = -EINVAL;
			goto err_read;
		}
		data->polarity = val;

		if (of_property_read_u32(pdev->dev.of_node,
				"pwm-period-ns", &val)) {
			ret = -EINVAL;
			goto err_read;
		}
		data->pwm_period_ns = val;

		if (of_property_read_string(pdev->dev.of_node,
			"pwm-request-label", &pwm_request_label)) {
			ret = -EINVAL;
			goto err_read;
		}

		if (of_property_read_u32(pdev->dev.of_node,
				"bl-on-delay", &val)) {
			bl_delay_on = 0;
		} else
			bl_delay_on = val;

		if (of_property_read_u32(pdev->dev.of_node,
				"pwm_pin_name", &val)) {
			pwm_pin = -1;
		} else
			pwm_pin = val;

		if (of_property_read_u32(pdev->dev.of_node,
				"pwm_pin_reboot_func", &val)) {
			pwm_pin_reboot_func = -1;
		} else
			pwm_pin_reboot_func = val;

		pdev->dev.platform_data = data;

	}
	if (!data) {
		dev_err(&pdev->dev, "failed to find platform data\n");
		return -EINVAL;
	}

	if (data->init) {
		ret = data->init(&pdev->dev);
		if (ret < 0)
			return ret;
	}

	pb = devm_kzalloc(&pdev->dev, sizeof(*pb), GFP_KERNEL);
	if (!pb) {
		dev_err(&pdev->dev, "no memory for state\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	pb->period = data->pwm_period_ns;

	pb->notify = data->notify;
	pb->notify_after = data->notify_after;
	pb->check_fb = data->check_fb;
	pb->lth_brightness = data->lth_brightness *
		(data->pwm_period_ns / data->max_brightness);
	pb->dev = &pdev->dev;
	if (pdev->dev.of_node)
		pb->pwm = pwm_request(data->pwm_id, pwm_request_label);
	else
		pb->pwm = pwm_request(data->pwm_id, "backlight");

	if (IS_ERR(pb->pwm)) {
		dev_err(&pdev->dev, "unable to request PWM for backlight\n");
		ret = PTR_ERR(pb->pwm);
		goto err_alloc;
	} else
		dev_dbg(&pdev->dev, "got pwm for backlight\n");

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = data->max_brightness;
	//bl = backlight_device_register(dev_name(&pdev->dev), &pdev->dev, pb,
	//			       &pwm_backlight_ops, &props);

	bl = backlight_device_register("panel", &pdev->dev, pb,
				       &pwm_backlight_ops, &props);

	if (IS_ERR(bl)) {
		dev_err(&pdev->dev, "failed to register backlight\n");
		ret = PTR_ERR(bl);
		goto err_bl;
	}

	bl->props.brightness = data->dft_brightness;
	pwm_set_polarity(pb->pwm, data->polarity);

	pr_info("pwm_backlight_probe bl-delay-on %d\r\n", bl_delay_on);
	pr_info("pwm_backlight_probe pwm_pin  %d\r\n", pwm_pin);
	pr_info("pwm_backlight_probe pwm_pin_reboot_func %d\r\n", pwm_pin_reboot_func);

	if (bl_delay_on == 0)
		backlight_update_status(bl);
	else {
		INIT_DELAYED_WORK(&(pb->bl_delay_on_work), bl_delay_on_func);
		schedule_delayed_work(&(pb->bl_delay_on_work),
			msecs_to_jiffies(bl_delay_on));
	}

	platform_set_drvdata(pdev, bl);
#ifdef CONFIG_HAS_EARLYSUSPEND
	pb->bd_early_suspend.suspend = backlight_driver_early_suspend;
	pb->bd_early_suspend.resume = backlight_driver_late_resume;
	pb->bd_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	register_early_suspend(&pb->bd_early_suspend);
#endif

	printk("[BACKLIGHT] %s : %d\n", __func__, __LINE__);
	return 0;

err_bl:
	pwm_free(pb->pwm);
err_alloc:
	if (data->exit)
		data->exit(&pdev->dev);
err_read:
	if (pdev->dev.of_node)
		kfree(data);
	return ret;
}

static int pwm_backlight_remove(struct platform_device *pdev)
{
	struct platform_pwm_backlight_data *data = pdev->dev.platform_data;
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct pwm_bl_data *pb = dev_get_drvdata(&bl->dev);
	struct pin_config new_pin_config;

	backlight_device_unregister(bl);
	pwm_config(pb->pwm, 0, pb->period);
	pwm_disable(pb->pwm);
	pwm_free(pb->pwm);
	if (data && data->exit)
		data->exit(&pdev->dev);
	if (data && pdev->dev.of_node) {
		kfree(data);
		pdev->dev.platform_data = NULL;
	}


	/*reset the pwm pin to GPIO function if defined in the kernel-dtb*/
	if (pwm_pin >= 0 && pwm_pin_reboot_func >= 0) {
		pr_info("remove reset the pwm pin to GPIO function\r\n");
		pr_err("remove reset the pwm pin to GPIO function\r\n");
		new_pin_config.name = pwm_pin;
		pinmux_get_pin_config(&new_pin_config);
		new_pin_config.func = pwm_pin_reboot_func;
		pinmux_set_pin_config(&new_pin_config);

		gpio_direction_output( 24, 0 );
		gpio_set_value( 24, 0 );

	}

	return 0;
}

static void pwm_backlight_shutdown(struct platform_device *pdev)
{
	struct pin_config new_pin_config;

	/*reset the pwm pin to GPIO function if defined in the kernel-dtb*/
	if (pwm_pin >= 0 && pwm_pin_reboot_func >= 0) {
		pr_info("reset the pwm pin to GPIO function\r\n");
		pr_err("reset the pwm pin to GPIO function\r\n");
		new_pin_config.name = pwm_pin;
		pinmux_get_pin_config(&new_pin_config);
		new_pin_config.func = pwm_pin_reboot_func;
		pinmux_set_pin_config(&new_pin_config);

		gpio_direction_output( 24, 0 );
		gpio_set_value( 24, 0 );

	}
}

#ifdef CONFIG_PM
static int pwm_backlight_suspend(struct device *dev)
{
	struct backlight_device *bl = dev_get_drvdata(dev);
	struct pwm_bl_data *pb = dev_get_drvdata(&bl->dev);

	if (pb->notify)
		pb->notify(pb->dev, 0);

	if (pb->notify_after)
		pb->notify_after(pb->dev, 0);
	return 0;
}

static int pwm_backlight_resume(struct device *dev)
{
	return 0;
}

static SIMPLE_DEV_PM_OPS(pwm_backlight_pm_ops, pwm_backlight_suspend,
			 pwm_backlight_resume);

#endif
static const struct of_device_id pwm_backlight_of_match[] = {
	{ .compatible = "bcm,pwm-backlight", },
	{},
}
MODULE_DEVICE_TABLE(of, pwm_backlight_of_match);
static struct platform_driver pwm_backlight_driver = {
	.driver		= {
		.name	= "pwm-backlight",
		.owner	= THIS_MODULE,
		.of_match_table = pwm_backlight_of_match,
#ifdef CONFIG_PM
		.pm	= &pwm_backlight_pm_ops,
#endif
	},
	.probe		= pwm_backlight_probe,
	.remove		= pwm_backlight_remove,
	.shutdown	= pwm_backlight_shutdown,
};

module_platform_driver(pwm_backlight_driver);

MODULE_DESCRIPTION("PWM based Backlight Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pwm-backlight");

