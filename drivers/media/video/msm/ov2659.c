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
#include "ov2659.h"

/* Micron OV2659 Registers and their values */
/* Sensor Core Registers */
#define  REG_OV2659_MODEL_ID 0x300A
#define  OV2659_MODEL_ID     0x26

struct ov2659_work {
	struct work_struct work;
};

static struct  ov2659_work *ov2659_sensorw;
static struct  i2c_client *ov2659_client;

struct ov2659_ctrl {
	const struct msm_camera_sensor_info *sensordata;
};


static struct ov2659_ctrl *ov2659_ctrl;


static DECLARE_WAIT_QUEUE_HEAD(ov2659_wait_queue);
DECLARE_MUTEX(ov2659_sem);
static int16_t ov2659_effect = CAMERA_EFFECT_OFF;

/*=============================================================
	EXTERNAL DECLARATIONS
==============================================================*/
extern struct ov2659_reg ov2659_regs;


/*=============================================================*/

static int ov2659_reset(const struct msm_camera_sensor_info *dev)
{
	int rc = 0;

	CDBG("ov2659_reset \n");

	rc = gpio_request(dev->sensor_reset, "ov2659");

	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 1);
		msleep(30);
		rc = gpio_direction_output(dev->sensor_reset, 0);
		msleep(30);
		rc = gpio_direction_output(dev->sensor_reset, 1);
		msleep(30);
	}

	gpio_free(dev->sensor_reset);
	return rc;
}

static int32_t ov2659_i2c_txdata(unsigned short saddr,
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

	if (i2c_transfer(ov2659_client->adapter, msg, 1) < 0) {
		CDBG("ov2659_i2c_txdata failed\n");
		return -EIO;
	}

	return 0;
}

static int32_t ov2659_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum ov2659_width width)
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

		rc = ov2659_i2c_txdata(saddr, buf, 4);
	}
		break;

	case BYTE_LEN: {
		buf[0] = (waddr & 0xFF00)>>8;
		buf[1] = (waddr & 0x00FF);
		buf[2] = (wdata & 0x00FF) ;
		rc = ov2659_i2c_txdata(saddr, buf, 3);
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		CDBG("i2c_write failed, addr = 0x%x, val = 0x%x!\n",waddr, wdata);

	return rc;
}

