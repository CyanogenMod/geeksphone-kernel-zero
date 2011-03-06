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
#include "ov7690.h"

/* Micron OV7690 Registers and their values */
/* Sensor Core Registers */
#define  REG_OV7690_MODEL_ID 0x0a
#define  OV7690_MODEL_ID     0x76


struct ov7690_work {
	struct work_struct work;
};

static struct  ov7690_work *ov7690_sensorw;
static struct  i2c_client *ov7690_client;

struct ov7690_ctrl {
	const struct msm_camera_sensor_info *sensordata;
};


static struct ov7690_ctrl *ov7690_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(ov7690_wait_queue);
DECLARE_MUTEX(ov7690_sem);
static int16_t ov7690_effect = CAMERA_EFFECT_OFF;

/*=============================================================
	EXTERNAL DECLARATIONS
==============================================================*/
extern struct ov7690_reg ov7690_regs;


/*=============================================================*/

static int ov7690_reset(const struct msm_camera_sensor_info *dev)
{
	int rc = 0;

	CDBG("ov7690_reset \n");

/*
	rc = gpio_request(dev->sensor_reset, "ov7690");

	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 1);
		mdelay(30);
		rc = gpio_direction_output(dev->sensor_reset, 0);
		mdelay(30);
		rc = gpio_direction_output(dev->sensor_reset, 1);
		mdelay(30);
	}

	gpio_free(dev->sensor_reset);
*/    
	return rc;
}

static int32_t ov7690_i2c_txdata(unsigned short saddr,
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

	if (i2c_transfer(ov7690_client->adapter, msg, 1) < 0) {
		CDBG("ov7690_i2c_txdata failed\n");
		return -EIO;
	}

	return 0;
}

static int32_t ov7690_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum ov7690_width width)
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

		rc = ov7690_i2c_txdata(saddr, buf, 4);
	}
		break;

	case BYTE_LEN: {
		buf[0] = (waddr & 0x00FF);
		buf[1] = (wdata & 0x00FF) ;
		rc = ov7690_i2c_txdata(saddr, buf, 2);
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		CDBG("i2c_write failed, addr = 0x%x, val = 0x%x!\n",waddr, wdata);

	return rc;
}

