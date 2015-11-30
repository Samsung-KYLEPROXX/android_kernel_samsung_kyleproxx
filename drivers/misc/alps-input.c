/* alps-input.c
 *
 * Input device driver for alps sensor
 *
 * Copyright (C) 2011 ALPS ELECTRIC CO., LTD. All Rights Reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <asm/uaccess.h> 
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/alps_io.h>

extern int accsns_get_acceleration_data(int *xyz);
extern int hscd_get_magnetic_field_data(int *xyz);
extern void hscd_activate(int flgatm, int flg, int dtime);
extern void accsns_activate(int flgatm, int flg, int dtime);
extern int hscd_self_test_A(void);
extern int hscd_self_test_B(void);

#if defined(CONFIG_SENSORS_CORE)
extern struct class *sensors_class;
#endif

static DEFINE_MUTEX(alps_lock);

static struct platform_device *pdev;
static struct input_polled_dev *alps_idev;

#define EVENT_TYPE_ACCEL_X          ABS_X
#define EVENT_TYPE_ACCEL_Y          ABS_Y
#define EVENT_TYPE_ACCEL_Z          ABS_Z
#define EVENT_TYPE_ACCEL_STATUS     ABS_WHEEL

#define EVENT_TYPE_YAW              ABS_RX
#define EVENT_TYPE_PITCH            ABS_RY
#define EVENT_TYPE_ROLL             ABS_RZ
#define EVENT_TYPE_ORIENT_STATUS    ABS_RUDDER

#define EVENT_TYPE_MAGV_X           ABS_HAT0X
#define EVENT_TYPE_MAGV_Y           ABS_HAT0Y
#define EVENT_TYPE_MAGV_Z           ABS_BRAKE

#define ALPS_POLL_INTERVAL		100	/* msecs */
#define ALPS_INPUT_FUZZ		0	/* input event threshold */
#define ALPS_INPUT_FLAT		0

/*#define POLL_STOP_TIME		400	*//* (msec) */

#define ALPS_DEBUG  0

#if ALPS_DEBUG
#define ALPSDBG(fmt, args...) printk(KERN_INFO fmt, ## args)
#else
#define ALPSDBG(fmt, args...)
#endif

static int flgM, flgA;
static int delay = 200;
/*static int poll_stop_cnt = 0;*/

///////////////////////////////////////////////////////////////////////////////
// for I/O Control

static long alps_ioctl( struct file* filp, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int ret = -1, tmpval;

	switch (cmd) {
		case ALPSIO_SET_MAGACTIVATE:
			ret = copy_from_user(&tmpval, argp, sizeof(tmpval));
			if (ret) {
				printk(KERN_ERR "error : alps_ioctl(cmd = ALPSIO_SET_MAGACTIVATE)\n" );
				return -EFAULT;
			}

			printk(KERN_INFO "[ALPS] ioctl(cmd = ALPSIO_SET_MAGACTIVATE), flgM = %d\n", tmpval);

			mutex_lock(&alps_lock);
			flgM = tmpval;
			hscd_activate(1, tmpval, delay);
			mutex_unlock(&alps_lock);
			break;

		case ALPSIO_SET_ACCACTIVATE:
			ret = copy_from_user(&tmpval, argp, sizeof(tmpval));
			if (ret) {
				printk(KERN_ERR "error : alps_ioctl(cmd = ALPSIO_SET_ACCACTIVATE)\n");
				return -EFAULT;
			}

			printk(KERN_INFO "[ALPS] ioctl(cmd = ALPSIO_SET_ACCACTIVATE), flgA = %d\n", tmpval);

			mutex_lock(&alps_lock);
			flgA = tmpval;
			accsns_activate(1, flgA, delay);
			mutex_unlock(&alps_lock);
			break;

		case ALPSIO_SET_DELAY:
			ret = copy_from_user(&tmpval, argp, sizeof(tmpval));
			if (ret) {
				printk(KERN_ERR "error : alps_ioctl(cmd = ALPSIO_SET_DELAY)\n" );
				return -EFAULT;
			}

			ALPSDBG("[ALPS] ioctl(cmd = ALPSIO_SET_DELAY), delay=%d\n", tmpval);

                        mutex_lock(&alps_lock);
#if defined(CONFIG_SENSORS_MAG_15BIT)
                        if (flgM) {
                            if      (tmpval <=  10) tmpval =  10;
                            else if (tmpval <=  20) tmpval =  20;
                            else if (tmpval <=  70) tmpval =  70;
                            else                    tmpval = 200;
                        }
                        else {
                            if      (tmpval <=  10) tmpval =  10;
                        }
#else                       
                        if (flgM) {
                            if      (tmpval <=  10) tmpval =  10;
                            else if (tmpval <=  20) tmpval =  20;
                            else if (tmpval <=  70) tmpval =  50;
                            else                    tmpval = 100;
                        }
                        else {
                            if      (tmpval <=  10) tmpval =  10;
                        }
#endif                        
			delay = tmpval;
			/*poll_stop_cnt = POLL_STOP_TIME / tmpval;*/
			hscd_activate(1, flgM, delay);
			accsns_activate(1, flgA, delay);
			mutex_unlock(&alps_lock);

			ALPSDBG("[ALPS] delay = %d\n", delay);

			break;

		default:
			return -ENOTTY;
	}
	return 0;
}

