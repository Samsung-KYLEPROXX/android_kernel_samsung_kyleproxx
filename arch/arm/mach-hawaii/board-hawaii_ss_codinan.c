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
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/mfd/bcm590xx/pmic.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/of_fdt.h>

#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif
#ifdef CONFIG_ION_BCM_NO_DT
#include <linux/ion.h>
#include <linux/broadcom/bcm_ion.h>
#endif
#include <linux/serial_8250.h>
#include <linux/i2c.h>
#include <linux/i2c-kona.h>
#include <linux/spi/spi.h>
#include <linux/clk.h>
#include <linux/bootmem.h>
#include <linux/input.h>
#include <linux/mfd/bcm590xx/core.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/i2c-kona.h>
#include <linux/i2c.h>
#include <linux/i2c/tango_ts.h>
#include <linux/i2c/melfas_ts.h>
#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include <asm/mach/map.h>
#include <asm/hardware/gic.h>
#include <mach/hardware.h>
#include <mach/hardware.h>
#include <mach/kona_headset_pd.h>
#include <mach/kona.h>
#if defined(CONFIG_BCM2079X_NFC_I2C)
#include <linux/nfc/bcm2079x.h>
//different with capri directory tree
//#include <bcm2079x_nfc_settings.h>
#endif
#include <mach/sdio_platform.h>
#include <mach/hawaii.h>
#include <mach/io_map.h>
#include <mach/irqs.h>
#include <mach/rdb/brcm_rdb_uartb.h>
#include <mach/clock.h>
#include <plat/spi_kona.h>
#include <plat/chal/chal_trace.h>
#include <plat/pi_mgr.h>
#include <plat/spi_kona.h>

#include <trace/stm.h>

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif

#include <mach/pinmux.h>

#include "devices.h"

#ifdef CONFIG_KEYBOARD_BCM
#include <mach/bcm_keypad.h>
#endif
#ifdef CONFIG_DMAC_PL330
#include <mach/irqs.h>
#include <plat/pl330-pdata.h>
#include <linux/dma-mapping.h>
#endif

#if defined(CONFIG_SPI_GPIO)
#include <linux/spi/spi_gpio.h>
#endif

#if defined(CONFIG_HAPTIC)
#include <linux/haptic.h>
#endif

#if (defined(CONFIG_BCM_RFKILL) || defined(CONFIG_BCM_RFKILL_MODULE))
#include <linux/broadcom/bcmbt_rfkill.h>
#endif

#ifdef CONFIG_BCM_BZHW
#include <linux/broadcom/bcm_bzhw.h>
#endif

#ifdef CONFIG_BCM_BT_LPM
#include <linux/broadcom/bcmbt_lpm.h>
#endif

#if defined(CONFIG_INPUT_MPU6050) || defined(CONFIG_INPUT_MPU6500)
#include <linux/mpu6k_input.h>
#endif

#if defined(CONFIG_SENSORS_GP2AP002)
#include <linux/gp2ap002_dev.h>
#endif

#if defined(CONFIG_BMP18X) || defined(CONFIG_BMP18X_I2C) || defined(CONFIG_BMP18X_I2C_MODULE)
#include <linux/bmp18x.h>
#include <mach/bmp18x_i2c_settings.h>
#endif

#if defined(CONFIG_AL3006) || defined(CONFIG_AL3006_MODULE)
#include <linux/al3006.h>
#include <mach/al3006_i2c_settings.h>
#endif

#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
#include <linux/input.h>
#include <linux/gpio_keys.h>
#endif

#ifdef CONFIG_BACKLIGHT_PWM
#include <linux/pwm_backlight.h>
#endif

#ifdef CONFIG_BACKLIGHT_KTD259B
#include <linux/ktd259b_bl.h>
#endif

#ifdef CONFIG_FB_BRCM_KONA
#include <video/kona_fb.h>
#endif

#if defined(CONFIG_BCM_ALSA_SOUND)
#include <mach/caph_platform.h>
#include <mach/caph_settings.h>
#endif
#ifdef CONFIG_VIDEO_UNICAM_CAMERA
#include <media/soc_camera.h>
#include "../../../drivers/media/video/camdrv_ss.h"
#endif

#ifdef CONFIG_VIDEO_KONA
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/kona-unicam.h>
#endif

#ifdef CONFIG_WD_TAPPER
#include <linux/broadcom/wd-tapper.h>
#endif

#ifdef CONFIG_I2C_GPIO
#include <linux/i2c-gpio.h>
#endif

#ifdef CONFIG_USB_SWITCH_TSU6721
#include <linux/mfd/bcmpmu.h>
#include <linux/power_supply.h>
#include <linux/i2c/tsu6721.h>
#endif
#if defined(CONFIG_SEC_CHARGING_FEATURE)
// Samsung charging feature
#include <linux/spa_power.h>
#endif
#if defined(CONFIG_TOUCHSCREEN_BCM915500) || defined(CONFIG_TOUCHSCREEN_BCM915500_MODULE)
#include <linux/i2c/bcm15500_i2c_ts.h>
#endif

#if defined(CONFIG_KEYBOARD_TC360_TOUCHKEY)
#include <linux/input/tc360-touchkey.h>
#endif

#ifdef CONFIG_USB_DWC_OTG
#include <linux/usb/bcm_hsotgctrl.h>
#include <linux/usb/otg.h>
#endif

#ifdef CONFIG_MOBICORE_OS
#include <linux/broadcom/mobicore.h>
#endif


#ifdef CONFIG_BRCM_UNIFIED_DHD_SUPPORT
#include "hawaii_wifi.h"

void send_usb_insert_event(enum bcmpmu_event_t event, void *para);
void send_chrgr_insert_event(enum bcmpmu_event_t event, void *para);

extern int hawaii_wifi_status_register(
		void (*callback)(int card_present, void *dev_id),
		void *dev_id);
#endif

/* SD */
#define SD_CARDDET_GPIO_PIN	-1

/* keypad map */
#define BCM_KEY_ROW_0  0
#define BCM_KEY_ROW_1  1
#define BCM_KEY_ROW_2  2
#define BCM_KEY_ROW_3  3
#define BCM_KEY_ROW_4  4
#define BCM_KEY_ROW_5  5
#define BCM_KEY_ROW_6  6
#define BCM_KEY_ROW_7  7

#define BCM_KEY_COL_0  0
#define BCM_KEY_COL_1  1
#define BCM_KEY_COL_2  2
#define BCM_KEY_COL_3  3
#define BCM_KEY_COL_4  4
#define BCM_KEY_COL_5  5
#define BCM_KEY_COL_6  6
#define BCM_KEY_COL_7  7

int configure_sdio_pullup(bool pull_up);

/* Touch */
#define TSC_GPIO_IRQ_PIN			73

#define TSC_GPIO_RESET_PIN			70
#define TANGO_I2C_TS_DRIVER_NUM_BYTES_TO_READ	14

/* NFC */


#define KONA_UART0_PA   UARTB_BASE_ADDR
#define KONA_UART1_PA   UARTB2_BASE_ADDR
#define KONA_UART2_PA   UARTB3_BASE_ADDR

