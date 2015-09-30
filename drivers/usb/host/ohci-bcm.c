/*****************************************************************************
* Copyright 2006 - 2010 Broadcom Corporation.  All rights reserved.
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL"). 
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a
* license other than the GPL, without Broadcom's express prior written
* consent.
*****************************************************************************/

#include <linux/platform_device.h>
#include <linux/version.h>

#include <mach/irqs.h>
#include "bcm_usbh.h"

#define BCM_USBOHCI_MODULE_DESCRIPTION      "Broadcom USB OHCI driver"
#define BCM_USBOHCI_MODULE_VERSION          "1.0.0"

#define BCM_USBOHCI_IRQF_FLAGS              (IRQF_DISABLED | IRQF_SHARED)
#define BCM_USBOHCI_NAME                    "bcm-ohci"

#define OHCI_INFO(pdev, fmt, args...) dev_info(&pdev->dev, fmt, ## args)
#define OHCI_ERR(pdev, fmt, args...) dev_err(&pdev->dev, fmt, ## args)

struct usb_cfg
{
	void *virt_reg_base;
	struct usb_hcd *hcd;
};

extern int usb_disabled(void);
static int bcm_ohci_start(struct usb_hcd *hcd);

static const struct hc_driver ohci_hcd_driver =
{
	.description = hcd_name,
	.product_desc = BCM_USBOHCI_NAME,
	.hcd_priv_size = sizeof(struct ohci_hcd),
   
	/*
	 * generic hardware linkage
	 */
	.irq = ohci_irq,
	.flags = HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.start = bcm_ohci_start,
	.stop = ohci_stop,
	.shutdown = ohci_shutdown,
#ifdef CONFIG_PM
	.bus_suspend = ohci_bus_suspend,
	.bus_resume = ohci_bus_resume,
#endif

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue = ohci_urb_enqueue,
	.urb_dequeue = ohci_urb_dequeue,
	.endpoint_disable = ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number = ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data = ohci_hub_status_data,
	.hub_control = ohci_hub_control,
};

static int bcm_ohci_start(struct usb_hcd *hcd)
{
	struct ohci_hcd *ohci;
	int err;
   
	if (!hcd) {
		printk(KERN_ERR "invalid hcd pointer in %s\n", __FUNCTION__);
		return -EINVAL;
	}

	ohci = hcd_to_ohci(hcd);

	if ((err = ohci_init(ohci)) < 0) {
		printk(KERN_ERR "busnum %d: ohci_init() failed, err=%d\n", hcd->self.busnum, err);
		return err;
	}

	if ((err = ohci_run(ohci)) < 0) {
		printk(KERN_ERR "busnum %d: ohci_run() failed, err=%d\n", hcd->self.busnum, err);
		ohci_stop(hcd);
		return err;
	}

	return 0;
}

