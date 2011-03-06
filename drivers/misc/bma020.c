



#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysctl.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>

#include <linux/i2c.h>
#include <mach/bma020.h>


static struct i2c_client *this_client;

static unsigned int PowerState = 1;

static MON_XYZ_INT 	cal45A;
static MON_XYZ_INT 	cal45B;
static MON_CAL_VAL	calvalue;

static const struct i2c_device_id bma_id[] = {
	{ "bma020", 0 },
	{ }
};

static u8 i2c_bma_read(u8 reg)
{
	if (this_client == NULL)	/* No global client pointer? */
		return -1;
	return i2c_smbus_read_byte_data(this_client, reg);
}

static u8 i2c_bma_readnbytes(u8 reg, u8 * pdata, u8 len)
{
	if (this_client == NULL)
		return -1;
	
	i2c_master_send(this_client, &reg, sizeof(reg));
	i2c_master_recv(this_client, pdata, len);

	//bma_i2c_tx_data(&reg,sizeof(reg));
	//bma_i2c_rx_data(pdata,len);
	return 0;
}

static int i2c_bma_write(u8 reg, u8 value)
{
	if (this_client == NULL)
		return -1;
	return i2c_smbus_write_byte_data(this_client, reg, value);	
}

static bool getxyz(PMON_XYZINT pOutPut)
{
	union Data16bit temp16;
	MON_XYZ_VALBITS TempXZY;
	u8 buffer[6];
	u8 temp;
	signed int temp32;

	// se should check the time or the int line, if the int line isn't low, we do nothing

	i2c_bma_readnbytes(0x2, buffer, 0x6);

	// out put the data
	temp16.Udata16 = 0;
	temp16.HByte |= buffer[3];
	temp16.LByte |= buffer[2];
	if (temp16.signbit)
	{
		temp16.signbit = 0;
		TempXZY.YVal = temp16.Udata16 >> 6;
	}
	else
	{
		temp16.signbit = 0;
		TempXZY.YVal = 0 - (temp16.Udata16 >> 6);
	}
	pOutPut->YVal = TempXZY.YVal;

	//printk( "[MON Ucaly] %d\r\n",pOutPut->YVal);
	temp32 = (pOutPut->YVal - calvalue.xyz.YVal);
	pOutPut->YVal = (temp32 * BMA020CAL_SCALE) /(calvalue.scaley);
	//printk( "[MON caly] %d\r\n",pOutPut->YVal);
	
	temp16.Udata16 = 0;
	temp16.HByte |= buffer[1];
	temp16.LByte |= buffer[0];
	if (temp16.signbit)
	{
		temp16.signbit = 0;
		TempXZY.XVal = temp16.Udata16 >> 6;
	}
	else
	{
		temp16.signbit = 0;
		TempXZY.XVal = 0 - (temp16.Udata16 >> 6);
	}
	pOutPut->XVal = TempXZY.XVal;

	//printk( "[MON Ucalx] %d\r\n",pOutPut->XVal);
	temp32 = (pOutPut->XVal - calvalue.xyz.XVal);
	pOutPut->XVal = (temp32 * BMA020CAL_SCALE) /(calvalue.scalex);
	//printk( "[MON calx] %d\r\n",pOutPut->XVal);
	
	temp16.Udata16 = 0;
	temp16.HByte |= buffer[5];
	temp16.LByte |= buffer[4];
	if (temp16.signbit)
	{
		temp16.signbit = 0;
		TempXZY.ZVal = temp16.Udata16 >> 6;
	}
	else
	{
		temp16.signbit = 0;
		TempXZY.ZVal = 0 - (temp16.Udata16 >> 6);
	}
	pOutPut->ZVal = TempXZY.ZVal;
	//printk("[MON ucalz] %d\r\n",pOutPut->ZVal);

	temp = i2c_bma_read(BMA_ID);

	return true;

}


static bool getxyz8(signed char * data)
{
	u8 buffer[6];
	u8 temp;
	// se should check the time or the int line, if the int line isn't low, we do nothing

	//temp = i2c_bma_read(BMA_STATUS);
	//printk( "bma020 status = 0x%x\n",temp);	

	i2c_bma_readnbytes(0x2, buffer, 0x6);
	data[0] = buffer[1];
	data[1] = buffer[3];
	data[2] = buffer[5];

	temp = i2c_bma_read(BMA_ID);
	//printk( "bma020 ID = 0x%x\n",temp);	
	return true;

}

static void DoCal_Step1()
{
    getxyz(&cal45A);
	printk("DoCal_Step1 x = %d, y = %d\n", cal45A.XVal, cal45A.YVal);
}

static void DoCal_Step2()
{
    getxyz(&cal45B);
	printk("DoCal_Step2 x = %d, y = %d\n", cal45B.XVal, cal45B.YVal);
}

