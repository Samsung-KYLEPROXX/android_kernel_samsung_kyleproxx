/*****************************************************************************
*  Copyright 2001 - 2011 Broadcom Corporation.  All rights reserved.
*
*  Unless you and Broadcom execute a separate written software license
*  agreement governing use of this software, this software is licensed to you
*  under the terms of the GNU General Public License version 2, available at
*  http://www.gnu.org/licenses/old-license/gpl-2.0.html (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a
*  license other than the GPL, without Broadcom's express prior written
*  consent.
*
*****************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/io.h>
#include <mach/rdb/brcm_rdb_hsotg_ctrl.h>
#include <mach/rdb/brcm_rdb_khub_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_chipreg.h>
#include <plat/pi_mgr.h>
#include <plat/clock.h>
#include <linux/usb/bcm_hsotgctrl.h>
#include <linux/usb/bcm_hsotgctrl_phy_mdio.h>

#define	PHY_MODE_OTG		2
#define BCCFG_SW_OVERWRITE_KEY 0x55560000
#define	BC_CONFIG_DELAY_MS 2
#define	PHY_PLL_DELAY_MS	2

#define USB_PHY_MDIO_ID 9
#define USB_PHY_MDIO0 0
#define USB_PHY_MDIO1 1
#define USB_PHY_MDIO2 2
#define USB_PHY_MDIO3 3
#define MDIO_ACCESS_KEY 0x00A5A501
#define PHY_MDIO_DELAY_IN_USECS 10
#define PHY_MDIO_CURR_REF_ADJUST_VALUE 0x18
#define PHY_MDIO_LDO_REF_VOLTAGE_ADJUST_VALUE 0x80

#define HSOTGCTRL_STEP_DELAY_IN_MS 2
#define HSOTGCTRL_ID_CHANGE_DELAY_IN_MS 200
#define PHY_PM_DELAY_IN_MS 1

struct bcm_hsotgctrl_drv_data {
	struct device *dev;
	struct clk *otg_clk;
	struct clk *mdio_master_clk;
	void *hsotg_ctrl_base;
	void *chipregs_base;
	struct workqueue_struct *bcm_hsotgctrl_work_queue;
	struct delayed_work wakeup_work;
	int hsotgctrl_irq;
	bool irq_enabled;
	bool allow_suspend;
	bool usb_active;
};

static struct bcm_hsotgctrl_drv_data *local_hsotgctrl_handle;
static send_core_event_cb_t local_wakeup_core_cb;

static ssize_t dump_hsotgctrl(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct bcm_hsotgctrl_drv_data *hsotgctrl_drvdata = dev_get_drvdata(dev);
	void __iomem *hsotg_ctrl_base = hsotgctrl_drvdata->hsotg_ctrl_base;
	int clk_cnt = clk_get_usage(hsotgctrl_drvdata->otg_clk);

	/* This could be done after USB is unplugged
	 * Turn on AHB clock so registers
	 * can be read even when USB is unplugged
	 */
	if (!clk_cnt)
		bcm_hsotgctrl_en_clock(true);

	pr_info("\nusbotgcontrol: 0x%08X",
		readl(hsotg_ctrl_base +
		    HSOTG_CTRL_USBOTGCONTROL_OFFSET));
	pr_info("\nphy_cfg: 0x%08X",
		readl(hsotg_ctrl_base +
		    HSOTG_CTRL_PHY_CFG_OFFSET));
	pr_info("\nphy_p1ctl: 0x%08X",
		readl(hsotg_ctrl_base +
		    HSOTG_CTRL_PHY_P1CTL_OFFSET));
	pr_info("\nbc11_status: 0x%08X",
		readl(hsotg_ctrl_base +
		    HSOTG_CTRL_BC_STATUS_OFFSET));
	pr_info("\nbc11_cfg: 0x%08X",
		readl(hsotg_ctrl_base +
		    HSOTG_CTRL_BC_CFG_OFFSET));
	pr_info("\ntp_in: 0x%08X",
		readl(hsotg_ctrl_base +
		    HSOTG_CTRL_TP_IN_OFFSET));
	pr_info("\ntp_out: 0x%08X",
		readl(hsotg_ctrl_base +
		    HSOTG_CTRL_TP_OUT_OFFSET));
	pr_info("\nphy_ctrl: 0x%08X",
		readl(hsotg_ctrl_base +
		    HSOTG_CTRL_PHY_CTRL_OFFSET));
	pr_info("\nusbreg: 0x%08X",
		readl(hsotg_ctrl_base +
		    HSOTG_CTRL_USBREG_OFFSET));
	pr_info("\nusbproben: 0x%08X",
		readl(hsotg_ctrl_base +
		    HSOTG_CTRL_USBPROBEN_OFFSET));

	/* We turned on the clock so turn it off */
	if (!clk_cnt)
		bcm_hsotgctrl_en_clock(false);

	return sprintf(buf, "hsotgctrl register dump\n");
}
static DEVICE_ATTR(hsotgctrldump, S_IRUGO, dump_hsotgctrl, NULL);


int bcm_hsotgctrl_en_clock(bool on)
{
	int rc = 0;
	struct bcm_hsotgctrl_drv_data *bcm_hsotgctrl_handle =
		local_hsotgctrl_handle;

	if (!bcm_hsotgctrl_handle || !bcm_hsotgctrl_handle->otg_clk)
		return -EIO;

	if (on) {
		bcm_hsotgctrl_handle->allow_suspend = false;
		pr_err("hsotgctrl_clk=on, clk_usage=%d\n",
			clk_get_usage(bcm_hsotgctrl_handle->otg_clk));
		if (!clk_get_usage(bcm_hsotgctrl_handle->otg_clk))
			rc = clk_enable(bcm_hsotgctrl_handle->otg_clk);
	} else {
		pr_err("hsotgctrl_clk=off, clk_usage=%d usb_active:%d\n",
			clk_get_usage(bcm_hsotgctrl_handle->otg_clk),
			bcm_hsotgctrl_handle->usb_active);
		/*
		* Only allow the clock to be shut off by the
		* PMU if usb is not active
		*/
		if (!bcm_hsotgctrl_handle->usb_active) {
			clk_disable(bcm_hsotgctrl_handle->otg_clk);
			bcm_hsotgctrl_handle->allow_suspend = true;
		}
	}

	if (rc)
		dev_warn(bcm_hsotgctrl_handle->dev,
		  "%s: error in controlling clock\n", __func__);

	return rc;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_en_clock);

