#include <linux/module.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/cm3623.h>
#include <mach/gpio.h>
#include <asm/uaccess.h>

#define CM3623_ARA                      (0x19>>1)
#define CM3623_ALS_WR                   (0x20>>1)
#define CM3623_ALS_RD_MSB               (0x21>>1)
#define CM3623_INIT                     (0x22>>1)
#define CM3623_ALS_RD_LSB               (0x23>>1)
#define CM3623_PS_WR                    (0xB0>>1)
#define CM3623_PS_RD                    (0xB1>>1)
#define CM3623_PS_THD_WR                (0xB2>>1)
#define CM3623_VENDORID                 (0x0001)

#define CM3623_ALS                      (0)
#define CM3623_PS                       (1)
#define CM3623_ALS_BIT                  (1<<CM3623_ALS)
#define CM3623_PS_BIT                   (1<<CM3623_PS)

MODULE_LICENSE("GPL v2");
MODULE_ALIAS("cm3623");

#define dim(x) (sizeof(x)/sizeof((x)[0]))

struct cm3623_sensor_data {
    int (*power)(int on);
    struct i2c_client *client;
    int gpio_int;
    bool use_irq;
    struct hrtimer timer;
    struct work_struct  work_pull_als;
    struct work_struct  work_pull_ps;
    struct work_struct  work_push;	
    struct work_struct  work_cfg;	
    struct mutex lock;
    int light;
    int distance;
    int open_push;
    int ps_threshold;
    bool ps_wake;
    bool fake_data;
};

static ssize_t cm3623_als_read(struct file *, char __user *, size_t, loff_t *) ;
static ssize_t cm3623_ps_read(struct file *, char __user *, size_t, loff_t *) ;

static struct workqueue_struct *cm3623_wq;

static struct file_operations cm3623_als_fops = {
	.owner = THIS_MODULE,
	.read = cm3623_als_read,
};

static struct miscdevice cm3623_als_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "cm3623_als",
	.fops = &cm3623_als_fops,
};

static struct file_operations cm3623_ps_fops = {
	.owner =   THIS_MODULE,
	.read = cm3623_ps_read,
};

static struct miscdevice cm3623_ps_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "cm3623_ps",
	.fops = &cm3623_ps_fops,
};

static enum hrtimer_restart cm3623_timer(struct hrtimer *timer)
{
    struct cm3623_sensor_data *data = container_of(timer, struct cm3623_sensor_data, timer);

    printk("cm3623_timer\n");
    queue_work(cm3623_wq, &data->work_push);
    hrtimer_start(&data->timer, ktime_set(0, 200000000), HRTIMER_MODE_REL);
    
    return HRTIMER_NORESTART;
}

static irqreturn_t cm3623_irq(int irq, void *dev_id)
{
    struct cm3623_sensor_data *data = dev_id;

    //printk("cm3623_irq\n");
    disable_irq_nosync(data->client->irq);
    queue_work(cm3623_wq, &data->work_push);
    
    return IRQ_HANDLED;
}