static int DoCal_Bosch()
{
    MON_XYZ_INT calNumber;

	printk("DoCal_Bosch\n");

	calvalue.xyz.XVal = 0;
	calvalue.xyz.YVal = 0;
	calvalue.xyz.ZVal = 0;
	calvalue.scalex = BMA020CAL_SCALE;
	calvalue.scaley = BMA020CAL_SCALE;
	
    getxyz(&calNumber);

	if((ABS(calNumber.XVal) > BMA020CAL_MAX_OFFSET) || (ABS(calNumber.YVal) > BMA020CAL_MAX_OFFSET))
	{
		printk("DoCal_Bosch large offset, cal fail\n");
		return -1;
	}
	else
	{
    	calvalue.xyz.XVal = calNumber.XVal;
    	calvalue.xyz.YVal = calNumber.YVal;
		return 0;
	}
	
    //CalData.calNumber.ZVal = calNumber.ZVal;
	//calvalue.scalex = BMA020CAL_SCALE;
	//calvalue.scaley = BMA020CAL_SCALE;

	//store the cal value to file so that next time read it out

}
static int motion_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -ENOIOCTLCMD;
    //printk("motion_ioctl++\r\n");

	switch (cmd) {
		
		case MOTION_GETXYZ:
		    //printk("MOTION_GETXYZ\r\n");

			if (!PowerState)
				ret = -EBUSY;
			else if (arg)
			{
				MON_XYZ_INT* OutPut = (MON_XYZ_INT*) arg;
				getxyz(OutPut);
				//printk("MOTION_GETXYZ X %d\r\n",OutPut->XVal);
       			//printk("MOTION_GETXYZ Y %d\r\n",OutPut->YVal);
      		    //printk("MOTION_GETXYZ Z %d\r\n",OutPut->ZVal);
				ret = 0;
			}
			break;

		case MOTION_GETXYZ8:
		    //printk("MOTION_GETXYZ8\r\n");
			if (!PowerState)
				ret = -EBUSY;
			else if  (arg)
			{
				signed char * cindex = (signed char *) arg;
				getxyz8(cindex);
 		       	//printk("XYZ %d %d %d\r\n", cindex[0], cindex[1], cindex[2]);
				ret = 0;
			}
			break;

		case MOTION_DOCAL:
		    printk("MOTION_DOCAL\r\n");
			if (!PowerState)
				ret = -EBUSY;
			else if (arg)
			{
				MON_CAL_VAL* pCaldata = (MON_CAL_VAL*) arg;
				ret = DoCal_Bosch();
				
				pCaldata->xyz.XVal = calvalue.xyz.XVal;
				pCaldata->xyz.YVal = calvalue.xyz.YVal;
				pCaldata->xyz.ZVal = calvalue.xyz.ZVal;
				pCaldata->scalex = calvalue.scalex;
				pCaldata->scaley = calvalue.scaley;
				// give the data to app to store
 		       	printk("XYZ %d %d %d, scalx %d, scaly %d\r\n", calvalue.xyz.XVal, calvalue.xyz.YVal, calvalue.xyz.ZVal, calvalue.scalex, calvalue.scaley);
			}
			break;

		case MOTION_DOCAL_STEP1:
			if (!PowerState)
				ret = -EBUSY;

			calvalue.xyz.XVal = 0;
			calvalue.xyz.YVal = 0;
			calvalue.xyz.ZVal = 0;
			calvalue.scalex = BMA020CAL_SCALE;
			calvalue.scaley = BMA020CAL_SCALE;
			
			DoCal_Step1();
			break;
			
		case MOTION_DOCAL_STEP2:
			if (!PowerState)
				ret = -EBUSY;
			
			if (arg)
			{
				MON_CAL_VAL* pCaldata = (MON_CAL_VAL*) arg;
				DoCal_Step2();

				calvalue.xyz.XVal = cal45B.XVal;
				calvalue.xyz.YVal = cal45A.YVal;
				calvalue.xyz.ZVal = 0;
				calvalue.scalex = cal45B.XVal - cal45A.XVal;
				calvalue.scaley = cal45B.YVal - cal45A.YVal;

				// check the data
				if ((ABS(calvalue.xyz.XVal) > BMA020CAL_MAX_OFFSET)
					||(ABS(calvalue.xyz.YVal) > BMA020CAL_MAX_OFFSET)
					||(calvalue.scalex < (BMA020CAL_SCALE - BMA020CAL_MAX_OFFSET))
					||(calvalue.scalex > (BMA020CAL_SCALE + BMA020CAL_MAX_OFFSET))
					||(calvalue.scaley < (BMA020CAL_SCALE - BMA020CAL_MAX_OFFSET))
					||(calvalue.scaley > (BMA020CAL_SCALE + BMA020CAL_MAX_OFFSET))
				)
				{
		    		printk("large offset exit cal\r\n");
					calvalue.xyz.XVal = 0;
					calvalue.xyz.YVal = 0;
					calvalue.xyz.ZVal = 0;
					calvalue.scalex = BMA020CAL_SCALE;
					calvalue.scaley = BMA020CAL_SCALE;
					ret = -1;
				}
				else
				{
					ret = 0;
				}
				
				pCaldata->xyz.XVal = calvalue.xyz.XVal;
				pCaldata->xyz.YVal = calvalue.xyz.YVal;
				pCaldata->xyz.ZVal = calvalue.xyz.ZVal;
				pCaldata->scalex = calvalue.scalex;
				pCaldata->scaley = calvalue.scaley;
				// give the data to app to store
 		       	printk("XYZ %d %d %d, scalx %d, scaly %d\r\n", calvalue.xyz.XVal, calvalue.xyz.YVal, calvalue.xyz.ZVal, calvalue.scalex, calvalue.scaley);
			}
			
			break;

		case MOTION_SETCAL:
		    printk("MOTION_SETCAL\r\n");
			if  (arg)
			{
				MON_CAL_VAL* pCaldata = (MON_CAL_VAL*) arg;
				calvalue.xyz.XVal = pCaldata->xyz.XVal;
				calvalue.xyz.YVal = pCaldata->xyz.YVal;
				calvalue.xyz.ZVal = pCaldata->xyz.ZVal;

				if (pCaldata->scalex && pCaldata->scaley)
				{
					calvalue.scalex = pCaldata->scalex;
					calvalue.scaley = pCaldata->scaley;
				}
				else
				{
					calvalue.scalex = BMA020CAL_SCALE;
					calvalue.scaley = BMA020CAL_SCALE;
				}
				
				//if (copy_to_user(&CalData, arg, sizeof(CalData)))
				//	ret = -EFAULT;
 		       	printk("XYZ %d %d %d, scalx %d, scaly %d\r\n", calvalue.xyz.XVal, calvalue.xyz.YVal, calvalue.xyz.ZVal, calvalue.scalex, calvalue.scaley);
				ret = 0;
			}
			
		default:
			break;
	}

	//printk("motion_ioctl--\r\n");

    return ret;
}