static int 
alps_io_open( struct inode* inode, struct file* filp )
{
	return 0;
}

static int 
alps_io_release( struct inode* inode, struct file* filp )
{
	return 0;
}

static struct file_operations alps_fops = {
	.owner   = THIS_MODULE,
	.open    = alps_io_open,
	.release = alps_io_release,
	.unlocked_ioctl = alps_ioctl,
};

static struct miscdevice alps_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "alps_io",
	.fops  = &alps_fops,
};


///////////////////////////////////////////////////////////////////////////////
// for input device

static ssize_t accsns_position_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int x,y,z;
	int xyz[3];

        printk(KERN_INFO "[ALPS] accsns_position_show\n");  

	if(accsns_get_acceleration_data(xyz) == 0) {
		x = xyz[0];
		y = xyz[1];
		z = xyz[2];
	} else {
		x = 0;
		y = 0;
		z = 0;
	}
	return snprintf(buf, PAGE_SIZE, "(%d %d %d)\n",x,y,z);
}

static ssize_t hscd_position_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int x,y,z;
	int xyz[3];

        printk(KERN_INFO "[ALPS] hscd_position_show\n");  

	if(hscd_get_magnetic_field_data(xyz) == 0) {
		x = xyz[0];
		y = xyz[1];
		z = xyz[2];
	} else {
		x = 0;
		y = 0;
		z = 0;
	}
	return snprintf(buf, PAGE_SIZE, "(%d %d %d)\n",x,y,z);
}

static ssize_t alps_position_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	size_t cnt = 0;

        printk(KERN_INFO "[ALPS] alps_position_show\n");      
        
	mutex_lock(&alps_lock);
	cnt += accsns_position_show(dev,attr,buf);
	cnt += hscd_position_show(dev,attr,buf);
	mutex_unlock(&alps_lock);
	return cnt;
}

static DEVICE_ATTR(position, 0444, alps_position_show, NULL);

static struct attribute *alps_attributes[] = {
	&dev_attr_position.attr,
	NULL,
};

static struct attribute_group alps_attribute_group = {
	.attrs = alps_attributes,
};