static int32_t ov2659_i2c_write_table(
	struct ov2659_i2c_reg_conf const *reg_conf_tbl,
	int num_of_items_in_table)
{
	int i;
	int32_t rc = -EIO;

	for (i = 0; i < num_of_items_in_table; i++) {
		rc = ov2659_i2c_write(ov2659_client->addr,
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

static int ov2659_i2c_rxdata(unsigned short saddr,
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

	if (i2c_transfer(ov2659_client->adapter, msgs, 2) < 0) {
		CDBG("ov2659_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t ov2659_i2c_read(unsigned short   saddr,
	unsigned short raddr, unsigned short *rdata, enum ov2659_width width)
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

		rc = ov2659_i2c_rxdata(saddr, buf, 2);
		if (rc < 0)
			return rc;

		*rdata = buf[0] << 8 | buf[1];
	}
		break;
	case BYTE_LEN: {
		buf[0] = (raddr & 0xFF00)>>8;
		buf[1] = (raddr & 0x00FF);

		rc = ov2659_i2c_rxdata(saddr, buf, 1);
		if (rc < 0)
			return rc;

		*rdata = buf[0];
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		CDBG("ov2659_i2c_read failed!\n");

	return rc;
}

static int32_t ov2659_set_lens_roll_off(void)
{
	int32_t rc = 0;
	return rc;
}

static long ov2659_reg_init(void)
{
	int32_t array_length;
	int32_t i;
	long rc;

	CDBG("ov2659_reg_init\n");

    // init

	rc = ov2659_i2c_write_table(&ov2659_regs.init_tbl[0],ov2659_regs.init_tbl_size);

	if (rc < 0)
		return rc;

	CDBG("ov2659_reg_init:success!\n");

	return 0;
}

static int32_t ov2659_set_brightness(int brightness)
	{
		long rc = 0;
		
		CDBG("ov2659_set_brightness:brightness=[%d]\n",brightness);
		switch (brightness) {
/*

#define CAMERA_MIN_BRIGHTNESS  0
#define CAMERA_DEF_BRIGHTNESS  3
#define CAMERA_MAX_BRIGHTNESS  6
#define CAMERA_BRIGHTNESS_STEP 1
	
*/
		default: 
			CDBG("ov2659_set_brightness:default\n");
		}
		return rc;
	}

static int32_t ov2659_set_scene(int scene)
{
	//uint16_t reg_addr;
	//uint16_t reg_val;
	long rc = 0;
	
	uint16_t af_status = 0;
	
	CDBG("ov2659_set_scene:scene=[%d]\n",scene);
	switch (scene) {		
	case CAMERA_BESTSHOT_OFF:
		CDBG("ov2659_set_scene:CAMERA_SCENE_AUTO\n");
		rc = ov2659_i2c_write_table(&ov2659_regs.scene_auto_tbl[0],
							ov2659_regs.scene_auto_tbl_size);
		
		//mdelay(200);		
		break;
			
	case CAMERA_BESTSHOT_NIGHT:
		CDBG("ov2659_set_scene:CAMERA_SCENE_NIGHT\n");
		rc = ov2659_i2c_write_table(&ov2659_regs.scene_night_tbl[0],
							ov2659_regs.scene_night_tbl_size);
		//mdelay(200);		
		break;

	default: 
		CDBG("ov2659_set_scene:default\n");
	}
	return rc;
}

static long ov2659_set_wb(int wb)
{
	long rc = 0;
		
	CDBG("ov2659_set_wb:wb=[%d] \n",wb);
	switch(wb){
	case CAMERA_WB_AUTO:
		CDBG("ov2659_set_wb:CAMERA_WB_AUTO\n");
		
    	rc = ov2659_i2c_write_table(&ov2659_regs.awb_tbl[0],
    					ov2659_regs.awb_tbl_size);
    					
    	if (rc < 0)
    		return rc;
		//mdelay(200);		
		break;
	case CAMERA_WB_INCANDESCENT :
		CDBG("ov2659_set_wb:CAMERA_WB_INCANDESCENT\n");
    	rc = ov2659_i2c_write_table(&ov2659_regs.mwb_incandescent_tbl[0],
    					ov2659_regs.mwb_incandescent_tbl_size);
    	if (rc < 0)
    		return rc;
		//mdelay(200);		
		break;
	case CAMERA_WB_FLUORESCENT: 
		CDBG("ov2659_set_wb:CAMERA_WB_FLUORESCENT\n");
    	rc = ov2659_i2c_write_table(&ov2659_regs.mwb_fluorescent_tbl[0],
    					ov2659_regs.mwb_fluorescent_tbl_size);
    	if (rc < 0)
    		return rc;
		//mdelay(200);		
		break;
	case CAMERA_WB_DAYLIGHT:
		CDBG("ov2659_set_wb:CAMERA_WB_DAYLIGHT\n");
    	rc = ov2659_i2c_write_table(&ov2659_regs.mwb_day_light_tbl[0],
    					ov2659_regs.mwb_day_light_tbl_size);
    	if (rc < 0)
    		return rc;
		//mdelay(200);		
		break;
	case CAMERA_WB_CLOUDY_DAYLIGHT:
		CDBG("ov2659_set_wb:CAMERA_WB_CLOUDY_DAYLIGHT\n");
    	rc = ov2659_i2c_write_table(&ov2659_regs.mwb_cloudy_tbl[0],
    					ov2659_regs.mwb_cloudy_tbl_size);
    	if (rc < 0)
    		return rc;
		//mdelay(200);		
		break;
	default: 
		CDBG("ov2659_set_wb:default\n");
		}
	return rc;
}

static long ov2659_set_effect(int effect)
	{
		//uint16_t reg_addr;
		//uint16_t reg_val;
		long rc = 0;
		uint16_t af_status = -1;
		
		CDBG("ov2659_set_effect:effect=[%d]\n",effect);
		switch (effect) {		
		case CAMERA_EFFECT_OFF:
			CDBG("ov2659_set_effect:CAMERA_EFFECT_OFF\n");
			rc = ov2659_i2c_write_table(&ov2659_regs.effect_off_tbl[0],
							ov2659_regs.effect_off_tbl_size);
			if (rc < 0)
				return rc;
			//mdelay(200);		
			break;
			
		case CAMERA_EFFECT_MONO:
			CDBG("ov2659_set_effect:CAMERA_EFFECT_MONO\n");
			rc = ov2659_i2c_write_table(&ov2659_regs.effect_mono_tbl[0],
							ov2659_regs.effect_mono_tbl_size);
			if (rc < 0)
				return rc;
			//mdelay(200);		
			break;
			
		case CAMERA_EFFECT_SEPIA:
			CDBG("ov2659_set_effect:CAMERA_EFFECT_SEPIA\n");
			rc = ov2659_i2c_write_table(&ov2659_regs.effect_sepia_tbl[0],
							ov2659_regs.effect_sepia_tbl_size);
			if (rc < 0)
				return rc;
			//mdelay(200);		
			break;
	
		case CAMERA_EFFECT_NEGATIVE: //wait OV ---green
			CDBG("ov2659_set_effect:CAMERA_EFFECT_NEGATIVE\n");					
			rc = ov2659_i2c_write_table(&ov2659_regs.effect_negative_tbl[0],
							ov2659_regs.effect_negative_tbl_size);
			if (rc < 0)
				return rc;
			//mdelay(200);		
			break;
		case CAMERA_EFFECT_SOLARIZE: //wait OV ---green
			CDBG("ov2659_set_effect:CAMERA_EFFECT_SOLARIZE\n");
 			rc = ov2659_i2c_write_table(&ov2659_regs.effect_solarize_tbl[0],
							ov2659_regs.effect_solarize_tbl_size);
			if (rc < 0)
				return rc;
			//mdelay(200);		
			break;
	

		case CAMERA_EFFECT_BLUISH:
			CDBG("ov2659_set_effect:CAMERA_EFFECT_BLUISH\n");
			rc = ov2659_i2c_write_table(&ov2659_regs.effect_bluish_tbl[0],
							ov2659_regs.effect_bluish_tbl_size);
			if (rc < 0)
				return rc;
			//mdelay(200);		
			break;
			
		case CAMERA_EFFECT_GREENISH:
			CDBG("ov2659_set_effect:CAMERA_EFFECT_GREENISH\n");
			rc = ov2659_i2c_write_table(&ov2659_regs.effect_greenish_tbl[0],
							ov2659_regs.effect_greenish_tbl_size);
			if (rc < 0)
				return rc;
			//mdelay(200);		
			break;

		case CAMERA_EFFECT_REDDISH:
			CDBG("ov2659_set_effect:CAMERA_EFFECT_REDDISH\n");
			rc = ov2659_i2c_write_table(&ov2659_regs.effect_reddish_tbl[0],
							ov2659_regs.effect_reddish_tbl_size);
			if (rc < 0)
				return rc;
			//mdelay(200);		
			break;

		default: 
			CDBG("ov2659_set_effect:default\n");
		}
		return rc;
	}

#define OV2659_R(reg, val)	ov2659_i2c_read(ov2659_client->addr,\
                            (unsigned short)reg, (unsigned short *)&val, BYTE_LEN)

#define OV2659_W(reg, val)	ov2659_i2c_write(ov2659_client->addr,\
                            (unsigned short)reg, (unsigned short)val, BYTE_LEN)

static void ov2659_exposure(void)
{
    unsigned int  m_gain_preview;
    unsigned int  m_gain_still;
    unsigned long  m_shutter_preview;
    unsigned long  m_shutter_still;
    unsigned long  m_dwFreq_preview=24;
    unsigned long  m_dwFreq_still=24;

    unsigned char   regv;
    unsigned long   linetp,shutter;
    unsigned int    gain16;

    unsigned int    framelines,framelines_banding;
    unsigned long   maxshutter;
    unsigned int    bandinglines;
    unsigned char    lightfreq;
    unsigned int    maxgain16;

    unsigned long  exposuretp;
    unsigned long  m_dbExposure; 

    //stop AEC/AGC
    OV2659_W(0x3503,0x07);    


    CDBG("OV2659Core_Get_ExposureValue()\r\n");    
    //Default Timing    
    OV2659_R(0x380c,regv);    
    linetp = regv&0x0F;
    linetp <<=8;
    OV2659_R(0x380d,regv);    
    linetp += regv;
    linetp ++;
    CDBG("linetp = %ld\r\n",linetp);    
    //Shutter
    OV2659_R(0x3500,regv);    
    shutter = regv&0x0F;
    shutter <<=8;
    OV2659_R(0x3501,regv);    
    shutter += regv;    
    shutter <<=8;
    OV2659_R(0x3502,regv);    
    shutter += regv;    
    CDBG("shutter = %ld\r\n",shutter);
    //Gain
    OV2659_R(0x350A,regv);
    gain16 = regv&0x01;
    gain16 <<=8;
    OV2659_R(0x350B,regv);        
    gain16+=regv;
    
    CDBG("gain16 = %d\r\n",gain16);    
    m_gain_preview = gain16;    
    m_shutter_preview=shutter;
    shutter >>=4;   
    m_dbExposure = shutter;
    m_dbExposure *=linetp;
    m_dbExposure *=gain16;
    m_dbExposure *=2;
    CDBG("m_dbExposure = %d\r\n",m_dbExposure);        

    // set capture
    ov2659_i2c_write_table(&ov2659_regs.snapshot_tbl[0],ov2659_regs.snapshot_tbl_size);


    CDBG("OV2659Core_Set_ExposureValue()\r\n");    
    if(m_dbExposure==0) return; //exposure value is invalid
   
    //get timing
    OV2659_R(0x380c,regv);    
    linetp = regv;
    linetp <<=8;
    OV2659_R(0x380d,regv);    
    linetp += regv;
    linetp ++;
    CDBG("linetp = %d\r\n",linetp);    
     
    OV2659_R(0x380e,regv);    
    framelines = regv;
    OV2659_R(0x350c,regv);    
    framelines += regv;
    framelines <<=8;
    OV2659_R(0x380f,regv);    
    framelines += regv;          
    OV2659_R(0x350d,regv);    
    framelines += regv;   
    CDBG("framelines = %d\r\n",framelines);  
    maxshutter = (unsigned int)(framelines-4);
    
    //get exposure
    CDBG("m_dbExposure = %d\r\n",m_dbExposure);      
    exposuretp = m_dbExposure;
    //exposuretp *= m_dwFreq_still;
    //exposuretp /= m_dwFreq_preview;
    exposuretp += exposuretp/3;
    
    CDBG("exposuretp = %d, before adjust\r\n",exposuretp);      
  
        
    CDBG("exposuretp = %d, after adjust, m_gain_preview=%x\r\n",exposuretp, m_gain_preview);        
    
    //get banding
    OV2659_R(0x3a08,regv);
    bandinglines=regv;
    bandinglines<<=8;
    OV2659_R(0x3a09,regv);        
    bandinglines+=regv;
    shutter = m_shutter_preview/bandinglines;
    if(m_shutter_preview == (shutter*bandinglines))
        lightfreq = 50;
    else
        lightfreq = 60;
        
    bandinglines = (unsigned int)((m_dwFreq_still*1000000*2+lightfreq*linetp)/(2*2*lightfreq*linetp));
    framelines_banding = (maxshutter/bandinglines)*bandinglines;
    CDBG("bandinglines = %d\r\n",bandinglines);    
    CDBG("lightfreq = %d\r\n",lightfreq);    
    
    maxgain16= 31*4;    
    if(exposuretp<(linetp*16))
    {
        CDBG("smaller than 1 line\r\n");
        exposuretp *=4;
        shutter = 1;
        gain16 = (unsigned int)((exposuretp*2+linetp)/linetp/2/4);
    }
    else if(exposuretp<(bandinglines*linetp*16))
    {
        CDBG("smaller than 1 banding, bandinglines=%d\r\n", bandinglines);
        shutter = exposuretp/(linetp*16);
        gain16 = 16;
    }
    else if(exposuretp<(framelines*linetp*16))
    {
        CDBG("smaller than 1 frame\r\n");
        shutter = (unsigned int)(exposuretp / (bandinglines*linetp*16));
        shutter *= bandinglines;
        gain16 = (unsigned int)((exposuretp*2+shutter*linetp)/(shutter*linetp)/2);
    }
    else
    {
        CDBG("larger than 1 frame\r\n");
    
        shutter = (unsigned int)((framelines/bandinglines)*bandinglines);
        gain16 = (unsigned int)((exposuretp*2+shutter*linetp)/(shutter*linetp)/2);
        if(gain16> maxgain16)
        {
            CDBG("larger than maxim gain & frame\r\n");
            shutter = (unsigned int)(exposuretp*11/(10*maxgain16*linetp));
            shutter /=bandinglines;
            shutter *=bandinglines;
            
            gain16  = (unsigned int)(exposuretp/(shutter*linetp));
        }
    }
    
    CDBG("shutter = %ld\r\n",shutter);    
    CDBG("gain16 = %d\r\n",gain16);    
    m_shutter_still    = shutter;
    m_gain_still = gain16;    
    CDBG("m_gain_still = %x\r\n",m_gain_still);    

    
    //set shutter
    if(maxshutter<=shutter)
    {
        maxshutter = shutter+4;
        regv =maxshutter&0xFF;
        OV2659_W(0x350d,0x00);
        OV2659_W(0x380f,regv);
        regv = maxshutter>>8;    
        OV2659_W(0x350c,0x00);    
        OV2659_W(0x380e,regv);    
    }
    
    regv =(shutter&0x0F)<<4;
    OV2659_W(0x3502,regv);
    regv =((shutter>>4)&0xFF);    
    OV2659_W(0x3501,regv);        
    regv =((shutter>>12)&0x0F);    
    OV2659_W(0x3500,regv);    
    
    //set gain
    if(m_gain_still>0x20)
    {
        gain16 = m_gain_still;
        gain16>>=1;
    }
    else if(m_gain_still<0x10)
    {
        gain16 = 0x18;
        gain16<<=1;
    }
    else
        gain16 = 0x08;


     
    regv = gain16&0xFF;
    OV2659_W(0x350B,regv);
    regv = (gain16>>8);
    OV2659_W(0x350A,regv);

}
static long ov2659_set_sensor_mode(int mode)
{
	//uint16_t clock;
	long rc = 0;

	CDBG("ov2659_set_sensor_mode:mode[%d]\n", mode);

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
    	rc = ov2659_i2c_write_table(&ov2659_regs.preview_tbl[0],ov2659_regs.preview_tbl_size);
    	if (rc < 0)
    		return rc;
		msleep(200);
		break;

	case SENSOR_SNAPSHOT_MODE:
        ov2659_exposure();
        
        /*
        rc = ov2659_i2c_write_table(&ov2659_regs.snapshot_tbl[0],ov2659_regs.snapshot_tbl_size);
    	if (rc < 0)
    		return rc;
        */
		
		msleep(400);
		break;

	case SENSOR_RAW_SNAPSHOT_MODE:
    	rc = ov2659_i2c_write_table(&ov2659_regs.snapshot_tbl[0],ov2659_regs.snapshot_tbl_size);
    	if (rc < 0)
    		return rc;
		msleep(200);
		break;

	default:
		return -EINVAL;
	}
    
	CDBG("ov2659_set_sensor_mode:mode[%d] OK!!! \n", mode);
	return 0;
}

static int ov2659_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	uint16_t model_id = 0;
	int rc = 0;

	//CDBG("init entry \n");
	CDBG("ov2659_sensor_init_probe \n");

	rc = ov2659_reset(data);
	if (rc < 0) 
		goto init_probe_fail;


	/* Read the Model ID of the sensor */
	rc = ov2659_i2c_read(ov2659_client->addr,REG_OV2659_MODEL_ID, &model_id, BYTE_LEN);
	if (rc < 0)
		goto init_probe_fail;

	CDBG("ov2659_sensor_init_probe:ov2659 model_id = 0x%x\n", model_id);

	/* Check if it matches it with the value in Datasheet */
	if (model_id != OV2659_MODEL_ID) {
		rc = -EINVAL;
		goto init_probe_fail;
	}

 
	rc = ov2659_reg_init();

	if (rc < 0)
		goto init_probe_fail;

	return rc;

init_probe_fail:
    
	CDBG("ov2659_sensor_init_probe:failed \n");
	return rc;
}