#define HAWAII_8250PORT(name, clk, freq, uart_name)		\
{								\
	.membase    = (void __iomem *)(KONA_##name##_VA),	\
	.mapbase    = (resource_size_t)(KONA_##name##_PA),	\
	.irq        = BCM_INT_ID_##name,			\
	.uartclk    = freq,					\
	.regshift   = 2,					\
	.iotype     = UPIO_MEM32,				\
	.type       = PORT_16550A,				\
	.flags      = UPF_BOOT_AUTOCONF | UPF_BUG_THRE |	\
			UPF_FIXED_TYPE | UPF_SKIP_TEST,		\
	.private_data = (void __iomem *)((KONA_##name##_VA) +	\
					UARTB_USR_OFFSET),	\
	.clk_name = clk,					\
	.port_name = uart_name,					\
}


#ifdef CONFIG_ANDROID_PMEM
struct android_pmem_platform_data android_pmem_data = {
	.name = "pmem",
	.cmasize = 0,
	.carveout_base = 0,
	.carveout_size = 0,
};
#endif

#ifdef CONFIG_ION_BCM_NO_DT
struct ion_platform_data ion_system_data = {
	.nr = 1,
	.heaps = {
		[0] = {
			.id    = 0,
			.type  = ION_HEAP_TYPE_SYSTEM,
			.name  = "ion-system",
			.base  = 0,
			.limit = 0,
			.size  = 0,
		},
	},
};

struct ion_platform_data ion_carveout_data = {
	.nr = 2,
	.heaps = {
		[0] = {
			.id    = 3,
			.type  = ION_HEAP_TYPE_CARVEOUT,
			.name  = "ion-carveout",
			.base  = 0xa0000000,
			.limit = 0xb0000000,
			.size  = (64 * SZ_1M),
#ifdef CONFIG_ION_OOM_KILLER
			.lmk_enable = 0,
			.lmk_min_score_adj = 411,
			.lmk_min_free = 32,
#endif
		},
		[1] = {
			.id    = 1,
			.type  = ION_HEAP_TYPE_CARVEOUT,
			.name  = "ion-carveout-extra",
			.base  = 0,
			.limit = 0xa0000000,
			.size  = (0 * SZ_1M),
#ifdef CONFIG_ION_OOM_KILLER
			.lmk_enable = 0,
			.lmk_min_score_adj = 411,
			.lmk_min_free = 32,
#endif
		},
	},
};

#ifdef CONFIG_CMA
struct ion_platform_data ion_cma_data = {
	.nr = 2,
	.heaps = {
		[0] = {
			.id = 2,
			.type  = ION_HEAP_TYPE_DMA,
			.name  = "ion-cma",
			.base  = 0xa0000000,
			.limit = 0xb0000000,
			.size  = (192 * SZ_1M),
#ifdef CONFIG_ION_OOM_KILLER
			.lmk_enable = 1,
			.lmk_min_score_adj = 411,
			.lmk_min_free = 32,
#endif
		},
		[1] = {
			.id = 0,
			.type  = ION_HEAP_TYPE_DMA,
			.name  = "ion-cma-extra",
			.base  = 0x00000000,
			.limit = 0xa0000000,
			.size  = (0 * SZ_1M),
#ifdef CONFIG_ION_OOM_KILLER
			.lmk_enable = 1,
			.lmk_min_score_adj = 411,
			.lmk_min_free = 32,
#endif
		},
	},
};
#endif /* CONFIG_CMA */
#endif /* CONFIG_ION_BCM_NO_DT */

#ifdef CONFIG_MOBICORE_OS
struct mobicore_data mobicore_plat_data = {
	.name = "mobicore",
	.mobicore_base = 0x9fe00000,
	.mobicore_size = SZ_2M,
};
#endif

#ifdef CONFIG_VIDEO_UNICAM_CAMERA

#define S5K4ECGX_I2C_ADDRESS (0xAC>>1)
#define SR030PC50_I2C_ADDRESS (0x60>>1)


#define SENSOR_0_GPIO_PWRDN             (002)
#define SENSOR_0_GPIO_RST               (111)
#define SENSOR_0_CLK                    "dig_ch0_clk" /* DCLK1 */
#define SENSOR_0_CLK_FREQ               (13000000)

#define SENSOR_1_CLK                    "dig_ch0_clk" /* DCLK1 */
#define SENSOR_1_CLK_FREQ               (26000000)

#define SENSOR_1_GPIO_PWRDN             (005)

static struct i2c_board_info hawaii_i2c_camera[] = {
	{
		I2C_BOARD_INFO("camdrv_ss", S5K4ECGX_I2C_ADDRESS),
	},
	{
		I2C_BOARD_INFO("camdrv_ss_sub", SR030PC50_I2C_ADDRESS),
	},
};

static int hawaii_camera_power(struct device *dev, int on)
{
	static struct pi_mgr_dfs_node unicam_dfs_node;
	int ret;

	printk(KERN_INFO "%s:camera power %s, %d\n", __func__,
		(on ? "on" : "off"), unicam_dfs_node.valid);

	if (!unicam_dfs_node.valid) {
		ret = pi_mgr_dfs_add_request(&unicam_dfs_node, "unicam",
						PI_MGR_PI_ID_MM,
						PI_MGR_DFS_MIN_VALUE);
		if (ret) {
			printk(
			KERN_ERR "%s: failed to register PI DFS request\n",
			__func__
			);
			return -1;
		}
	}

	if (on) {
		if (pi_mgr_dfs_request_update(&unicam_dfs_node, PI_OPP_TURBO)) {
			printk(
			KERN_ERR "%s:failed to update dfs request for unicam\n",
			__func__
			);
			return -1;
		}
	}

	if (!camdrv_ss_power(0, (bool)on)) {
		printk(
		KERN_ERR "%s,camdrv_ss_power failed for MAIN CAM!!\n",
		__func__
		);
		return -1;
	}

	if (!on) {
		if (pi_mgr_dfs_request_update(&unicam_dfs_node,
						PI_MGR_DFS_MIN_VALUE)) {
			printk(
			KERN_ERR"%s: failed to update dfs request for unicam\n",
			__func__);
		}
	}

	return 0;
}
static int hawaii_camera_reset(struct device *dev)
{
	/* reset the camera gpio */
	printk(KERN_INFO "%s:camera reset\n", __func__);
	return 0;
}

static int hawaii_camera_power_sub(struct device *dev, int on)
{
	static struct pi_mgr_dfs_node unicam_dfs_node;
	int ret;

	printk(KERN_INFO "%s:camera power %s, %d\n", __func__,
		(on ? "on" : "off"), unicam_dfs_node.valid);

	if (!unicam_dfs_node.valid) {
		ret = pi_mgr_dfs_add_request(&unicam_dfs_node, "unicam",
						PI_MGR_PI_ID_MM,
						PI_MGR_DFS_MIN_VALUE);
		if (ret) {
			printk(
			KERN_ERR "%s: failed to register PI DFS request\n",
			__func__
			);
			return -1;
		}
	}

	if (on) {
		if (pi_mgr_dfs_request_update(&unicam_dfs_node, PI_OPP_TURBO)) {
			printk(
			KERN_ERR "%s:failed to update dfs request for unicam\n",
			 __func__
			);
			return -1;
		}
	}

	if (!camdrv_ss_power(1, (bool)on)) {
		printk(KERN_ERR "%s, camdrv_ss_power failed for SUB CAM!!\n",
		__func__);
		return -1;
	}

	if (!on) {
		if (pi_mgr_dfs_request_update(&unicam_dfs_node,
						PI_MGR_DFS_MIN_VALUE)) {
			printk(
			KERN_ERR"%s: failed to update dfs request for unicam\n",
			__func__);
		}
	}

	return 0;
}


static int hawaii_camera_reset_sub(struct device *dev)
{
	/* reset the camera gpio */
	printk(KERN_INFO "%s:camera reset\n", __func__);
	return 0;

}

static struct v4l2_subdev_sensor_interface_parms s5k4ecgx_if_params = {
	.if_type = V4L2_SUBDEV_SENSOR_SERIAL,
	.if_mode = V4L2_SUBDEV_SENSOR_MODE_SERIAL_CSI2,
	.orientation = V4L2_SUBDEV_SENSOR_ORIENT_90,
	.facing = V4L2_SUBDEV_SENSOR_BACK,
	.parms.serial = {
		 .lanes = 2,
		 .channel = 0,
		 .phy_rate = 0,
		 .pix_clk = 0,
		 .hs_term_time = 0x7
	},
};

static struct soc_camera_link iclink_s5k4ecgx = {
	.bus_id = 0,
	.board_info = &hawaii_i2c_camera[0],
	.i2c_adapter_id = 0,
	.module_name = "camdrv_ss",
	.power = &hawaii_camera_power,
	.reset = &hawaii_camera_reset,
	.priv =  &s5k4ecgx_if_params,
};

static struct platform_device hawaii_camera = {
	.name = "soc-camera-pdrv",
	 .id = 0,
	 .dev = {
		 .platform_data = &iclink_s5k4ecgx,
	 },
};

static struct v4l2_subdev_sensor_interface_parms sr030pc50_if_params = {
	.if_type = V4L2_SUBDEV_SENSOR_SERIAL,
	.if_mode = V4L2_SUBDEV_SENSOR_MODE_SERIAL_CSI2,
	.orientation =V4L2_SUBDEV_SENSOR_ORIENT_270,
	.facing = V4L2_SUBDEV_SENSOR_FRONT,
	.parms.serial = {
		.lanes = 1,
		.channel = 1,
		.phy_rate = 0,
		.pix_clk = 0,
		.hs_term_time = 0x7
	},
};
static struct soc_camera_link iclink_sr030pc50 = {
	.bus_id		= 0,
	.board_info	= &hawaii_i2c_camera[1],
	.i2c_adapter_id	= 0,
	.module_name	= "camdrv_ss_sub",
	.power		= &hawaii_camera_power_sub,
	.reset		= &hawaii_camera_reset_sub,
	.priv		= &sr030pc50_if_params,
};

static struct platform_device hawaii_camera_sub = {
	.name	= "soc-camera-pdrv",
	.id		= 1,
	.dev	= {
		.platform_data = &iclink_sr030pc50,
	},
};
#endif /* CONFIG_VIDEO_UNICAM_CAMERA */



static struct plat_serial8250_port hawaii_uart_platform_data[] = {
	HAWAII_8250PORT(UART0, UARTB_PERI_CLK_NAME_STR, 48000000, "bluetooth"),
	HAWAII_8250PORT(UART1, UARTB2_PERI_CLK_NAME_STR, 26000000, "gps"),
	HAWAII_8250PORT(UART2, UARTB3_PERI_CLK_NAME_STR, 26000000, "console"),
	{
		.flags = 0,
	},
};

static struct bsc_adap_cfg bsc_i2c_cfg[] = {
	{
		.speed = BSC_BUS_SPEED_400K,
		.dynamic_speed = 1,
		.bsc_clk = "bsc1_clk",
		.bsc_apb_clk = "bsc1_apb_clk",
		.retries = 1,
		.is_pmu_i2c = false,
		.fs_ref = BSC_BUS_REF_13MHZ,
		.hs_ref = BSC_BUS_REF_104MHZ,
	},

	{
		.speed = BSC_BUS_SPEED_400K,
		.dynamic_speed = 1,
		.bsc_clk = "bsc2_clk",
		.bsc_apb_clk = "bsc2_apb_clk",
		.retries = 3,
		.is_pmu_i2c = false,
		.fs_ref = BSC_BUS_REF_13MHZ,
		.hs_ref = BSC_BUS_REF_104MHZ,
	},

	{
		.speed = BSC_BUS_SPEED_400K,
		.dynamic_speed = 1,
		.bsc_clk = "bsc3_clk",
		.bsc_apb_clk = "bsc3_apb_clk",
		.retries = 1,
		.is_pmu_i2c = false,
		.fs_ref = BSC_BUS_REF_13MHZ,
		.hs_ref = BSC_BUS_REF_104MHZ,
	},

	{
		.speed = BSC_BUS_SPEED_400K,
		.dynamic_speed = 1,
		.bsc_clk = "bsc4_clk",
		.bsc_apb_clk = "bsc4_apb_clk",
		.retries = 1,
		.is_pmu_i2c = false,
		.fs_ref = BSC_BUS_REF_13MHZ,
		.hs_ref = BSC_BUS_REF_104MHZ,
	},

	{
#if defined(CONFIG_KONA_PMU_BSC_HS_MODE)
		.speed = BSC_BUS_SPEED_HS,
		/* No dynamic speed in HS mode */
		.dynamic_speed = 0,
		/*
		 * PMU can NAK certain I2C read commands, while write
		 * is in progress; and it takes a while to synchronise
		 * writes between HS clock domain(3.25MHz) and
		 * internal clock domains (32k). In such cases, we retry
		 * PMU reads until the writes are through. PMU need more
		 * retry counts in HS mode to handle this.
		 */
		.retries = 5,
#elif defined(CONFIG_KONA_PMU_BSC_HS_1MHZ)
		.speed = BSC_BUS_SPEED_HS_1MHZ,
		.dynamic_speed = 0,
		.retries = 5,
#elif defined(CONFIG_KONA_PMU_BSC_HS_1625KHZ)
		.speed = BSC_BUS_SPEED_HS_1625KHZ,
		.dynamic_speed = 0,
		.retries = 5,
#else
		.speed = BSC_BUS_SPEED_50K,
		.dynamic_speed = 1,
		.retries = 3,
#endif
		.bsc_clk = "pmu_bsc_clk",
		.bsc_apb_clk = "pmu_bsc_apb",
		.is_pmu_i2c = true,
		.fs_ref = BSC_BUS_REF_13MHZ,
		.hs_ref = BSC_BUS_REF_26MHZ,
	 },
};

static struct spi_kona_platform_data hawaii_ssp0_info = {
#ifdef CONFIG_DMAC_PL330
	.enable_dma = 1,
#else
	.enable_dma = 0,
#endif
	.cs_line = 1,
	.mode = SPI_LOOP | SPI_MODE_3,
};

static struct spi_kona_platform_data hawaii_ssp1_info = {
#ifdef CONFIG_DMAC_PL330
	.enable_dma = 1,
#else
	.enable_dma = 0,
#endif
};
#if defined(CONFIG_BCM2079X_NFC_I2C)

#define NFC_INT	90
#define NFC_WAKE 25
#define NFC_ENABLE 100

#define BCM_NFC_IRQ_GPIO	(90)       			//NFC_IRQ
#define BCM_NFC_WAKE_GPIO	(25)       			//NFC_WAKE
#define BCM_NFC_EN_GPIO		(100)      			//NFC_EN


#define BCM_NFC_SCL_GPIO	(16)       			//NFC_SCL
#define BCM_NFC_SDA_GPIO	(17)       			//NFC_SDA
#define BCM_NFC_BUSID  		(1)
//#define BCM_NFC_BUSID  		(7)
#define BCM_NFC_ADDR   		(0x77)
static struct bcm2079x_platform_data bcm2079x_pdata = {
	.irq_gpio = BCM_NFC_IRQ_GPIO,
	.en_gpio = BCM_NFC_EN_GPIO,
	.wake_gpio = BCM_NFC_WAKE_GPIO,
};

static struct i2c_board_info __initdata i2c_devs_nfc[] = {
	{
	 I2C_BOARD_INFO("bcm2079x-i2c", BCM_NFC_ADDR),
	 .platform_data = (void *)&bcm2079x_pdata,
	 .irq = gpio_to_irq(BCM_NFC_IRQ_GPIO),
	 },

};

static struct i2c_gpio_platform_data nfc_i2c_gpio_data = {
	.sda_pin    = BCM_NFC_SDA_GPIO,
	.scl_pin    = BCM_NFC_SCL_GPIO,
	.udelay  = 3, 
	.timeout = 100,
};

static struct platform_device nfc_i2c_gpio_device = {
        .name   = "i2c-gpio",
        .id     = BCM_NFC_BUSID,
        .dev        = {
		.platform_data  = &nfc_i2c_gpio_data,
        },
};

static struct platform_device *nfc_i2c_devices[] __initdata = {
        &nfc_i2c_gpio_device,
};
#endif

#ifdef CONFIG_STM_TRACE
static struct stm_platform_data hawaii_stm_pdata = {
	.regs_phys_base = STM_BASE_ADDR,
	.channels_phys_base = SWSTM_BASE_ADDR,
	.id_mask = 0x0,		/* Skip ID check/match */
	.final_funnel = CHAL_TRACE_FIN_FUNNEL,
};
#endif

#if defined(CONFIG_USB_DWC_OTG)
static struct bcm_hsotgctrl_platform_data hsotgctrl_plat_data = {
	.hsotgctrl_virtual_mem_base = KONA_USB_HSOTG_CTRL_VA,
	.chipreg_virtual_mem_base = KONA_CHIPREG_VA,
	.irq = BCM_INT_ID_HSOTG_WAKEUP,
	.usb_ahb_clk_name = USB_OTG_AHB_BUS_CLK_NAME_STR,
	.mdio_mstr_clk_name = MDIOMASTER_PERI_CLK_NAME_STR,
};
#endif

struct platform_device *hawaii_common_plat_devices[] __initdata = {
	&hawaii_serial_device,
	&hawaii_i2c_adap_devices[0],
	&hawaii_i2c_adap_devices[1],
	&hawaii_i2c_adap_devices[2],
	&hawaii_i2c_adap_devices[3],
	&hawaii_i2c_adap_devices[4],
	&pmu_device,
	&hawaii_pwm_device,
	&hawaii_ssp0_device,

#ifdef CONFIG_SENSORS_KONA
	&thermal_device,
#endif

#ifdef CONFIG_STM_TRACE
/*	&hawaii_stm_device, */
#endif

#if defined(CONFIG_HW_RANDOM_KONA)
	&rng_device,
#endif

#if defined(CONFIG_USB_DWC_OTG)
	&hawaii_usb_phy_platform_device,
	&hawaii_hsotgctrl_platform_device,
	&hawaii_otg_platform_device,
#endif

#ifdef CONFIG_KONA_AVS
	&avs_device,
#endif

#ifdef CONFIG_KONA_CPU_FREQ_DRV
	&kona_cpufreq_device,
#endif

#ifdef CONFIG_CRYPTO_DEV_BRCM_SPUM_HASH
	&hawaii_spum_device,
#endif

#ifdef CONFIG_CRYPTO_DEV_BRCM_SPUM_AES
	&hawaii_spum_aes_device,
#endif

#ifdef CONFIG_UNICAM
	&hawaii_unicam_device,
#endif

#ifdef CONFIG_VIDEO_UNICAM_CAMERA
	&hawaii_camera_device,
	&hawaii_camera,
	&hawaii_camera_sub,
#endif

#ifdef CONFIG_SND_BCM_SOC
	&hawaii_audio_device,
	&caph_i2s_device,
	&caph_pcm_device,
	&spdif_dit_device,

#endif

#ifdef CONFIG_KONA_MEMC
	&kona_memc_device,
#endif
};

struct regulator_consumer_supply hv6_supply[] = {
	{.supply = "vdd_sdxc"},
	{.supply = "sddat_debug_bus"},
};

struct regulator_consumer_supply sd_supply[] = {
	{.supply = "sdldo_uc"},
	REGULATOR_SUPPLY("vddmmc", "sdhci.3"), /* 0x3f1b0000.sdhci */
	{.supply = "vdd_sdio"},
};

struct regulator_consumer_supply sdx_supply[] = {
	{.supply = "sdxldo_uc"},
	REGULATOR_SUPPLY("vddo", "sdhci.3"), /* 0x3f1b0000.sdhci */
	{.supply = "vdd_sdxc"},
};

#ifdef CONFIG_KEYBOARD_BCM
static struct bcm_keymap hawaii_keymap[] = {
	{BCM_KEY_ROW_0, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_1, "Vol Down Key", KEY_VOLUMEDOWN},
	{BCM_KEY_ROW_0, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_3, "Vol Up Key", KEY_VOLUMEUP},
	{BCM_KEY_ROW_0, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_7, "unused", 0},
};

static struct bcm_keypad_platform_info hawaii_keypad_data = {
	.row_num = 1,
	.col_num = 5,
	.keymap = hawaii_keymap,
	.bcm_keypad_base = (void *)__iomem HW_IO_PHYS_TO_VIRT(KEYPAD_BASE_ADDR),
};

#endif

#define GPIO_KEYS_SETTINGS { { KEY_HOME, 10, 1, "HOME", EV_KEY, 0, 64}, }

#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
static struct gpio_keys_button board_gpio_keys[] = GPIO_KEYS_SETTINGS;

static struct gpio_keys_platform_data gpio_keys_data = {
        .nbuttons = ARRAY_SIZE(board_gpio_keys),
        .buttons = board_gpio_keys,
};

//#define board_gpio_keys_device concatenate(ISLAND_BOARD_ID, _gpio_keys_device)
static struct platform_device board_gpio_keys_device = {
        .name = "gpio-keys",
        .id = -1,
        .dev = {
                .platform_data = &gpio_keys_data,
        },
};
#endif

#if  defined(CONFIG_BMP18X) || defined(CONFIG_BMP18X_I2C) || defined(CONFIG_BMP18X_I2C_MODULE)
static struct i2c_board_info __initdata i2c_bmp18x_info[] = {
	{
		I2C_BOARD_INFO(BMP18X_NAME, BMP18X_I2C_ADDRESS),
	},
};
#endif

#if defined(CONFIG_AL3006) || defined(CONFIG_AL3006_MODULE)
static struct al3006_platform_data al3006_pdata = {
#ifdef AL3006_IRQ_GPIO
	.irq_gpio = AL3006_IRQ_GPIO,
#else
	.irq_gpio = -1,
#endif
};

static struct i2c_board_info __initdata i2c_al3006_info[] = {
	{
		I2C_BOARD_INFO("al3006", AL3006_I2C_ADDRESS),
		.platform_data = &al3006_pdata,
	},
};
#endif


#if defined(CONFIG_INPUT_MPU6050) || defined(CONFIG_INPUT_MPU6500)
static void sensors_regulator_on(bool onoff)
{

}

static struct mpu6k_input_platform_data mpu6k_pdata = {
	.power_on = sensors_regulator_on,
	.orientation = {
	0, -1, 0,
        1, 0, 0,
        0, 0, 1},
	.acc_cal_path = "/efs/calibration_data",
	.gyro_cal_path = "/efs/gyro_cal_data",
};
#endif

#if defined(CONFIG_INPUT_MPU6050)
#define GYRO_INT_GPIO_PIN   	(24)
static struct i2c_board_info __initdata bsc3_i2c_boardinfo[] =
{

#if defined(CONFIG_INPUT_MPU6050)
	{
		I2C_BOARD_INFO("mpu6050_input", 0x68),
		.platform_data = &mpu6k_pdata,
		.irq = gpio_to_irq(GYRO_INT_GPIO_PIN),
	},
#endif

#if defined (CONFIG_INPUT_YAS_SENSORS)
	{
		I2C_BOARD_INFO("geomagnetic", 0x2e),
	},
#endif

};

#endif


#if defined  (CONFIG_SENSORS_GP2AP002)
static void gp2ap002_power_onoff(bool onoff)
{                 
	if (onoff) {
            struct regulator *power_regulator = NULL;
            int ret=0;
            power_regulator = regulator_get(NULL, "tcxldo_uc");
            if (IS_ERR(power_regulator)){
                printk(KERN_ERR "[GP2A] can not get prox_regulator (SENSOR_3.0V) \n");
            } else {
                ret = regulator_set_voltage(power_regulator,3000000,3000000);
                printk(KERN_INFO "[GP2A] regulator_set_voltage : %d\n", ret);
                ret = regulator_enable(power_regulator);
                printk(KERN_INFO "[GP2A] regulator_enable : %d\n", ret);
                regulator_put(power_regulator);
                mdelay(5);
            }
	}
}

static void gp2ap002_led_onoff(bool onoff)
{            
        struct regulator *led_regulator = NULL;
        int ret=0;
            
	if (onoff) {	
                led_regulator = regulator_get(NULL, "sim2_vcc");
                if (IS_ERR(led_regulator)){
                    printk(KERN_ERR "[GP2A] can not get prox_regulator (SENSOR_LED_3.0V) \n");
                } else {
                    ret = regulator_set_voltage(led_regulator,3000000,3000000);
                    printk(KERN_INFO "[GP2A] regulator_set_voltage : %d\n", ret);
                    ret = regulator_enable(led_regulator);
                    printk(KERN_INFO "[GP2A] regulator_enable : %d\n", ret);
                    regulator_put(led_regulator);
                    mdelay(5);
                }
	} else {
                led_regulator = regulator_get(NULL, "sim2_vcc");
		ret = regulator_disable(led_regulator); 
                printk(KERN_INFO "[GP2A] regulator_disable : %d\n", ret);
                regulator_put(led_regulator);
	}
}

#define GPIO_SENSOR_I2C_SCL  87
#define GPIO_SENSOR_I2C_SDA  85
#define PROXI_INT_GPIO_PIN  89
static struct gp2ap002_platform_data gp2ap002_platform_data = {
    	.power_on = gp2ap002_power_onoff,
    	.led_on = gp2ap002_led_onoff,
	.irq_gpio = PROXI_INT_GPIO_PIN,
	.irq = gpio_to_irq(PROXI_INT_GPIO_PIN),        
};

static struct i2c_board_info __initdata sensor_gpio_i2c_devices[] = {
	{
		I2C_BOARD_INFO("gp2ap002", 0x44),
		.platform_data = &gp2ap002_platform_data,            
	},
};

static struct i2c_gpio_platform_data sensor_i2c_gpio_data = {
        .sda_pin    = GPIO_SENSOR_I2C_SDA,
        .scl_pin    = GPIO_SENSOR_I2C_SCL,
        .udelay  = 3, 
        .timeout = 100,
};

static struct platform_device sensor_i2c_gpio_device = {
        .name       = "i2c-gpio",
        .id     = 7,
        .dev        = {
                .platform_data  = &sensor_i2c_gpio_data,
        },
};

static struct platform_device *sensor_i2c_devices[] __initdata = {
        &sensor_i2c_gpio_device,
};

#endif


#ifdef CONFIG_KONA_HEADSET_MULTI_BUTTON

#define HS_IRQ		gpio_to_irq(121)
#define HSB_IRQ		BCM_INT_ID_AUXMIC_COMP2
#define HSB_REL_IRQ	BCM_INT_ID_AUXMIC_COMP2_INV
static unsigned int hawaii_button_adc_values[3][2] = {
	/* SEND/END Min, Max*/
	{0,	10},
	/* Volume Up  Min, Max*/
	{11, 30},
	/* Volue Down Min, Max*/
	{30, 680},
};

static unsigned int hawaii_button_adc_values_2_1[3][2] = {
	/* SEND/END Min, Max*/
	{0,     110},
	/* Volume Up  Min, Max*/
	{111,   250},
	/* Volue Down Min, Max*/
	{251,   500},
};
static struct kona_headset_pd hawaii_headset_data = {
	/* GPIO state read is 0 on HS insert and 1 for
	 * HS remove
	 */

	.hs_default_state = 1,
	/*
	 * Because of the presence of the resistor in the MIC_IN line.
	 * The actual ground may not be 0, but a small offset is added to it.
	 * This needs to be subtracted from the measured voltage to determine the
	 * correct value. This will vary for different HW based on the resistor
	 * values used.
	 *
	 * if there is a resistor present on this line, please measure the load
	 * value and put it here, otherwise 0.
	 *
	 */
	.phone_ref_offset = 0,

	/*
	 * Inform the driver whether there is a GPIO present on the board to
	 * detect accessory insertion/removal _OR_ should the driver use the
	 * COMP1 for the same.
	 */
	.gpio_for_accessory_detection = 1,

	/*
	 * Pass the board specific button detection range
	 */
	/* .button_adc_values_low = hawaii_button_adc_values,
	*/
	.button_adc_values_low = 0,

	/*
	 * Pass the board specific button detection range
	 */
	.button_adc_values_high = hawaii_button_adc_values_2_1,
	.ldo_id = "audldo_uc",
};
#endif /* CONFIG_KONA_HEADSET_MULTI_BUTTON */

#ifdef CONFIG_DMAC_PL330
static struct kona_pl330_data hawaii_pl330_pdata =	{
	/* Non Secure DMAC virtual base address */
	.dmac_ns_base = KONA_DMAC_NS_VA,
	/* Secure DMAC virtual base address */
	.dmac_s_base = KONA_DMAC_S_VA,
	/* # of PL330 dmac channels 'configurable' */
	.num_pl330_chans = 8,
	/* irq number to use */
	.irq_base = BCM_INT_ID_DMAC0,
	/* # of PL330 Interrupt lines connected to GIC */
	.irq_line_count = 8,
};
#endif

#if (defined(CONFIG_BCM_RFKILL) || defined(CONFIG_BCM_RFKILL_MODULE))
#define BCMBT_VREG_GPIO		28
#define BCMBT_N_RESET_GPIO	(-1)   /* 71 */
#define BCMBT_AUX0_GPIO		(-1)   /* clk32 */
#define BCMBT_AUX1_GPIO		(-1)   /* UARTB_SEL */

static struct bcmbt_rfkill_platform_data hawaii_bcmbt_rfkill_cfg = {
	.vreg_gpio = BCMBT_VREG_GPIO,
	.n_reset_gpio = BCMBT_N_RESET_GPIO,
	.aux0_gpio = BCMBT_AUX0_GPIO,  /* CLK32 */
	.aux1_gpio = BCMBT_AUX1_GPIO,  /* UARTB_SEL, probably not required */
};

static struct platform_device hawaii_bcmbt_rfkill_device = {
	.name = "bcmbt-rfkill",
	.id = -1,
	.dev =	{
		.platform_data = &hawaii_bcmbt_rfkill_cfg,
	},
};
#endif

#ifdef CONFIG_BCM_BZHW
#define GPIO_BT_WAKE	32
#define GPIO_HOST_WAKE	72
static struct bcm_bzhw_platform_data bcm_bzhw_data = {
	.gpio_bt_wake = GPIO_BT_WAKE,
	.gpio_host_wake = GPIO_HOST_WAKE,
};

static struct platform_device hawaii_bcm_bzhw_device = {
	.name = "bcm_bzhw",
	.id = -1,
	.dev =	{
		.platform_data = &bcm_bzhw_data,
	},
};
#endif


#ifdef CONFIG_BCM_BT_LPM
#define GPIO_BT_WAKE	32
#define GPIO_HOST_WAKE	72

static struct bcm_bt_lpm_platform_data brcm_bt_lpm_data = {
	.gpio_bt_wake = GPIO_BT_WAKE,
	.gpio_host_wake = GPIO_HOST_WAKE,
};

static struct platform_device board_bcmbt_lpm_device = {
	.name = "bcmbt-lpm",
	.id = -1,
	.dev = {
		.platform_data = &brcm_bt_lpm_data,
	},
};
#endif



#ifdef CONFIG_BYPASS_WIFI_DEVTREE
static struct board_wifi_info brcm_wifi_data = {
	.wl_reset_gpio = 3,
	.host_wake_gpio = 74,
	.board_nvram_file = "/system/vendor/firmware/fw_wifi_nvram_4330.txt",
	.module_name = "bcm-wifi",
};
static struct platform_device board_wifi_driver_device = {

	.name = "bcm_wifi",
	.id = -1,
	.dev = {
		.platform_data = &brcm_wifi_data,
	},
};
#endif
/*
 * SPI board info for the slaves
 */
static struct spi_board_info spi_slave_board_info[] __initdata = {
#ifdef CONFIG_SPI_SPIDEV
	{
	 .modalias = "spidev",	/* use spidev generic driver */
	 .max_speed_hz = 13000000,	/* use max speed */
	 .bus_num = 0,		/* framework bus number */
	 .chip_select = 0,	/* for each slave */
	 .platform_data = NULL,	/* no spi_driver specific */
	 .irq = 0,		/* IRQ for this device */
	 .mode = SPI_LOOP,	/* SPI mode 0 */
	 },
#endif
};

#if defined(CONFIG_HAPTIC_SAMSUNG_PWM)
void haptic_gpio_setup(void)
{
	/* Board specific configuration like pin mux & GPIO */
}

static struct haptic_platform_data haptic_control_data = {
	/* Haptic device name: can be device-specific name like ISA1000 */
	.name = "pwm_vibra",
	/* PWM interface name to request */
	.pwm_id = 2,
	/* Invalid gpio for now, pass valid gpio number if connected */
	.gpio = ARCH_NR_GPIOS,
	.setup_pin = haptic_gpio_setup,
};

struct platform_device haptic_pwm_device = {
	.name   = "samsung_pwm_haptic",
	.id     = -1,
	.dev	=	 {	.platform_data = &haptic_control_data,}
};

#endif /* CONFIG_HAPTIC_SAMSUNG_PWM */

static struct sdio_platform_cfg hawaii_sdio_param[] = {
        {
                .id = 0,
                .data_pullup = 0,
                .cd_gpio = SD_CARDDET_GPIO_PIN,
                .devtype = SDIO_DEV_TYPE_SDMMC,
                .flags = KONA_SDIO_FLAGS_DEVICE_REMOVABLE,
                .peri_clk_name = "sdio1_clk",
                .ahb_clk_name = "sdio1_ahb_clk",
                .sleep_clk_name = "sdio1_sleep_clk",
                .peri_clk_rate = 48000000,
                /*The SD card regulator*/
                .vddo_regulator_name = "vdd_sdio",
                /*The SD controller regulator*/
                .vddsdxc_regulator_name = "vdd_sdxc",
                .configure_sdio_pullup = configure_sdio_pullup,
        },
        {
                .id = 1,
                .data_pullup = 0,
                .is_8bit = 1,
                .devtype = SDIO_DEV_TYPE_EMMC,
                .flags = KONA_SDIO_FLAGS_DEVICE_NON_REMOVABLE ,
                .peri_clk_name = "sdio2_clk",
                .ahb_clk_name = "sdio2_ahb_clk",
                .sleep_clk_name = "sdio2_sleep_clk",
                .peri_clk_rate = 52000000,
        },
        {
                .id = 2,
                .data_pullup = 0,
                .devtype = SDIO_DEV_TYPE_WIFI,
                .flags = KONA_SDIO_FLAGS_DEVICE_REMOVABLE,
                .peri_clk_name = "sdio3_clk",
                .ahb_clk_name = "sdio3_ahb_clk",
                .sleep_clk_name = "sdio3_sleep_clk",
                .peri_clk_rate = 48000000,
#ifdef CONFIG_BRCM_UNIFIED_DHD_SUPPORT
                .wifi_gpio = {
                        .reset          = 3,
                        .reg            = -1,
                        .host_wake      = 74,
                        .shutdown       = -1,
                },
		.register_status_notify = hawaii_wifi_status_register,
#endif
        },
};


void dump_rhea_pin_config(struct pin_config *debug_pin_config)
{

	printk("%s-drv_sth:%d, input_dis:%d, slew_rate_ctrl:%d, pull_up:%d, pull_dn:%d, hys_en:%d, sel:%d\n"
				,__func__
				,debug_pin_config->reg.b.drv_sth
				,debug_pin_config->reg.b.input_dis
				,debug_pin_config->reg.b.slew_rate_ctrl
				,debug_pin_config->reg.b.pull_up
				,debug_pin_config->reg.b.pull_dn
				,debug_pin_config->reg.b.hys_en
				,debug_pin_config->reg.b.sel);

}


int configure_sdio_pullup(bool pull_up)
{
    int ret=0;
	char i;
	struct pin_config new_pin_config;

	pr_err("%s, Congifure Pin with pull_up:%d\n",__func__,pull_up);
	
	new_pin_config.name = PN_SDCMD;

	ret = pinmux_get_pin_config(&new_pin_config);
	if(ret){
		printk("%s, Error pinmux_get_pin_config():%d\n",__func__,ret);
		return ret;
	}
	
	printk("%s-Before setting pin with new setting\n",__func__);
	dump_rhea_pin_config(&new_pin_config);	
	if(pull_up){
		new_pin_config.reg.b.pull_up =PULL_UP_ON;
		new_pin_config.reg.b.pull_dn =PULL_DN_OFF;
	}
	else{
		new_pin_config.reg.b.pull_up =PULL_UP_OFF;
		new_pin_config.reg.b.pull_dn =PULL_DN_ON;
	}
		
	ret = pinmux_set_pin_config(&new_pin_config);
	if(ret){
		pr_err("%s - fail to configure mmc_cmd:%d\n",__func__,ret);
		return ret;
	}
		
	ret = pinmux_get_pin_config(&new_pin_config);
	if(ret){
		pr_err("%s, Error pinmux_get_pin_config():%d\n",__func__,ret);
		return ret;
	}
	printk("%s-after setting pin with new setting\n",__func__);
	dump_rhea_pin_config(&new_pin_config);

	for(i=0;i<4;i++){
		new_pin_config.name = (PN_SDDAT0+i);	
		ret = pinmux_get_pin_config(&new_pin_config);
		if(ret){
			printk("%s, Error pinmux_get_pin_config():%d\n",__func__,ret);
			return ret;
		}
		printk("%s-Before setting pin with new setting\n",__func__);
		dump_rhea_pin_config(&new_pin_config);	
		if(pull_up){
			new_pin_config.reg.b.pull_up =PULL_UP_ON;
			new_pin_config.reg.b.pull_dn =PULL_DN_OFF;
		}
		else{
			new_pin_config.reg.b.pull_up =PULL_UP_OFF;
			new_pin_config.reg.b.pull_dn =PULL_DN_ON;
		}
		
		ret = pinmux_set_pin_config(&new_pin_config);
		if(ret){
			pr_err("%s - fail to configure mmc_cmd:%d\n",__func__,ret);
			return ret;
		}
		
		ret = pinmux_get_pin_config(&new_pin_config);
		if(ret){
			pr_err("%s, Error pinmux_get_pin_config():%d\n",__func__,ret);
			return ret;
		}
		printk("%s-after setting pin with new setting\n",__func__);
		dump_rhea_pin_config(&new_pin_config);
	}	

	return ret;
}





#ifdef CONFIG_BACKLIGHT_PWM

static struct platform_pwm_backlight_data hawaii_backlight_data = {
/* backlight */
	.pwm_id		= 2,
	.max_brightness	= 32,   /* Android calibrates to 32 levels*/
	.dft_brightness	= 32,
	.polarity	= 1,    /* Inverted polarity */
	.pwm_period_ns	= 1000000,
};

#endif /*CONFIG_BACKLIGHT_PWM */

#ifdef CONFIG_BACKLIGHT_KTD259B

static struct platform_ktd259b_backlight_data bcm_ktd259b_backlight_data = {
	.max_brightness = 255,
	.dft_brightness = 160,
	.ctrl_pin = 24,
};

struct platform_device hawaii_backlight_device = {
	.name           = "panel",
	.id 		= -1,
	.dev 	= {
		.platform_data  =  &bcm_ktd259b_backlight_data,
	},
};

#endif /* CONFIG_BACKLIGHT_KTD259B */

/* Remove this comment when camera data for Hawaii is updated */

#ifdef CONFIG_WD_TAPPER
static struct wd_tapper_platform_data wd_tapper_data = {
	/* Set the count to the time equivalent to the time-out in seconds
	 * required to pet the PMU watchdog to overcome the problem of reset in
	 * suspend*/
	.count = 300,
	.ch_num = 1,
	.name = "aon-timer",
};

static struct platform_device wd_tapper = {
	.name = "wd_tapper",
	.id = 0,
	.dev = {
		.platform_data = &wd_tapper_data,
	},
};
#endif

#if defined (CONFIG_TOUCHKEY_BACKLIGHT)
static struct platform_device touchkeyled_device = {
	.name 		= "touchkey-led",
	.id 		= -1,
};
#endif

#if defined(CONFIG_KEYBOARD_TC360_TOUCHKEY)
#define GPIO_3_TOUCH_LDO_EN -1
#define GPIO_3_TOUCH_INT 26
#define GPIO_3_TOUCH_SCL 92
#define GPIO_3_TOUCH_SDA 91

int tc360_keycodes[] = {KEY_MENU, KEY_BACK};

static struct regulator *touchkey_regulator=NULL;
static struct regulator *touchkeyled_regulator=NULL;
extern u8 touch_pressed;

static int tc360_setup_power(struct device *dev, bool setup)
{
	int ret;
		
	printk(KERN_ERR "%s \n", __func__);
	if (setup) {
		if(touchkey_regulator == NULL){
		touchkey_regulator = regulator_get(NULL, "gpldo1_uc"); 
			if(IS_ERR(touchkey_regulator)){
				ret = PTR_ERR(touchkey_regulator);
				printk("Fail to get Touch Key regulator (%d)\n", ret);
				goto err_get_gpldo1_regulator;
			}
			ret = regulator_set_voltage(touchkey_regulator,2500000,2500000); //@Fixed me, HW		
			if(ret < 0)	{
				printk("[TSP] Fail to set voltage 2.5v ret = %d \n", ret);  		
				goto err_set_gpldo1_voltage;
			}
	}
	
		if(touchkeyled_regulator == NULL){
			touchkeyled_regulator = regulator_get(NULL, "gpldo3_uc"); 
			if(IS_ERR(touchkeyled_regulator)){
				ret = PTR_ERR(touchkeyled_regulator);
				printk("Fail to get Touch Key regulator (%d)\n", ret);
				goto err_get_gpldo3_regulator;
			}
			ret = regulator_set_voltage(touchkeyled_regulator,3300000,3300000); //@Fixed me, HW		
			if(ret < 0)	{
				printk("[TSP] Fail to set voltage 2.5v ret = %d \n", ret);  		
				goto err_set_gpldo3_voltage;
			}
		}		
		
	}
	 else {
		regulator_force_disable(touchkey_regulator);
		regulator_put(touchkey_regulator);

		regulator_force_disable(touchkeyled_regulator);
		regulator_put(touchkeyled_regulator);		
	}

	return ret;

err_get_gpldo1_regulator:
err_set_gpldo1_voltage:
	regulator_put(touchkey_regulator);
err_get_gpldo3_regulator:
err_set_gpldo3_voltage:
	regulator_put(touchkeyled_regulator);	
	
	return ret;
}

static void tc360_power(bool on)
{
	int ret;
	
	if(!touchkey_regulator) {
		printk(KERN_ERR "%s: No regulator. \n", __func__);
		return;
	}
	
	printk(KERN_ERR "%s %s\n", __func__, (on) ? "on" : "off");	
	
	if (on)
	{
		ret = regulator_enable(touchkey_regulator);
	}
	else
	{
		ret = regulator_disable(touchkey_regulator);
	}

	printk(KERN_INFO "%s: %s (%d)\n", __func__, (on) ? "on" : "off", ret);
}


static void tc360_led_power(bool on)
{
	int ret;

	if (!touchkeyled_regulator) {
		printk(KERN_ERR "%s: No regulator.\n", __func__);
		return;
	}

	if (on)
		ret = regulator_enable(touchkeyled_regulator);
	else
		ret = regulator_disable(touchkeyled_regulator);

	printk(KERN_INFO "%s: %s (%d)\n", __func__, (on) ? "on" : "off", ret);
}

static void tc360_pin_configure(bool to_gpios)
{
	/* the below routine is commented out.
	 * because the 'golden' use s/w i2c for tc360 touchkey.
	 */
	if (to_gpios) {
		gpio_direction_output(GPIO_3_TOUCH_SCL, 1);
		gpio_direction_output(GPIO_3_TOUCH_SDA, 1);
	} else {
		gpio_direction_output(GPIO_3_TOUCH_SCL, 1);
		gpio_direction_output(GPIO_3_TOUCH_SDA, 1);
	}

}

static void tc360_int_set_pull(bool to_up)
{
#if 0
	int ret;
	int pull = (to_up) ? NMK_GPIO_PULL_UP : NMK_GPIO_PULL_DOWN;

	ret = nmk_gpio_set_pull(TOUCHKEY_INT_GOLDEN_BRINGUP, pull);
	if (ret < 0)
		printk(KERN_ERR "%s: fail to set pull-%s on interrupt pin\n",
		       __func__,
		       (pull == NMK_GPIO_PULL_UP) ? "up" : "down");
#endif		       
}


struct tc360_platform_data tc360_pdata = {
	.gpio_scl = GPIO_3_TOUCH_SCL,
	.gpio_sda = GPIO_3_TOUCH_SDA,
	.gpio_int = GPIO_3_TOUCH_INT,
	.gpio_en = GPIO_3_TOUCH_LDO_EN,
	.udelay = 6,
	.num_key = ARRAY_SIZE(tc360_keycodes),
	.keycodes = tc360_keycodes,
	.suspend_type = TC360_SUSPEND_WITH_POWER_OFF,
	.setup_power = tc360_setup_power,
	.power = tc360_power,
	.led_power = tc360_led_power,
	.pin_configure = tc360_pin_configure,
	.int_set_pull = tc360_int_set_pull,
	.touchscreen_is_pressed= &touch_pressed,
};

static struct i2c_board_info __initdata touchkey_gpio_i2c_devices[] = {
	{
		I2C_BOARD_INFO(TC360_DEVICE, 0x20),
		.platform_data	= &tc360_pdata,
		.irq = gpio_to_irq(GPIO_3_TOUCH_INT),
	},
};

static struct i2c_gpio_platform_data touch_i2c_gpio_data = {
        .sda_pin    = GPIO_3_TOUCH_SDA,
        .scl_pin    = GPIO_3_TOUCH_SCL,
        .udelay  = 3,
        .timeout = 100,
};


static struct platform_device touchkey_device = {
        .name       = "i2c-gpio",
        .id     = 5,
        .dev        = {
            .platform_data  = &touch_i2c_gpio_data,
        },
};

#endif	/*CONFIG_KEYBOARD_TC360_TOUCHKEY*/

#if defined (CONFIG_TOUCHSCREEN_IST30XX)
#define TSP_INT_GPIO_PIN   	(73)
static struct i2c_board_info __initdata zinitix_i2c_devices[] = {
	  {
		I2C_BOARD_INFO("sec_touch", 0x50),
		.irq = gpio_to_irq(TSP_INT_GPIO_PIN),
	  },
};
#endif

#ifdef CONFIG_TOUCHSCREEN_FT5306
static int ts_power(ts_power_status vreg_en)
{
	struct regulator *reg = NULL;
	if (!reg) {
/* Remove this comment when the regulator references are fixed here for Hawaii */
		reg = regulator_get(NULL, "hv8");
		if (!reg || IS_ERR(reg)) {
			pr_err("No Regulator available for ldo_hv8\n");
			return -1;
		}
	}
	if (reg) {
		if (vreg_en) {
			regulator_set_voltage(reg, 3000000, 3000000);
			pr_err("Turn on TP (ldo_hv8) to 2.8V\n");
			regulator_enable(reg);
		} else {
			pr_err("Turn off TP (ldo_hv8)\n");
			regulator_disable(reg);
		}
	} else {
		pr_err("TP Regulator Alloc Failed");
		return -1;
	}
	return 0;
}

static struct Synaptics_ts_platform_data ft5306_plat_data = {
	.power = ts_power,
};

static struct i2c_board_info __initdata ft5306_info[] = {
	{	/* New touch screen i2c slave address. */
		I2C_BOARD_INFO("FocalTech-Ft5306", (0x70>>1)),
		.platform_data = &ft5306_plat_data,
		.irq = gpio_to_irq(TSC_GPIO_IRQ_PIN),
	},
};
#endif

#if defined(CONFIG_TOUCHSCREEN_BCM915500) || defined(CONFIG_TOUCHSCREEN_BCM915500_MODULE)
static struct bcm915500_platform_data bcm915500_i2c_param = {
	.id = 3,
	.i2c_adapter_id = 3,
	.gpio_reset = TSC_GPIO_RESET_PIN,
	.gpio_interrupt = TSC_GPIO_IRQ_PIN,
};

static struct i2c_board_info bcm915500_i2c_boardinfo[] = {
	{
		.type = BCM915500_TSC_NAME,
		.addr = HW_BCM915500_SLAVE_SPM,
		.platform_data = &bcm915500_i2c_param,
		.irq = gpio_to_irq(TSC_GPIO_IRQ_PIN),
	},
};
#endif

#if defined(CONFIG_BCM_ALSA_SOUND)
static struct caph_platform_cfg board_caph_platform_cfg =
#ifdef HW_CFG_CAPH
HW_CFG_CAPH;
#else
{
	.aud_ctrl_plat_cfg = {
				.ext_aud_plat_cfg = {
					.ihf_ext_amp_gpio = -1,
					.dock_aud_route_gpio = -1,
					.mic_sel_aud_route_gpio = -1,
					}
				}
};
#endif

static struct platform_device board_caph_device = {
	.name = "brcm_caph_device",
	.id = -1, /*Indicates only one device */
	.dev = {
		.platform_data = &board_caph_platform_cfg,
	},
};

#endif /* CONFIG_BCM_ALSA_SOUND */

#if defined(CONFIG_USB_SWITCH_TSU6721)

enum cable_type_t{
        CABLE_TYPE_USB,
        CABLE_TYPE_AC,
        CABLE_TYPE_NONE
};


#ifdef CONFIG_HAS_WAKELOCK
static struct wake_lock tsu6721_jig_wakelock;
#endif
#ifdef CONFIG_KONA_PI_MGR
static struct pi_mgr_qos_node qos_node;
#endif

static void tsu6721_wakelock_init(void)
{
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_init(&tsu6721_jig_wakelock, WAKE_LOCK_SUSPEND, 
		"tsu6721_jig_wakelock");
#endif

#ifdef CONFIG_KONA_PI_MGR
	pi_mgr_qos_add_request(&qos_node, "tsu6721_jig_qos", 
		PI_MGR_PI_ID_ARM_SUB_SYSTEM, PI_MGR_QOS_DEFAULT_VALUE);
#endif
}
enum cable_type_t set_cable_status;
static void tsu6721_usb_cb(bool attached)
{
	enum bcmpmu_chrgr_type_t chrgr_type;
	enum bcmpmu_usb_type_t usb_type;

	#if defined(CONFIG_SEC_CHARGING_FEATURE)	
	int spa_data=POWER_SUPPLY_TYPE_BATTERY;
#endif
	pr_info("tsu6721_usb_cb attached %d\n", attached);

	set_cable_status = attached ? CABLE_TYPE_USB : CABLE_TYPE_NONE;

	switch (set_cable_status) {
	case CABLE_TYPE_USB:
			usb_type = PMU_USB_TYPE_SDP;
			chrgr_type = PMU_CHRGR_TYPE_SDP;
#if defined(CONFIG_SEC_CHARGING_FEATURE)	
		spa_data = POWER_SUPPLY_TYPE_USB_CDP;
#endif
			pr_info("%s USB attached\n",__func__);
			//send_usb_insert_event(BCMPMU_USB_EVENT_USB_DETECTION, &usb_type); //remove to fix enumeration failure issue
			break;
		case CABLE_TYPE_NONE:
			usb_type = PMU_USB_TYPE_NONE;
			chrgr_type = PMU_CHRGR_TYPE_NONE;
			spa_data = POWER_SUPPLY_TYPE_BATTERY;
			pr_info("%s USB removed\n",__func__);
			//set_usb_connection_status(&usb_type); // for unplug, we only set status, but not send event
			break;

	}
	send_chrgr_insert_event(BCMPMU_CHRGR_EVENT_CHGR_DETECTION,&chrgr_type);
#if defined(CONFIG_SEC_CHARGING_FEATURE)
	spa_event_handler(SPA_EVT_CHARGER, spa_data);
#endif	
}

#if defined (CONFIG_TOUCHSCREEN_IST30XX)
extern void ist30xx_set_ta_mode(bool charging);
#endif

static void tsu6721_charger_cb(bool attached)
{
	enum bcmpmu_chrgr_type_t chrgr_type;
	enum cable_type_t set_cable_status;
#if defined(CONFIG_SEC_CHARGING_FEATURE)	
	int spa_data=POWER_SUPPLY_TYPE_BATTERY;
#endif

	pr_info("tsu6721_charger_cb attached %d\n", attached);

	set_cable_status = attached ? CABLE_TYPE_AC : CABLE_TYPE_NONE;
	switch (set_cable_status) {
	case CABLE_TYPE_AC:
			chrgr_type = PMU_CHRGR_TYPE_DCP;
			pr_info("%s TA attached\n", __func__);
			#if defined (CONFIG_TOUCHSCREEN_IST30XX)
				ist30xx_set_ta_mode(1);
			#endif			
#if defined(CONFIG_SEC_CHARGING_FEATURE)	
		spa_data = POWER_SUPPLY_TYPE_USB_DCP;
#endif
			break;
		case CABLE_TYPE_NONE:
			chrgr_type = PMU_CHRGR_TYPE_NONE;
			pr_info("%s TA removed\n", __func__);
			#if defined (CONFIG_TOUCHSCREEN_IST30XX)
				ist30xx_set_ta_mode(0);
			#endif				
			break;
		default:
			break;
	}
	send_chrgr_insert_event(BCMPMU_CHRGR_EVENT_CHGR_DETECTION,
			&chrgr_type);
#if defined(CONFIG_SEC_CHARGING_FEATURE)
	spa_event_handler(SPA_EVT_CHARGER, spa_data);
#endif		
}

static void tsu6721_jig_cb(bool attached)
{
	pr_info("tsu6721_jig_cb attached %d\n", attached);

	if (attached) {
#ifndef CONFIG_SEC_MAKE_LCD_TEST		
#ifdef CONFIG_HAS_WAKELOCK
		if ( !wake_lock_active( &tsu6721_jig_wakelock ) )
			wake_lock( &tsu6721_jig_wakelock );
#endif
#ifdef CONFIG_KONA_PI_MGR
			pi_mgr_qos_request_update(&qos_node, 0);
#endif
#endif
	} else {
#ifndef CONFIG_SEC_MAKE_LCD_TEST	
#ifdef CONFIG_HAS_WAKELOCK
		if ( wake_lock_active( &tsu6721_jig_wakelock ) )
			wake_unlock( &tsu6721_jig_wakelock );
#endif
#ifdef CONFIG_KONA_PI_MGR
		pi_mgr_qos_request_update(&qos_node,
				PI_MGR_QOS_DEFAULT_VALUE);
#endif
#endif
	}
}
extern int musb_info_handler(struct notifier_block *nb, unsigned long event, void *para);
static void tsu6721_uart_cb(bool attached)
{
	pr_info("%s attached %d\n",__func__, attached);
	
		if (attached) {
#ifndef CONFIG_SEC_MAKE_LCD_TEST
#ifdef CONFIG_HAS_WAKELOCK
			if ( !wake_lock_active( &tsu6721_jig_wakelock ) )
			wake_lock( &tsu6721_jig_wakelock );
#endif
#ifdef CONFIG_KONA_PI_MGR
			pi_mgr_qos_request_update(&qos_node, 0);
#endif
#endif
			musb_info_handler(NULL, 0, 1);
	} else {
#ifndef CONFIG_SEC_MAKE_LCD_TEST	
#ifdef CONFIG_HAS_WAKELOCK
			if ( wake_lock_active( &tsu6721_jig_wakelock ) )
			wake_unlock( &tsu6721_jig_wakelock );
#endif
#ifdef CONFIG_KONA_PI_MGR
			pi_mgr_qos_request_update(&qos_node, 
				PI_MGR_QOS_DEFAULT_VALUE);
#endif
#endif
			musb_info_handler(NULL, 0, 0);
		}

}

void uas_jig_force_sleep(void)
{

#ifdef CONFIG_HAS_WAKELOCK
	if(wake_lock_active(&tsu6721_jig_wakelock))
	{
		wake_unlock(&tsu6721_jig_wakelock);
		pr_info("Force unlock jig_uart_wl\n");
	}
#else
	pr_info("Warning : %s - Empty function!!!\n", __func__);
#endif
#ifdef CONFIG_KONA_PI_MGR
	pi_mgr_qos_request_update(&qos_node, PI_MGR_QOS_DEFAULT_VALUE);
#endif
	return;
}

static struct tsu6721_platform_data tsu6721_pdata = {
        .usb_cb = tsu6721_usb_cb,
        .charger_cb = tsu6721_charger_cb,
        .jig_cb = tsu6721_jig_cb,
        .uart_cb = tsu6721_uart_cb,
};

static struct i2c_board_info  __initdata micro_usb_i2c_devices_info[]  = {
        {
                I2C_BOARD_INFO("tsu6721", 0x4A >> 1),
                .platform_data = &tsu6721_pdata,
                .irq = gpio_to_irq(GPIO_USB_INT),
        },
};

static struct i2c_gpio_platform_data mUSB_i2c_gpio_data={
        .sda_pin        = GPIO_USB_I2C_SDA,
                .scl_pin= GPIO_USB_I2C_SCL,
                .udelay                 = 2,
        };

static struct platform_device mUSB_i2c_gpio_device = {
        .name                   = "i2c-gpio",
        .id                     = TSU6721_I2C_BUS_ID,
        .dev                    ={
                .platform_data  = &mUSB_i2c_gpio_data,
        },
};

static struct platform_device *mUSB_i2c_devices[] __initdata = {
        &mUSB_i2c_gpio_device,
};

#endif

#ifdef CONFIG_BCM_SS_VIBRA
struct platform_device bcm_vibrator_device = {
	.name = "vibrator",
	.id = 0,
	.dev = {
		.platform_data="vibldo_uc",
	},
};
#endif



static struct platform_device *hawaii_devices[] __initdata = {
#ifdef CONFIG_KEYBOARD_BCM
	&hawaii_kp_device,
#endif
#ifdef CONFIG_KONA_HEADSET_MULTI_BUTTON
	&hawaii_headset_device,
#endif

#ifdef CONFIG_DMAC_PL330
	&hawaii_pl330_dmac_device,
#endif

#ifdef CONFIG_HAPTIC_SAMSUNG_PWM
	&haptic_pwm_device,
#endif

#if	defined(CONFIG_BACKLIGHT_PWM) || defined(CONFIG_BACKLIGHT_KTD259B)
	&hawaii_backlight_device,
#endif

#if (defined(CONFIG_BCM_RFKILL) || defined(CONFIG_BCM_RFKILL_MODULE))
	&hawaii_bcmbt_rfkill_device,
#endif

#ifdef CONFIG_BCM_BZHW
	&hawaii_bcm_bzhw_device,
#endif

#ifdef CONFIG_BCM_BT_LPM
	&board_bcmbt_lpm_device,
#endif

#ifdef CONFIG_BYPASS_WIFI_DEVTREE
	&board_wifi_driver_device,
#endif

#ifdef CONFIG_VIDEO_KONA
	&hawaii_unicam_device,
#endif

#ifdef CONFIG_WD_TAPPER
	&wd_tapper,
#endif

#if defined(CONFIG_BCM_ALSA_SOUND)
	        &board_caph_device,
#endif

#if defined(CONFIG_KEYBOARD_TC360_TOUCHKEY)
	&touchkey_device,
#endif

#if defined(CONFIG_TOUCHKEY_BACKLIGHT)
        &touchkeyled_device
#endif        
};

struct platform_device *hawaii_sdio_devices[] __initdata = {
        &hawaii_sdio2_device,
        &hawaii_sdio3_device,
        &hawaii_sdio1_device,
};

#if defined (CONFIG_TOUCHSCREEN_ATMEL_MXT224S)
extern void mxt_init(void);
#endif

static void __init hawaii_add_i2c_devices(void)
{

#ifdef CONFIG_VIDEO_ADP1653
	i2c_register_board_info(0, adp1653_flash, ARRAY_SIZE(adp1653_flash));
#endif
#ifdef CONFIG_TOUCHSCREEN_TANGO
	i2c_register_board_info(3, tango_info, ARRAY_SIZE(tango_info));
#endif
#if defined (CONFIG_TOUCHSCREEN_IST30XX)
	i2c_register_board_info(3, zinitix_i2c_devices, ARRAY_SIZE(zinitix_i2c_devices));
#endif

#if defined (CONFIG_TOUCHSCREEN_ATMEL_MXT224S)
	mxt_init();
#endif

#ifdef CONFIG_TOUCHSCREEN_FT5306
	i2c_register_board_info(3, ft5306_info, ARRAY_SIZE(ft5306_info));
#endif

#if defined(CONFIG_TOUCHSCREEN_BCM915500) || defined(CONFIG_TOUCHSCREEN_BCM915500_MODULE)
	i2c_register_board_info(3, bcm915500_i2c_boardinfo,
				ARRAY_SIZE(bcm915500_i2c_boardinfo));
#endif

#if defined(CONFIG_INPUT_MPU6050)
	i2c_register_board_info(2, bsc3_i2c_boardinfo, ARRAY_SIZE(bsc3_i2c_boardinfo));
#endif
#if defined(CONFIG_USB_SWITCH_TSU6721)
        pr_info("tsu6721\n");
#if defined( CONFIG_HAS_WAKELOCK ) || defined( CONFIG_KONA_PI_MGR )
		tsu6721_wakelock_init();
#endif
        i2c_register_board_info(TSU6721_I2C_BUS_ID, micro_usb_i2c_devices_info,ARRAY_SIZE(micro_usb_i2c_devices_info));
#endif

#if defined(CONFIG_USB_SWITCH_TSU6721)
        pr_info("tsu6721 mUSB_i2c_devices\n");
        platform_add_devices(mUSB_i2c_devices, ARRAY_SIZE(mUSB_i2c_devices));
#endif

#if defined(CONFIG_BCM2079X_NFC_I2C)
	pr_info("add i2c_devs_nfc\n");
	i2c_register_board_info(BCM_NFC_BUSID, i2c_devs_nfc, ARRAY_SIZE(i2c_devs_nfc));
#endif
#if  defined(CONFIG_BMP18X) || defined(CONFIG_BMP18X_I2C) || defined(CONFIG_BMP18X_I2C_MODULE)
	i2c_register_board_info(
#ifdef BMP18X_I2C_BUS_ID
			BMP18X_I2C_BUS_ID,
#else
			-1,
#endif
			i2c_bmp18x_info, ARRAY_SIZE(i2c_bmp18x_info));
#endif


#if defined(CONFIG_AL3006) || defined(CONFIG_AL3006_MODULE)
#ifdef AL3006_IRQ
	i2c_al3006_info[0].irq = gpio_to_irq(AL3006_IRQ_GPIO);
#else
	i2c_al3006_info[0].irq = -1;
#endif
	i2c_register_board_info(
#ifdef AL3006_I2C_BUS_ID
		AL3006_I2C_BUS_ID,
#else
		-1,
#endif
		i2c_al3006_info, ARRAY_SIZE(i2c_al3006_info));
#endif /* CONFIG_AL3006 */

#if defined(CONFIG_KEYBOARD_TC360_TOUCHKEY)
	i2c_register_board_info(5, touchkey_gpio_i2c_devices, ARRAY_SIZE(touchkey_gpio_i2c_devices));
#endif


#if defined(CONFIG_SENSORS_GP2AP002)   
	i2c_register_board_info(7, sensor_gpio_i2c_devices, ARRAY_SIZE(sensor_gpio_i2c_devices));
        platform_add_devices(sensor_i2c_devices, ARRAY_SIZE(sensor_i2c_devices));
#endif


}

static void hawaii_add_pdata(void)
{
	hawaii_serial_device.dev.platform_data = &hawaii_uart_platform_data;
	hawaii_i2c_adap_devices[0].dev.platform_data = &bsc_i2c_cfg[0];
	hawaii_i2c_adap_devices[1].dev.platform_data = &bsc_i2c_cfg[1];
	hawaii_i2c_adap_devices[2].dev.platform_data = &bsc_i2c_cfg[2];
	hawaii_i2c_adap_devices[3].dev.platform_data = &bsc_i2c_cfg[3];
	hawaii_i2c_adap_devices[4].dev.platform_data = &bsc_i2c_cfg[4];
	hawaii_sdio1_device.dev.platform_data = &hawaii_sdio_param[0];
	hawaii_sdio2_device.dev.platform_data = &hawaii_sdio_param[1];
	hawaii_sdio3_device.dev.platform_data = &hawaii_sdio_param[2];
	hawaii_ssp0_device.dev.platform_data = &hawaii_ssp0_info;
	hawaii_ssp1_device.dev.platform_data = &hawaii_ssp1_info;
	hawaii_stm_device.dev.platform_data = &hawaii_stm_pdata;
	hawaii_headset_device.dev.platform_data = &hawaii_headset_data;
	hawaii_pl330_dmac_device.dev.platform_data = &hawaii_pl330_pdata;
#ifdef 	CONFIG_BACKLIGHT_PWM
	hawaii_backlight_device.dev.platform_data = &hawaii_backlight_data;
#endif

#ifdef CONFIG_USB_DWC_OTG
	hawaii_hsotgctrl_platform_device.dev.platform_data = &hsotgctrl_plat_data;
	hawaii_usb_phy_platform_device.dev.platform_data =	&hsotgctrl_plat_data;
#endif
}

void __init hawaii_add_common_devices(void)
{
#ifdef CONFIG_ANDROID_PMEM
	platform_device_register(&android_pmem);
#endif

	platform_add_devices(hawaii_common_plat_devices,
			ARRAY_SIZE(hawaii_common_plat_devices));
}

static void __init hawaii_add_devices(void)
{

	hawaii_add_pdata();

#ifdef CONFIG_ION_BCM_NO_DT
	platform_device_register(&ion_carveout_device);
#ifdef CONFIG_CMA
	platform_device_register(&ion_cma_device);
#endif
#endif

#ifdef CONFIG_KEYBOARD_BCM
	hawaii_kp_device.dev.platform_data = &hawaii_keypad_data;
#endif

#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
        platform_device_register(&board_gpio_keys_device);
#endif

	platform_add_devices(hawaii_devices, ARRAY_SIZE(hawaii_devices));

	hawaii_add_i2c_devices();
	spi_register_board_info(spi_slave_board_info,
				ARRAY_SIZE(spi_slave_board_info));
#ifdef CONFIG_BCM_SS_VIBRA
	platform_device_register( &bcm_vibrator_device);
#endif

}
#ifdef CONFIG_MOBICORE_OS
static void hawaii_mem_reserve(void)
{
	mobicore_device.dev.platform_data = &mobicore_plat_data;
	hawaii_reserve();
}
#endif

static void __init hawaii_add_sdio_devices(void)
{
        platform_add_devices(hawaii_sdio_devices,
                                ARRAY_SIZE(hawaii_sdio_devices));
}

#ifdef CONFIG_FB_BRCM_KONA
/*
 * KONA FRAME BUFFER DISPLAY DRIVER PLATFORM CONFIG
 */
struct kona_fb_platform_data konafb_devices[] __initdata = {
	{
		.name = "S6E63M0X3",
		.reg_name = "cam2",
		.rst =  {
			.gpio = 22,
			.setup = 5,
			.pulse = 20,
			.hold = 10000,
			.active = false,
		},
		.vmode = true,
		.vburst = true,
		.cmnd_LP = true,
		.te_ctrl = false,
		.col_mod_i = 3,  //DISPDRV_FB_FORMAT_xBGR8888
		.col_mod_o = 2, //DISPDRV_FB_FORMAT_xRGB8888
		.width = 480,
		.height = 800,
		.fps = 60,
		.lanes = 2,
		.hs_bps = 350000000,
		.lp_bps = 5000000,
#ifdef CONFIG_IOMMU_API
		.pdev_iommu = &iovmm_mm_device,
#endif
#ifdef CONFIG_BCM_IOVMM
		.pdev_iovmm = &iovmm_mm_256mb_device,
#endif
	},
};

#include "kona_fb_init.c"
#endif /* #ifdef CONFIG_FB_BRCM_KONA */


static void __init hawaii_init(void)
{
	hawaii_add_devices();
#ifdef CONFIG_FB_BRCM_KONA
	konafb_init();
#endif
	hawaii_add_common_devices();
	return;
}

static int __init hawaii_add_lateinit_devices(void)
{
        hawaii_add_sdio_devices();

#ifdef CONFIG_BRCM_UNIFIED_DHD_SUPPORT
	hawaii_wlan_init();
#endif

	return 0;
}

late_initcall(hawaii_add_lateinit_devices);

MACHINE_START(HAWAII, "hawaii_ss_codinan")
	.atag_offset = 0x100,
	.map_io = hawaii_map_io,
	.init_irq = kona_init_irq,
	.handle_irq = gic_handle_irq,
	.timer = &kona_timer,
	.init_machine = hawaii_init,
#ifdef CONFIG_MOBICORE_OS
	.reserve = hawaii_mem_reserve,
#else
	.reserve = hawaii_reserve,
#endif
	.restart = hawaii_restart,
MACHINE_END
