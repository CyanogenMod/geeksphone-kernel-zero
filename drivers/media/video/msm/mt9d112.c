/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include "mt9d112.h"

/* Micron MT9D112 Registers and their values */
/* Sensor Core Registers */
#define  REG_MT9D112_MODEL_ID 0x0000
#define  MT9D112_MODEL_ID     0x2880
#define  MT9D112_AF_STATUS    0xB000
/*  SOC Registers Page 1  */
#define  REG_MT9D112_SENSOR_RESET     0x301A
#define  REG_MT9D112_STANDBY_CONTROL  0x3202
#define  REG_MT9D112_MCU_BOOT         0x3386

struct mt9d112_work {
	struct work_struct work;
};

static struct  mt9d112_work *mt9d112_sensorw;
static struct  i2c_client *mt9d112_client;

struct mt9d112_ctrl {
	const struct msm_camera_sensor_info *sensordata;
};


static struct mt9d112_ctrl *mt9d112_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(mt9d112_wait_queue);
DECLARE_MUTEX(mt9d112_sem);
static int16_t mt9d112_effect = CAMERA_EFFECT_OFF;
static int16_t mt9d112_scene = CAMERA_BESTSHOT_OFF;

/*=============================================================
	EXTERNAL DECLARATIONS
==============================================================*/
extern struct mt9d112_reg mt9d112_regs;


/*=============================================================*/

static int mt9d112_reset(const struct msm_camera_sensor_info *dev)
{
	int rc = 0;

	CDBG("mt9d112_reset \n");

	rc = gpio_request(dev->sensor_reset, "mt9d112");

	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 1);
		mdelay(10);
		rc = gpio_direction_output(dev->sensor_reset, 0);
		mdelay(30);
		rc = gpio_direction_output(dev->sensor_reset, 1);
		mdelay(10);
	}

	gpio_free(dev->sensor_reset);
	return rc;
}

static int32_t mt9d112_i2c_txdata(unsigned short saddr,
	unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};

	if (i2c_transfer(mt9d112_client->adapter, msg, 1) < 0) {
		CDBG("mt9d112_i2c_txdata failed\n");
		return -EIO;
	}

	return 0;
}