static int32_t ov7690_i2c_write_table(
	struct ov7690_i2c_reg_conf const *reg_conf_tbl,
	int num_of_items_in_table)
{
	int i;
	int32_t rc = -EIO;

	for (i = 0; i < num_of_items_in_table; i++) {
		rc = ov7690_i2c_write(ov7690_client->addr,
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

static int ov7690_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
	{
		.addr   = saddr,
		.flags = 0,
		.len   = 1,
		.buf   = rxdata,
	},
	{
		.addr   = saddr,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

	if (i2c_transfer(ov7690_client->adapter, msgs, 2) < 0) {
		CDBG("ov7690_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t ov7690_i2c_read(unsigned short   saddr,
	unsigned short raddr, unsigned short *rdata, enum ov7690_width width)
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

		rc = ov7690_i2c_rxdata(saddr, buf, 2);
		if (rc < 0)
			return rc;

		*rdata = buf[0] << 8 | buf[1];
	}
		break;
	case BYTE_LEN: {
		buf[0] = (raddr & 0x00FF);

		rc = ov7690_i2c_rxdata(saddr, buf, 1);
		if (rc < 0)
			return rc;

		*rdata = buf[0];
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		CDBG("ov7690_i2c_read failed!\n");

	return rc;
}

static int32_t ov7690_set_lens_roll_off(void)
{
	int32_t rc = 0;

	return rc;
}

static long ov7690_reg_init(void)
{
	int32_t array_length;
	int32_t i;
	long rc;

	CDBG("ov7690_reg_init\n");

    // init data
    /*
	array_length = ov7690_regs.init_settings_size;
	for (i = 0; i < array_length; i++) {
		rc = ov7690_i2c_write(ov7690_client->addr,
		    ov7690_regs.init_settings[i].register_address,
		    ov7690_regs.init_settings[i].register_value,
		    BYTE_LEN);
		if (rc < 0)
			return rc;
	}
    */
    
	rc = ov7690_i2c_write_table(&ov7690_regs.init_settings[0],ov7690_regs.init_settings_size);

	if (rc < 0)
		return rc;
    msleep(100);
    /*
    // qxga to xga
	array_length = ov7690_regs.qxga_xga_settings_size;
	for (i = 0; i < array_length; i++) {
		rc = ov7690_i2c_write(ov7690_client->addr,
		    ov7690_regs.qxga_xga_settings[i].register_address,
		    ov7690_regs.qxga_xga_settings[i].register_value,
		    BYTE_LEN);
		if (rc < 0)
			return rc;
	}
*/
		rc = ov7690_i2c_write_table(&ov7690_regs.qxga_xga_settings[0],ov7690_regs.qxga_xga_settings_size);
		
		if (rc < 0)
			return rc;
		
		CDBG("ov7690_reg_init:success!\n");

	return 0;
}
static int32_t ov7690_set_scene(int scene)
{
	long rc = 0;
	
	uint16_t af_status = 0;
	CDBG("ov7690_set_scene:scene=[%d]\n",scene);
	switch (scene) {		
	case CAMERA_BESTSHOT_OFF:
		CDBG("ov7690_set_scene:CAMERA_SCENE_AUTO\n");
		rc = ov7690_i2c_write_table(&ov7690_regs.scene_auto_tbl[0],
							ov7690_regs.scene_auto_tbl_size);
		
		//mdelay(200);		
		break;
			
	case CAMERA_BESTSHOT_NIGHT:

		CDBG("ov7690_set_scene:CAMERA_SCENE_NIGHT\n");
		rc = ov7690_i2c_write_table(&ov7690_regs.scene_night_tbl[0],
							ov7690_regs.scene_night_tbl_size);
		//mdelay(200);		
		break;

	default: 
		CDBG("ov7690_set_scene:default\n");
	}
	return rc;
}

static long ov7690_set_wb(int wb)
{
	long rc = 0;	

	switch(wb){
	case CAMERA_WB_AUTO:
		CDBG("ov7690_set_wb:CAMERA_WB_AUTO\n");
		
    	rc = ov7690_i2c_write_table(&ov7690_regs.awb_tbl[0],
    					ov7690_regs.awb_tbl_size);
    					
    	if (rc < 0)
    		return rc;
		//mdelay(200);		
		break;
	case CAMERA_WB_INCANDESCENT :
		CDBG("ov7690_set_wb:CAMERA_WB_INCANDESCENT\n");
    	rc = ov7690_i2c_write_table(&ov7690_regs.mwb_incandescent_tbl[0],
    					ov7690_regs.mwb_incandescent_tbl_size);
    	if (rc < 0)
    		return rc;
		//mdelay(200);		
		break;
	case CAMERA_WB_FLUORESCENT: 
		CDBG("ov7690_set_wb:CAMERA_WB_FLUORESCENT\n");
    	rc = ov7690_i2c_write_table(&ov7690_regs.mwb_fluorescent_tbl[0],
    					ov7690_regs.mwb_fluorescent_tbl_size);
    	if (rc < 0)
    		return rc;
		//mdelay(200);		
		break;
	case CAMERA_WB_DAYLIGHT:
		CDBG("ov7690_set_wb:CAMERA_WB_DAYLIGHT\n");
    	rc = ov7690_i2c_write_table(&ov7690_regs.mwb_day_light_tbl[0],
    					ov7690_regs.mwb_day_light_tbl_size);
    	if (rc < 0)
    		return rc;
		//mdelay(200);		
		break;
	case CAMERA_WB_CLOUDY_DAYLIGHT:
		CDBG("ov7690_set_wb:CAMERA_WB_CLOUDY_DAYLIGHT\n");
    	rc = ov7690_i2c_write_table(&ov7690_regs.mwb_cloudy_tbl[0],
    					ov7690_regs.mwb_cloudy_tbl_size);
    	if (rc < 0)
    		return rc;
		//mdelay(200);		
		break;
	default: 
		CDBG("ov7690_set_wb:default\n");
		}
	return rc;
}

static long ov7690_set_effect(int mode, int effect)
	{
	//uint16_t reg_addr;
	//uint16_t reg_val;
	long rc = 0;
	uint16_t sccb_value = 0;
	CDBG("ov7690_set_effect:mode[%d],effect[%d]\n", mode,effect);
#if 1
	switch (effect) {		
	case CAMERA_EFFECT_OFF:

		CDBG("ov7690_set_effect:CAMERA_EFFECT_OFF\n");

		rc = ov7690_i2c_write_table(&ov7690_regs.effect_off_tbl[0],
						ov7690_regs.effect_off_tbl_size);
		if (rc < 0)
			return rc;
		//mdelay(200);		


		break;
		
	case CAMERA_EFFECT_MONO:
		CDBG("ov7690_set_effect:CAMERA_EFFECT_MONO\n");

		rc = ov7690_i2c_write_table(&ov7690_regs.effect_mono_tbl[0],
						ov7690_regs.effect_mono_tbl_size);
		if (rc < 0)
			return rc;
		//mdelay(200);	
		break;
		
	case CAMERA_EFFECT_SEPIA:
		CDBG("ov7690_set_effect:CAMERA_EFFECT_SEPIA\n");

		rc = ov7690_i2c_write_table(&ov7690_regs.effect_sepia_tbl[0],
						ov7690_regs.effect_sepia_tbl_size);
		if (rc < 0)
			return rc;
		//mdelay(200);	
		break;

	case CAMERA_EFFECT_NEGATIVE:
		CDBG("ov7690_set_effect:CAMERA_EFFECT_NEGATIVE\n");	
		rc = ov7690_i2c_write_table(&ov7690_regs.effect_negative_tbl[0],
						ov7690_regs.effect_negative_tbl_size);

		//mdelay(200);	

		break;

	case CAMERA_EFFECT_SOLARIZE:
		CDBG("ov7690_set_effect:CAMERA_EFFECT_SOLARIZE\n");

		break;

	default: 
		CDBG("ov7690_set_effect:default\n");
	}
#endif
	return rc;
}

static long ov7690_set_sensor_mode(int mode)
{
	//uint16_t clock;
	long rc = 0;

	CDBG("ov7690_set_sensor_mode:mode[%d]\n", mode);

#if 0
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x338C, 0xA20C, WORD_LEN);
		if (rc < 0)
			return rc;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x3390, 0x0004, WORD_LEN);
		if (rc < 0)
			return rc;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x338C, 0xA215, WORD_LEN);
		if (rc < 0)
			return rc;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x3390, 0x0004, WORD_LEN);
		if (rc < 0)
			return rc;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x338C, 0xA20B, WORD_LEN);
		if (rc < 0)
			return rc;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x3390, 0x0000, WORD_LEN);
		if (rc < 0)
			return rc;

		clock = 0x0250;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x341C, clock, WORD_LEN);
		if (rc < 0)
			return rc;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x338C, 0xA103, WORD_LEN);
		if (rc < 0)
			return rc;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x3390, 0x0001, WORD_LEN);
		if (rc < 0)
			return rc;
		mdelay(5);

		break;

	case SENSOR_SNAPSHOT_MODE:
		/* Switch to lower fps for Snapshot */
		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x341C, 0x0120, WORD_LEN);
		if (rc < 0)
			return rc;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x338C, 0xA120, WORD_LEN);
		if (rc < 0)
			return rc;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x3390, 0x0002, WORD_LEN);
		if (rc < 0)
			return rc;

		mdelay(5);

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x338C, 0xA103, WORD_LEN);
		if (rc < 0)
			return rc;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x3390, 0x0002, WORD_LEN);
		if (rc < 0)
			return rc;
		break;

	case SENSOR_RAW_SNAPSHOT_MODE:
		/* Setting the effect to CAMERA_EFFECT_OFF */
		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x338C, 0x279B, WORD_LEN);
		if (rc < 0)
			return rc;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
			0x3390, 0x6440, WORD_LEN);
		if (rc < 0)
			return rc;

		/* Switch to lower fps for Snapshot */
		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x341C, 0x0120, WORD_LEN);
		if (rc < 0)
			return rc;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x338C, 0xA120, WORD_LEN);
		if (rc < 0)
			return rc;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x3390, 0x0002, WORD_LEN);
		if (rc < 0)
			return rc;

		mdelay(5);

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x338C, 0xA103, WORD_LEN);
		if (rc < 0)
			return rc;

		rc =
			mt9d112_i2c_write(mt9d112_client->addr,
				0x3390, 0x0002, WORD_LEN);
		if (rc < 0)
			return rc;
		break;

	default:
		return -EINVAL;
	}
    