static ssize_t hscd_fs_selftest_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        int result = 0;    

        if (!flgM) {
            mutex_lock(&alps_lock);
            hscd_activate(0, 1, 10);
            mutex_unlock(&alps_lock);
            mdelay(5);
            flgM = 1;
        }

        mutex_lock(&alps_lock);
	result = hscd_self_test_A();
        mutex_unlock(&alps_lock);

    	printk(KERN_INFO "[HSCD] self test A result = %d\n", result);
        if(result != 1){
    		goto out_self_test;
        }

        mutex_lock(&alps_lock);
	result = hscd_self_test_B();
        mutex_unlock(&alps_lock);
        
	printk(KERN_INFO "[HSCD] self test B result = %d\n", result);        

out_self_test:

	return sprintf(buf, "%d\n", result);
}

static ssize_t hscd_fs_readADC_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int d[3];

        if (!flgM) {
            mutex_lock(&alps_lock);
            hscd_activate(0, 1, 10);
            mutex_unlock(&alps_lock);
            mdelay(5);
            flgM = 1;
        }

	mutex_lock(&alps_lock);
	hscd_get_magnetic_field_data(d);
        mutex_unlock(&alps_lock);
        
    	printk(KERN_INFO "[HSCD] read ADC x:%d y:%d z:%d\n",d[0],d[1],d[2]);

	return sprintf(buf, "%d,%d,%d\n",d[0],d[1],d[2]);
}

static ssize_t hscd_fs_readDAC_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d,%d,%d\n",0,0,0);;
}

static ssize_t hscd_fs_raw_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int d[3];

        if (!flgM) {
            mutex_lock(&alps_lock);
            hscd_activate(0, 1, 10);
            mutex_unlock(&alps_lock);
            mdelay(5);
            flgM = 1;
        }
        
	mutex_lock(&alps_lock);
	hscd_get_magnetic_field_data(d);      
        mutex_unlock(&alps_lock);
        
	return sprintf(buf, "%d,%d,%d\n",d[0],d[1],d[2]);
}

static DEVICE_ATTR(status, S_IRUGO | S_IRUSR | S_IROTH, hscd_fs_selftest_show, NULL);
static DEVICE_ATTR(selftest, S_IRUGO | S_IRUSR | S_IROTH, hscd_fs_selftest_show, NULL);
static DEVICE_ATTR(adc, S_IRUGO | S_IRUSR | S_IROTH, hscd_fs_readADC_show, NULL);
static DEVICE_ATTR(dac, S_IRUGO | S_IRUSR | S_IROTH, hscd_fs_readDAC_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO | S_IRUSR | S_IROTH, hscd_fs_raw_data_show, NULL);

static int alps_probe(struct platform_device *dev)
{
	printk(KERN_INFO "[ALPS] alps_probe\n");
	return 0;
}

static int alps_remove(struct platform_device *dev)
{
	printk(KERN_INFO "[ALPS] alps_remove\n");
	return 0;
}

static struct platform_driver alps_driver = {
	.driver	= {
		.name = "alps-input",
		.owner = THIS_MODULE,
	},
    .probe = alps_probe,
    .remove = alps_remove,
};

static void accsns_poll(struct input_dev *idev)
{
	int xyz[3];

	if(accsns_get_acceleration_data(xyz) == 0) {
		input_report_abs(idev, EVENT_TYPE_ACCEL_X, xyz[0]);
		input_report_abs(idev, EVENT_TYPE_ACCEL_Y, xyz[1]);
		input_report_abs(idev, EVENT_TYPE_ACCEL_Z, xyz[2]);
		idev->sync = 0;
		input_event(idev, EV_SYN, SYN_REPORT, 1);
	}
}

static void hscd_poll(struct input_dev *idev)
{
	int xyz[3];

	if(hscd_get_magnetic_field_data(xyz) == 0) {
		input_report_abs(idev, EVENT_TYPE_MAGV_X, xyz[0]);
		input_report_abs(idev, EVENT_TYPE_MAGV_Y, xyz[1]);
		input_report_abs(idev, EVENT_TYPE_MAGV_Z, xyz[2]);
		idev->sync = 0;
		input_event(idev, EV_SYN, SYN_REPORT, 2);
	}
}