static int32_t mt9d112_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum mt9d112_width width)
{
	int32_t rc = -EIO;
	unsigned char buf[4];

	memset(buf, 0, sizeof(buf));
	switch (width) {
	case WORD_LEN: {
		buf[0] = (waddr & 0xFF00)>>8;
		buf[1] = (waddr & 0x00FF);
		buf[2] = (wdata & 0xFF00)>>8;
		buf[3] = (wdata & 0x00FF);

		rc = mt9d112_i2c_txdata(saddr, buf, 4);
	}
		break;

	case BYTE_LEN: {
		buf[0] = (waddr & 0xFF00)>>8;
		buf[1] = (waddr & 0x00FF);
		buf[2] = (wdata & 0x00FF) ;
		rc = mt9d112_i2c_txdata(saddr, buf, 3);
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		CDBG("i2c_write failed, addr = 0x%x, val = 0x%x!\n",waddr, wdata);

	return rc;
}

static int32_t mt9d112_i2c_write_table(
	struct mt9d112_i2c_reg_conf const *reg_conf_tbl,
	int num_of_items_in_table)
{
	int i;
	int32_t rc = -EIO;

	for (i = 0; i < num_of_items_in_table; i++) {
		rc = mt9d112_i2c_write(mt9d112_client->addr,
			reg_conf_tbl->waddr, reg_conf_tbl->wdata,
			reg_conf_tbl->width);
		if (rc < 0)
			break;
		if (reg_conf_tbl->mdelay_time != 0)
			msleep(reg_conf_tbl->mdelay_time);
		reg_conf_tbl++;
	}

	return rc;
}

static int mt9d112_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
	{
		.addr   = saddr,
		.flags = 0,
		.len   = 2,
		.buf   = rxdata,
	},
	{
		.addr   = saddr,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

	if (i2c_transfer(mt9d112_client->adapter, msgs, 2) < 0) {
		CDBG("mt9d112_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t mt9d112_i2c_read(unsigned short   saddr,
	unsigned short raddr, unsigned short *rdata, enum mt9d112_width width)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	switch (width) {
	case WORD_LEN: {
		buf[0] = (raddr & 0xFF00)>>8;
		buf[1] = (raddr & 0x00FF);

		rc = mt9d112_i2c_rxdata(saddr, buf, 2);
		if (rc < 0)
			return rc;

		*rdata = buf[0] << 8 | buf[1];
	}
		break;
	case BYTE_LEN: {
		buf[0] = (raddr & 0xFF00)>>8;
		buf[1] = (raddr & 0x00FF);

		rc = mt9d112_i2c_rxdata(saddr, buf, 1);
		if (rc < 0)
			return rc;

		*rdata = buf[0];
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		CDBG("mt9d112_i2c_read failed!\n");

	return rc;
}

static int32_t mt9d112_set_lens_roll_off(void)
{
	int32_t rc = 0;
	//rc = mt9d112_i2c_write_table(&mt9d112_regs.rftbl[0],
	//							 mt9d112_regs.rftbl_size);
	return rc;
}

static long mt9d112_reg_init(void)
{
	int32_t array_length;
	int32_t i;
	long rc;

	CDBG("mt9d112_reg_init\n");

    // init
	rc = mt9d112_i2c_write_table(&mt9d112_regs.init_tbl[0],
					mt9d112_regs.init_tbl_size);

		if (rc < 0)
			return rc;

	CDBG("mt9d112_reg_init:success!\n");


	return 0;
}

static long mt9d112_set_effect(int mode, int effect)
{
	long rc = 0;

	CDBG("mt9d112_set_effect:mode[%d],effect[%d]\n", mode,effect);
	switch (effect) {		
	case CAMERA_EFFECT_OFF:
		rc = mt9d112_i2c_write_table(&mt9d112_regs.EFFECT_OFF_tbl[0],
						mt9d112_regs.EFFECT_OFF_tbl_size);
		//mdelay(200);		
		break;

	case CAMERA_EFFECT_MONO:
		rc = mt9d112_i2c_write_table(&mt9d112_regs.EFFECT_MONO_tbl[0],
						mt9d112_regs.EFFECT_MONO_tbl_size);
		//mdelay(200);		
		break;

	case CAMERA_EFFECT_SEPIA:
		rc = mt9d112_i2c_write_table(&mt9d112_regs.EFFECT_SEPIA_tbl[0],
						mt9d112_regs.EFFECT_SEPIA_tbl_size);
		//mdelay(200);		
		break;

	case CAMERA_EFFECT_NEGATIVE:
		rc = mt9d112_i2c_write_table(&mt9d112_regs.EFFECT_NEGATIVE_tbl[0],
						mt9d112_regs.EFFECT_NEGATIVE_tbl_size);
		//mdelay(200);		
		break;

	case CAMERA_EFFECT_SOLARIZE:
		rc = mt9d112_i2c_write_table(&mt9d112_regs.EFFECT_SOLARIZE_tbl[0],
						mt9d112_regs.EFFECT_SOLARIZE_tbl_size);
		//mdelay(200);		
			break;

	default: 
		return -EINVAL;
	}
			return rc;
}

static long mt9d112_set_wb(int mode, int wb)
{
	long rc = 0;	
	CDBG("mt9d112_set_wb:mode[%d],wb[%d]\n", mode,wb);
	switch(wb){
	case CAMERA_WB_AUTO:
    	rc = mt9d112_i2c_write_table(&mt9d112_regs.awb_tbl[0],
    					mt9d112_regs.awb_tbl_size);

		//mdelay(200);		
		break;
	case CAMERA_WB_INCANDESCENT :
    	rc = mt9d112_i2c_write_table(&mt9d112_regs.MWB_INCANDESCENT_tbl[0],
    					mt9d112_regs.MWB_INCANDESCENT_tbl_size);

		//mdelay(200);		
		break;
	case CAMERA_WB_FLUORESCENT: 
    	rc = mt9d112_i2c_write_table(&mt9d112_regs.MWB_FLUORESCENT_tbl[0],
    					mt9d112_regs.MWB_FLUORESCENT_tbl_size);

		//mdelay(200);		
		break;
	case CAMERA_WB_DAYLIGHT:
    	rc = mt9d112_i2c_write_table(&mt9d112_regs.MWB_Day_light_tbl[0],
    					mt9d112_regs.MWB_Day_light_tbl_size);

		//mdelay(200);		
		break;
	case CAMERA_WB_CLOUDY_DAYLIGHT:
    	rc = mt9d112_i2c_write_table(&mt9d112_regs.MWB_Cloudy_tbl[0],
    					mt9d112_regs.MWB_Cloudy_tbl_size);
		//mdelay(200);		
		break;
	default: 
		return -EINVAL;
		}
			return rc;
}




static int32_t mt9d112_set_default_focus(void)
{
	long rc = 0;
	uint16_t af_status = 0;
	uint16_t icount,itotal;
	CDBG("mt9d112_set_default_focus\n");
	
	rc = mt9d112_i2c_write_table(&mt9d112_regs.VCM_Enable_full_scan_tbl[0],
					mt9d112_regs.VCM_Enable_full_scan_tbl_size);
	mdelay(10);
	rc = mt9d112_i2c_write_table(&mt9d112_regs.AF_Trigger_tbl[0],
					mt9d112_regs.AF_Trigger_tbl_size);

	//rc = mt9d112_i2c_write_table(&mt9d112_regs.VCM_Enable_Continue_scan_tbl[0],
					//mt9d112_regs.VCM_Enable_Continue_scan_tbl_size);

    if(mt9d112_scene==CAMERA_BESTSHOT_NIGHT)
        itotal=60;
    else
        itotal=30;
	for(icount=0; icount<itotal; icount++)
	{
		rc = mt9d112_i2c_read(mt9d112_client->addr,
			MT9D112_AF_STATUS, &af_status, WORD_LEN);
		CDBG("mt9d112_set_default_focus:mt9d112 af_status = 0x%x  icount=%d\n", af_status,icount);		
		if(af_status&(0x1<<4))
		{	
			return rc;
	}
		msleep(100);
	}

	return 0;
}

static int32_t mt9d112_set_scene(int scene)
{
	long rc = 0;

	CDBG("mt9d112_set_scene:scene=[%d]\n",scene);
	switch (scene) {		
	case CAMERA_BESTSHOT_OFF:
		rc = mt9d112_i2c_write_table(&mt9d112_regs.SCENE_AUTO_tbl[0],
							mt9d112_regs.SCENE_AUTO_tbl_size);
		//mdelay(200);		
		break;

	case CAMERA_BESTSHOT_NIGHT:
		rc = mt9d112_i2c_write_table(&mt9d112_regs.SCENE_NIGHT_tbl[0],
							mt9d112_regs.SCENE_NIGHT_tbl_size);
		//mdelay(200);		
		break;

	default: 
		return -EINVAL;
	}

    mt9d112_scene=scene;
			return rc;
}


static long mt9d112_set_sensor_mode(int mode)
{
	uint16_t reg = 0;
    int i;
	long rc = 0;

	CDBG("mt9d112_set_sensor_mode:mode[%d]\n", mode);

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
    	//rc = mt9d112_reg_init();

    	rc = mt9d112_i2c_write_table(&mt9d112_regs.preview_tbl[0],
    					mt9d112_regs.preview_tbl_size);
		//msleep(100);
		break;

	case SENSOR_SNAPSHOT_MODE:
    	rc = mt9d112_i2c_write_table(&mt9d112_regs.snapshot_tbl[0],
    					mt9d112_regs.snapshot_tbl_size);
    	for(i=0; i<50; i++)
    	{
            mt9d112_i2c_write(mt9d112_client->addr,0x098E,0x8405,WORD_LEN);	        
            mt9d112_i2c_read(mt9d112_client->addr,0x8405, &reg, BYTE_LEN);
    		CDBG("mt9d112_set_sensor_mode:0x8405 = 0x%x  i=%d\n", reg,i);
    		if(reg==0x07)
    			break;        
    		msleep(100);
    	}
		break;

	case SENSOR_RAW_SNAPSHOT_MODE:
    	rc = mt9d112_i2c_write_table(&mt9d112_regs.snapshot_tbl[0],
    					mt9d112_regs.snapshot_tbl_size);
    	for(i=0; i<50; i++)
    	{
            mt9d112_i2c_write(mt9d112_client->addr,0x098E,0x8405,WORD_LEN);	        
            mt9d112_i2c_read(mt9d112_client->addr,0x8405, &reg, BYTE_LEN);
    		CDBG("mt9d112_set_sensor_mode:0x8405 = 0x%x  i=%d\n", reg,i);
    		if(reg==0x07)
    			break;        
    		msleep(100);
    	}
		break;

	default:
		return -EINVAL;
	}

	return rc;
}

static int mt9d112_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	uint16_t model_id = 0;
	int rc = 0;

	//CDBG("init entry \n");
	CDBG("mt9d112_sensor_init_probe \n");

	rc = mt9d112_reset(data);
	if (rc < 0)
		goto init_probe_fail;


	/* Read the Model ID of the sensor */
	rc = mt9d112_i2c_read(mt9d112_client->addr,
		REG_MT9D112_MODEL_ID, &model_id, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;

	CDBG("mt9d112_sensor_init_probe:mt9d112 model_id = 0x%x\n", model_id);

	/* Check if it matches it with the value in Datasheet */
	if (model_id != MT9D112_MODEL_ID) {
		rc = -EINVAL;
		goto init_probe_fail;
	}

	rc = mt9d112_reg_init();
	if (rc < 0)
		goto init_probe_fail;

	return rc;

init_probe_fail:
    
	CDBG("mt9d112_sensor_init_probe:failed \n");
	return rc;
}

int mt9d112_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	CDBG("mt9d112_sensor_init\n");

	mt9d112_ctrl = kzalloc(sizeof(struct mt9d112_ctrl), GFP_KERNEL);
	if (!mt9d112_ctrl) {
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		mt9d112_ctrl->sensordata = data;

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);
	mdelay(5);

	msm_camio_camif_pad_reg_reset();

	rc = mt9d112_sensor_init_probe(data);

	if (rc < 0) {
		goto init_fail;
	}

init_done:
	return rc;

init_fail:
	kfree(mt9d112_ctrl);
	return rc;
}

static int mt9d112_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&mt9d112_wait_queue);
	return 0;
}

int mt9d112_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	if (copy_from_user(&cfg_data,
			(void *)argp,
			sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	CDBG("mt9d112_sensor_config : cfgtype [%d], mode[%d]\n",cfg_data.cfgtype, cfg_data.mode);

		switch (cfg_data.cfgtype) {
		case CFG_SET_MODE:
			rc = mt9d112_set_sensor_mode(cfg_data.mode);
			break;
		case CFG_SET_EFFECT:
			rc = mt9d112_set_effect(cfg_data.mode,cfg_data.cfg.effect);
			break;
		case CFG_SET_WB://add by lijiankun 2010-9-3
			rc = mt9d112_set_wb(cfg_data.mode,cfg_data.cfg.wb);
			break;
		case CFG_SET_DEFAULT_FOCUS:
			rc = mt9d112_set_default_focus();
			break;
		case CFG_SET_SCENE://add by lijiankun 2010-9-3
			rc = mt9d112_set_scene(cfg_data.cfg.scene);
			break;
		default:
			rc = -EINVAL;
			break;
		}


	return rc;
}

int mt9d112_sensor_release(void)
{
	int rc = 0;

	CDBG("mt9d112_sensor_release!\n");


	kfree(mt9d112_ctrl);

	return rc;
}

static int mt9d112_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	mt9d112_sensorw =
		kzalloc(sizeof(struct mt9d112_work), GFP_KERNEL);

	if (!mt9d112_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, mt9d112_sensorw);
	mt9d112_init_client(client);
	mt9d112_client = client;

	CDBG("mt9d112_i2c_probe succeeded!\n");

	return 0;

probe_failure:
	kfree(mt9d112_sensorw);
	mt9d112_sensorw = NULL;
	CDBG("mt9d112_i2c_probe failed!\n");
	return rc;
}

static const struct i2c_device_id mt9d112_i2c_id[] = {
	{ "mt9d112", 0},
	{ },
};

static struct i2c_driver mt9d112_i2c_driver = {
	.id_table = mt9d112_i2c_id,
	.probe  = mt9d112_i2c_probe,
	.remove = __exit_p(mt9d112_i2c_remove),
	.driver = {
		.name = "mt9d112",
	},
};

static int mt9d112_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
	int rc = i2c_add_driver(&mt9d112_i2c_driver);
	if (rc < 0 || mt9d112_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_done;
	}

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);
	mdelay(5);

	rc = mt9d112_sensor_init_probe(info);

	if (rc < 0){
		i2c_del_driver(&mt9d112_i2c_driver);     
		goto probe_done;
	}
	s->s_init = mt9d112_sensor_init;
	s->s_release = mt9d112_sensor_release;
	s->s_config  = mt9d112_sensor_config;

probe_done:
	//CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;
}

static int __mt9d112_probe(struct platform_device *pdev)
{
	CDBG("__mt9d112_probe...\n");

	return msm_camera_drv_start(pdev, mt9d112_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __mt9d112_probe,
	.driver = {
		.name = "msm_camera_mt9d112",
		.owner = THIS_MODULE,
	},
};

static int __init mt9d112_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(mt9d112_init);