static struct file_operations motion_fops = {
	owner:		THIS_MODULE,
	ioctl:		motion_ioctl,
};

static struct miscdevice motion_misc = {
	minor : MOTION_MINOR,
	name  : "mot",
	fops  : &motion_fops,
};

static int bma020_probe(
	struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	unsigned char data = 0;

	printk( "bma020_probe\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: functionality check failed\n", __FUNCTION__);
		ret = -ENODEV;
	}
	this_client = client;

	data = i2c_bma_read(BMA_ID);
	printk( "bma020 ID = 0x%x\n",data);

	if (data != 0x2)
	{
		printk( "bma020 not supported sensor, return\n");
		return -1;
	}

	ret = misc_register(&motion_misc);
	if (ret)
	{
		pr_err("%s: bma020_device register failed\n", __FUNCTION__);
	}

	// disable interrupt mode
    data = i2c_bma_read(BMA_CONTROL1);
	//data |= (1<<5);
	data &= ~(1<<5);
	i2c_bma_write(BMA_CONTROL1, data);

	i2c_bma_write(BMA_STATUS, 0);	

   	data = i2c_bma_read(BMA_CONTROL2);
	// set scale to 2G
   	data &= ~(3<<3);
	// set bandwidth to 50Hz	
   	data &= ~(7);
	data |= (1);
	i2c_bma_write(BMA_CONTROL2, data);	
	
    data = i2c_bma_read(0xB);
	printk( "bma020 0xB = 0x%x\n",data);

	// set default cal value
	calvalue.xyz.XVal = 0;
	calvalue.xyz.YVal = 0;
	calvalue.xyz.ZVal = 0;
	calvalue.scalex = BMA020CAL_SCALE;
	calvalue.scaley = BMA020CAL_SCALE;

	return 0;
}


static int bma020_remove(struct i2c_client *client)
{
	return 0;
}

static int bma020_pm_resume(struct device *dev)
{
	PowerState = 1;
	printk( "bma020_pm_resume\n");
	i2c_bma_write(BMA_STATUS, 0);	
	return 0;
}

static int bma020_pm_suspend(struct device *dev)
{
	PowerState = 0;
	printk( "bma020_pm_suspend\n");
	i2c_bma_write(BMA_STATUS, 1);	
	return 0;
}

static const struct dev_pm_ops bma_pm_ops = {
	.suspend = bma020_pm_suspend,
	.resume = bma020_pm_resume,
};

static struct i2c_driver i2c_bma020_driver = {
	.probe = bma020_probe,
	.remove = bma020_remove,
	.id_table = bma_id,
	.driver = {
		.pm = &bma_pm_ops,
		.name = "bma020",
		},
};
static int __init bma020_init(void)
{
	int ret;
	printk( "bma020_init\n");

	if ((ret = i2c_add_driver(&i2c_bma020_driver))) {
		printk("bma020: Driver registration failed," " module not inserted.\n");
	}

	printk("bma020_init--\n");
	return ret;
}

static void __exit bma020_exit(void)
{
	printk( "bma020_exit\n");

	i2c_del_driver(&i2c_bma020_driver);
} 

module_init(bma020_init);
module_exit(bma020_exit);

MODULE_AUTHOR("liguangyu <guangyu.li@sim.com>");
MODULE_DESCRIPTION("bma020 Driver");
MODULE_LICENSE("GPL");