int bcm_ohci_probe(struct platform_device *pdev)
{
	struct usb_cfg *usb;
	struct usb_hcd *hcd;
	struct resource *iomem, *ioarea;
	int ret, irq;

	if (usb_disabled())
		return -ENODEV;

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iomem) {
		OHCI_ERR(pdev, "no mem resource\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	/* get the irq info */
	irq = platform_get_irq(pdev, 0);
	if (!irq) {
		OHCI_ERR(pdev, "no irq resource\n");
		ret = -ENODEV;
		goto err_exit;
	}
	
	ioarea = request_mem_region(iomem->start, resource_size(iomem), pdev->name);
	if (!ioarea) {
		OHCI_ERR(pdev, "memory region already claimed\n");
		ret = -EBUSY;
		goto err_exit;
	}
	
	usb = kzalloc(sizeof(*usb), GFP_KERNEL);
	if (!usb) {
		OHCI_ERR(pdev, "unable to allocate memory for private data\n");
		ret = -ENOMEM;
		goto err_free_iomem;
	}

	usb->virt_reg_base = ioremap(iomem->start, resource_size(iomem));
	if (!usb->virt_reg_base) {
		OHCI_ERR(pdev, "ioremap failed\n");
		ret = -ENOMEM;
		goto err_free_private_mem;
	}

	/* enable clock and PHY */
	ret = bcm_usbh_init(pdev->id);
	if (ret < 0) {
		OHCI_ERR(pdev, "clock and PHY initialization failed\n");
		goto err_io_unmap;
	}
	
	hcd = usb_create_hcd(&ohci_hcd_driver, &pdev->dev, (char *)pdev->name);
	if (!hcd) {
		OHCI_ERR(pdev, "usb_create_hcd failed\n");
		ret = -ENOMEM;
		goto err_usb_term;
	}

	/* struct ohci_regs def'd in Linux ohci.h which is included by Linux ohci-hcd.c */
	usb->hcd = hcd;
	hcd->rsrc_start = (unsigned int)usb->virt_reg_base;
	hcd->rsrc_len = sizeof(struct ohci_regs);
	hcd->regs = usb->virt_reg_base;

	ohci_hcd_init(hcd_to_ohci(hcd));

	ret = usb_add_hcd(hcd, irq, BCM_USBOHCI_IRQF_FLAGS);
	if (ret) {
		OHCI_ERR(pdev, "usb_add_hcd failed\n");
		goto err_remove_hcd;
	}

	platform_set_drvdata(pdev, usb);

	OHCI_INFO(pdev, "probe done\n");
	return 0;
   
err_remove_hcd:
	usb_remove_hcd(hcd);
	usb_put_hcd(hcd);

err_usb_term:
	bcm_usbh_term(pdev->id);

err_io_unmap:
	iounmap(usb->virt_reg_base);

err_free_private_mem:
	kfree(usb);

err_free_iomem:
	release_mem_region(iomem->start, resource_size(iomem));

err_exit:
	OHCI_ERR(pdev, "probe failed: %d\n", ret);
	return ret;
}

int bcm_ohci_remove(struct platform_device *pdev)
{
	struct usb_cfg *usb = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = usb->hcd;
	struct resource *iomem;

	usb_remove_hcd(hcd);
	usb_put_hcd(hcd);

	bcm_usbh_term(pdev->id);
	iounmap(usb->virt_reg_base);
	kfree(usb);

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(iomem->start, resource_size(iomem));

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static void bcm_ohci_shutdown(struct platform_device *pdev)
{
	struct usb_cfg *usb = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = usb->hcd;
   
	if (hcd->driver->shutdown)
		hcd->driver->shutdown(hcd);
}

#ifdef CONFIG_PM
static int bcm_ohci_suspend(struct platform_device *pdev, pm_message_t message)
{
	struct usb_cfg *usb = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = usb->hcd;
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);
	unsigned long flags;
	int rc = 0;

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible, bail out if RH has been resumed. Use
	 * the spinlock to properly synchronize with possible pending
	 * RH suspend or resume activity.
	 *
	 * This is still racy as hcd->state is manipulated outside of
	 * any locks =P But that will be a different fix.
	 */
	spin_lock_irqsave(&ohci->lock, flags);
	if (hcd->state != HC_STATE_SUSPENDED) {
		rc = -EINVAL;
		goto bail;
	}
	ohci_writel(ohci, OHCI_INTR_MIE, &ohci->regs->intrdisable);
	(void)ohci_readl(ohci, &ohci->regs->intrdisable);

	/* make sure snapshot being resumed re-enumerates everything */
	if (message.event == PM_EVENT_PRETHAW)
		ohci_usb_reset(ohci);
   
	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	rc = bcm_usbh_suspend(pdev->id);

bail:
	spin_unlock_irqrestore(&ohci->lock, flags);
	return rc;
}

static int bcm_ohci_resume(struct platform_device *pdev)
{
	struct usb_cfg *usb = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = usb->hcd;
   
	bcm_usbh_resume(pdev->id);
   
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	ohci_finish_controller_resume(hcd);
   
	return 0;
}
#endif /* CONFIG_PM */

/*
 * Generic platform device driver definition.
 */
static struct platform_driver ohci_bcm_driver =
{
	.probe = bcm_ohci_probe,
	.remove = bcm_ohci_remove,
	.shutdown = bcm_ohci_shutdown,
#ifdef CONFIG_PM
	.suspend = bcm_ohci_suspend,
	.resume = bcm_ohci_resume,
#endif
	.driver = {
		.name = BCM_USBOHCI_NAME,
		.owner = THIS_MODULE,
	},
};

MODULE_DESCRIPTION(BCM_USBOHCI_MODULE_DESCRIPTION);
MODULE_LICENSE("GPL");
MODULE_VERSION(BCM_USBOHCI_MODULE_VERSION);
