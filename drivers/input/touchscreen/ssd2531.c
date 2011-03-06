/* drivers/input/touchscreen/ssd2531.c
 *
 * Copyright (C) 2007 Google, Inc.
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

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/vrpanel.h>
#include <linux/ssd2531.h>
#include <mach/gpio.h>

#define FINGER_NUM_CAP 4

#ifdef swap
#undef swap
#endif
#define swap(x, y) do { typeof(x) z = x; x = y; y = z; } while (0)
#define dim(x) (sizeof(x)/sizeof((x)[0]))

static struct workqueue_struct *ssd2531_wq;

struct ssd2531_reg{
    u8 num;
    u8 addr;
    u8 param[4];
};

struct ssd2531_reg_group {
    struct ssd2531_reg reg;
    int delay;
};

struct ssd2531_finger{
    u16 State;
    u16 Px;
    u16 Py;
    u16 Wx;
    u16 Wy;
    u16 Z;
};

struct ssd2531_data {
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_irq;
    int pin_reset;
	struct hrtimer timer;
	struct work_struct  work;
	int (*power)(int on);
	struct early_suspend early_suspend;
    struct ssd2531_finger finger[FINGER_NUM_CAP];
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ssd2531_early_suspend(struct early_suspend *h);
static void ssd2531_late_resume(struct early_suspend *h);
#endif

struct ssd2531_reg_group ssd2531_poweron_params[] =
{
	{{1, 0x23, {0x00}}      , 10},
	{{1, 0x2b, {0x03}}      , 0},
	{{1, 0x06, {0x0A}}      , 0},
	{{1, 0x07, {0x05}}      , 0},
	{{1, 0x2A, {0x02}}      , 0},
	{{1, 0x2c, {0x02}}      , 0},
	{{1, 0x33, {0x02}}      , 0},
	{{1, 0x34, {0x40}}      , 0},
	{{2, 0x35, {0x00, 0x10}}, 0},
	{{1, 0x36, {0x1E}}      , 0},
	{{1, 0x37, {0x00}}      , 0},
	{{1, 0x38, {0x01}}      , 0},
	{{1, 0x39, {0x00}}      , 0},
	{{1, 0x53, {0x08}}      , 0},
	{{1, 0x54, {0x30}}      , 0},
	{{1, 0x55, {0x30}}      , 0},
	{{1, 0x56, {0x01}}      , 0},
	{{1, 0x59, {0x01}}      , 0},
	{{1, 0x5a, {0x01}}      , 0},
	{{1, 0x5b, {0x05}}      , 0},
	{{1, 0x65, {0x03}}      , 0},
	{{2, 0x7A, {0xff, 0xff}}, 0},
	{{1, 0x7B, {0xfc}}      , 0},
	{{1, 0xc1, {0x0F}}      , 0},
	{{1, 0xd5, {0x0F}}      , 0},
	{{1, 0xd9, {0x01}}      , 0},
	{{1, 0x08, {0x03}}      , 0},
	{{1, 0x09, {0x0A}}      , 0},
	{{1, 0x0A, {0x04}}      , 0},
	{{1, 0x0B, {0x0B}}      , 0},
	{{1, 0x0C, {0x05}}      , 0},
	{{1, 0x0D, {0x0C}}      , 0},
	{{1, 0x0E, {0x06}}      , 0},
	{{1, 0x0F, {0x0D}}      , 0},
	{{1, 0x10, {0x07}}      , 0},
	{{1, 0x11, {0x0E}}      , 0},
	{{1, 0x12, {0x08}}      , 0},
	{{1, 0x13, {0x0F}}      , 0},
	{{1, 0x14, {0x09}}      , 0},
	{{1, 0x15, {0x10}}      , 0},
	{{1, 0x16, {0x12}}      , 0},
	{{1, 0x17, {0x11}}      , 0},
	{{1, 0x25, {0x06}}      , 100},
	{{1, 0xa2, {0x00}}      , 300},
	{{1, 0x00, {0x00}}      , 0},
};

struct ssd2531_reg_group ssd2531_poweroff_params[] =
{
	{{1, 0xc1, {0x00}}      , 50},
	{{1, 0x25, {0x00}}      , 0},
	{{1, 0x24, {0x00}}      , 100},
};

static void ssd2531_dump_finger_state(struct ssd2531_finger *finger, int num) {
    int i = 0;

    return;
    
	printk(KERN_INFO "ssd2531_dump_finger_state\n");
    for (i = 0; i < num; i++) {
    	printk(KERN_INFO "<finger:%d>State:0x%x, Px:%d, Py:%d, Wx:%d, Wy:%d, Z:%d\n",
            i,
            finger[i].State, 
            finger[i].Px, 
            finger[i].Py, 
            finger[i].Wx, 
            finger[i].Wy, 
            finger[i].Z);
    }
    
}

static void ssd2531_dump_buf(const char* str, u8 *buf, u32 size) {
    int i = 0;

    return;
    
    if (str) {
        printk("dump %s\n", str);
    }
    for (i = 0; i < size; i++) {        
        printk("0x%x ", buf[i]);
    }
    printk("\n");
}

static int ssd2531_read(struct ssd2531_data *ts, struct ssd2531_reg *reg) {
    int ret;
    u8 *buf;
	struct i2c_msg msg[2];

    buf = (u8*)reg + 1;
	msg[0].addr = ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;
	msg[1].addr = ts->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = reg->num;
	msg[1].buf = ++buf;
    
	ret = i2c_transfer(ts->client->adapter, msg, 2);
	if (2 != ret) {
		printk(KERN_ERR "ssd2531 read failed\n");
	}
    else {
        ssd2531_dump_buf("read reg", (u8*)reg, sizeof(struct ssd2531_reg));
        ret = 0;
    }
    return ret;
}

static int ssd2531_write(struct ssd2531_data *ts, struct ssd2531_reg *reg) {
    int ret;
    u8 *buf;
	struct i2c_msg msg;    
    
    buf = (u8*)reg + 1;
	msg.addr = ts->client->addr;
	msg.flags = 0;
	msg.len = reg->num + 1;
	msg.buf = buf;

    ssd2531_dump_buf("write i2c msg", msg.buf, msg.len);  
    
	ret = i2c_transfer(ts->client->adapter, &msg, 1);
	if (1 != ret) {
		printk(KERN_ERR "ssd2531 write failed\n");
	}
    else {
        ret = 0;
    }
    return ret;
}

static void ssd2531_recal_pos(struct ssd2531_data *ts, int *x, int *y) {

    *x = *x * 320 / 320;
    *y = *y * 512 / 480;
    
	//printk(KERN_INFO "nx %d, ny %d\n", *x, *y);
}

static void ssd2531_get_finger_data(struct ssd2531_finger *finger, u8 *data) {
    
    finger->Px = data[0];
    finger->Px |= (data[2] >> 4) << 8;
    finger->Py = data[1];
    finger->Py |= (data[2] & 0x0F) << 8;
    
    finger->Wx = (data[3] >> 4);
    finger->Wy = (data[3] >> 4);

    finger->Z = 1;
}

static void ssd2531_scan_finger_state(struct ssd2531_data *ts, u8 status) {
    int i;
    struct ssd2531_reg reg;
    struct ssd2531_finger *finger = ts->finger;

    memset(finger, 0, sizeof(struct ssd2531_finger) * FINGER_NUM_CAP);
    reg.num = 4;
    
    for (i = 0; i < FINGER_NUM_CAP; i++) {
        finger->State = status & 1;
        if (finger->State) {
            reg.addr = 0x7C + i;
            if (ssd2531_read(ts, &reg)) {
            	printk(KERN_ERR "ssd2531 read finger data failed\n");
                break;
            }
            ssd2531_get_finger_data(finger, reg.param);
        }
        status >>= 1;
        finger++;
    }
}

static int ssd2531_get_fingers(const struct ssd2531_finger *all, struct ssd2531_finger *finger, int num) {
    int i, count;

    memset(finger, 0, sizeof(struct ssd2531_finger) * num);
    
    for (i = count = 0; i < FINGER_NUM_CAP; i++) {
        if (all->State) {
            memcpy(finger, all, sizeof(struct ssd2531_finger));
            count++;
            finger++;
            if (count > num) {
                break;
            }
        }
        all++;
    }

    return count;
}

static int ssd2531_send_reg_group(
    struct ssd2531_data *ts, 
    struct ssd2531_reg_group *group, 
    int num)
{
    int ret = 0;
    int i;
    
	printk(KERN_ERR "ssd2531_send_reg_group\n");
    
    for (i = 0; i < num; i++) {
    	printk("num:%d addr:0x%x param[%d]:0x%x delay:%d\n", 
            group[i].reg.num, 
            group[i].reg.addr, 
            (group[i].reg.num - 1),
            group[i].reg.param[(group[i].reg.num - 1)],
            group[i].delay);
        
        ret = ssd2531_write(ts, &(group[i].reg));
        if (ret) {
            if (0x23 != group[i].reg.addr &&
                0x24 != group[i].reg.addr) {
        		printk(KERN_ERR "ssd2531_send_reg_group failed\n");
                break;
            }
        }
        msleep(group[i].delay);
    }

    return ret;
}

static int ssd2531_poweron_panel(struct ssd2531_data *ts)
{   
	int ret;
    struct ssd2531_reg reg;

    msleep(1);
    gpio_direction_output(ts->pin_reset, 0);
    msleep(1);
    gpio_direction_output(ts->pin_reset, 1);

    do {
        
        ret = ssd2531_send_reg_group(ts, ssd2531_poweron_params, dim(ssd2531_poweron_params));
        if (ret) {
            printk(KERN_ERR "ssd2531_send_reg_group failed");
            break;
        }
        
        reg.addr = 2;
        reg.num = 2;
        ret = ssd2531_read(ts, &reg);

        if (ret) {
            printk(KERN_ERR "ssd2531 read device ID failed");
            break;
        }
        else {       
            printk("ssd2531 device ID: 0x%x%x", reg.param[0], reg.param[1]);
        }

        if (reg.param[0] != 0x25 || reg.param[1] != 0x31) {
            ret = -1;
        }
        
    } while(0);
    
	return ret;
}

static int ssd2531_poweroff_panel(struct ssd2531_data *ts)
{   
	int ret;
    do {
        
        ret = ssd2531_send_reg_group(ts, ssd2531_poweroff_params, dim(ssd2531_poweroff_params));
        if (ret) {
            printk(KERN_ERR "ssd2531_send_reg_group failed");
            break;
        }
    } while(0);
    
	return ret;
}

static void ssd2531_report(
    struct ssd2531_data *ts, 
    struct ssd2531_finger *finger,
    int num)
{
    int invalid_x = 0, invalid_y = 0;
    int pos[num][2];
    int f, z = 0, w = 0;
    static int s_num;
    
    for (f = 0; f < num; f++) {
        pos[f][0] = finger[f].Px;
        pos[f][1] = finger[f].Py;
        ssd2531_recal_pos(ts, &pos[f][0], &pos[f][1]);
    }
    
    if (s_num > num) {
        for (f = 0; f < (s_num - num); f++) {
            input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, z);
            input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
            input_mt_sync(ts->input_dev);
        }
    }
    
    for (f = s_num = 0; f < num; f++) {
        if (finger[f].State) {
            if (pos[f][1] < 480) {
            	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, pos[f][0]);
            	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, pos[f][1]);
                z = finger[f].Z;
                w = (finger[f].Wx + finger[f].Wy) / 2;
                s_num++;
            }
            else {
                invalid_x = pos[f][0];
                invalid_y = pos[f][1];
            }
        }
        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, z);
        input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
        input_mt_sync(ts->input_dev);
    }
    input_sync(ts->input_dev);
    
#ifdef CONFIG_TOUCHSCREEN_VRPANEL
    if (num > s_num) {
        VrpCallback((TouchSampleValidFlag|TouchSampleDownFlag), invalid_x, invalid_y);
    }
    else {
		VrpCallback(TouchSampleValidFlag, 0, 0);
    }
#endif
}

static int ssd2351_get_event_status(struct ssd2531_data *ts, u8 *status) {
	int ret;
    struct ssd2531_reg reg;
    
    reg.num = 1;
    reg.addr = 0x79;
    ret = ssd2531_read(ts, &reg);
    if (0 == ret) {
        *status = reg.param[0];
    }
    return ret;
}

static void ssd2531_work_func(struct work_struct *work)
{
	int ret, num;
    u8 status;
    static u8 s_status;
    struct ssd2531_finger finger[2];
	struct ssd2531_data *ts = container_of(work, struct ssd2531_data, work);

    do {
        ret = ssd2351_get_event_status(ts, &status);
        if (ret) {
            printk(KERN_ERR "ssd2351_get_event_status failed");
            break;
        }
        if (0 == status &&
            0 == s_status) {
            break;
        }
        //printk("event status 0x%x\n", status);
        s_status = status;
        ssd2531_scan_finger_state(ts, status);
        num = ssd2531_get_fingers(ts->finger, finger, dim(finger));
        ssd2531_dump_finger_state(finger, dim(finger));
        ssd2531_report(ts, finger, num);
        
    } while(1);
	if (ts->use_irq)
		enable_irq(ts->client->irq);
}

static enum hrtimer_restart ssd2531_timer_func(struct hrtimer *timer)
{
	struct ssd2531_data *ts = container_of(timer, struct ssd2531_data, timer);
	/* printk("ssd2531_timer_func\n"); */

	queue_work(ssd2531_wq, &ts->work);

	hrtimer_start(&ts->timer, ktime_set(0, 12500000), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

static irqreturn_t ssd2531_irq_handler(int irq, void *dev_id)
{
	struct ssd2531_data *ts = dev_id;

	/* printk("ssd2531_irq_handler\n"); */
	disable_irq_nosync(ts->client->irq);
	queue_work(ssd2531_wq, &ts->work);
	return IRQ_HANDLED;
}

static int ssd2531_probe(
	struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ssd2531_data *ts;
	struct ssd2531_platform_data *pdata;
	int ret = 0;

	printk(KERN_INFO "ssd2531_probe\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "ssd2531_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
	INIT_WORK(&ts->work, ssd2531_work_func);
	ts->client = client;
	i2c_set_clientdata(client, ts);
	pdata = client->dev.platform_data;
	if (pdata)
		ts->power = pdata->power;
	if (ts->power) {
		ret = ts->power(1);
		if (ret < 0) {
			printk(KERN_ERR "ssd2531_probe power on failed\n");
			goto err_power_failed;
		}
	}
    
    ts->pin_reset = pdata->pin_reset;
    ret = gpio_request(ts->pin_reset, "ssd2531_reset");
    if (ret) {
        printk(KERN_ERR "gpio request reset failed\n");
        goto err_requeset_reset;
    }
    gpio_tlmm_config(GPIO_CFG(ts->pin_reset, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_16MA), GPIO_ENABLE);
    
	ret = ssd2531_poweron_panel(ts);
	if (ret < 0) {
		printk(KERN_ERR "ssd2531_poweron_panel failed\n");
		goto err_detect_failed;
	}

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		printk(KERN_ERR "ssd2531_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	ts->input_dev->name = "ssd2531-touchscreen";
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(EV_ABS, ts->input_dev->evbit);
    
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, 320, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, 480, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 15, 0, 0);
	/* ts->input_dev->name = ts->keypad_info->name; */
	ret = input_register_device(ts->input_dev);
	if (ret) {
		printk(KERN_ERR "ssd2531_probe: Unable to register %s input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	}
    client->irq = 0;
	if (client->irq) {
		ret = request_irq(client->irq, ssd2531_irq_handler, 0, client->name, ts);
		if (ret == 0)
			ts->use_irq = 1;
		else
			dev_err(&client->dev, "request_irq failed\n");
	}
	if (!ts->use_irq) {
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = ssd2531_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = ssd2531_early_suspend;
	ts->early_suspend.resume = ssd2531_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	printk(KERN_INFO "ssd2531_probe: Start touchscreen %s in %s mode\n", ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");

#ifdef CONFIG_TOUCHSCREEN_VRPANEL
	VrpInit(ts->input_dev, NULL);
#endif

	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
err_detect_failed:
    gpio_free(ts->pin_reset);
err_requeset_reset:    
	if (ts->power) {
		ts->power(0);
	}
err_power_failed:
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
	return ret;
}

static int ssd2531_remove(struct i2c_client *client)
{
	struct ssd2531_data *ts = i2c_get_clientdata(client);
	unregister_early_suspend(&ts->early_suspend);
	if (ts->use_irq)
		free_irq(client->irq, ts);
	else
		hrtimer_cancel(&ts->timer);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	return 0;
}

static int ssd2531_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct ssd2531_data *ts = i2c_get_clientdata(client);

	if (ts->use_irq)
		disable_irq(client->irq);
	else
		hrtimer_cancel(&ts->timer);
	ret = cancel_work_sync(&ts->work);
	if (ret && ts->use_irq) /* if work was pending disable-count is now 2 */
		enable_irq(client->irq);

	ret = ssd2531_poweroff_panel(ts);
	if (ret < 0)
		printk(KERN_ERR "ssd2531_poweroff_panel failed\n");
    
	if (ts->power) {
		ret = ts->power(0);
		if (ret < 0)
			printk(KERN_ERR "ssd2531_resume power off failed\n");
	}
	return 0;
}

static int ssd2531_resume(struct i2c_client *client)
{
	int ret;
	struct ssd2531_data *ts = i2c_get_clientdata(client);

	if (ts->power) {
		ret = ts->power(1);
		if (ret < 0)
			printk(KERN_ERR "ssd2531_resume power on failed\n");
	}
    
	ret = ssd2531_poweron_panel(ts);
	if (ret < 0) {
		printk(KERN_ERR "ssd2531_poweron_panel failed\n");
	}

	if (ts->use_irq)
		enable_irq(client->irq);

	if (!ts->use_irq)
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ssd2531_early_suspend(struct early_suspend *h)
{
	struct ssd2531_data *ts;
	ts = container_of(h, struct ssd2531_data, early_suspend);
	ssd2531_suspend(ts->client, PMSG_SUSPEND);
}

static void ssd2531_late_resume(struct early_suspend *h)
{
	struct ssd2531_data *ts;
	ts = container_of(h, struct ssd2531_data, early_suspend);
	ssd2531_resume(ts->client);
}
#endif

static const struct i2c_device_id ssd2531_id[] = {
	{ SSD2531_NAME, 0 },
	{ }
};

static struct i2c_driver ssd2531_driver = {
	.probe		= ssd2531_probe,
	.remove		= ssd2531_remove,
	.id_table	= ssd2531_id,
	.driver = {
		.name	= SSD2531_NAME,
	},
};

static int __devinit ssd2531_init(void)
{
	ssd2531_wq = create_singlethread_workqueue("ssd2531_wq");
	if (!ssd2531_wq)
		return -ENOMEM;
	return i2c_add_driver(&ssd2531_driver);
}

static void __exit ssd2531_exit(void)
{
	i2c_del_driver(&ssd2531_driver);
	if (ssd2531_wq)
		destroy_workqueue(ssd2531_wq);
}

module_init(ssd2531_init);
module_exit(ssd2531_exit);

MODULE_DESCRIPTION("SSD2531 Touchscreen Driver");
MODULE_LICENSE("GPL");