int bcm_hsotgctrl_phy_init(bool id_device)
{
	int val;
	int rc=0;
	struct bcm_hsotgctrl_drv_data *bcm_hsotgctrl_handle =
		local_hsotgctrl_handle;

	if (NULL == local_hsotgctrl_handle)
		return -ENODEV;

	if ((!bcm_hsotgctrl_handle->otg_clk) ||
		  (!bcm_hsotgctrl_handle->hsotg_ctrl_base) ||
		  (!bcm_hsotgctrl_handle->dev))
		return -EIO;

	bcm_hsotgctrl_handle->usb_active = true;
	rc = bcm_hsotgctrl_en_clock(true);
	if(rc){
		printk("%s-Fail to Enable CLK\n",__func__);
		return -EBUSY;
	}
	
	
	mdelay(HSOTGCTRL_STEP_DELAY_IN_MS);
	/* clear bit 15 RDB error */
	val = readl(bcm_hsotgctrl_handle->hsotg_ctrl_base +
		HSOTG_CTRL_PHY_P1CTL_OFFSET);
	val &= ~HSOTG_CTRL_PHY_P1CTL_USB11_OEB_IS_TXEB_MASK;
	writel(val, bcm_hsotgctrl_handle->hsotg_ctrl_base +
			HSOTG_CTRL_PHY_P1CTL_OFFSET);

	mdelay(HSOTGCTRL_STEP_DELAY_IN_MS);

	/* Enable software control of PHY-PM */
	bcm_hsotgctrl_set_soft_ldo_pwrdn(true);

	/* Put PHY in reset state */
	bcm_hsotgctrl_set_phy_resetb(false);

	/* Reset PHY and AHB clock domain */
	bcm_hsotgctrl_reset_clk_domain();

	/* Power up ALDO */
	bcm_hsotgctrl_set_aldo_pdn(true);
	mdelay(PHY_PM_DELAY_IN_MS);

	/* Enable pad, internal PLL etc */
	bcm_hsotgctrl_set_phy_off(false);

	bcm_hsotgctrl_set_ldo_suspend_mask();

	/* Remove PHY isolation */
	bcm_hsotgctrl_set_phy_iso(false);
	mdelay(PHY_PM_DELAY_IN_MS);

	/* PHY clock request */
	bcm_hsotgctrl_set_phy_clk_request(true);
	mdelay(PHY_PLL_DELAY_MS);

	/* Bring Put PHY out of reset state */
	bcm_hsotgctrl_set_phy_resetb(true);

	/* Don't disable software control of PHY-PM
	 * We want to control the PHY LDOs from software
	 */
	bcm_hsotgctrl_phy_mdio_initialization();

	if (id_device) {
		/* Set correct ID value */
		bcm_hsotgctrl_phy_set_id_stat(true);

		/* Set Vbus valid state */
		bcm_hsotgctrl_phy_set_vbus_stat(true);
	} else {
		/* Set correct ID value */
		bcm_hsotgctrl_phy_set_id_stat(false);
		/* Clear non-driving */
		bcm_hsotgctrl_phy_set_non_driving(false);
	}

	msleep(HSOTGCTRL_ID_CHANGE_DELAY_IN_MS);

	bcm_hsotgctrl_bc_enable_sw_ovwr();
	
	return 0;

}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_phy_init);

int bcm_hsotgctrl_phy_deinit(void)
{
	struct bcm_hsotgctrl_drv_data *bcm_hsotgctrl_handle =
		local_hsotgctrl_handle;

	if (NULL == local_hsotgctrl_handle)
		return -ENODEV;

	if ((!bcm_hsotgctrl_handle->otg_clk) || (!bcm_hsotgctrl_handle->dev))
		return -EIO;

	if (bcm_hsotgctrl_handle->irq_enabled) {

		/* We are shutting down USB so ensure wake IRQ
		 * is disabled
		 */
		disable_irq(bcm_hsotgctrl_handle->hsotgctrl_irq);
		bcm_hsotgctrl_handle->irq_enabled = false;

	}

	if (work_pending(&bcm_hsotgctrl_handle->wakeup_work.work)) {

		/* Cancel scheduled work */
		cancel_delayed_work(&bcm_hsotgctrl_handle->
			wakeup_work);

		/* Make sure work queue is flushed */
		flush_workqueue(bcm_hsotgctrl_handle->
			bcm_hsotgctrl_work_queue);

	}

	/* Disable wakeup condition */
	bcm_hsotgctrl_phy_wakeup_condition(false);

	/* Stay disconnected */
	bcm_hsotgctrl_wakeup_core();
	bcm_hsotgctrl_phy_set_non_driving(true);

	/* Disable pad, internal PLL etc. */
	bcm_hsotgctrl_set_phy_off(true);

	/* Enable software control of PHY-PM */
	bcm_hsotgctrl_set_soft_ldo_pwrdn(true);

	/* Isolate PHY */
	bcm_hsotgctrl_set_phy_iso(true);

	/* Power down ALDO */
	bcm_hsotgctrl_set_aldo_pdn(false);

	/* Clear PHY reference clock request */
	bcm_hsotgctrl_set_phy_clk_request(false);

	/* Clear Vbus valid state */
	bcm_hsotgctrl_phy_set_vbus_stat(false);

	/* Disable the OTG core AHB clock */
	bcm_hsotgctrl_handle->usb_active = false;
	bcm_hsotgctrl_en_clock(false);
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_phy_deinit);