static int cm3623_open_push(struct cm3623_sensor_data *data)
{
    int rc = 0;

    if (!data->open_push) {
        data->use_irq = false;
        if (data->client->irq) {
            rc = request_irq(data->client->irq,
                     &cm3623_irq,
                     IRQF_TRIGGER_LOW,
                     CM3623_NAME,
                     data);
            data->use_irq = rc ? false : true;
        }
        else {
            hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
            data->timer.function = cm3623_timer;
            rc = hrtimer_start(&data->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
        }
    }
    if (!rc) {
        data->open_push++;
    }
    return rc;
}

static void cm3623_release_push(struct cm3623_sensor_data *data)
{    
    if (data->open_push)
        data->open_push--;
    
    if (!data->open_push) {
        if (data->use_irq) {
            free_irq(data->client->irq, data);
            data->use_irq = false;
        }
        else {
            hrtimer_cancel(&data->timer);
        }
    }
}

#define cm3623_calc_light(code) ((int)(code * 7 /10))

const int n[] = {8, 20, 40, 80, 180};
const int k[] = {8125, 1250, 350, 125, 50}; // 26% Gray level
const int b[] = {102, 37, 22, 15, 9, 0}; // 26% Gray level

static int cm3623_calc_distance(int code) {
    int i, v;

    for (v = i = 0; i < dim(n); i++) {
        if (code <= n[i]) {
            v = (b[i] - (code * k[i] / 1000));
            break;
        }
    }

    return v;
}

static int cm3623_calc_code(int distance) {
    int i, v;
	
    v = 0;
    if (distance < b[0]) {        
        for (i = 0; i < (dim(b) - 1); i++) {
            if (distance > b[i + 1]) {
                v = (b[i] - distance) * 1000 / k[i];
                break;
            }
        }
    }

    return v;
}

static void cm3623_report(struct cm3623_sensor_data *sd, int type)
{
    u16 val;
    int rc;

    do {

        if (type & CM3623_ALS_BIT) {
    	    sd->client->addr = CM3623_ALS_RD_LSB;
    	    rc = i2c_smbus_read_byte(sd->client);
                 if (rc < 0) 
                     break;
    	    // printk("CM3623_ALS_RD_LSB 0x%X\n", rc);
    	    rc &= 0xFF;
    	    val = rc;
    	    sd->client->addr = CM3623_ALS_RD_MSB;
    	    rc = i2c_smbus_read_byte(sd->client);
                 if (rc < 0) 
                     break;
    	    // printk("CM3623_ALS_RD_MSB 0x%X\n", rc);
    	    rc &= 0xFF;
    	    val |= (rc << 8);
    	    sd->light = cm3623_calc_light(val);
    	    //printk("light %d lux\n", sd->light);
        }
        
        if (type & CM3623_PS_BIT) {
    	    sd->client->addr = CM3623_PS_RD;
    	    rc = i2c_smbus_read_byte(sd->client);
                 if (rc < 0) 
                     break;
    	    sd->distance = cm3623_calc_distance(rc);
    	    //printk("distance %d mm\n", sd->distance);
        }
    } while(0);
}

static int cm3623_cfg_tigger(struct cm3623_sensor_data *sd) {
    int rc = 0;
    
    printk("CM3623_INT_THRESHOLD %d\n", sd->ps_threshold);
    if (sd->ps_threshold) {
        sd->client->addr = CM3623_PS_THD_WR;
        rc = i2c_smbus_write_byte(sd->client, sd->ps_threshold);
        if (sd->gpio_int) {
            rc = gpio_get_value(sd->gpio_int);
            printk("CM3623_INT_PIN %d %d\n", sd->gpio_int, rc);
            set_irq_type(sd->client->irq, rc ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
        }
    }
    else {
        rc = -EINVAL;
    }
    
    return rc;
}

static void cm3623_work_pull_als(struct work_struct *work_pull_als)
{
    struct cm3623_sensor_data *sd =
        container_of(work_pull_als, struct cm3623_sensor_data, work_pull_als);

    //printk("cm3623_pull_als\n");
    mutex_lock(&sd->lock);
    cm3623_report(sd, CM3623_ALS_BIT);
    mutex_unlock(&sd->lock);    
}

static void cm3623_work_pull_ps(struct work_struct *work_pull_ps)
{
    struct cm3623_sensor_data *sd =
        container_of(work_pull_ps, struct cm3623_sensor_data, work_pull_ps);

    //printk("cm3623_pull_ps\n");
    mutex_lock(&sd->lock);
    cm3623_report(sd, CM3623_PS_BIT);
    mutex_unlock(&sd->lock);    
}

static void cm3623_work_push(struct work_struct *work_push)
{
    int rc;
    struct cm3623_sensor_data *sd =
        container_of(work_push, struct cm3623_sensor_data, work_push);

    mutex_lock(&sd->lock);
#if 0
    if (sd->gpio_int) {
        for (val = 0; val < 2; val++) {
            rc = gpio_get_value(sd->gpio_int);
            printk("CM3623_INT %d %d\n", sd->gpio_int, rc);
            if (rc) {
                break;
            }
            sd->client->addr = CM3623_ARA;
            rc = i2c_smbus_read_byte(sd->client);
            printk("CM3623_ARA 0x%X\n", rc);
        }
    }
#endif
    if (sd->ps_threshold)
    {
        cm3623_report(sd, CM3623_PS_BIT);
    }
    if (sd->use_irq) {
        if (sd->gpio_int) {
            rc = gpio_get_value(sd->gpio_int);
            printk("CM3623_INT %d %d\n", sd->gpio_int, rc);
            set_irq_type(sd->client->irq, rc ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
        }
        enable_irq(sd->client->irq);
    }
    
    mutex_unlock(&sd->lock);    
}

static void cm3623_work_cfg(struct work_struct *work_cfg)
{
    struct cm3623_sensor_data *sd =
        container_of(work_cfg, struct cm3623_sensor_data, work_cfg);
    
    mutex_lock(&sd->lock);
    cm3623_cfg_tigger(sd);
    mutex_unlock(&sd->lock);
}

static ssize_t cm3623_als_read(struct file *fp, char __user *buf, size_t count, loff_t *pos)
{
	ssize_t rc = 0;	
	struct i2c_client *client = 
		container_of(cm3623_als_device.parent, struct i2c_client, dev);
	struct cm3623_sensor_data *sd = i2c_get_clientdata(client);

	do {
        if (sd->fake_data) {
            sd->light = -1;
        }
        else {
    		flush_work(&sd->work_pull_als);
    		queue_work(cm3623_wq, &sd->work_pull_als);
    		flush_work(&sd->work_pull_als);
        }
		if (copy_to_user(buf, &(sd->light), sizeof(sd->light))) {
			rc = -EFAULT;
			break;
		}
		rc = sizeof(sd->light);
	} while(0);
	
	//printk("cm3623_als_read %d %d\n", sd->light, rc);
	return rc;
}

static ssize_t cm3623_ps_read(struct file *fp, char __user *buf, size_t count, loff_t *pos)
{
	ssize_t rc = 0;	
	struct i2c_client *client = 
		container_of(cm3623_ps_device.parent, struct i2c_client, dev);
	struct cm3623_sensor_data *sd = i2c_get_clientdata(client);

	do {
        if (sd->fake_data) {
            sd->distance = -1;
        }
        else {
    		flush_work(&sd->work_pull_ps);
    		queue_work(cm3623_wq, &sd->work_pull_ps);
    		flush_work(&sd->work_pull_ps);
        }
		if (copy_to_user(buf, &(sd->distance), sizeof(sd->distance))) {
			rc = -EFAULT;
			break;
		}
		rc = sizeof(sd->distance);
	} while(0);
	
	//printk("cm3623_ps_read %d %d\n", sd->distance, rc);
	return rc;
}

static int cm3623_ps_trigger(struct cm3623_sensor_data *sd, const char *arg)
{
    int threshold;
    int rc = 0;

    rc = sscanf(arg, "%d", &threshold);
    if (1 != rc) {
        printk("Invalid arguments. Usage: <threshold>\n");
        rc = -EINVAL;
        return rc;
    }

    printk("cm3623_ps_trigger %d \n", threshold);

    sd->ps_threshold = cm3623_calc_code(threshold);
    if (sd->ps_threshold < 2) sd->ps_threshold = 2;
        
    if (sd->use_irq && sd->ps_threshold) {
        flush_work(&sd->work_cfg);
        disable_irq(sd->client->irq);
    	queue_work(cm3623_wq, &sd->work_cfg);
    	flush_work(&sd->work_cfg);
        enable_irq(sd->client->irq);
    }
    return 0;
}

static ssize_t cm3623_ps_trigger_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
    int rc = 0;
    struct i2c_client *client = container_of(cm3623_ps_device.parent, struct i2c_client, dev);
    struct cm3623_sensor_data *sd = i2c_get_clientdata(client);

    rc = cm3623_ps_trigger(sd, buf);

    return rc ? : size;
}

static DEVICE_ATTR(ps_trigger, S_IWUGO, NULL, cm3623_ps_trigger_store);

static int cm3623_ps_wake(struct cm3623_sensor_data *sd, const char *arg)
{
    int wake;
    int rc = 0;

    rc = sscanf(arg, "%d", &wake);
    if (1 != rc) {
        printk("Invalid arguments. Usage: <wake>\n");
        rc = -EINVAL;
        return rc;
    }

    printk("cm3623_ps_wake %d \n", wake);

    mutex_lock(&sd->lock);
    sd->ps_wake = (wake ? true : false);
    mutex_unlock(&sd->lock);
    
    return 0;
}

static ssize_t cm3623_ps_wake_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
    int rc = 0;
    struct i2c_client *client = container_of(cm3623_ps_device.parent, struct i2c_client, dev);
    struct cm3623_sensor_data *sd = i2c_get_clientdata(client);

    rc = cm3623_ps_wake(sd, buf);

    return rc ? : size;
}

static DEVICE_ATTR(ps_wake, S_IWUGO, NULL, cm3623_ps_wake_store);

static int cm3623_fake_data(struct cm3623_sensor_data *sd, const char *arg)
{
    int fake_data;
    int rc = 0;

    rc = sscanf(arg, "%d", &fake_data);
    if (1 != rc) {
        printk("Invalid arguments. Usage: <fake_data>\n");
        rc = -EINVAL;
        return rc;
    }

    printk("cm3623_fake_data %d \n", fake_data);

    mutex_lock(&sd->lock);
    sd->fake_data = (fake_data ? true : false);
    mutex_unlock(&sd->lock);
    
    return 0;
}

static ssize_t cm3623_fake_data_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
    int rc = 0;
    struct i2c_client *client = container_of(cm3623_ps_device.parent, struct i2c_client, dev);
    struct cm3623_sensor_data *sd = i2c_get_clientdata(client);

    rc = cm3623_fake_data(sd, buf);

    return rc ? : size;
}

