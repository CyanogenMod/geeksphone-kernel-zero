/* drivers/input/keyboard/synaptics_i2c_rmi.c
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
#include <linux/synaptics_i2c_rmi.h>
#include <linux/vrpanel.h>
#include <linux/gpio.h>

#define FINGER_NUM_CAP 5

#ifdef swap
#undef swap
#endif
#define swap(x, y) do { typeof(x) z = x; x = y; y = z; } while (0)
#define dim(x) (sizeof(x)/sizeof((x)[0]))

static struct workqueue_struct *synaptics_wq;

struct synaptics_ts_finger{
    u16 State;
    u16 Px;
    u16 Py;
    u16 Wx;
    u16 Wy;
    u16 Z;
};

struct synaptics_ts_data {
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_irq;
	struct hrtimer timer;
	struct work_struct  work;
	uint16_t max[2];
	int snap_state[2][2];
	int snap_down_on[2];
	int snap_down_off[2];
	int snap_up_on[2];
	int snap_up_off[2];
	int snap_down[2];
	int snap_up[2];
	uint32_t flags;
	int (*power)(int on);
	struct early_suspend early_suspend;
    struct synaptics_ts_finger finger[12];
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_ts_early_suspend(struct early_suspend *h);
static void synaptics_ts_late_resume(struct early_suspend *h);
#endif

#if 0
static int synaptics_dump_page(struct i2c_client *client) {
    int i, ret;
    
	printk(KERN_INFO "synaptics_dump_page\n");
    for (i = 0; i < 0x100; i++) {
        ret = i2c_smbus_read_byte_data(client, i);
	if (ret < 0) {
    		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");
    		goto err_detect_failed;
	}
    	printk(KERN_INFO "reg:0x%02x, val:0x%02x\n", i, ret);
    }
    ret = 0;
err_detect_failed:
    return ret;
}

static int synaptics_dump_f11_data(struct i2c_client *client) {
    int i, ret;

	printk(KERN_INFO "synaptics_dump_f11_data\n");
    for (i = 0x15; i < 0x4f; i++) {
        ret = i2c_smbus_read_byte_data(client, i);
    	if (ret < 0) {
    		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");
    		goto err_detect_failed;
    	}
    	printk(KERN_INFO "reg:0x%02x, val:0x%02x\n", i, ret);
    }
    ret = 0;
err_detect_failed:
	return ret;
}

static void synaptics_dump_finger_state(struct synaptics_ts_finger *finger, int num) {
	int i;

	printk(KERN_INFO "synaptics_dump_finger_state\n");
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
#endif

static void synaptics_recal_pos(struct synaptics_ts_data *ts, int *x, int *y) {

    *x = *x * 320 / ts->max[0];
    *y = *y * 540 / ts->max[1];
    
	//printk(KERN_INFO "nx %d, ny %d\n", *x, *y);
}

static void synaptics_get_finger_data(struct synaptics_ts_finger *finger, u8 *data) {
    finger->Px = (data[0] << 4);
    finger->Px |= (data[2] & 0x0F);
    finger->Py = (data[1] << 4);
    finger->Py |= (data[2] >> 4);
    
    finger->Wx = (data[3] & 0x0F);
    finger->Wy = (data[3] >> 4);

    finger->Z = data[4];
}

static void synaptics_scan_finger_state(struct synaptics_ts_finger *finger, u8 *data) {
    int i;
    u32 state;

    memset(finger, 0, sizeof(struct synaptics_ts_finger) * FINGER_NUM_CAP);
    
    state = 0;
    for (i = 1; i >= 0; i--) {
        state <<= 8;
        state |= data[i];
			}

    data = data + 2;
    for (i = 0; i < FINGER_NUM_CAP; i++) {
        finger->State = (state & 0x3);
        if (1 == finger->State ||
            2 == finger->State) {
            synaptics_get_finger_data(finger, data);
        }
        state >>= 2;
        data += 5;
        finger++;
    }
}

static int synaptics_get_fingers(const struct synaptics_ts_finger *all, struct synaptics_ts_finger *finger, int num) {
    int i, count;

    memset(finger, 0, sizeof(struct synaptics_ts_finger) * num);
    
    for (i = count = 0; i < FINGER_NUM_CAP; i++) {
        if (1 == all->State) {
            memcpy(finger, all, sizeof(struct synaptics_ts_finger));
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

static int synaptics_init_panel(struct synaptics_ts_data *ts)
{
	int ret;
    
	ret = i2c_smbus_write_byte_data(ts->client, 0x39, 0x80); /* normal operation, 80 reports per second */
	if (ret < 0)
		printk(KERN_ERR "synaptics_ts_resume: i2c_smbus_write_byte_data failed\n");
	return ret;
}