int bcm_hsotgctrl_bc_reset(void)
{
	int val;
	struct bcm_hsotgctrl_drv_data *bcm_hsotgctrl_handle =
		local_hsotgctrl_handle;
	int clk_cnt ;

	if (NULL == local_hsotgctrl_handle)
		return -ENODEV;

	if ((!bcm_hsotgctrl_handle->otg_clk) ||
		  (!bcm_hsotgctrl_handle->dev))
		return -EIO;

	clk_cnt = clk_get_usage(bcm_hsotgctrl_handle->otg_clk);
	if (!clk_cnt)
		bcm_hsotgctrl_en_clock(true);

	val = readl(bcm_hsotgctrl_handle->hsotg_ctrl_base +
			HSOTG_CTRL_BC_CFG_OFFSET);

	/* Clear overwrite key */
	val &= ~(HSOTG_CTRL_BC_CFG_BC_OVWR_KEY_MASK |
		HSOTG_CTRL_BC_CFG_SW_OVWR_EN_MASK);
	/*We need this key written for this register access*/
	val |= (BCCFG_SW_OVERWRITE_KEY |
			HSOTG_CTRL_BC_CFG_SW_OVWR_EN_MASK);
	val |= HSOTG_CTRL_BC_CFG_SW_RST_MASK;

	/*Reset BC1.1 state machine */
	writel(val, bcm_hsotgctrl_handle->hsotg_ctrl_base +
			HSOTG_CTRL_BC_CFG_OFFSET);

	msleep_interruptible(BC_CONFIG_DELAY_MS);

	val &= ~HSOTG_CTRL_BC_CFG_SW_RST_MASK;
	writel(val, bcm_hsotgctrl_handle->hsotg_ctrl_base +
			HSOTG_CTRL_BC_CFG_OFFSET); /*Clear reset*/

	val = readl(bcm_hsotgctrl_handle->hsotg_ctrl_base +
			HSOTG_CTRL_BC_CFG_OFFSET);

	/* Clear overwrite key so we don't accidently write to these bits */
	val &= ~(HSOTG_CTRL_BC_CFG_BC_OVWR_KEY_MASK |
			HSOTG_CTRL_BC_CFG_SW_OVWR_EN_MASK);
	writel(val, bcm_hsotgctrl_handle->hsotg_ctrl_base +
			HSOTG_CTRL_BC_CFG_OFFSET);

	val = readl(bcm_hsotgctrl_handle->hsotg_ctrl_base +
			HSOTG_CTRL_BC_CFG_OFFSET);
	if (!clk_cnt)
		bcm_hsotgctrl_en_clock(false);
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_bc_reset);

int bcm_hsotgctrl_bc_enable_sw_ovwr(void)
{
	int val;
	struct bcm_hsotgctrl_drv_data *bcm_hsotgctrl_handle =
		local_hsotgctrl_handle;
	int clk_cnt = clk_get_usage(bcm_hsotgctrl_handle->otg_clk);

	if (NULL == local_hsotgctrl_handle) {
		dev_warn(bcm_hsotgctrl_handle->dev,
		  "%s: error invalid handle\n", __func__);
		return -ENODEV;
	}
	if (!clk_cnt)
		bcm_hsotgctrl_en_clock(true);

	val = readl(bcm_hsotgctrl_handle->hsotg_ctrl_base +
			HSOTG_CTRL_BC_CFG_OFFSET);

	/* Clear overwrite key */
	val &= ~(HSOTG_CTRL_BC_CFG_BC_OVWR_KEY_MASK |
		HSOTG_CTRL_BC_CFG_SW_OVWR_EN_MASK);

	/*We need this key written for this register access*/
	val |= (BCCFG_SW_OVERWRITE_KEY |
			HSOTG_CTRL_BC_CFG_SW_OVWR_EN_MASK);

	/*Enable SW overwrite*/
	writel(val, bcm_hsotgctrl_handle->hsotg_ctrl_base +
			HSOTG_CTRL_BC_CFG_OFFSET);

	msleep_interruptible(BC_CONFIG_DELAY_MS);

	if (!clk_cnt)
		bcm_hsotgctrl_en_clock(false);
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_bc_enable_sw_ovwr);


int bcm_hsotgctrl_bc_status(unsigned long *status)
{
	unsigned int val;
	int clk_cnt;
	struct bcm_hsotgctrl_drv_data *bcm_hsotgctrl_handle =
		local_hsotgctrl_handle;

	if (NULL == local_hsotgctrl_handle)
		return -ENODEV;

	if ((!bcm_hsotgctrl_handle->otg_clk) ||
		  (!bcm_hsotgctrl_handle->dev) || !status)
		return -EIO;

	clk_cnt = clk_get_usage(bcm_hsotgctrl_handle->otg_clk);
	dev_info(bcm_hsotgctrl_handle->dev, "clk cnt %d\n", clk_cnt);
	val = readl(bcm_hsotgctrl_handle->hsotg_ctrl_base +
			HSOTG_CTRL_BC_STATUS_OFFSET);
	*status = val;

	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_bc_status);

int bcm_hsotgctrl_bc_vdp_src_off(void)
{
	int val;
	struct bcm_hsotgctrl_drv_data *bcm_hsotgctrl_handle =
		local_hsotgctrl_handle;

	if (NULL == local_hsotgctrl_handle)
		return -ENODEV;

	if ((!bcm_hsotgctrl_handle->otg_clk) ||
		  (!bcm_hsotgctrl_handle->dev))
		return -EIO;

	val = readl(bcm_hsotgctrl_handle->hsotg_ctrl_base +
			HSOTG_CTRL_BC_CFG_OFFSET);

	/* Clear overwrite key */
	val &= ~(HSOTG_CTRL_BC_CFG_BC_OVWR_KEY_MASK |
			HSOTG_CTRL_BC_CFG_SW_OVWR_EN_MASK);
	/*We need this key written for this register access */
	val |= (BCCFG_SW_OVERWRITE_KEY |
			HSOTG_CTRL_BC_CFG_SW_OVWR_EN_MASK);
	val &= ~HSOTG_CTRL_BC_CFG_BC_OVWR_SET_P0_MASK;

	/*Reset BC1.1 state machine */
	writel(val, bcm_hsotgctrl_handle->hsotg_ctrl_base +
			HSOTG_CTRL_BC_CFG_OFFSET);

	msleep_interruptible(BC_CONFIG_DELAY_MS);

	/* Clear overwrite key so we don't accidently write
	 * to these bits
	 */
	val &= ~(HSOTG_CTRL_BC_CFG_BC_OVWR_KEY_MASK |
			HSOTG_CTRL_BC_CFG_SW_OVWR_EN_MASK);

	writel(val, bcm_hsotgctrl_handle->hsotg_ctrl_base +
		HSOTG_CTRL_BC_CFG_OFFSET);

	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_bc_vdp_src_off);

