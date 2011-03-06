/* drivers/input/touchscreen/msm_touch.c
 *
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
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
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/io.h>
#include <linux/vrpanel.h>
#include <mach/msm_touch.h>
#include "tch_cal/touchp.h"

/* HW register map */
#define TSSC_CTL_REG      0x100
#define TSSC_SI_REG       0x108
#define TSSC_OPN_REG      0x104
#define TSSC_STATUS_REG   0x10C
#define TSSC_AVG12_REG    0x110

/* status bits */
#define TSSC_STS_OPN_SHIFT 0x6
#define TSSC_STS_OPN_BMSK  0x1C0
#define TSSC_STS_NUMSAMP_SHFT 0x1
#define TSSC_STS_NUMSAMP_BMSK 0x3E

/* CTL bits */
#define TSSC_CTL_EN		(0x1 << 0)
#define TSSC_CTL_SW_RESET	(0x1 << 2)
#define TSSC_CTL_MASTER_MODE	(0x3 << 3)
#define TSSC_CTL_AVG_EN		(0x1 << 5)
#define TSSC_CTL_DEB_EN		(0x1 << 6)
#define TSSC_CTL_DEB_12_MS	(0x2 << 7)	/* 1.2 ms */
#define TSSC_CTL_DEB_16_MS	(0x3 << 7)	/* 1.6 ms */
#define TSSC_CTL_DEB_2_MS	(0x4 << 7)	/* 2 ms */
#define TSSC_CTL_DEB_3_MS	(0x5 << 7)	/* 3 ms */
#define TSSC_CTL_DEB_4_MS	(0x6 << 7)	/* 4 ms */
#define TSSC_CTL_DEB_6_MS	(0x7 << 7)	/* 6 ms */
#define TSSC_CTL_INTR_FLAG1	(0x1 << 10)
#define TSSC_CTL_DATA		(0x1 << 11)
#define TSSC_CTL_SSBI_CTRL_EN	(0x1 << 13)

/* control reg's default state */
#define TSSC_CTL_STATE	  ( \
		TSSC_CTL_DEB_12_MS | \
		TSSC_CTL_DEB_EN | \
		TSSC_CTL_AVG_EN | \
		TSSC_CTL_MASTER_MODE | \
		TSSC_CTL_EN)

#define TSSC_NUMBER_OF_OPERATIONS 2
#define TS_PENUP_TIMEOUT_MS 10

#define TS_DRIVER_NAME "msm_touchscreen"

#ifdef CONFIG_TOUCHSCREEN_VRPANEL
	#ifdef CONFIG_TOUCHSCREEN_VRPANEL_THREE_KEY
	#define Y_MAX_VRP (Y_MAX+85)
   	#endif
	
	#ifdef CONFIG_TOUCHSCREEN_VRPANEL_FOUR_KEY
	#define Y_MAX_VRP (Y_MAX+65)
	#endif
	
	#ifdef CONFIG_TOUCHSCREEN_VRPANEL_WVGA_FOUR_KEY
	#define Y_MAX_VRP (Y_MAX+65)
	#endif

	#ifdef CONFIG_TOUCHSCREEN_VRPANEL_SHARP_WVGA_FOUR_KEY
	#define Y_MAX_VRP (Y_MAX+65)
	#endif
#endif

#if defined(CONFIG_FB_MSM_LCDC_ILI9418_HVGA_PANEL) || defined(CONFIG_FB_MSM_LCDC_HVGA)
	#define X_MAX	320
	#define Y_MAX	480
	#define TOUCH_X_MAX	1024
	#define TOUCH_Y_MAX	1024
#else
	#define X_MAX	480
	#define Y_MAX	800
	#define TOUCH_X_MAX	1024
	#define TOUCH_Y_MAX	1024
#endif

#define P_MAX	255
#define CALIBRATION_NUM 5

enum cal_state_t {
    CAL_FAILED,
    CAL_UNCAL,
    CAL_CALED,
    CAL_CALING,
};

struct ts {
	struct input_dev *input;
	struct timer_list timer;
	int irq;
	unsigned int x_max;
	unsigned int y_max;
};

struct cal_param_t {
    int raw_x[CALIBRATION_NUM];
    int raw_y[CALIBRATION_NUM];
    int ref_x[CALIBRATION_NUM];
    int ref_y[CALIBRATION_NUM];
};

