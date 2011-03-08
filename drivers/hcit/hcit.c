#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/gpio.h>
#include <asm/delay.h>
#include <linux/delay.h>


#include <mach/pmic.h>
#include <mach/vreg.h>

#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#define HCIT_MAGIC 'h'	//Recommand range from A0-BF 

#define VIBRATOR_MOTOR_OFF_VOLT	0
#define VIBRATOR_MOTOR_ON_VOLT	3000

#define PMIC_FLASH_CURRENT_DOWN         0
#define PMIC_FLASH_CURRENT_HIGH         200

#define  PMIC_LED_LCD__LEVEL0 	0
#define  PMIC_LED_LCD__LEVEL1 	1
#define  PMIC_LED_LCD__LEVEL2 	2
#define  PMIC_LED_LCD__LEVEL3 	3
#define  PMIC_LED_LCD__LEVEL4 	4

 typedef enum 
{
    LED_RED = 1,
    LED_GREEN,
    LED_BLUE,
    LED_RED_GREEN,
    LED_RED_BLUE,
    LED_GREEN_BLUE,
    LED_RED_GREEN_BLUE,
    LED_INDEX_MAX
    
}led_index_enum_type;

typedef enum 
{
    LED_MODE_OFF = 0,
    LED_MODE_ON,
    LED_MODE_BLINK,
    LED_MODE_MAX
    
}led_mode_enum_type;

typedef struct     
{
  char index;
  char mode;
  char reserve1;
  char reserve2;  
  unsigned long period;     
  unsigned long ontime;  
} led_rpc_cmd_type;

enum
{
  GPIOLED_RED = 0,
  GPIOLED_GREEN,
  GPIOLED_BLUE,

  GPIOLED_MAX
};

enum
{
    HCITCMD_DEFAULT = 0xA0,
    HCITCMD_KEYLED_ON = HCITCMD_DEFAULT,
    HCITCMD_KEYLED_OFF,
    HCITCMD_VIB_ON,
    HCITCMD_VIB_OFF,
    HCITCMD_CAMERA_FLASHLED_ON,
    HCITCMD_CAMERA_FLASHLED_OFF,
    HCITCMD_LCD_BACKLIGHT_ON,
    HCITCMD_LCD_BACKLIGHT_OFF,
    HCITCMD_GPIOLED_ON,
    HCITCMD_GPIOLED_OFF,
    HCITCMD_BLUETOOTH_ON,
    HCITCMD_BLUETOOTH_OFF,

    HCITCMD_MAX
};

//for upper keyled control
#define HCIT_IOCTL_KEYLED_ON 	 _IOW(HCIT_MAGIC, HCITCMD_KEYLED_ON, int)
#define HCIT_IOCTL_KEYLED_OFF	 _IOW(HCIT_MAGIC, HCITCMD_KEYLED_OFF, int)

//for vibrator control
#define HCIT_IOCTL_VIB_ON 	 _IOW(HCIT_MAGIC, HCITCMD_VIB_ON, int)
#define HCIT_IOCTL_VIB_OFF	 _IOW(HCIT_MAGIC, HCITCMD_VIB_OFF, int)

//for camera flash led
#define HCIT_IOCTL_CAMERA_FLASHLED_ON 	 _IOW(HCIT_MAGIC, HCITCMD_CAMERA_FLASHLED_ON, int)
#define HCIT_IOCTL_CAMERA_FLASHLED_OFF	 _IOW(HCIT_MAGIC, HCITCMD_CAMERA_FLASHLED_OFF, int)

//for LCD backlight led
#define HCIT_IOCTL_LCD_BACKLIGHT_ON 	 _IOW(HCIT_MAGIC, HCITCMD_LCD_BACKLIGHT_ON, int)
#define HCIT_IOCTL_LCD_BACKLIGHT_OFF	 _IOW(HCIT_MAGIC, HCITCMD_LCD_BACKLIGHT_OFF, int)

//for GPIO led
#define HCIT_IOCTL_GPIOLED_ON 	 _IOW(HCIT_MAGIC, HCITCMD_GPIOLED_ON, int)
#define HCIT_IOCTL_GPIOLED_OFF	 _IOW(HCIT_MAGIC, HCITCMD_GPIOLED_OFF, int)

//for bluetooth led
#define HCIT_IOCTL_BLUETOOTH_ON 	 _IOW(HCIT_MAGIC, HCITCMD_BLUETOOTH_ON, int)
#define HCIT_IOCTL_BLUETOOTH_OFF	 _IOW(HCIT_MAGIC, HCITCMD_BLUETOOTH_OFF, int)

static int bluetooth_power(int on);

extern int oem_rpc_client_register(int id);
extern void set_data_to_arm9(int id, char *in,int insize);
#define RPC_CHANNEL_LED 1

static unsigned sd_boot_mode[2] = {0,0};