static void alps_poll(struct input_polled_dev *dev)
{
	struct input_dev *idev = dev->input;

	mutex_lock(&alps_lock);
	dev->poll_interval = delay;
	/*if (poll_stop_cnt-- < 0) {*/
	/*	poll_stop_cnt = -1;*/
		if (flgM) hscd_poll(idev);
		if (flgA) accsns_poll(idev);
	/*}
	else printk("pollinf stop. delay = %d, poll_stop_cnt = %d\n", delay, poll_stop_cnt);*/
	mutex_unlock(&alps_lock);
}

static int __init alps_init(void)
{
	struct input_dev *idev;
        struct device *dev_t;    
	int ret;

        printk(KERN_INFO "[ALPS] alps_init\n");  

	flgM = 0;
	flgA = 0;
	ret = platform_driver_register(&alps_driver);
	if (ret)
		goto out_region;
	printk(KERN_INFO "[ALPS] platform_driver_register\n");

	pdev = platform_device_register_simple("alps", -1, NULL, 0);
	if (IS_ERR(pdev)) {
		ret = PTR_ERR(pdev);
		goto out_driver;
	}
	printk(KERN_INFO "[ALPS] platform_device_register_simple\n");

	ret = sysfs_create_group(&pdev->dev.kobj, &alps_attribute_group);
	if (ret)
		goto out_device;
	printk(KERN_INFO "[ALPS] sysfs_create_group\n");

	alps_idev = input_allocate_polled_device();
	if (!alps_idev) {
		ret = -ENOMEM;
		goto out_group;
	}
	printk(KERN_INFO "[ALPS] input_allocate_polled_device\n");

	alps_idev->poll = alps_poll;
	alps_idev->poll_interval = ALPS_POLL_INTERVAL;

	/* initialize the input class */
	idev = alps_idev->input;
	idev->name = "magnetic_sensor";
	idev->phys = "alps/input2";
	idev->id.bustype = BUS_HOST;
	idev->dev.parent = &pdev->dev;
	idev->evbit[0] = BIT_MASK(EV_ABS);

#if defined(CONFIG_SENSORS_ACC_12BIT)
	input_set_abs_params(idev, EVENT_TYPE_ACCEL_X,
	        -2048, 2047, ALPS_INPUT_FUZZ, ALPS_INPUT_FLAT);
	input_set_abs_params(idev, EVENT_TYPE_ACCEL_Y,
                -2048, 2047, ALPS_INPUT_FUZZ, ALPS_INPUT_FLAT);
	input_set_abs_params(idev, EVENT_TYPE_ACCEL_Z,
                -2048, 2047, ALPS_INPUT_FUZZ, ALPS_INPUT_FLAT);
#else
	input_set_abs_params(idev, EVENT_TYPE_ACCEL_X, 
	        -512, 511, ALPS_INPUT_FUZZ, ALPS_INPUT_FLAT);
	input_set_abs_params(idev, EVENT_TYPE_ACCEL_Y, 
                -512, 511, ALPS_INPUT_FUZZ, ALPS_INPUT_FLAT);
	input_set_abs_params(idev, EVENT_TYPE_ACCEL_Z, 
                -512, 511, ALPS_INPUT_FUZZ, ALPS_INPUT_FLAT);
#endif

#if defined(CONFIG_SENSORS_MAG_15BIT)
	input_set_abs_params(idev, EVENT_TYPE_MAGV_X,
	        -16384, 16383, ALPS_INPUT_FUZZ, ALPS_INPUT_FLAT);
	input_set_abs_params(idev, EVENT_TYPE_MAGV_Y,
                -16384, 16383, ALPS_INPUT_FUZZ, ALPS_INPUT_FLAT);
	input_set_abs_params(idev, EVENT_TYPE_MAGV_Z,
                -16384, 16383, ALPS_INPUT_FUZZ, ALPS_INPUT_FLAT);
#else
	input_set_abs_params(idev, EVENT_TYPE_MAGV_X, 
	        -4096, 4095, ALPS_INPUT_FUZZ, ALPS_INPUT_FLAT);
	input_set_abs_params(idev, EVENT_TYPE_MAGV_Y, 
                -4096, 4095, ALPS_INPUT_FUZZ, ALPS_INPUT_FLAT);
	input_set_abs_params(idev, EVENT_TYPE_MAGV_Z, 
                -4096, 4095, ALPS_INPUT_FUZZ, ALPS_INPUT_FLAT);
#endif

	ret = input_register_polled_device(alps_idev);
	if (ret)
		goto out_alc_poll;
	printk(KERN_INFO "[ALPS] input_register_polled_device\n");

	ret = misc_register(&alps_device);
	if (ret) {
		printk(KERN_ERR "[ALPS] alps_io_device register failed\n");
		goto out_reg_poll;
	}
	printk(KERN_INFO "[ALPS] misc_register\n");

#if defined(CONFIG_SENSORS_CORE)
        dev_t = device_create( sensors_class, NULL, 0, NULL, "magnetic_sensor");

	if (device_create_file(dev_t, &dev_attr_status) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_status.attr.name);

	if (device_create_file(dev_t, &dev_attr_selftest) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_selftest.attr.name);

	if (device_create_file(dev_t, &dev_attr_adc) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_adc.attr.name);

	if (device_create_file(dev_t, &dev_attr_dac) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_dac.attr.name);

	if (device_create_file(dev_t, &dev_attr_raw_data) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_raw_data.attr.name);

	if (IS_ERR(dev_t)) 	{
		printk(KERN_ERR "[ALPS] device_create_file failed\n");        
		goto out_dev_file;
	}
#endif

	return 0;

out_dev_file:
	misc_deregister(&alps_device);    
	printk(KERN_ERR "[ALPS] misc_deregister\n");    
out_reg_poll:
	input_unregister_polled_device(alps_idev);
	printk(KERN_ERR "[ALPS] input_unregister_polled_device\n");
out_alc_poll:
	input_free_polled_device(alps_idev);
	printk(KERN_ERR "[ALPS] input_free_polled_device\n");
out_group:
	sysfs_remove_group(&pdev->dev.kobj, &alps_attribute_group);
	printk(KERN_ERR "[ALPS] sysfs_remove_group\n");
out_device:
	platform_device_unregister(pdev);
	printk(KERN_ERR "[ALPS] platform_device_unregister\n");
out_driver:
	platform_driver_unregister(&alps_driver);
	printk(KERN_ERR "[ALPS] platform_driver_unregister\n");
out_region:
	return ret;
}

static void __exit alps_exit(void)
{
        printk(KERN_INFO "[ALPS] alps_exit\n");      

#if defined(CONFIG_SENSORS_CORE)    
        device_destroy(sensors_class, 0);    
#endif            
	misc_deregister(&alps_device);
	printk(KERN_INFO "[ALPS] misc_deregister\n");
	input_unregister_polled_device(alps_idev);
	printk(KERN_INFO "[ALPS] input_unregister_polled_device\n");
	input_free_polled_device(alps_idev);
	printk(KERN_INFO "[ALPS] input_free_polled_device\n");
	sysfs_remove_group(&pdev->dev.kobj, &alps_attribute_group);
	printk(KERN_INFO "[ALPS] sysfs_remove_group\n");
	platform_device_unregister(pdev);
	printk(KERN_INFO "[ALPS] platform_device_unregister\n");
	platform_driver_unregister(&alps_driver);
	printk(KERN_INFO "[ALPS] platform_driver_unregister\n");
    
}

module_init(alps_init);
module_exit(alps_exit);

MODULE_DESCRIPTION("Alps Input Device");
MODULE_AUTHOR("ALPS ELECTRIC CO., LTD.");
MODULE_LICENSE("GPL v2");