static enum cal_state_t ts_cal_state;
static struct cal_param_t ts_cal_param;
static int ts_raw_x;
static int ts_raw_y;
static bool vrp_state = true;
static void __iomem *virt;
#define TSSC_REG(reg) (virt + TSSC_##reg##_REG)

static void ts_update_pen_state(struct ts *ts, int x, int y, int pressure)
{
	//printk("%s:  lx = %d,ly = %d, pressure = %d\n", __func__, x, y, pressure);
	
	if (pressure) {
		u32 lcd_x, lcd_y;
		ts_raw_x = x;
        ts_raw_y = y;
#ifdef CONFIG_TOUCHSCREEN_VRPANEL

        if (CAL_FAILED == ts_cal_state ||
            CAL_CALING == ts_cal_state) {
    	#ifdef CONFIG_TOUCHSCREEN_VRPANEL_THREE_KEY
    		lcd_x = (-23697*y + 22393665)/65536;
    		lcd_y = (-38370*x + 37987110)/65536;
    	#endif

    	#ifdef CONFIG_TOUCHSCREEN_VRPANEL_FOUR_KEY
    		lcd_x = (24385*x - 1950839)/65536;
    		lcd_y = (37205*y - 1116160)/65536;
    	#endif

    	#ifdef CONFIG_TOUCHSCREEN_VRPANEL_WVGA_FOUR_KEY
    		lcd_x = (35747*x - 2502284)/65536;
    		lcd_y = (58231*y - 2329259)/65536;
    	#endif

    	#ifdef CONFIG_TOUCHSCREEN_VRPANEL_SHARP_WVGA_FOUR_KEY
    		lcd_x = (35951*x - 2696338)/65536;
    		lcd_y = (58745*y - 1762341)/65536;
    	#endif
        
    		if (lcd_x > X_MAX) lcd_x = X_MAX;
            
       		lcd_y = ((lcd_y < Y_MAX_VRP) ? lcd_y : Y_MAX_VRP);
    		lcd_y = Y_MAX_VRP - lcd_y;
        }
		//printk("[ts to lcd] x:%d, y:%d \n", lcd_x, lcd_y);

        switch (ts_cal_state) {
        case CAL_UNCAL:
            ts_cal_state = CAL_FAILED;
            if (!TouchPanelSetCalibration(
                CALIBRATION_NUM, 
                ts_cal_param.ref_x,
                ts_cal_param.ref_y,
                ts_cal_param.raw_x,
                ts_cal_param.raw_y))
            {
                printk(KERN_ERR "TouchPanelSetCalibration fail\n");
                break;
            }            
            ts_cal_state = CAL_CALED;
        case CAL_CALED:
            TouchPanelCalibrateAPoint(x, y, &lcd_x, &lcd_y);
        case CAL_CALING:
            if (CAL_CALING == ts_cal_state) {
                x = lcd_x * TOUCH_X_MAX / X_MAX;
                y = lcd_y * TOUCH_Y_MAX / Y_MAX;
            }
        case CAL_FAILED:
            if (vrp_state) {
    		    VrpCallback((TouchSampleValidFlag|TouchSampleDownFlag), lcd_x, lcd_y);
            }
            break;
        default:
            ts_cal_state = ts_cal_state;
        }
        
		if (lcd_y > (Y_MAX + 10))
		{
			input_report_abs(ts->input, ABS_PRESSURE, 0);
			input_report_key(ts->input, BTN_TOUCH, 0);
		}
		else
		{
			//printk("[ts to lcd] x:%d, y:%d \n", lcd_x, lcd_y);
			input_report_abs(ts->input, ABS_X, x & 0xfff);
			input_report_abs(ts->input, ABS_Y, y & 0xfff);
			input_report_abs(ts->input, ABS_PRESSURE, 255);
			input_report_key(ts->input, BTN_TOUCH, 1);
		}
#else

		lcd_x = (36157*x - 2530990)/65536;
		lcd_y = (55188*y - 1655640)/65536;

		if (lcd_x > X_MAX) lcd_x = X_MAX;
		if (lcd_y > Y_MAX) lcd_y = Y_MAX;

		lcd_y = Y_MAX - lcd_y;

	//	printk("[ts to lcd] x:%d, y:%d \n", lcd_x, lcd_y);

		input_report_abs(ts->input, ABS_X, lcd_x & 0xfff);
		input_report_abs(ts->input, ABS_Y, lcd_y & 0xfff);

		input_report_abs(ts->input, ABS_PRESSURE, 255);
		input_report_key(ts->input, BTN_TOUCH, 1);
#endif
	
	} else {
#ifdef CONFIG_TOUCHSCREEN_VRPANEL
		VrpCallback(TouchSampleValidFlag, 0, 0);
#endif
		input_report_abs(ts->input, ABS_PRESSURE, 0);
		input_report_key(ts->input, BTN_TOUCH, 0);
	}

	input_sync(ts->input);
}