static void synaptics_ts_report(
    struct synaptics_ts_data *ts, 
    struct synaptics_ts_finger *finger,
    int num)
{
#ifdef CONFIG_TOUCHSCREEN_VRPANEL
    int invalid_x = 0, invalid_y = 0;
#endif
    int pos[num][2];
				int f, a;
    int z = 0, w = 0;
    static int s_num;

    for (f = 0; f < num; f++) {
					uint32_t flip_flag = SYNAPTICS_FLIP_X;
					for (a = 0; a < 2; a++) {
    		int p = a ? finger[f].Py : finger[f].Px;
						if (ts->flags & flip_flag)
							p = ts->max[a] - p;
						if (ts->flags & SYNAPTICS_SNAP_TO_INACTIVE_EDGE) {
							if (ts->snap_state[f][a]) {
								if (p <= ts->snap_down_off[a])
									p = ts->snap_down[a];
								else if (p >= ts->snap_up_off[a])
									p = ts->snap_up[a];
								else
									ts->snap_state[f][a] = 0;
							} else {
								if (p <= ts->snap_down_on[a]) {
									p = ts->snap_down[a];
									ts->snap_state[f][a] = 1;
								} else if (p >= ts->snap_up_on[a]) {
									p = ts->snap_up[a];
									ts->snap_state[f][a] = 1;
								}
							}
						}
						pos[f][a] = p;
						flip_flag <<= 1;
					}
					if (ts->flags & SYNAPTICS_SWAP_XY)
						swap(pos[f][0], pos[f][1]);
        synaptics_recal_pos(ts, &pos[f][0], &pos[f][1]);
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
#ifdef CONFIG_TOUCHSCREEN_VRPANEL
            if (pos[f][1] < 480) {
#endif
            	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, pos[f][0]);
            	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, pos[f][1]);
                z = finger[f].Z;
                w = (finger[f].Wx + finger[f].Wy) / 2;
                s_num++;
#ifdef CONFIG_TOUCHSCREEN_VRPANEL
            }
            else {
                invalid_x = pos[f][0];
                invalid_y = pos[f][1];
            }
#endif
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

static void synaptics_ts_work_func(struct work_struct *work)
{
	int i;
	int ret;
	int bad_data = 0;
	struct i2c_msg msg[2];
	uint8_t start_reg;
	uint8_t buf[0x35-0x14];
    struct synaptics_ts_finger finger[2];
	struct synaptics_ts_data *ts = container_of(work, struct synaptics_ts_data, work);
    int num;
    
	msg[0].addr = ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &start_reg;
	start_reg = 0x14;
	msg[1].addr = ts->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = sizeof(buf);
	msg[1].buf = buf;

	/* printk("synaptics_ts_work_func\n"); */    
	for (i = 0; i < ((ts->use_irq && !bad_data) ? 1 : 1); i++) {
		ret = i2c_transfer(ts->client->adapter, msg, 2);
		if (ret < 0) {
			printk(KERN_ERR "synaptics_ts_work_func: i2c_transfer failed\n");
			bad_data = 1;
		} else {
			// printk(KERN_INFO "synaptics_ts_work_func: intr 0x%x\n", buf[0]);
            if (!buf[0]) {
                break;
            }
			bad_data = 0;
            synaptics_scan_finger_state(ts->finger, &buf[1]);
            num = synaptics_get_fingers(ts->finger, finger, dim(finger));
            // synaptics_dump_finger_state(finger, dim(finger));
            synaptics_ts_report(ts, finger, num);
		}
	}
	if (ts->use_irq)
		enable_irq(ts->client->irq);
}