#endif

	return 0;
}

static int ov7690_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	uint16_t model_id = 0;
	int rc = 0;

	//CDBG("init entry \n");
	CDBG("ov7690_sensor_init_probe \n");

	rc = ov7690_reset(data);
	if (rc < 0) 
		goto init_probe_fail;

	/* Soft reset */
	rc = ov7690_i2c_write(ov7690_client->addr,0x12,0x80,BYTE_LEN);
    if (rc < 0)
		return rc;

	msleep(50);

    
	rc = ov7690_reg_init();
	if (rc < 0)
		goto init_probe_fail;

	msleep(50);


	/* Read the Model ID of the sensor */
	rc = ov7690_i2c_read(ov7690_client->addr,
		REG_OV7690_MODEL_ID, &model_id, BYTE_LEN);
	if (rc < 0)
		goto init_probe_fail;

	CDBG("ov7690_sensor_init_probe:ov7690 model_id = 0x%x\n", model_id);

	/* Check if it matches it with the value in Datasheet */
	if (model_id != OV7690_MODEL_ID) {
		rc = -EINVAL;
		goto init_probe_fail;
	}


	return rc;

init_probe_fail:
    
	CDBG("ov7690_sensor_init_probe:failed\n");
	return rc;
}

int ov7690_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	CDBG("ov7690_sensor_init\n");

	ov7690_ctrl = kzalloc(sizeof(struct ov7690_ctrl), GFP_KERNEL);
	if (!ov7690_ctrl) {
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		ov7690_ctrl->sensordata = data;

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);
	msleep(10);

	msm_camio_camif_pad_reg_reset();

	rc = ov7690_sensor_init_probe(data);

	if (rc < 0) {
		goto init_fail;
	}