void bcm_hsotgctrl_wakeup_core(void)
{
	struct bcm_hsotgctrl_drv_data *bcm_hsotgctrl_handle =
		local_hsotgctrl_handle;

	if (NULL == local_hsotgctrl_handle)
		return;

	bcm_hsotgctrl_handle->usb_active = true;

	/* Enable OTG AHB clock */
	bcm_hsotgctrl_en_clock(true);

	/* Disable wakeup interrupt */
	bcm_hsotgctrl_phy_wakeup_condition(false);

	/* Enable software control of PHY-PM */
	bcm_hsotgctrl_set_soft_ldo_pwrdn(true);

	/* PHY isolation */
	bcm_hsotgctrl_set_phy_iso(true);

	/* Power up ALDO */
	bcm_hsotgctrl_set_aldo_pdn(true);
	mdelay(PHY_PM_DELAY_IN_MS);

	/* Put PHY in reset state */
	bcm_hsotgctrl_set_phy_resetb(false);
	mdelay(PHY_PM_DELAY_IN_MS);

	/* De-assert PHY reset */
	bcm_hsotgctrl_set_phy_resetb(true);

	/* Remove PHY isolation */
	bcm_hsotgctrl_set_phy_iso(false);
	mdelay(PHY_PM_DELAY_IN_MS);

	/* Request PHY clock */
	bcm_hsotgctrl_set_phy_clk_request(true);
	mdelay(PHY_PM_DELAY_IN_MS);

	/* Do MDIO init values after PHY is up */
	bcm_hsotgctrl_phy_mdio_initialization();

	if (local_wakeup_core_cb) {
		local_wakeup_core_cb();
		local_wakeup_core_cb = NULL;
	}

}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_wakeup_core);

static void bcm_hsotgctrl_delayed_wakeup_handler(struct work_struct *work)
{
	struct bcm_hsotgctrl_drv_data *bcm_hsotgctrl_handle =
		container_of(work, struct bcm_hsotgctrl_drv_data,
			 wakeup_work.work);

	if (NULL == local_hsotgctrl_handle)
		return;

	if (bcm_hsotgctrl_handle !=	local_hsotgctrl_handle) {
		dev_warn(local_hsotgctrl_handle->dev,
			"Invalid HSOTGCTRL wakeup handler");
		return;
	}

	dev_info(bcm_hsotgctrl_handle->dev, "Do HSOTGCTRL wakeup\n");

	/* Use the PHY-core wakeup sequence */
	bcm_hsotgctrl_wakeup_core();
}

static irqreturn_t bcm_hsotgctrl_wake_irq(int irq, void *dev)
{
	struct bcm_hsotgctrl_drv_data *bcm_hsotgctrl_handle =
		local_hsotgctrl_handle;

	if (NULL == local_hsotgctrl_handle)
		return -ENODEV;

	if ((!bcm_hsotgctrl_handle->otg_clk) || (!bcm_hsotgctrl_handle->dev))
		return IRQ_NONE;

	/* Disable the IRQ since already waking up */
	disable_irq_nosync(bcm_hsotgctrl_handle->hsotgctrl_irq);
	bcm_hsotgctrl_handle->irq_enabled = false;
	schedule_delayed_work(&bcm_hsotgctrl_handle->wakeup_work,
	  msecs_to_jiffies(BCM_HSOTGCTRL_WAKEUP_PROCESSING_DELAY));

	return IRQ_HANDLED;
}

int bcm_hsotgctrl_get_clk_count(void)
{
	struct bcm_hsotgctrl_drv_data *bcm_hsotgctrl_handle =
		local_hsotgctrl_handle;

	if (NULL == local_hsotgctrl_handle)
		return -ENODEV;

	if ((!bcm_hsotgctrl_handle->otg_clk) ||
		  (!bcm_hsotgctrl_handle->dev))
		return -EIO;

	return clk_get_usage(bcm_hsotgctrl_handle->otg_clk);
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_get_clk_count);