static DEVICE_ATTR(fake_data, S_IWUGO, NULL, cm3623_fake_data_store);

static int cm3623_config(struct cm3623_sensor_data *sd)
{
    int rc;
    u8 val;
    
    printk("cm3623_config\n");

    sd->client->addr = CM3623_INIT;
    val = 0x20;
    rc = i2c_smbus_write_byte(sd->client, val);
    
    sd->client->addr = CM3623_ALS_WR;
    val = (0 << 0);   // SD_ALS
    val |= (1 << 1);  // WDM
    val |= (3 << 2);  // IT_ALS
    val |= (0 << 4);  // THD_ALS
    val |= (0 << 6);  // GAIN_ALS
    rc = i2c_smbus_write_byte(sd->client, val);
    
    sd->client->addr = CM3623_PS_THD_WR;
    val = sd->ps_threshold; // THD_PS
    rc = i2c_smbus_write_byte(sd->client, val);
    
    sd->client->addr = CM3623_PS_WR;
    val = (0 << 0);   // SD_PS
    val |= (0 << 2);  // INT_PS
    val |= (0 << 3);  // INT_ALS
    val |= (3 << 4);  // IT_PS    
    val |= (0 << 6);  // DR_PS    
    rc = i2c_smbus_write_byte(sd->client, val);
   
    return rc;
}