int ov2659_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	CDBG("ov2659_sensor_init\n");

	ov2659_ctrl = kzalloc(sizeof(struct ov2659_ctrl), GFP_KERNEL);
	if (!ov2659_ctrl) {
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		ov2659_ctrl->sensordata = data;

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);
	msleep(10);

	msm_camio_camif_pad_reg_reset();

	rc = ov2659_sensor_init_probe(data);

	if (rc < 0) {
		goto init_fail;
	}

init_done:
	return rc;

init_fail:
	kfree(ov2659_ctrl);
	return rc;
}

static int ov2659_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&ov2659_wait_queue);
	return 0;
}

int ov2659_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	if (copy_from_user(&cfg_data,
			(void *)argp,
			sizeof(struct sensor_cfg_data)))
		return -EFAULT;

		CDBG("ov2659_sensor_config : cfgtype [%d], mode[%d]\n",cfg_data.cfgtype, cfg_data.mode);

		switch (cfg_data.cfgtype) {
		case CFG_SET_MODE:
			rc = ov2659_set_sensor_mode(cfg_data.mode);
			break;

		case CFG_SET_EFFECT:
			rc = ov2659_set_effect(cfg_data.cfg.effect);
			break;

		case CFG_SET_WB:
			rc = ov2659_set_wb(cfg_data.cfg.wb);
			break;

		case CFG_SET_SCENE:
			rc = ov2659_set_scene(cfg_data.cfg.scene);
			break;

		case CFG_SET_BRIGHTNESS:
			rc = ov2659_set_brightness(cfg_data.cfg.brightness);
			break;


		case CFG_GET_AF_MAX_STEPS:
		default:
			rc = -EINVAL;
			break;
		}

	return rc;
}