int bcm_hsotgctrl_handle_bus_suspend(send_core_event_cb_t suspend_core_cb,
		send_core_event_cb_t wakeup_core_cb)
{
	struct bcm_hsotgctrl_drv_data *bcm_hsotgctrl_handle =
		local_hsotgctrl_handle;

	if (NULL == local_hsotgctrl_handle)
		return -ENODEV;

	if ((!bcm_hsotgctrl_handle->otg_clk) ||
		  (!bcm_hsotgctrl_handle->dev))
		return -EIO;

	if (suspend_core_cb)
		suspend_core_cb();
	if (wakeup_core_cb)
		local_wakeup_core_cb = wakeup_core_cb;

	/* Enable software control of PHY-PM */
	bcm_hsotgctrl_set_soft_ldo_pwrdn(true);

	/* PHY isolation */
	bcm_hsotgctrl_set_phy_iso(true);
	mdelay(PHY_PM_DELAY_IN_MS);

	/* Clear PHY clock request */
	bcm_hsotgctrl_set_phy_clk_request(true);

	/* Power down ALDO */
	bcm_hsotgctrl_set_aldo_pdn(false);

	/* Enable wakeup interrupt */
	bcm_hsotgctrl_phy_wakeup_condition(true);

	/* Disable OTG AHB clock */
	bcm_hsotgctrl_handle->usb_active = false;
	bcm_hsotgctrl_en_clock(false);

	if (bcm_hsotgctrl_handle->irq_enabled == false) {
		/* Enable wake IRQ */
		bcm_hsotgctrl_handle->irq_enabled = true;
		enable_irq(bcm_hsotgctrl_handle->hsotgctrl_irq);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_handle_bus_suspend);

static int __devinit bcm_hsotgctrl_probe(struct platform_device *pdev)
{
	int error = 0;
	unsigned int val;
	struct bcm_hsotgctrl_drv_data *hsotgctrl_drvdata;
	struct bcm_hsotgctrl_platform_data *plat_data =
	  (struct bcm_hsotgctrl_platform_data *)pdev->dev.platform_data;

	if (plat_data == NULL) {
		dev_err(&pdev->dev, "platform_data failed\n");
		return -ENODEV;
	}

	hsotgctrl_drvdata = kzalloc(sizeof(*hsotgctrl_drvdata), GFP_KERNEL);
	if (!hsotgctrl_drvdata) {
		dev_warn(&pdev->dev, "Memory allocation failed\n");
		return -ENOMEM;
	}

	local_hsotgctrl_handle = hsotgctrl_drvdata;

	hsotgctrl_drvdata->hsotg_ctrl_base =
		(void *)plat_data->hsotgctrl_virtual_mem_base;
	if (!hsotgctrl_drvdata->hsotg_ctrl_base) {
		dev_warn(&pdev->dev, "No vaddr for HSOTGCTRL!\n");
		kfree(hsotgctrl_drvdata);
		return -ENOMEM;
	}

	hsotgctrl_drvdata->chipregs_base =
		(void *)plat_data->chipreg_virtual_mem_base;
	if (!hsotgctrl_drvdata->chipregs_base) {
		dev_warn(&pdev->dev, "No vaddr for CHIPREG!\n");
		kfree(hsotgctrl_drvdata);
		return -ENOMEM;
	}

	hsotgctrl_drvdata->dev = &pdev->dev;
	hsotgctrl_drvdata->otg_clk = clk_get(NULL,
		plat_data->usb_ahb_clk_name);

	if (IS_ERR(hsotgctrl_drvdata->otg_clk)) {
		error = PTR_ERR(hsotgctrl_drvdata->otg_clk);
		dev_warn(&pdev->dev, "OTG clock allocation failed\n");
		kfree(hsotgctrl_drvdata);
		return error;
	}

	hsotgctrl_drvdata->mdio_master_clk = clk_get(NULL,
		plat_data->mdio_mstr_clk_name);

	if (IS_ERR(hsotgctrl_drvdata->mdio_master_clk)) {
		error = PTR_ERR(hsotgctrl_drvdata->mdio_master_clk);
		dev_warn(&pdev->dev, "MDIO Mst clk alloc failed\n");
		kfree(hsotgctrl_drvdata);
		return error;
	}

	hsotgctrl_drvdata->allow_suspend = true;
	platform_set_drvdata(pdev, hsotgctrl_drvdata);

	/* Init the PHY */
	hsotgctrl_drvdata->usb_active = true;
	bcm_hsotgctrl_en_clock(true);

	mdelay(HSOTGCTRL_STEP_DELAY_IN_MS);

	/* clear bit 15 RDB error */
	val = readl(hsotgctrl_drvdata->hsotg_ctrl_base +
			HSOTG_CTRL_PHY_P1CTL_OFFSET);
	val &= ~HSOTG_CTRL_PHY_P1CTL_USB11_OEB_IS_TXEB_MASK;
	writel(val, hsotgctrl_drvdata->hsotg_ctrl_base +
			HSOTG_CTRL_PHY_P1CTL_OFFSET);
	mdelay(HSOTGCTRL_STEP_DELAY_IN_MS);

	/* S/W reset Phy, active low */
	val = readl(hsotgctrl_drvdata->hsotg_ctrl_base +
			HSOTG_CTRL_PHY_P1CTL_OFFSET);
	val &= ~HSOTG_CTRL_PHY_P1CTL_SOFT_RESET_MASK;
	writel(val, hsotgctrl_drvdata->hsotg_ctrl_base +
			HSOTG_CTRL_PHY_P1CTL_OFFSET);

	mdelay(HSOTGCTRL_STEP_DELAY_IN_MS);

	/* bring Phy out of reset */
	val = readl(hsotgctrl_drvdata->hsotg_ctrl_base +
			HSOTG_CTRL_PHY_P1CTL_OFFSET);
	val &= ~HSOTG_CTRL_PHY_P1CTL_PHY_MODE_MASK;
	val |= HSOTG_CTRL_PHY_P1CTL_SOFT_RESET_MASK;
	/* use OTG mode */
	val |= PHY_MODE_OTG << HSOTG_CTRL_PHY_P1CTL_PHY_MODE_SHIFT;
	writel(val, hsotgctrl_drvdata->hsotg_ctrl_base +
		HSOTG_CTRL_PHY_P1CTL_OFFSET);

	mdelay(HSOTGCTRL_STEP_DELAY_IN_MS);

	/* Enable pad, internal PLL etc */
	bcm_hsotgctrl_set_phy_off(false);

	mdelay(HSOTGCTRL_STEP_DELAY_IN_MS);

	/*Come up as device until we check PMU ID status
	 * to avoid turning on Vbus before checking
	 */
	val =	HSOTG_CTRL_USBOTGCONTROL_OTGSTAT_CTRL_MASK |
			HSOTG_CTRL_USBOTGCONTROL_UTMIOTG_IDDIG_SW_MASK |
			HSOTG_CTRL_USBOTGCONTROL_USB_HCLK_EN_DIRECT_MASK |
			HSOTG_CTRL_USBOTGCONTROL_USB_ON_IS_HCLK_EN_MASK |
			HSOTG_CTRL_USBOTGCONTROL_USB_ON_MASK |
			HSOTG_CTRL_USBOTGCONTROL_PRST_N_SW_MASK |
			HSOTG_CTRL_USBOTGCONTROL_HRESET_N_SW_MASK;

	writel(val, hsotgctrl_drvdata->hsotg_ctrl_base +
			HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	mdelay(HSOTGCTRL_STEP_DELAY_IN_MS);

	error = device_create_file(&pdev->dev, &dev_attr_hsotgctrldump);

	if (error) {
		dev_warn(&pdev->dev, "Failed to create HOST file\n");
		goto Error_bcm_hsotgctrl_probe;
	}


#ifndef CONFIG_USB_OTG_UTILS
	/* Clear non-driving as default in case there
	 * is no transceiver hookup */
	bcm_hsotgctrl_phy_set_non_driving(false);
#endif

#ifdef CONFIG_NOP_USB_XCEIV
	/* Clear non-driving as default in case there
	 * is no transceiver hookup */
	bcm_hsotgctrl_phy_set_non_driving(false);
#endif

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	hsotgctrl_drvdata->hsotgctrl_irq = platform_get_irq(pdev, 0);

	/* Create a work queue for wakeup work items */
	hsotgctrl_drvdata->bcm_hsotgctrl_work_queue =
		create_workqueue("bcm_hsotgctrl_events");

	if (hsotgctrl_drvdata->bcm_hsotgctrl_work_queue == NULL) {
		dev_warn(&pdev->dev,
			 "BCM HSOTGCTRL events work queue creation failed\n");
		/* Treat this as non-fatal error */
	}

	INIT_DELAYED_WORK(&hsotgctrl_drvdata->wakeup_work,
			  bcm_hsotgctrl_delayed_wakeup_handler);

	/* disable Bvalid interrupt bit
	 * This interrupt is not currently used as the STAT2 detection
	 * happens from the PMU side. Beacsue of not clearing this bit
	 * Master clock gating feature was not working in Java. This
	 * is not a issue in case of Hawaii
	 * */
	val = readl(hsotgctrl_drvdata->hsotg_ctrl_base +
				HSOTG_CTRL_USBOTGCONTROL_OFFSET);
	val |= 1 << HSOTG_CTRL_USBOTGCONTROL_BVALID_CLR_SHIFT;
	writel(val, hsotgctrl_drvdata->hsotg_ctrl_base +
			HSOTG_CTRL_USBOTGCONTROL_OFFSET);
	/* request_irq enables irq */
	hsotgctrl_drvdata->irq_enabled = true;
	error = request_irq(hsotgctrl_drvdata->hsotgctrl_irq,
			bcm_hsotgctrl_wake_irq,
			IRQF_TRIGGER_HIGH | IRQF_NO_SUSPEND,
			"bcm_hsotgctrl", (void *)hsotgctrl_drvdata);
	if (error) {
		hsotgctrl_drvdata->irq_enabled = false;
		hsotgctrl_drvdata->hsotgctrl_irq = 0;
		dev_warn(&pdev->dev, "Failed to request IRQ for wakeup\n");
	}

	local_wakeup_core_cb = NULL;
	return 0;

Error_bcm_hsotgctrl_probe:
	clk_put(hsotgctrl_drvdata->otg_clk);
	clk_put(hsotgctrl_drvdata->mdio_master_clk);
	kfree(hsotgctrl_drvdata);
	return error;
}

static int bcm_hsotgctrl_remove(struct platform_device *pdev)
{
	struct bcm_hsotgctrl_drv_data *hsotgctrl_drvdata =
				platform_get_drvdata(pdev);

	device_remove_file(&pdev->dev, &dev_attr_hsotgctrldump);

	if (hsotgctrl_drvdata->hsotgctrl_irq)
		free_irq(hsotgctrl_drvdata->hsotgctrl_irq,
		    (void *)hsotgctrl_drvdata);

	pm_runtime_disable(&pdev->dev);
	clk_put(hsotgctrl_drvdata->otg_clk);
	clk_put(hsotgctrl_drvdata->mdio_master_clk);
	local_hsotgctrl_handle = NULL;
	local_wakeup_core_cb = NULL;
	kfree(hsotgctrl_drvdata);

	return 0;
}

static int bcm_hsotgctrl_pm_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	int status = 0;
	struct bcm_hsotgctrl_drv_data *bcm_hsotgctrl_handle =
		local_hsotgctrl_handle;

	if (bcm_hsotgctrl_handle && bcm_hsotgctrl_handle->allow_suspend)
		status = 0;
	else
		status = -EBUSY;

	return status;
}

static int bcm_hsotgctrl_pm_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver bcm_hsotgctrl_driver = {
	.driver = {
		   .name = "bcm_hsotgctrl",
		   .owner = THIS_MODULE,
	},
	.probe = bcm_hsotgctrl_probe,
	.remove = bcm_hsotgctrl_remove,
	.suspend = bcm_hsotgctrl_pm_suspend,
	.resume = bcm_hsotgctrl_pm_resume,
};

static int __init bcm_hsotgctrl_init(void)
{
	pr_info("Broadcom USB HSOTGCTRL Driver\n");

	return platform_driver_register(&bcm_hsotgctrl_driver);
}
arch_initcall(bcm_hsotgctrl_init);

static void __exit bcm_hsotgctrl_exit(void)
{
	platform_driver_unregister(&bcm_hsotgctrl_driver);
}
module_exit(bcm_hsotgctrl_exit);

int bcm_hsotgctrl_phy_set_vbus_stat(bool on)
{
	unsigned long val;
	void *hsotg_ctrl_base;

	if (NULL != local_hsotgctrl_handle)
		hsotg_ctrl_base = local_hsotgctrl_handle->hsotg_ctrl_base;
	else
		return -ENODEV;

	val = readl(hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	if (on) {
		val |= (HSOTG_CTRL_USBOTGCONTROL_REG_OTGSTAT2_MASK |
			HSOTG_CTRL_USBOTGCONTROL_REG_OTGSTAT1_MASK);
	} else {
		val &= ~(HSOTG_CTRL_USBOTGCONTROL_REG_OTGSTAT2_MASK |
			 HSOTG_CTRL_USBOTGCONTROL_REG_OTGSTAT1_MASK);
	}

	writel(val, hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_phy_set_vbus_stat);

int bcm_hsotgctrl_phy_set_non_driving(bool on)
{
	unsigned long val;
	void *hsotg_ctrl_base;

	if (NULL != local_hsotgctrl_handle)
		hsotg_ctrl_base = local_hsotgctrl_handle->hsotg_ctrl_base;
	else
		return -ENODEV;

	/* set Phy to driving mode */
	val = readl(hsotg_ctrl_base + HSOTG_CTRL_PHY_P1CTL_OFFSET);

	if (on)
		val |= HSOTG_CTRL_PHY_P1CTL_NON_DRIVING_MASK;
	else
		val &= ~HSOTG_CTRL_PHY_P1CTL_NON_DRIVING_MASK;

	writel(val, hsotg_ctrl_base + HSOTG_CTRL_PHY_P1CTL_OFFSET);
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_phy_set_non_driving);

int bcm_hsotgctrl_reset_clk_domain(void)
{
	unsigned long val;
	void *hsotg_ctrl_base;

	if (NULL != local_hsotgctrl_handle)
		hsotg_ctrl_base = local_hsotgctrl_handle->hsotg_ctrl_base;
	else
		return -ENODEV;

	val = readl(hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	/* Reset PHY and AHB clock domains */
	val &= ~(HSOTG_CTRL_USBOTGCONTROL_PRST_N_SW_MASK |
			HSOTG_CTRL_USBOTGCONTROL_HRESET_N_SW_MASK);
	writel(val, hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	val = readl(hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	/* De-assert PHY and AHB clock domain reset */
	val |= (HSOTG_CTRL_USBOTGCONTROL_PRST_N_SW_MASK |
			HSOTG_CTRL_USBOTGCONTROL_HRESET_N_SW_MASK);
	writel(val, hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	msleep_interruptible(PHY_PM_DELAY_IN_MS);
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_reset_clk_domain);

int bcm_hsotgctrl_set_phy_off(bool on)
{
	unsigned long val;
	void *hsotg_ctrl_base;

	if (NULL != local_hsotgctrl_handle)
		hsotg_ctrl_base = local_hsotgctrl_handle->hsotg_ctrl_base;
	else
		return -ENODEV;

	val = readl(hsotg_ctrl_base + HSOTG_CTRL_PHY_CFG_OFFSET);
	if (on)
		val |= HSOTG_CTRL_PHY_CFG_PHY_IDDQ_I_MASK;
	else
		val &= ~HSOTG_CTRL_PHY_CFG_PHY_IDDQ_I_MASK;

	writel(val, hsotg_ctrl_base + HSOTG_CTRL_PHY_CFG_OFFSET);

	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_phy_off);

int bcm_hsotgctrl_set_phy_iso(bool on)
{
	unsigned long val;
	void *hsotg_ctrl_base;

	if (NULL != local_hsotgctrl_handle)
		hsotg_ctrl_base = local_hsotgctrl_handle->hsotg_ctrl_base;
	else
		return -ENODEV;

	val = readl(hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	if (on)
		val |= HSOTG_CTRL_USBOTGCONTROL_PHY_ISO_I_MASK;
	else
		val &= ~HSOTG_CTRL_USBOTGCONTROL_PHY_ISO_I_MASK;

	writel(val, hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_phy_iso);

int bcm_hsotgctrl_set_bc_iso(bool on)
{
	unsigned long val;
	void *hsotg_ctrl_base;

	if (NULL != local_hsotgctrl_handle)
		hsotg_ctrl_base = local_hsotgctrl_handle->hsotg_ctrl_base;
	else
		return -ENODEV;

	val = readl(hsotg_ctrl_base + HSOTG_CTRL_PHY_CFG_OFFSET);

	if (on)
		val |= HSOTG_CTRL_PHY_CFG_BC_ISO_I_MASK;
	else
		val &= ~HSOTG_CTRL_PHY_CFG_BC_ISO_I_MASK;

	writel(val, hsotg_ctrl_base + HSOTG_CTRL_PHY_CFG_OFFSET);

	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_bc_iso);


int bcm_hsotgctrl_set_soft_ldo_pwrdn(bool on)
{
	unsigned long val;
	void *hsotg_ctrl_base;

	if (NULL != local_hsotgctrl_handle)
		hsotg_ctrl_base = local_hsotgctrl_handle->hsotg_ctrl_base;
	else
		return -ENODEV;

	val = readl(hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	if (on)
		val |= HSOTG_CTRL_USBOTGCONTROL_SOFT_LDO_PWRDN_MASK;
	else
		val &= ~HSOTG_CTRL_USBOTGCONTROL_SOFT_LDO_PWRDN_MASK;

	writel(val, hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_soft_ldo_pwrdn);

int bcm_hsotgctrl_set_aldo_pdn(bool on)
{
	unsigned long val;
	void *hsotg_ctrl_base;

	if (NULL != local_hsotgctrl_handle)
		hsotg_ctrl_base = local_hsotgctrl_handle->hsotg_ctrl_base;
	else
		return -ENODEV;

	val = readl(hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	if (on)
		val |= (HSOTG_CTRL_USBOTGCONTROL_SOFT_ALDO_PDN_MASK |
				HSOTG_CTRL_USBOTGCONTROL_SOFT_DLDO_PDN_MASK);
	else
		val &= ~(HSOTG_CTRL_USBOTGCONTROL_SOFT_ALDO_PDN_MASK |
				HSOTG_CTRL_USBOTGCONTROL_SOFT_DLDO_PDN_MASK);

	writel(val, hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_aldo_pdn);

int bcm_hsotgctrl_set_phy_resetb(bool on)
{
	unsigned long val;
	void *hsotg_ctrl_base;

	if (NULL != local_hsotgctrl_handle)
		hsotg_ctrl_base = local_hsotgctrl_handle->hsotg_ctrl_base;
	else
		return -ENODEV;

	val = readl(hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	if (on)
		val |= HSOTG_CTRL_USBOTGCONTROL_SOFT_PHY_RESETB_MASK;
	else
		val &= ~HSOTG_CTRL_USBOTGCONTROL_SOFT_PHY_RESETB_MASK;

	writel(val, hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_phy_resetb);

/* NOTE: PLL reset will be required in future if PHY_RESETB does not
 * automatically reset PHY PLL
 */
int bcm_hsotgctrl_set_phy_pll_resetb(bool on)
{
	unsigned long val;
	void *hsotg_ctrl_base;

	if (NULL != local_hsotgctrl_handle)
		hsotg_ctrl_base = local_hsotgctrl_handle->hsotg_ctrl_base;
	else
		return -ENODEV;

	val = readl(hsotg_ctrl_base + HSOTG_CTRL_PHY_CFG_OFFSET);

	if (on)
		val |= HSOTG_CTRL_PHY_CFG_PLL_RESETB_MASK;
	else
		val &= ~HSOTG_CTRL_PHY_CFG_PLL_RESETB_MASK;

	writel(val, hsotg_ctrl_base + HSOTG_CTRL_PHY_CFG_OFFSET);

	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_phy_pll_resetb);

int bcm_hsotgctrl_set_phy_clk_request(bool on)
{
	unsigned long val;
	void *hsotg_ctrl_base;

	if (NULL != local_hsotgctrl_handle)
		hsotg_ctrl_base = local_hsotgctrl_handle->hsotg_ctrl_base;
	else
		return -ENODEV;

	val = readl(hsotg_ctrl_base + HSOTG_CTRL_PHY_P1CTL_OFFSET);

	if (on) {
		/* Set PHY clk req */
		val |= HSOTG_CTRL_PHY_P1CTL_PHY_CLOCK_REQUEST_MASK;
		writel(val, hsotg_ctrl_base + HSOTG_CTRL_PHY_P1CTL_OFFSET);

		/* Set phy clk req clear bit */
		val = readl(hsotg_ctrl_base + HSOTG_CTRL_PHY_P1CTL_OFFSET);
		val |= HSOTG_CTRL_PHY_P1CTL_PHY_CLOCK_REQ_CLEAR_MASK;
		writel(val, hsotg_ctrl_base + HSOTG_CTRL_PHY_P1CTL_OFFSET);
	} else {
		/* Clear PHY clk req */
		val &= ~HSOTG_CTRL_PHY_P1CTL_PHY_CLOCK_REQUEST_MASK;
		writel(val, hsotg_ctrl_base + HSOTG_CTRL_PHY_P1CTL_OFFSET);
		/* Clear phy clk req clear bit */
		val = readl(hsotg_ctrl_base + HSOTG_CTRL_PHY_P1CTL_OFFSET);
		val &= ~HSOTG_CTRL_PHY_P1CTL_PHY_CLOCK_REQ_CLEAR_MASK;
		writel(val, hsotg_ctrl_base + HSOTG_CTRL_PHY_P1CTL_OFFSET);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_phy_clk_request);

int bcm_hsotgctrl_set_ldo_suspend_mask(void)
{
	unsigned long val;
	void *hsotg_ctrl_base;

	if (NULL != local_hsotgctrl_handle)
		hsotg_ctrl_base = local_hsotgctrl_handle->hsotg_ctrl_base;
	else
		return -ENODEV;

	val = readl(hsotg_ctrl_base + HSOTG_CTRL_PHY_CTRL_OFFSET);

	val |= HSOTG_CTRL_PHY_CTRL_SUSPEND_MASK_MASK;
	writel(val, hsotg_ctrl_base + HSOTG_CTRL_PHY_CTRL_OFFSET);

	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_ldo_suspend_mask);


int bcm_hsotgctrl_phy_set_id_stat(bool floating)
{
	unsigned long val;
	void *hsotg_ctrl_base;

	if (NULL != local_hsotgctrl_handle)
		hsotg_ctrl_base = local_hsotgctrl_handle->hsotg_ctrl_base;
	else
		return -ENODEV;

	val = readl(hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	if (floating)
		val |= HSOTG_CTRL_USBOTGCONTROL_UTMIOTG_IDDIG_SW_MASK;
	else
		val &= ~HSOTG_CTRL_USBOTGCONTROL_UTMIOTG_IDDIG_SW_MASK;

	writel(val, hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	return 0;

}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_phy_set_id_stat);

int bcm_hsotgctrl_phy_wakeup_condition(bool set)
{
	unsigned long val;
	void *hsotg_ctrl_base;

	if (NULL != local_hsotgctrl_handle)
		hsotg_ctrl_base = local_hsotgctrl_handle->hsotg_ctrl_base;
	else
		return -ENODEV;

	val = readl(hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	if (set) {
		val |= HSOTG_CTRL_USBOTGCONTROL_WAKEUP_INT_MODE_MASK |
			HSOTG_CTRL_USBOTGCONTROL_WAKEUP_INT_INV_MASK;
	} else {
		val &= ~(HSOTG_CTRL_USBOTGCONTROL_WAKEUP_INT_MODE_MASK |
			HSOTG_CTRL_USBOTGCONTROL_WAKEUP_INT_INV_MASK);
	}

	writel(val, hsotg_ctrl_base + HSOTG_CTRL_USBOTGCONTROL_OFFSET);

	return 0;

}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_phy_wakeup_condition);

int bcm_hsotgctrl_is_suspend_allowed(bool *suspend_allowed)
{
	if ((NULL != local_hsotgctrl_handle) &&
		    suspend_allowed) {
		/* Return the status */
		*suspend_allowed = local_hsotgctrl_handle->allow_suspend;
		pr_info("bcm_hsotgctrl_is_suspend_allowed: %d\n",
			local_hsotgctrl_handle->allow_suspend);
	} else {
		/* No device handle */
		return -ENODEV;
	}

	return 0;

}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_is_suspend_allowed);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("USB HSOTGCTRL driver");
MODULE_LICENSE("GPL");