init_done:
	return rc;

init_fail:
	kfree(ov7690_ctrl);
	return rc;
}

static int ov7690_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&ov7690_wait_queue);
	return 0;
}

int ov7690_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	if (copy_from_user(&cfg_data,
			(void *)argp,
			sizeof(struct sensor_cfg_data)))
		return -EFAULT;


	CDBG("ov7690_sensor_config : cfgtype [%d], mode[%d]\n",cfg_data.cfgtype, cfg_data.mode);

		switch (cfg_data.cfgtype) {
		case CFG_SET_MODE:
			rc = ov7690_set_sensor_mode(cfg_data.mode);
			break;

		case CFG_SET_EFFECT:
			rc = ov7690_set_effect(cfg_data.mode,cfg_data.cfg.effect);
			break;

		
		case CFG_SET_WB:
			rc = ov7690_set_wb(cfg_data.cfg.wb);
			break;
		
		case CFG_SET_SCENE:
			rc = ov7690_set_scene(cfg_data.cfg.scene);
			break;
		
		case CFG_GET_AF_MAX_STEPS:
		default:
			rc = -EINVAL;
	}
	
	return rc;
}

int ov7690_sensor_release(void)
{
	int rc = 0;

	CDBG("ov7690_sensor_release!\n");

	kfree(ov7690_ctrl);

	return rc;
}

static int ov7690_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	ov7690_sensorw =
		kzalloc(sizeof(struct ov7690_work), GFP_KERNEL);

	if (!ov7690_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, ov7690_sensorw);
	ov7690_init_client(client);
	ov7690_client = client;

	CDBG("ov7690_i2c_probe succeeded!\n");

	return 0;

probe_failure:
	kfree(ov7690_sensorw);
	ov7690_sensorw = NULL;
	CDBG("ov7690_i2c_probe failed!\n");
	return rc;
}

static const struct i2c_device_id ov7690_i2c_id[] = {
	{ "ov7690", 0},
	{ },
};

static struct i2c_driver ov7690_i2c_driver = {
	.id_table = ov7690_i2c_id,
	.probe  = ov7690_i2c_probe,
	.remove = __exit_p(ov7690_i2c_remove),
	.driver = {
		.name = "ov7690",
	},
};

static int ov7690_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{


	int rc = i2c_add_driver(&ov7690_i2c_driver);
	if (rc < 0 || ov7690_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_done;
	}

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);
	msleep(10);

	rc = ov7690_sensor_init_probe(info);

	if (rc < 0){
		i2c_del_driver(&ov7690_i2c_driver);     
		goto probe_done;
	}

	s->s_init = ov7690_sensor_init;
	s->s_release = ov7690_sensor_release;
	s->s_config  = ov7690_sensor_config;

probe_done:
	//CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;
}

static int __ov7690_probe(struct platform_device *pdev)
{

	return msm_camera_drv_start(pdev, ov7690_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __ov7690_probe,
	.driver = {
		.name = "msm_camera_ov7690",
		.owner = THIS_MODULE,
	},
};

static int __init ov7690_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(ov7690_init);