static enum hrtimer_restart synaptics_ts_timer_func(struct hrtimer *timer)
{
	struct synaptics_ts_data *ts = container_of(timer, struct synaptics_ts_data, timer);
	/* printk("synaptics_ts_timer_func\n"); */

	queue_work(synaptics_wq, &ts->work);

	hrtimer_start(&ts->timer, ktime_set(0, 12500000), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

static irqreturn_t synaptics_ts_irq_handler(int irq, void *dev_id)
{
	struct synaptics_ts_data *ts = dev_id;

	/* printk("synaptics_ts_irq_handler\n"); */
	disable_irq_nosync(ts->client->irq);
	queue_work(synaptics_wq, &ts->work);
	return IRQ_HANDLED;
}

static int synaptics_ts_probe(
	struct i2c_client *client, const struct i2c_device_id *id)
{
	struct synaptics_ts_data *ts;
	uint8_t buf0[4];
	uint8_t buf1[8];
	struct i2c_msg msg[2];
	int ret = 0;
	uint16_t max_x, max_y;
	int fuzz_x, fuzz_y, fuzz_p, fuzz_w;
	struct synaptics_i2c_rmi_platform_data *pdata;
	int inactive_area_left;
	int inactive_area_right;
	int inactive_area_top;
	int inactive_area_bottom;
	int snap_left_on;
	int snap_left_off;
	int snap_right_on;
	int snap_right_off;
	int snap_top_on;
	int snap_top_off;
	int snap_bottom_on;
	int snap_bottom_off;
	uint32_t panel_version;

	printk(KERN_INFO "synaptics_ts_probe\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "synaptics_ts_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
	INIT_WORK(&ts->work, synaptics_ts_work_func);
	ts->client = client;
	i2c_set_clientdata(client, ts);
	pdata = client->dev.platform_data;
	if (pdata)
		ts->power = pdata->power;
	if (ts->power) {
		ret = ts->power(1);
		if (ret < 0) {
			printk(KERN_ERR "synaptics_ts_probe power on failed\n");
			goto err_power_failed;
		}
	}

	ret = i2c_smbus_write_byte_data(ts->client, 0x8c, 0x01); /* device command = reset */
	if (ret < 0) {
		printk(KERN_ERR "i2c_smbus_write_byte_data failed\n");
		/* fail? */
	}
	{
		int retry = 10;
		while (retry-- > 0) {
			msleep(100);
			ret = i2c_smbus_read_byte_data(ts->client, 0x9a);
			if (ret >= 0)
				break;
		}
	}
	if (ret < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");
		goto err_detect_failed;
	}
	printk(KERN_INFO "synaptics_ts_probe: Product Major Version %x\n", ret);
	panel_version = ret << 8;
	ret = i2c_smbus_read_byte_data(ts->client, 0x9b);
	if (ret < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");
		goto err_detect_failed;
	}
	printk(KERN_INFO "synaptics_ts_probe: Product Minor Version %x\n", ret);
	panel_version |= ret;

	ret = i2c_smbus_read_byte_data(ts->client, 0x99);
	if (ret < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");
		goto err_detect_failed;
	}
	printk(KERN_INFO "synaptics_ts_probe: product property %x\n", ret);

	{
        u8 i, product_id[11];
        for (i = 0; i < (sizeof(product_id) - 1); i++) {
            ret = i2c_smbus_read_byte_data(ts->client, (0xa3 + i));
        	if (ret < 0) {
        		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");
        		goto err_detect_failed;
        	}
            product_id[i] = (u8)ret;
        }
        product_id[i] = (u8)0;
    	printk(KERN_INFO "synaptics_ts_probe: product id %s\n", product_id);
	}

	if (pdata) {
		while (pdata->version > panel_version)
			pdata++;
		ts->flags = pdata->flags;
		inactive_area_left = pdata->inactive_left;
		inactive_area_right = pdata->inactive_right;
		inactive_area_top = pdata->inactive_top;
		inactive_area_bottom = pdata->inactive_bottom;
		snap_left_on = pdata->snap_left_on;
		snap_left_off = pdata->snap_left_off;
		snap_right_on = pdata->snap_right_on;
		snap_right_off = pdata->snap_right_off;
		snap_top_on = pdata->snap_top_on;
		snap_top_off = pdata->snap_top_off;
		snap_bottom_on = pdata->snap_bottom_on;
		snap_bottom_off = pdata->snap_bottom_off;
		fuzz_x = pdata->fuzz_x;
		fuzz_y = pdata->fuzz_y;
		fuzz_p = pdata->fuzz_p;
		fuzz_w = pdata->fuzz_w;
	} else {
		inactive_area_left = 0;
		inactive_area_right = 0;
		inactive_area_top = 0;
		inactive_area_bottom = 0;
		snap_left_on = 0;
		snap_left_off = 0;
		snap_right_on = 0;
		snap_right_off = 0;
		snap_top_on = 0;
		snap_top_off = 0;
		snap_bottom_on = 0;
		snap_bottom_off = 0;
		fuzz_x = 0;
		fuzz_y = 0;
		fuzz_p = 0;
		fuzz_w = 0;
	}

	ret = i2c_smbus_read_byte_data(ts->client, 0x39);
	if (ret < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");
		goto err_detect_failed;
	}
	printk(KERN_INFO "synaptics_ts_probe: device control %x\n", ret);

	ret = i2c_smbus_read_byte_data(ts->client, 0x3a);
	if (ret < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");
		goto err_detect_failed;
	}
	printk(KERN_INFO "synaptics_ts_probe: interrupt enable %x\n", ret);

	ret = i2c_smbus_write_byte_data(ts->client, 0x3a, 0); /* disable interrupt */
	if (ret < 0) {
		printk(KERN_ERR "i2c_smbus_write_byte_data failed\n");
		goto err_detect_failed;
	}

	msg[0].addr = ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf0;
	buf0[0] = 0xdd;
	msg[1].addr = ts->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 8;
	msg[1].buf = buf1;
	ret = i2c_transfer(ts->client->adapter, msg, 2);
	if (ret < 0) {
		printk(KERN_ERR "i2c_transfer failed\n");
		goto err_detect_failed;
	}
	printk(KERN_INFO "synaptics_ts_probe: 0xe0: %x %x %x %x %x %x %x %x\n",
	       buf1[0], buf1[1], buf1[2], buf1[3],
	       buf1[4], buf1[5], buf1[6], buf1[7]);

	ret = i2c_smbus_read_word_data(ts->client, 0x41);
	if (ret < 0) {
		printk(KERN_ERR "i2c_smbus_read_word_data failed\n");
		goto err_detect_failed;
	}
	ts->max[0] = max_x = (ret & 0xfff);
	ret = i2c_smbus_read_word_data(ts->client, 0x43);
	if (ret < 0) {
		printk(KERN_ERR "i2c_smbus_read_word_data failed\n");
		goto err_detect_failed;
	}
	ts->max[1] = max_y = (ret & 0xfff);
	if (ts->flags & SYNAPTICS_SWAP_XY)
		swap(max_x, max_y);

	ret = synaptics_init_panel(ts); /* will also switch back to page 0x04 */
	if (ret < 0) {
		printk(KERN_ERR "synaptics_init_panel failed\n");
		goto err_detect_failed;
	}

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		printk(KERN_ERR "synaptics_ts_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	ts->input_dev->name = "synaptics-rmi-touchscreen";
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(EV_ABS, ts->input_dev->evbit);
	inactive_area_left = inactive_area_left * max_x / 0x10000;
	inactive_area_right = inactive_area_right * max_x / 0x10000;
	inactive_area_top = inactive_area_top * max_y / 0x10000;
	inactive_area_bottom = inactive_area_bottom * max_y / 0x10000;
	snap_left_on = snap_left_on * max_x / 0x10000;
	snap_left_off = snap_left_off * max_x / 0x10000;
	snap_right_on = snap_right_on * max_x / 0x10000;
	snap_right_off = snap_right_off * max_x / 0x10000;
	snap_top_on = snap_top_on * max_y / 0x10000;
	snap_top_off = snap_top_off * max_y / 0x10000;
	snap_bottom_on = snap_bottom_on * max_y / 0x10000;
	snap_bottom_off = snap_bottom_off * max_y / 0x10000;
	fuzz_x = fuzz_x * max_x / 0x10000;
	fuzz_y = fuzz_y * max_y / 0x10000;
	ts->snap_down[!!(ts->flags & SYNAPTICS_SWAP_XY)] = -inactive_area_left;
	ts->snap_up[!!(ts->flags & SYNAPTICS_SWAP_XY)] = max_x + inactive_area_right;
	ts->snap_down[!(ts->flags & SYNAPTICS_SWAP_XY)] = -inactive_area_top;
	ts->snap_up[!(ts->flags & SYNAPTICS_SWAP_XY)] = max_y + inactive_area_bottom;
	ts->snap_down_on[!!(ts->flags & SYNAPTICS_SWAP_XY)] = snap_left_on;
	ts->snap_down_off[!!(ts->flags & SYNAPTICS_SWAP_XY)] = snap_left_off;
	ts->snap_up_on[!!(ts->flags & SYNAPTICS_SWAP_XY)] = max_x - snap_right_on;
	ts->snap_up_off[!!(ts->flags & SYNAPTICS_SWAP_XY)] = max_x - snap_right_off;
	ts->snap_down_on[!(ts->flags & SYNAPTICS_SWAP_XY)] = snap_top_on;
	ts->snap_down_off[!(ts->flags & SYNAPTICS_SWAP_XY)] = snap_top_off;
	ts->snap_up_on[!(ts->flags & SYNAPTICS_SWAP_XY)] = max_y - snap_bottom_on;
	ts->snap_up_off[!(ts->flags & SYNAPTICS_SWAP_XY)] = max_y - snap_bottom_off;
	printk(KERN_INFO "synaptics_ts_probe: max_x %d, max_y %d\n", max_x, max_y);
	printk(KERN_INFO "synaptics_ts_probe: inactive_x %d %d, inactive_y %d %d\n",
	       inactive_area_left, inactive_area_right,
	       inactive_area_top, inactive_area_bottom);
	printk(KERN_INFO "synaptics_ts_probe: snap_x %d-%d %d-%d, snap_y %d-%d %d-%d\n",
	       snap_left_on, snap_left_off, snap_right_on, snap_right_off,
	       snap_top_on, snap_top_off, snap_bottom_on, snap_bottom_off);
    
    inactive_area_left = -inactive_area_left;
    inactive_area_top = -inactive_area_top;
    inactive_area_right = max_x + inactive_area_right;
    inactive_area_bottom = max_y + inactive_area_bottom;
#if 0
    synaptics_recal_pos(ts, &inactive_area_left, &inactive_area_top);
    synaptics_recal_pos(ts, &inactive_area_right, &inactive_area_bottom);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, inactive_area_left, inactive_area_right, fuzz_x, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, inactive_area_top, inactive_area_bottom, fuzz_y, 0);
#else
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, 320, fuzz_x, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, 480, fuzz_y, 0);
#endif
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, fuzz_p, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 15, fuzz_w, 0);
	/* ts->input_dev->name = ts->keypad_info->name; */
	ret = input_register_device(ts->input_dev);
	if (ret) {
		printk(KERN_ERR "synaptics_ts_probe: Unable to register %s input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	}
    //client->irq = 0;
	if (client->irq) {
        ret = 124;
		printk("attentino gpio:%d\n", ret);
        if (gpio_request(ret, "attention")) {
    		printk(KERN_ERR "gpio_request irq failed\n");
        }
        if (gpio_direction_input(ret)) {
    		printk(KERN_ERR "gpio_direction_input irq failed\n");
        }
		ret = request_irq(client->irq, synaptics_ts_irq_handler, 0, client->name, ts);
		if (ret == 0) {
			ret = i2c_smbus_write_byte_data(ts->client, 0x3a, 0x04); /* enable abs int */
			if (ret)
				free_irq(client->irq, ts);
		}
		if (ret == 0)
			ts->use_irq = 1;
		else
			dev_err(&client->dev, "request_irq failed\n");
	}
	if (!ts->use_irq) {
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = synaptics_ts_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = synaptics_ts_early_suspend;
	ts->early_suspend.resume = synaptics_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	printk(KERN_INFO "synaptics_ts_probe: Start touchscreen %s in %s mode\n", ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");

#ifdef CONFIG_TOUCHSCREEN_VRPANEL
	VrpInit(ts->input_dev, NULL);
#else
	set_bit(KEY_HOME, ts->input_dev->keybit);
	set_bit(KEY_MENU, ts->input_dev->keybit);
	set_bit(KEY_BACK, ts->input_dev->keybit);
	set_bit(KEY_SEARCH, ts->input_dev->keybit);
#endif

	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
err_detect_failed:
	if (ts->power) {
		ts->power(0);
	}
err_power_failed:
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
	return ret;
}

static int synaptics_ts_remove(struct i2c_client *client)
{
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	unregister_early_suspend(&ts->early_suspend);
	if (ts->use_irq)
		free_irq(client->irq, ts);
	else
		hrtimer_cancel(&ts->timer);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	return 0;
}

static int synaptics_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);

	if (ts->use_irq)
		disable_irq(client->irq);
	else
		hrtimer_cancel(&ts->timer);
	ret = cancel_work_sync(&ts->work);
	if (ret && ts->use_irq) /* if work was pending disable-count is now 2 */
		enable_irq(client->irq);
	ret = i2c_smbus_write_byte_data(ts->client, 0x3a, 0); /* disable interrupt */
	if (ret < 0)
		printk(KERN_ERR "synaptics_ts_suspend: i2c_smbus_write_byte_data failed\n");

	ret = i2c_smbus_write_byte_data(ts->client, 0x39, 0x81); /* normal operation, 80 reports per second */
	if (ret < 0)
		printk(KERN_ERR "synaptics_ts_suspend: i2c_smbus_write_byte_data failed\n");
	if (ts->power) {
		ret = ts->power(0);
		if (ret < 0)
			printk(KERN_ERR "synaptics_ts_resume power off failed\n");
	}
	return 0;
}

static int synaptics_ts_resume(struct i2c_client *client)
{
	int ret;
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);

	if (ts->power) {
		ret = ts->power(1);
		if (ret < 0)
			printk(KERN_ERR "synaptics_ts_resume power on failed\n");
	}

	ret = i2c_smbus_write_byte_data(ts->client, 0x8c, 0x01); /* device command = reset */
	if (ret < 0) {
		printk(KERN_ERR "reset device failed\n");
		/* fail? */
	}
    
	ret = i2c_smbus_write_byte_data(ts->client, 0x3a, 0); /* disable interrupt */
	if (ret < 0) {
		printk(KERN_ERR "disable interrupt failed\n");
	}
    
	synaptics_init_panel(ts);

	if (ts->use_irq)
		enable_irq(client->irq);

	if (!ts->use_irq)
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	else
		i2c_smbus_write_byte_data(ts->client, 0x3a, 0x04); /* enable abs int */

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_ts_early_suspend(struct early_suspend *h)
{
	struct synaptics_ts_data *ts;
	ts = container_of(h, struct synaptics_ts_data, early_suspend);
	synaptics_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void synaptics_ts_late_resume(struct early_suspend *h)
{
	struct synaptics_ts_data *ts;
	ts = container_of(h, struct synaptics_ts_data, early_suspend);
	synaptics_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id synaptics_ts_id[] = {
	{ SYNAPTICS_I2C_RMI_NAME, 0 },
	{ }
};

static struct i2c_driver synaptics_ts_driver = {
	.probe		= synaptics_ts_probe,
	.remove		= synaptics_ts_remove,
	.suspend	= synaptics_ts_suspend,
	.resume		= synaptics_ts_resume,
	.id_table	= synaptics_ts_id,
	.driver = {
		.name	= SYNAPTICS_I2C_RMI_NAME,
	},
};

static int __devinit synaptics_ts_init(void)
{
	synaptics_wq = create_singlethread_workqueue("synaptics_wq");
	if (!synaptics_wq)
		return -ENOMEM;
	return i2c_add_driver(&synaptics_ts_driver);
}

static void __exit synaptics_ts_exit(void)
{
	i2c_del_driver(&synaptics_ts_driver);
	if (synaptics_wq)
		destroy_workqueue(synaptics_wq);
}

module_init(synaptics_ts_init);
module_exit(synaptics_ts_exit);

MODULE_DESCRIPTION("Synaptics Touchscreen Driver");
MODULE_LICENSE("GPL");