static void ts_timer(unsigned long arg)
{
	struct ts *ts = (struct ts *)arg;

	//mdelay(100);
	//printk("****ts_timer **** \n");
	ts_update_pen_state(ts, 0, 0, 0);
}

static irqreturn_t ts_interrupt(int irq, void *dev_id)
{
	u32 avgs, x, y, lx, ly;
	u32 num_op, num_samp;
	u32 status;

	struct ts *ts = dev_id;

	status = readl(TSSC_REG(STATUS));
	avgs = readl(TSSC_REG(AVG12));
	x = avgs & 0xFFFF;
	y = avgs >> 16;

	/* For pen down make sure that the data just read is still valid.
	 * The DATA bit will still be set if the ARM9 hasn't clobbered
	 * the TSSC. If it's not set, then it doesn't need to be cleared
	 * here, so just return.
	 */
	if (!(readl(TSSC_REG(CTL)) & TSSC_CTL_DATA))
		goto out;

	/* Data has been read, OK to clear the data flag */
	writel(TSSC_CTL_STATE, TSSC_REG(CTL));

	/* Valid samples are indicated by the sample number in the status
	 * register being the number of expected samples and the number of
	 * samples collected being zero (this check is due to ADC contention).
	 */
	num_op = (status & TSSC_STS_OPN_BMSK) >> TSSC_STS_OPN_SHIFT;
	num_samp = (status & TSSC_STS_NUMSAMP_BMSK) >> TSSC_STS_NUMSAMP_SHFT;

	if ((num_op == TSSC_NUMBER_OF_OPERATIONS) && (num_samp == 0)) {
		/* TSSC can do Z axis measurment, but driver doesn't support
		 * this yet.
		 */

		/*
		 * REMOVE THIS:
		 * These x, y co-ordinates adjustments will be removed once
		 * Android framework adds calibration framework.
		 */
//#ifdef CONFIG_ANDROID_TOUCHSCREEN_MSM_HACKS
//		lx = ts->x_max - x;
//		ly = ts->y_max - y;
//#else
		lx = x;
		ly = y;
//#endif
		ts_update_pen_state(ts, lx, ly, 255);
		/* kick pen up timer - to make sure it expires again(!) */
		mod_timer(&ts->timer,
			jiffies + TS_PENUP_TIMEOUT_MS);//msecs_to_jiffies(TS_PENUP_TIMEOUT_MS));

	} else
		printk(KERN_INFO "Ignored interrupt: {%3d, %3d},"
				" op = %3d samp = %3d\n",
				 x, y, num_op, num_samp);

out:
	return IRQ_HANDLED;
}

static ssize_t ts_raw_x_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", ts_raw_x);
}
static DEVICE_ATTR(raw_x, (S_IRUGO), ts_raw_x_show, NULL);

static ssize_t ts_raw_y_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", ts_raw_y);
}
static DEVICE_ATTR(raw_y, (S_IRUGO), ts_raw_y_show, NULL);

static ssize_t ts_cal_state_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", ts_cal_state);
}

static ssize_t ts_cal_state_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
    int state;
    int rc = 0;

    rc = sscanf(buf, "%d", &state);
    if (1 != rc) {
        printk("Invalid arguments. Usage: <state>\n");
        rc = -EINVAL;
        return rc;
    }

    printk("ts_cal_state_store %d \n", state);

    ts_cal_state = state;
    
    return 0;
}

static DEVICE_ATTR(cal_state, (S_IRUGO|S_IWUGO), ts_cal_state_show, ts_cal_state_store);