#ifdef CONFIG_PM
static int cm3623_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cm3623_sensor_data *data = i2c_get_clientdata(client);

    printk("cm3623_suspend\n");

    mutex_lock(&data->lock);
    
    //shutdown ALS
    data->client->addr = CM3623_ALS_WR;
    if (i2c_smbus_write_byte(data->client, 1) < 0)
        printk(KERN_WARNING "cm3623 shutdown ALS fail\n");

    printk("ps_wake %d\n", data->ps_wake);
    if (!data->ps_wake) {
        if (data->use_irq)
            disable_irq(data->client->irq);
        //shutdown PS
        data->client->addr = CM3623_PS_WR;
        if (i2c_smbus_write_byte(data->client, 1) < 0)
            printk(KERN_WARNING "cm3623 shutdown PS fail\n");
#if 0        
        if (data->power) {
            data->power(0);
        }
#endif
    }
    return 0;
}

static int cm3623_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cm3623_sensor_data *data = i2c_get_clientdata(client);
    
    printk("cm3623_resume\n");
    
    if (data->power) {
        data->power(1);
    }
    cm3623_config(data);
    
    if (!data->ps_wake) {
        if (data->use_irq) {
            enable_irq(data->client->irq);
        }
    }
    
    mutex_unlock(&data->lock);
    return 0;
}
#else
#define cm3623_suspend NULL
#define cm3623_resume NULL
#endif /* CONFIG_PM */