int ov2659_sensor_release(void)
{
	int rc = 0;

	CDBG("ov2659_sensor_release!\n");


	kfree(ov2659_ctrl);

	return rc;
}

static int ov2659_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	ov2659_sensorw =
		kzalloc(sizeof(struct ov2659_work), GFP_KERNEL);

	if (!ov2659_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, ov2659_sensorw);
	ov2659_init_client(client);
	ov2659_client = client;

	CDBG("ov2659_i2c_probe succeeded!\n");

	return 0;

probe_failure:
	kfree(ov2659_sensorw);
	ov2659_sensorw = NULL;
	CDBG("ov2659_i2c_probe failed!\n");
	return rc;
}

static const struct i2c_device_id ov2659_i2c_id[] = {
	{ "ov2659", 0},
	{ },
};

static struct i2c_driver ov2659_i2c_driver = {
	.id_table = ov2659_i2c_id,
	.probe  = ov2659_i2c_probe,
	.remove = __exit_p(ov2659_i2c_remove),
	.driver = {
		.name = "ov2659",
	},
};

static int ov2659_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{


	int rc = i2c_add_driver(&ov2659_i2c_driver);
	if (rc < 0 || ov2659_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_done;
	}

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);
	msleep(10);

	rc = ov2659_sensor_init_probe(info);
    
	if (rc < 0){
		i2c_del_driver(&ov2659_i2c_driver);     
		goto probe_done;
	}
	s->s_init = ov2659_sensor_init;
	s->s_release = ov2659_sensor_release;
	s->s_config  = ov2659_sensor_config;

probe_done:
	//CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;
}

static int __ov2659_probe(struct platform_device *pdev)
{
	CDBG("__ov2659_probe...\n");

	return msm_camera_drv_start(pdev, ov2659_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __ov2659_probe,
	.driver = {
		.name = "msm_camera_ov2659",
		.owner = THIS_MODULE,
	},
};

static int __init ov2659_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(ov2659_init);