static ssize_t ts_cal_param_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, 
        "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", 
        ts_cal_param.raw_x[0],ts_cal_param.raw_y[0],
        ts_cal_param.raw_x[1],ts_cal_param.raw_y[1],
        ts_cal_param.raw_x[2],ts_cal_param.raw_y[2],
        ts_cal_param.raw_x[3],ts_cal_param.raw_y[3],
        ts_cal_param.raw_x[4],ts_cal_param.raw_y[4],
        ts_cal_param.ref_x[0],ts_cal_param.ref_y[0],
        ts_cal_param.ref_x[1],ts_cal_param.ref_y[1],
        ts_cal_param.ref_x[2],ts_cal_param.ref_y[2],
        ts_cal_param.ref_x[3],ts_cal_param.ref_y[3],
        ts_cal_param.ref_x[4],ts_cal_param.ref_y[4]);
}

static ssize_t ts_cal_param_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
    int rc = 0;

    rc = sscanf(buf, 
        "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", 
        &ts_cal_param.raw_x[0],&ts_cal_param.raw_y[0],
        &ts_cal_param.raw_x[1],&ts_cal_param.raw_y[1],
        &ts_cal_param.raw_x[2],&ts_cal_param.raw_y[2],
        &ts_cal_param.raw_x[3],&ts_cal_param.raw_y[3],
        &ts_cal_param.raw_x[4],&ts_cal_param.raw_y[4],
        &ts_cal_param.ref_x[0],&ts_cal_param.ref_y[0],
        &ts_cal_param.ref_x[1],&ts_cal_param.ref_y[1],
        &ts_cal_param.ref_x[2],&ts_cal_param.ref_y[2],
        &ts_cal_param.ref_x[3],&ts_cal_param.ref_y[3],
        &ts_cal_param.ref_x[4],&ts_cal_param.ref_y[4]);
    if (20 != rc) {
        printk("Invalid arguments. Usage: <raw_x[0] raw_y[0] ... ref_x[0] ref_y[0] ...>\n");
        rc = -EINVAL;
        return rc;
    }
    
    return 0;
}

static DEVICE_ATTR(cal_param, (S_IRUGO|S_IWUGO), ts_cal_param_show, ts_cal_param_store);

static ssize_t vrp_state_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", vrp_state);
}

static ssize_t vrp_state_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
    int state;
    int rc = 0;

    rc = sscanf(buf, "%d", &state);
    if (1 != rc) {
        printk("Invalid arguments. Usage: <state>\n");
        rc = -EINVAL;
        return rc;
    }

    printk("vrp_state_store %d \n", state);

    vrp_state = state;
    
    return 0;
}

static DEVICE_ATTR(vrp_state, (S_IRUGO|S_IWUGO), vrp_state_show, vrp_state_store);

static int __devinit ts_probe(struct platform_device *pdev)
{
	int result, status;
	struct input_dev *input_dev;
	struct resource *res, *ioarea;
	struct ts *ts;
	unsigned int x_max, y_max, pressure_max;
	struct msm_ts_platform_data *pdata = pdev->dev.platform_data;

	/* The primary initialization of the TS Hardware
	 * is taken care of by the ADC code on the modem side
	 */

	ts = kzalloc(sizeof(struct ts), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!input_dev || !ts) {
		result = -ENOMEM;
		goto fail_alloc_mem;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Cannot get IORESOURCE_MEM\n");
		result = -ENOENT;
		goto fail_alloc_mem;
	}

	ts->irq = platform_get_irq(pdev, 0);
	if (!ts->irq) {
		dev_err(&pdev->dev, "Could not get IORESOURCE_IRQ\n");
		result = -ENODEV;
		goto fail_alloc_mem;
	}

	ioarea = request_mem_region(res->start, resource_size(res), pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "Could not allocate io region\n");
		result = -EBUSY;
		goto fail_alloc_mem;
	}

	virt = ioremap(res->start, resource_size(res));
	if (!virt) {
		dev_err(&pdev->dev, "Could not ioremap region\n");
		result = -ENOMEM;
		goto fail_ioremap;
	}

	input_dev->name = TS_DRIVER_NAME;
	input_dev->phys = "msm_touch/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0002;
	input_dev->id.version = 0x0100;
	input_dev->dev.parent = &pdev->dev;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) | BIT_MASK(EV_SYN);
	input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
	input_dev->absbit[BIT_WORD(ABS_MISC)] = BIT_MASK(ABS_MISC);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	if (pdata) {
		x_max = pdata->x_max ? : TOUCH_X_MAX;
		y_max = pdata->y_max ? : TOUCH_Y_MAX;
		pressure_max = pdata->pressure_max ? : P_MAX;
	} else {
		x_max = TOUCH_X_MAX;
		y_max = TOUCH_Y_MAX;
		pressure_max = P_MAX;
	}
	printk("x_max = %d,y_max = %d, pressure_max = %d\n",  x_max, y_max, pressure_max);
	ts->x_max = x_max;
	ts->y_max = y_max;

	input_set_abs_params(input_dev, ABS_X, 0, x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_TOOL_WIDTH, 0, 15, 0, 0);

	writel(0x4, TSSC_REG(SI));
	status = readl(TSSC_REG(SI));
	printk("touch SI = 0x%x\n",  status);

	result = input_register_device(input_dev);
	if (result)
		goto fail_ip_reg;

	ts->input = input_dev;

	setup_timer(&ts->timer, ts_timer, (unsigned long)ts);
	result = request_irq(ts->irq, ts_interrupt, IRQF_TRIGGER_RISING,
				 "touchscreen", ts);
	if (result)
		goto fail_req_irq;

	platform_set_drvdata(pdev, ts);