static int cm3623_probe(
    struct i2c_client *client, const struct i2c_device_id *id)
{
    int rc;
    struct cm3623_sensor_data *sensor_data;
    struct cm3623_platform_data *platform_data;
    
    printk(KERN_INFO "cm3623_probe\n");
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk(KERN_ERR "cm3623_probe: need I2C_FUNC_I2C\n");
        rc = -ENODEV;
        goto probe_err_check;
    }

    sensor_data = kzalloc(sizeof(*sensor_data), GFP_KERNEL);
    if (!sensor_data) {
        rc = -ENOMEM;
        goto probe_err_alloc;
    }
    
    INIT_WORK(&sensor_data->work_pull_als, cm3623_work_pull_als);
    INIT_WORK(&sensor_data->work_pull_ps, cm3623_work_pull_ps);	
    INIT_WORK(&sensor_data->work_push, cm3623_work_push);
    INIT_WORK(&sensor_data->work_cfg, cm3623_work_cfg);
    sensor_data->client = client;
    i2c_set_clientdata(client, sensor_data);
    platform_data = client->dev.platform_data;
    if (platform_data) {
        sensor_data->power = platform_data->power;
            
        if (sensor_data->power) {
            rc = sensor_data->power(1);
            if (rc < 0) {
                printk(KERN_ERR "cm3623_probe power on failed\n");
                goto probe_err_power;
            }
        }

        sensor_data->gpio_int = platform_data->gpio_int;
    }    
    else {
        sensor_data->power = NULL;
        sensor_data->gpio_int = 0;
    }
    
    if (sensor_data->gpio_int) {
        rc = gpio_request(sensor_data->gpio_int, "cm3623_irq");
        if (rc) {
            printk(KERN_ERR "cm3623_probe gpio_request failed\n");
            goto probe_err_gpio;
        }
        gpio_tlmm_config(GPIO_CFG(sensor_data->gpio_int, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        client->irq = gpio_to_irq(sensor_data->gpio_int);
    }
    else {
        client->irq = 0;
    }

    sensor_data->ps_wake = false;
    sensor_data->ps_threshold = 0x60;
    rc = cm3623_config(sensor_data);
    if (rc < 0)
        goto probe_err_cfg;

    cm3623_als_device.parent = &(client->dev);
    rc = misc_register(&cm3623_als_device);
    if (rc) {
        printk(KERN_ERR "cm3623 can't register ALS misc device\n");
        goto probe_err_reg_misc_als;
    }

    cm3623_ps_device.parent = &(client->dev);
    rc = misc_register(&cm3623_ps_device);
    if (rc) {
        printk(KERN_ERR "cm3623 can't register PS misc device\n");
        goto probe_err_reg_misc_ps;
    }

    cm3623_open_push(sensor_data);
    
    rc = device_create_file(cm3623_ps_device.this_device, &dev_attr_ps_trigger);
    if (rc) {
        printk(KERN_ERR "cm3623 can't create PS trigger file\n");
        goto probe_err_create_ps_trigger;
    }

    rc = device_create_file(cm3623_ps_device.this_device, &dev_attr_ps_wake);
    if (rc) {
        printk(KERN_ERR "cm3623 can't create PS wake file\n");
        goto probe_err_create_ps_wake;
    }

    rc = device_create_file(cm3623_ps_device.this_device, &dev_attr_fake_data);
    if (rc) {
        printk(KERN_ERR "cm3623 can't create fake data file\n");
        goto probe_err_create_fake_data;
    }

    mutex_init(&sensor_data->lock);

#ifdef CONFIG_CM3623_FAKE_DATA
    sensor_data->fake_data = 1;
#endif

    return rc;

probe_err_create_fake_data:
    device_remove_file(cm3623_ps_device.this_device, &dev_attr_ps_wake);
probe_err_create_ps_wake:
    device_remove_file(cm3623_ps_device.this_device, &dev_attr_ps_trigger);
probe_err_create_ps_trigger:
    cm3623_release_push(sensor_data);
probe_err_reg_misc_ps:
    misc_deregister(&cm3623_als_device);	
probe_err_reg_misc_als:
    i2c_set_clientdata(client, NULL);
probe_err_cfg:
    if (sensor_data->gpio_int) {
        gpio_free(sensor_data->gpio_int);
    }
probe_err_gpio:
probe_err_power:
    kfree(sensor_data);
probe_err_alloc:
probe_err_check:
    return rc;
}

static int __devexit cm3623_remove(struct i2c_client *client)
{
    struct cm3623_sensor_data *sensor_data;

    sensor_data = i2c_get_clientdata(client);
    
    device_remove_file(cm3623_ps_device.this_device, &dev_attr_fake_data);
    device_remove_file(cm3623_ps_device.this_device, &dev_attr_ps_wake);
    device_remove_file(cm3623_ps_device.this_device, &dev_attr_ps_trigger);
    cm3623_release_push(sensor_data);
    
    misc_deregister(&cm3623_ps_device);	
    misc_deregister(&cm3623_als_device);	
    i2c_set_clientdata(client, NULL);
    if (sensor_data->gpio_int) {
        gpio_free(sensor_data->gpio_int);
    }
    kfree(sensor_data);

    return 0;
}

static const struct i2c_device_id cm3623_id[] = {
    { CM3623_NAME, 0 },
    { }
};

static const struct dev_pm_ops cm3623_pm_ops = {
	.suspend = cm3623_suspend,
	.resume = cm3623_resume,
};

static struct i2c_driver cm3623_driver = {
    .probe      = cm3623_probe,
    .remove     = __devexit_p(cm3623_remove),
    .id_table   = cm3623_id,
    .driver = {
        .name   = CM3623_NAME,
        .pm     = &cm3623_pm_ops,
    },
};

static int __init cm3623_init(void)
{
    int rc = 0;

    do {
        cm3623_wq = create_singlethread_workqueue("cm3623_wq");
        if (!cm3623_wq) {
            rc = -ENOMEM;
            break;
        }

        rc = i2c_add_driver(&cm3623_driver);
        if (rc < 0) {
            break;
        }
        
    } while(0);

    return rc;
}
module_init(cm3623_init);

static void __exit cm3623_exit(void)
{
    i2c_del_driver(&cm3623_driver);
    if (cm3623_wq)
        destroy_workqueue(cm3623_wq);
}
module_exit(cm3623_exit);