void hcit_gpioled_ctl(unsigned long arg)
{
    led_rpc_cmd_type led;

    if (copy_from_user(&led, (led_rpc_cmd_type *)arg, sizeof(led_rpc_cmd_type)))
    {
      printk("[HCIT] hcit_gpioled_ctl() copy_from_user failure \r\n");
      return ;
    }
    printk("[HCIT]hcit_gpioled_ctl() , index<%d> mode<%d> period<%ld> ontime<%ld>\r\n",led.index, led.mode, led.period, led.ontime);

    printk("[HCIT]hcit_gpioled_ctl() , sizeof(ledcmd)<%d> sizeof(ul)<%d> sizeof(int)<%d> sizeof(char)<%d>\r\n",
      sizeof(led_rpc_cmd_type), sizeof(unsigned long) , sizeof(int), sizeof(char));

    //Add rpc interface here
    oem_rpc_client_register(RPC_CHANNEL_LED);
    set_data_to_arm9(RPC_CHANNEL_LED, &led, sizeof(led_rpc_cmd_type));

}

static ssize_t hcit_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
      //Currently only used for get boot mode. If want to use for other target, ioctl should be invoked at first. 
	if (copy_to_user(buf, sd_boot_mode, sizeof(sd_boot_mode)))
		return -EFAULT;
	return 1;
}


static int hcit_ioctl(struct inode *ino, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;
    
    switch(cmd)
    {
    case HCIT_IOCTL_KEYLED_ON:
        printk("[HCIT] HCIT_IOControl : IOCTL_GPIO_KEYLEDON \r\n");
        pmic_set_led_intensity(LED_KEYPAD, PMIC_LED_LCD__LEVEL4);
        break;
    case HCIT_IOCTL_KEYLED_OFF:
        printk("[HCIT] HCIT_IOControl : IOCTL_GPIO_KEYLEDOFF\r\n");
        pmic_set_led_intensity(LED_KEYPAD, PMIC_LED_LCD__LEVEL0);
        break;
    case HCIT_IOCTL_VIB_ON:
        //printk("[HCIT] HCIT_IOControl : HCIT_IOCTL_VIB_ON \r\n");
        {
            int time_ms;
            
            time_ms = *((int*)arg);
         
            //printk("[HCIT] HCIT_IOControl : HCIT_IOCTL_VIB_ON time_ms<%d>\r\n", time_ms);

            pmic_vib_mot_set_volt(VIBRATOR_MOTOR_ON_VOLT);
            if( time_ms > 0)
            {
                msleep(time_ms);
                pmic_vib_mot_set_volt(VIBRATOR_MOTOR_OFF_VOLT);
            }
        }
        break;
    case HCIT_IOCTL_VIB_OFF:
        //printk("[HCIT] HCIT_IOControl : HCIT_IOCTL_VIB_OFF\r\n");
        pmic_vib_mot_set_volt(VIBRATOR_MOTOR_OFF_VOLT);
        break;
    case HCIT_IOCTL_CAMERA_FLASHLED_ON:
        printk("[HCIT] HCIT_IOControl : IOCTL_GPIO_CAMERA_FLASHLED_ON \r\n");
        pmic_flash_led_set_current(PMIC_FLASH_CURRENT_HIGH);
        break;
    case HCIT_IOCTL_CAMERA_FLASHLED_OFF:
        printk("[HCIT] HCIT_IOControl : IOCTL_GPIO_CAMERA_FLASHLED_OFF \r\n");
        pmic_flash_led_set_current(PMIC_FLASH_CURRENT_DOWN);
        break;
    case HCIT_IOCTL_LCD_BACKLIGHT_ON:
        printk("[HCIT] HCIT_IOControl : HCIT_IOCTL_LCD_BACKLIGHT_ON \r\n");
        gpio_direction_output(29,1);
        break;
    case HCIT_IOCTL_LCD_BACKLIGHT_OFF:
        printk("[HCIT] HCIT_IOControl : HCIT_IOCTL_LCD_BACKLIGHT_OFF \r\n");
        gpio_direction_output(29,0);
        break;
    case HCIT_IOCTL_GPIOLED_ON:
    case HCIT_IOCTL_GPIOLED_OFF:
        printk("[HCIT] HCIT_IOControl : HCIT_IOCTL_GPIOLED_ON / OFF\r\n");
        hcit_gpioled_ctl(arg);
        break;
    case HCIT_IOCTL_BLUETOOTH_ON:
        err = bluetooth_power(1);
        printk("[HCIT] HCIT_IOControl : HCIT_IOCTL_BLUETOOTH_ON err<%d>\r\n", err);
        break;
    case HCIT_IOCTL_BLUETOOTH_OFF:
        err = bluetooth_power(0);
        printk("[HCIT] HCIT_IOControl : HCIT_IOCTL_BLUETOOTH_OFF err<%d>\r\n", err);
        break;
    default:
        printk("[HCIT] HCIT_IOControl : unknown cmd=%d\r\n", cmd);
        break;
    }
    
    return 0;
}