#ifdef CONFIG_TOUCHSCREEN_VRPANEL
	VrpInit(input_dev, NULL);
#endif

    result = device_create_file(&pdev->dev, &dev_attr_cal_state);
    if (result) {
        printk(KERN_ERR "msm_touch can't create cal_state file\n");
        goto fail_cal_state;
    }

    result = device_create_file(&pdev->dev, &dev_attr_cal_param);
    if (result) {
        printk(KERN_ERR "msm_touch can't create cal_param file\n");
        goto fail_cal_param;
    }

    result = device_create_file(&pdev->dev, &dev_attr_vrp_state);
    if (result) {
        printk(KERN_ERR "msm_touch can't create vrp_state file\n");
        goto fail_vrp_state;
    }

    result = device_create_file(&pdev->dev, &dev_attr_raw_x);
    if (result) {
        printk(KERN_ERR "msm_touch can't create raw_x file\n");
        goto fail_raw_x;
    }

    result = device_create_file(&pdev->dev, &dev_attr_raw_y);
    if (result) {
        printk(KERN_ERR "msm_touch can't create raw_y file\n");
        goto fail_raw_y;
    }

	return 0;

fail_raw_y:
    device_remove_file(&pdev->dev, &dev_attr_raw_x); 
fail_raw_x:
    device_remove_file(&pdev->dev, &dev_attr_vrp_state); 
fail_vrp_state:    
    device_remove_file(&pdev->dev, &dev_attr_cal_param);
fail_cal_param:
    device_remove_file(&pdev->dev, &dev_attr_cal_state);
fail_cal_state:
	free_irq(ts->irq, ts);
fail_req_irq:
	input_unregister_device(input_dev);
	input_dev = NULL;
fail_ip_reg:
	iounmap(virt);
fail_ioremap:
	release_mem_region(res->start, resource_size(res));
fail_alloc_mem:
	input_free_device(input_dev);
	kfree(ts);
	return result;
}

static int __devexit ts_remove(struct platform_device *pdev)
{
	struct resource *res;
	struct ts *ts = platform_get_drvdata(pdev);

    device_remove_file(&pdev->dev, &dev_attr_raw_y); 
    device_remove_file(&pdev->dev, &dev_attr_raw_x); 
    device_remove_file(&pdev->dev, &dev_attr_vrp_state);
    device_remove_file(&pdev->dev, &dev_attr_cal_param);
    device_remove_file(&pdev->dev, &dev_attr_cal_state);
    
	free_irq(ts->irq, ts);
	del_timer_sync(&ts->timer);

	input_unregister_device(ts->input);
	iounmap(virt);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(res->start, resource_size(res));
	platform_set_drvdata(pdev, NULL);
	kfree(ts);

	return 0;
}

static struct platform_driver ts_driver = {
	.probe		= ts_probe,
	.remove		= __devexit_p(ts_remove),
	.driver		= {
		.name = TS_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init ts_init(void)
{
	return platform_driver_register(&ts_driver);
}
module_init(ts_init);

static void __exit ts_exit(void)
{
	platform_driver_unregister(&ts_driver);
}
module_exit(ts_exit);

MODULE_DESCRIPTION("MSM Touch Screen driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:msm_touchscreen");