static unsigned bt_config_power_on[] = {
	GPIO_CFG(42, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* WAKE */
	GPIO_CFG(43, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* RFR */
	GPIO_CFG(44, 2, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* CTS */
	GPIO_CFG(45, 2, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* Rx */
	GPIO_CFG(46, 3, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* Tx */
	GPIO_CFG(68, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_DOUT */
	GPIO_CFG(69, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_DIN */
	GPIO_CFG(70, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_SYNC */
	GPIO_CFG(71, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_CLK */
	GPIO_CFG(83, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* HOST_WAKE */
	//GPIO_CFG(20, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	//GPIO_CFG(94, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};
static unsigned bt_config_power_off[] = {
	GPIO_CFG(42, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* WAKE */
	GPIO_CFG(43, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* RFR */
	GPIO_CFG(44, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* CTS */
	GPIO_CFG(45, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* Rx */
	GPIO_CFG(46, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* Tx */
	GPIO_CFG(68, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_DOUT */
	GPIO_CFG(69, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_DIN */
	GPIO_CFG(70, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_SYNC */
	GPIO_CFG(71, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_CLK */
	GPIO_CFG(83, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* HOST_WAKE */
	//GPIO_CFG(20, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	//GPIO_CFG(94, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static int bluetooth_power(int on)
{
	struct vreg *vreg_bt;
	int pin, rc;

	printk(KERN_DEBUG "%s\n", __func__);

	/* do not have vreg bt defined, gp6 is the same */
	/* vreg_get parameter 1 (struct device *) is ignored */
	vreg_bt = vreg_get(NULL, "gp6");

	if (IS_ERR(vreg_bt)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_bt));
		return PTR_ERR(vreg_bt);
	}

	if (on) {
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_on); pin++) {
			rc = gpio_tlmm_config(bt_config_power_on[pin],
					      GPIO_CFG_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_on[pin], rc);
				return -EIO;
			}
		}

		/* units of mV, steps of 50 mV */
		rc = vreg_set_level(vreg_bt, 2600);
		if (rc) {
			printk(KERN_ERR "%s: vreg set level failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}
		rc = vreg_enable(vreg_bt);
		if (rc) {
			printk(KERN_ERR "%s: vreg enable failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}
		msleep(100);

		printk(KERN_ERR "BlueZ required power up * QCOM\r\n");
		gpio_direction_output(94,0);
		gpio_direction_output(20,0);
		msleep(1);
		printk(KERN_ERR "BlueZ required power up * QCOM delay 1ms\r\n");
		printk(KERN_ERR "BlueZ required power up * QCOM delay 100ms\r\n");
		gpio_direction_output(94,1);
		msleep(100);
		gpio_direction_output(20,1);
		msleep(100);

	} else {
		msleep(100);
		rc = vreg_disable(vreg_bt);
		if (rc) {
			printk(KERN_ERR "%s: vreg disable failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}
		msleep(100);
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_off); pin++) {
			rc = gpio_tlmm_config(bt_config_power_off[pin],
					      GPIO_CFG_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_off[pin], rc);
				return -EIO;
			}
		}
		printk(KERN_ERR "BlueZ required power down * QCOM\r\n");
		gpio_direction_output(94,0);
		gpio_direction_output(20,0);
	}
	return 0;
}


#define HCIT_VERSION	"0.1"

static int hcit_probe(struct platform_device *pdev)
{
	return 0;
}

static int __devexit hcit_remove(struct platform_device *pdev)
{

	return 0;
}

static int hcit_suspend(struct platform_device *dev,		pm_message_t state)
{

	return 0;
}

static int hcit_resume(struct platform_device *dev)
{

	return 0;
}
#define HCIT_NAME "hcit_misc"

static struct platform_driver hcit_driver = {
	.probe     = hcit_probe,
	.remove   = hcit_remove,
	.suspend  = hcit_suspend,
	.resume   = hcit_resume,
	.driver		= {
		.name	= "hcit",
		.owner	= THIS_MODULE,
	},
};

static struct platform_device device_hcit = {
	.name   = "hcit",
	.id = -1,
};

static const struct file_operations hcit_fops = {/*SWH*/
	.owner		= THIS_MODULE,
	.read		= hcit_read,
	.ioctl             = hcit_ioctl,
};

static struct miscdevice hcit_miscdev = {
	.fops		= &hcit_fops,
	.name		= HCIT_NAME,
	.minor		= MISC_DYNAMIC_MINOR,
};

extern void get_sd_boot_mode(unsigned *pMode);
static int __init hcit_init(void)
{
      get_sd_boot_mode(sd_boot_mode);
      printk(KERN_ERR "[boot] HCIT read boot mode ={<0x%x>, <0x%x>} \n", sd_boot_mode[0], sd_boot_mode[1]);

	platform_device_register(&device_hcit);
	platform_driver_register(&hcit_driver);
	misc_register(&hcit_miscdev);

   return 0;
}

static void __exit hcit_exit(void)
{
	misc_deregister(&hcit_miscdev);
	platform_driver_unregister(&hcit_driver);
   platform_device_unregister(&device_hcit);

}

module_init(hcit_init);
module_exit(hcit_exit);

MODULE_AUTHOR("SWH");
MODULE_DESCRIPTION("Provides hcit.");
MODULE_LICENSE("GPL");
MODULE_VERSION(HCIT_VERSION);

