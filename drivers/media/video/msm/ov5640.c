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
#include "ov5640.h"


/* Micron OV5640 Registers and their values */
/* Sensor Core Registers */
#define  REG_OV5640_MODEL_ID 0x300A

#ifdef OV5642
#define  OV5640_MODEL_ID     0x56
#else
#define  OV5640_MODEL_ID     0x36
#endif
/*  SOC Registers Page 1  */
#define  REG_OV5640_SENSOR_RESET     0x301A
#define  REG_OV5640_STANDBY_CONTROL  0x3202
#define  REG_OV5640_MCU_BOOT         0x3386

struct ov5640_work {
	struct work_struct work;
};

static struct  ov5640_work *ov5640_sensorw;
static struct  i2c_client *ov5640_client;

struct ov5640_ctrl {
	const struct msm_camera_sensor_info *sensordata;
};


static struct ov5640_ctrl *ov5640_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(ov5640_wait_queue);
DECLARE_MUTEX(ov5640_sem);
static int16_t ov5640_effect = CAMERA_EFFECT_OFF;

/*=============================================================
	EXTERNAL DECLARATIONS
==============================================================*/
extern struct ov5640_reg ov5640_regs;


/*=============================================================*/

static int ov5640_reset(const struct msm_camera_sensor_info *dev)
{
	int rc = 0;

	CDBG("ov5640_reset \n");

	rc = gpio_request(dev->sensor_reset, "ov5640");

	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 1);
		msleep(30);
		rc = gpio_direction_output(dev->sensor_reset, 0);
		msleep(100);
		rc = gpio_direction_output(dev->sensor_reset, 1);
		msleep(100);
	}

	gpio_free(dev->sensor_reset);
    
	return rc;
}

static int32_t ov5640_i2c_txdata(unsigned short saddr,
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

	if (i2c_transfer(ov5640_client->adapter, msg, 1) < 0) {
		CDBG("ov5640_i2c_txdata failed\n");
		return -EIO;
	}

	return 0;
}

static int32_t ov5640_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum ov5640_width width)
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

		rc = ov5640_i2c_txdata(saddr, buf, 4);
	}
		break;

	case BYTE_LEN: {
		buf[0] = (waddr & 0xFF00)>>8;
		buf[1] = (waddr & 0x00FF);
		buf[2] = (wdata & 0x00FF) ;
		rc = ov5640_i2c_txdata(saddr, buf, 3);
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		CDBG("i2c_write failed, addr = 0x%x, val = 0x%x!\n",waddr, wdata);

	return rc;
}

static int32_t ov5640_i2c_write_table(
	struct register_address_value_pair const *reg_conf_tbl,
	uint16_t size)
{
	uint16_t i;
	int32_t rc = -EIO;

	for (i = 0; i < size; i++) {
		rc = ov5640_i2c_write(ov5640_client->addr,
		    reg_conf_tbl[i].register_address,
		    reg_conf_tbl[i].register_value,
		    BYTE_LEN);
		if (rc < 0)
			return rc;
	}

	return rc;
}

static int ov5640_i2c_rxdata(unsigned short saddr,
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

	if (i2c_transfer(ov5640_client->adapter, msgs, 2) < 0) {
		CDBG("ov5640_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t ov5640_i2c_read(unsigned short   saddr,
	unsigned short raddr, unsigned short *rdata, enum ov5640_width width)
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

		rc = ov5640_i2c_rxdata(saddr, buf, 2);
		if (rc < 0)
			return rc;

		*rdata = buf[0] << 8 | buf[1];
	}
		break;
	case BYTE_LEN: {
		buf[0] = (raddr & 0xFF00)>>8;
		buf[1] = (raddr & 0x00FF);

		rc = ov5640_i2c_rxdata(saddr, buf, 1);
		if (rc < 0)
			return rc;

		*rdata = buf[0];
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		CDBG("ov5640_i2c_read failed!\n");

	return rc;
}

static int32_t ov5640_set_lens_roll_off(void)
{
	int32_t rc = 0;
	//rc = ov5640_i2c_write_table(&ov5640_regs.rftbl[0],
	//							 ov5640_regs.rftbl_size);
	return rc;
}

static long ov5640_reg_init(void)
{

	long rc;

	CDBG("ov5640_reg_init\n");

#ifdef OV5642
    // soft reset
	CDBG("ov5640_reg_init:P15:OV5642\n");
	ov5640_i2c_write(ov5640_client->addr,0x3103,0x93,BYTE_LEN);
	ov5640_i2c_write(ov5640_client->addr,0x3008,0x82,BYTE_LEN);
#else
    // soft reset
	CDBG("ov5640_reg_init:PE28:OV3640\n");
	rc = ov5640_i2c_write(ov5640_client->addr,0x3012,0x80,BYTE_LEN);
    if (rc < 0)
		return rc;
#endif

	msleep(30);
    
    // init data
    rc = ov5640_i2c_write_table(ov5640_regs.init_settings,
                            ov5640_regs.init_settings_size);
	if (rc < 0)
		return rc;


#ifdef OV5642
    // AF
	msleep(100);
    rc = ov5640_i2c_write_table(ov5640_regs.af_settings,
                            ov5640_regs.af_settings_size);
	if (rc < 0)
		return rc;

	msleep(500);

#endif

	CDBG("ov5640_reg_init:success!\n");


	return 0;
}
static int32_t ov5640_set_brightness(int brightness)
	{
		long rc = 0;
		
		CDBG("ov5640_set_brightness:brightness=[%d]\n",brightness);
		switch (brightness) {

		default: 
			CDBG("ov5640_set_brightness:default\n");
		}
		return rc;
	}

static int32_t ov5640_set_scene(int scene)
{

	long rc = 0;
	
	CDBG("ov5640_set_scene:scene=[%d]\n",scene);
	switch (scene) {		
	case CAMERA_BESTSHOT_OFF:
		CDBG("ov5640_set_scene:CAMERA_SCENE_AUTO\n");
    	ov5640_i2c_write(ov5640_client->addr,0x3011,0x01,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3015,0x12,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3071,0xeb,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3358,0x44,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3359,0x44,BYTE_LEN);
		//mdelay(200);		
		break;
			
	case CAMERA_BESTSHOT_NIGHT:
		CDBG("ov5640_set_scene:CAMERA_SCENE_NIGHT\n");
    	ov5640_i2c_write(ov5640_client->addr,0x3011,0x02,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3015,0x13,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3071,0x9d,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3358,0x34,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3359,0x34,BYTE_LEN);
		//mdelay(200);		
		break;

	default: 
		CDBG("ov5640_set_scene:default\n");
	}
	return rc;
}

static long ov5640_set_wb(int wb)
{
	long rc = 0;
		
	CDBG("ov5640_set_wb:wb=[%d] \n",wb);
	switch(wb){
	case CAMERA_WB_AUTO:
		CDBG("ov5640_set_wb:CAMERA_WB_AUTO\n");
    	ov5640_i2c_write(ov5640_client->addr,0x3317, 0x04,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3316, 0xf8,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3312, 0x2c,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3314, 0x42,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3313, 0x24,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3315, 0x40,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3311, 0xd0,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3310, 0xbd,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x330c, 0x18,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x330d, 0x18,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x330e, 0x68,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x330f, 0x60,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x330b, 0x28,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3306, 0x5c,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3307, 0x11,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3308, 0x25,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x332b, 0x00,BYTE_LEN);
        
		//mdelay(200);		
		break;
	case CAMERA_WB_INCANDESCENT :
		CDBG("ov5640_set_wb:CAMERA_WB_INCANDESCENT\n");
    	ov5640_i2c_write(ov5640_client->addr,0x332b, 0x08,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x33a7, 0x44,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x33a8, 0x40,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x33a9, 0x70,BYTE_LEN);
		//mdelay(200);		
		break;
	case CAMERA_WB_FLUORESCENT: 
    	ov5640_i2c_write(ov5640_client->addr,0x332b, 0x08,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x33a7, 0x52,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x33a8, 0x40,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x33a9, 0x58,BYTE_LEN);
		CDBG("ov5640_set_wb:CAMERA_WB_FLUORESCENT\n");
		//mdelay(200);		
		break;
	case CAMERA_WB_DAYLIGHT:
    	ov5640_i2c_write(ov5640_client->addr,0x332b, 0x08,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x33a7, 0x5e,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x33a8, 0x40,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x33a9, 0x46,BYTE_LEN);

		CDBG("ov5640_set_wb:CAMERA_WB_DAYLIGHT\n");
		//mdelay(200);		
		break;
	case CAMERA_WB_CLOUDY_DAYLIGHT:
		CDBG("ov5640_set_wb:CAMERA_WB_CLOUDY_DAYLIGHT\n");
    	ov5640_i2c_write(ov5640_client->addr,0x332b, 0x08,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x33a7, 0x68,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x33a8, 0x40,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x33a9, 0x4e,BYTE_LEN);
		//mdelay(200);		
		break;
	default: 
		CDBG("ov5640_set_wb:default\n");
		}
	return rc;
}

static long ov5640_set_effect(int effect)
{

		long rc = 0;

		CDBG("ov5640_set_effect:effect=[%d]\n",effect);
		switch (effect) {		
		case CAMERA_EFFECT_OFF:
			CDBG("ov5640_set_effect:CAMERA_EFFECT_OFF\n");
        	ov5640_i2c_write(ov5640_client->addr,0x3302, 0xef,BYTE_LEN);
        	ov5640_i2c_write(ov5640_client->addr,0x3355, 0x02,BYTE_LEN);
			//mdelay(200);		
			break;
			
		case CAMERA_EFFECT_MONO:
			CDBG("ov5640_set_effect:CAMERA_EFFECT_MONO\n");
        	ov5640_i2c_write(ov5640_client->addr,0x3302, 0xef,BYTE_LEN);
        	ov5640_i2c_write(ov5640_client->addr,0x3355, 0x18,BYTE_LEN);
        	ov5640_i2c_write(ov5640_client->addr,0x335a, 0x80,BYTE_LEN);
        	ov5640_i2c_write(ov5640_client->addr,0x335b, 0x80,BYTE_LEN);
            //mdelay(200);		
			break;
			
		case CAMERA_EFFECT_SEPIA:
			CDBG("ov5640_set_effect:CAMERA_EFFECT_SEPIA\n");
        	ov5640_i2c_write(ov5640_client->addr,0x3302, 0xef,BYTE_LEN);
        	ov5640_i2c_write(ov5640_client->addr,0x3355, 0x18,BYTE_LEN);
        	ov5640_i2c_write(ov5640_client->addr,0x335a, 0x40,BYTE_LEN);
        	ov5640_i2c_write(ov5640_client->addr,0x335b, 0xa6,BYTE_LEN);
			//mdelay(200);		
			break;
	
		case CAMERA_EFFECT_NEGATIVE: //wait OV ---green
			CDBG("ov5640_set_effect:CAMERA_EFFECT_NEGATIVE\n");					
       	    ov5640_i2c_write(ov5640_client->addr,0x3302, 0xef,BYTE_LEN);
        	ov5640_i2c_write(ov5640_client->addr,0x3355, 0x40,BYTE_LEN);
			//mdelay(200);		
			break;
		case CAMERA_EFFECT_SOLARIZE: //wait OV ---green
			CDBG("ov5640_set_effect:CAMERA_EFFECT_SOLARIZE\n");
			//mdelay(200);		
			break;
	

		case CAMERA_EFFECT_BLUISH:
			CDBG("ov5640_set_effect:CAMERA_EFFECT_BLUISH\n");
			//mdelay(200);		
			break;
			
		case CAMERA_EFFECT_GREENISH:
			CDBG("ov5640_set_effect:CAMERA_EFFECT_GREENISH\n");
			//mdelay(200);		
			break;

		case CAMERA_EFFECT_REDDISH:
			CDBG("ov5640_set_effect:CAMERA_EFFECT_REDDISH\n");
			//mdelay(200);		
			break;

		default: 
			CDBG("ov5640_set_effect:default\n");
		}
		return rc;
}

static int32_t ov5640_set_default_focus(void)
{
	long rc = 0;
    unsigned char reg;
	uint16_t icount,itotal;
	CDBG("ov5640_set_default_focus\n");

    
	ov5640_i2c_write(ov5640_client->addr,0x3025, 0xff,BYTE_LEN);
	ov5640_i2c_write(ov5640_client->addr,0x3024, 0x03,BYTE_LEN);
    msleep(100);

    itotal=30;
	for(icount=0; icount<itotal; icount++)
	{
    	rc = ov5640_i2c_read(ov5640_client->addr,0x3027, &reg, BYTE_LEN);
        reg = reg & (0x03);

		CDBG("ov5640_set_default_focus:mt9d112 af_status = 0x%x  icount=%d\n", reg,icount);		
		if(reg==0x02)
		{	
			return rc;
		}
		msleep(100);
	}
	
	return 0;
}

#ifdef OV5642
static void camsensor_ov5642AF_capture_gain(void)
{

#define Capture_Framerate 300
#define Preview_FrameRate 1500

    int Lines_10ms;
    int Capture_MaxLines;
    int Preview_Maxlines;
    long ulCapture_Exposure;
    long ulCapture_Exposure_Gain;
    long ulPreviewExposure;
    long iCapture_Gain;
    unsigned char Gain ;
    unsigned char ExposureLow;
    unsigned char ExposureMid ;
    unsigned char ExposureHigh ;
    unsigned char PreviewMaxlineHigh ;
    unsigned char PreviewMaxlineLow ;
    unsigned char CaptureMaxlineHigh ;
    unsigned char CaptureMaxlineLow ;

    ov5640_i2c_write(ov5640_client->addr,0x3503, 0x07,BYTE_LEN);

    ov5640_i2c_read(ov5640_client->addr,0x350b, &Gain, BYTE_LEN);
    ov5640_i2c_read(ov5640_client->addr,0x3502, &ExposureLow, BYTE_LEN);
    ov5640_i2c_read(ov5640_client->addr,0x3501, &ExposureMid, BYTE_LEN);
    ov5640_i2c_read(ov5640_client->addr,0x3500, &ExposureHigh, BYTE_LEN);

    ov5640_i2c_read(ov5640_client->addr,0x350c, &PreviewMaxlineHigh, BYTE_LEN);
    ov5640_i2c_read(ov5640_client->addr,0x350d, &PreviewMaxlineLow, BYTE_LEN);

    ov5640_i2c_read(ov5640_client->addr,0x350c, &CaptureMaxlineHigh, BYTE_LEN);
    ov5640_i2c_read(ov5640_client->addr,0x350d, &CaptureMaxlineLow, BYTE_LEN);

    Preview_Maxlines = 256 * PreviewMaxlineHigh + PreviewMaxlineLow;
    Capture_MaxLines = 256 * CaptureMaxlineHigh + CaptureMaxlineLow;
    Lines_10ms = Capture_Framerate * Capture_MaxLines / 10000 * 13 / 12;
    ulPreviewExposure = ((unsigned long)(ExposureHigh)) << 12 ;
    ulPreviewExposure += ((unsigned long)ExposureMid) << 4 ;
    ulPreviewExposure += (ExposureLow >> 4);
    if (0 == Preview_Maxlines || 0 == Preview_FrameRate || 0 == Lines_10ms)
    {
        return ;
    }
    ulCapture_Exposure =
        ((ulPreviewExposure * (Capture_Framerate) * (Capture_MaxLines)) / (((Preview_Maxlines) * (Preview_FrameRate)))) * 5;
    iCapture_Gain = (Gain & 0x0f) + 16;
    if (Gain & 0x10)
    {
        iCapture_Gain = iCapture_Gain << 1;
    }
    if (Gain & 0x20)
    {
        iCapture_Gain = iCapture_Gain << 1;
    }
    if (Gain & 0x40)
    {
        iCapture_Gain = iCapture_Gain << 1;
    }
    if (Gain & 0x80)
    {
        iCapture_Gain = iCapture_Gain << 1;
    }
    ulCapture_Exposure_Gain = ulCapture_Exposure * iCapture_Gain;
    if (ulCapture_Exposure_Gain < ((long)(Capture_MaxLines)*16))
    {
        ulCapture_Exposure = ulCapture_Exposure_Gain / 16;
        if (ulCapture_Exposure > Lines_10ms)
        {
            ulCapture_Exposure /= Lines_10ms;
            ulCapture_Exposure *= Lines_10ms;
        }
    }
    else
    {
        ulCapture_Exposure = Capture_MaxLines;
    }
    if (ulCapture_Exposure == 0)
    {
        ulCapture_Exposure = 1;
    }
    iCapture_Gain = (ulCapture_Exposure_Gain * 2 / ulCapture_Exposure + 1) / 2;
    ExposureLow = ((unsigned char)ulCapture_Exposure) << 4;
    ExposureMid = (unsigned char)(ulCapture_Exposure >> 4) & 0xff;
    ExposureHigh = (unsigned char)(ulCapture_Exposure >> 12);
    Gain = 0;
    if (iCapture_Gain > 31)
    {
        Gain |= 0x10;
        iCapture_Gain = iCapture_Gain >> 1;
    }
    if (iCapture_Gain > 31)
    {
        Gain |= 0x20;
        iCapture_Gain = iCapture_Gain >> 1;
    }
    if (iCapture_Gain > 31)
    {
        Gain |= 0x40;
        iCapture_Gain = iCapture_Gain >> 1;
    }
    if (iCapture_Gain > 31)
    {
        Gain |= 0x80;
        iCapture_Gain = iCapture_Gain >> 1;
    }
    if (iCapture_Gain > 16)
    {
        Gain |= ((iCapture_Gain - 16) & 0x0f);
    }
    if (Gain == 0x10)
    {
        Gain = 0x11;
    }
    ov5640_i2c_write(ov5640_client->addr,0x350b, Gain,BYTE_LEN);
    ov5640_i2c_write(ov5640_client->addr,0x3502, ExposureLow,BYTE_LEN);
    ov5640_i2c_write(ov5640_client->addr,0x3501, ExposureMid,BYTE_LEN);
    ov5640_i2c_write(ov5640_client->addr,0x3500, ExposureHigh,BYTE_LEN);

}
#endif

static unsigned long sub_gain_reg2real ( unsigned char gainreg ) //n*16
{
	unsigned long real;
		
	real = gainreg & 0x000f;
	real+=16;
	
	if( gainreg & 0x10 )
	{
		real = real <<1;
	}
	
	if( gainreg & 0x20 )
	{
		real = real <<1;
	}
	
	if( gainreg & 0x40 )
	{
		real = real <<1;
	}
	
	if( gainreg & 0x80 )
	{
		real = real <<1;
	}
		
	return real;
}

static unsigned long sub_gain_real2reg(unsigned long gainreal) //n*16
{
	unsigned long gain;
	
	gain = 0;
	
	if( gainreal >31 )
	{
		gain |= 0x10;
		gainreal = gainreal >>1;
	}
	
	if( gainreal >31 )
	{
		gain |= 0x20;
		gainreal = gainreal >>1;
	}
	
	if( gainreal >31 )
	{
		gain |= 0x40;
		gainreal = gainreal >>1;
	}
	
	if( gainreal >31 )
	{
		gain |= 0x80;
		gainreal = gainreal >>1;
	}
	
	if( gainreal >16 )
	{
		gain |= (( gainreal -16 ) & 0x0f );
	}
	
	return gain;
}



/*** Pixel Clock Gene ***/
static void getpclkgene(unsigned int* numerator, unsigned int* denominator)
{
	unsigned char ucRegValue;
	unsigned int  uiNumerator;
	unsigned int  uiDenominator; 
	
	ov5640_i2c_read(ov5640_client->addr,0x300E, &ucRegValue, BYTE_LEN);       //0x300E,PLL contol 1
    
	uiNumerator = 0x40 - (ucRegValue & 0x3F);   //PLLDiv = 64-Rx_PLL[5:0]
	
	ov5640_i2c_read(ov5640_client->addr,0x300F, &ucRegValue, BYTE_LEN);       //0x300F,PLL contol 2
	switch((ucRegValue >>6) & 0x03) //Bit[7:6] FreDiv control bit
	{
		case 0x01:                  //FreDiv = 1.5
			uiNumerator *=3;
			uiDenominator = 2;
			break;
		case 0x02:                  //FreDiv = 2
			uiNumerator *= 2;
			uiDenominator = 1;
			break;
		case 0x03:                  //FreDiv = 3
			uiNumerator *= 3;
			uiDenominator = 1;
			break;
		default:                    //FreDiv = 1
			uiDenominator = 1;
			break;
	}
	switch((ucRegValue >>4) & 0x03) //Bit[5:4] Bit8Div control bit
	{
		case 0x02:                  //Bit8Div = 4
			uiNumerator *= 4;
			break;
		case 0x03:                  //Bit8Div = 5
			uiNumerator *= 5;
			break;
		default:                    //Bit8Div = 1
			break;
	}
	switch((ucRegValue >>0) & 0x03) //Bit[1:0] InDiv control bit ??
	{
		case 0x01:                  //InDiv = 1.5
			uiNumerator *=2;
			uiDenominator *= 3;
			break;
		case 0x02:                  //InDiv = 2
			uiDenominator *= 2;
			break;
		case 0x03:                  //InDiv = 3
			uiDenominator *= 3;
			break;
		default:                    //InDiv = 1
			break;
	}

	ov5640_i2c_read(ov5640_client->addr,0x3010, &ucRegValue, BYTE_LEN);   //0x3010,PLL contol 3
	if(ucRegValue & 0x10)
	{
		uiDenominator *= 2;	                //SensorDiv = 2
	}

	ov5640_i2c_read(ov5640_client->addr,0x3011, &ucRegValue, BYTE_LEN);   //0x3011,Clock rate contol 
	uiDenominator *=(1+ucRegValue & 0x3F);  //CLK = XVCLK/(CLK[5:0]+1)
	if(ucRegValue & 0x80)
	{
		uiDenominator *=2;          //Digital frequency doubler ON ??
	}
	else
	{
		uiDenominator *=4;	        //Digital frequency doubler OFF ??
	}


	*numerator = uiNumerator;
	*denominator = uiDenominator;
    //RETAILMSG(1,(L"[CAM]:[Ov3640]:getpclkgene():  uiNumerator<%d> / uiDenominator<%d> \r\n",uiNumerator,uiDenominator));
} 

#define MAX_EXPOSURE 1568

static int regs_setexposure_qxga(unsigned long preexpgain)
{
	unsigned char  ucRegValue;
	unsigned int   uiExposure;
	unsigned int   uiDigitalGain;
	unsigned long  expgain = preexpgain;	
	unsigned long temp;
	unsigned int   uiNumerator,uiDenominator;
	unsigned long wlines10ms;
	
	getpclkgene(&uiNumerator, &uiDenominator);
	
	expgain *= 2;
	expgain *= uiNumerator;
	expgain /= uiDenominator;
	expgain += (2376/2);
	expgain /=2376;
		
	// lines per 10ms
	temp = 13000000 * uiNumerator;
	temp /= (uiDenominator*2376*100);
	wlines10ms  =  temp;
	
	if( expgain < (MAX_EXPOSURE*16) ) //??
	{
		uiExposure = expgain/16;
		if(uiExposure > wlines10ms)
		{
			uiExposure /= wlines10ms;
			uiExposure *= wlines10ms;
		}
	}
	else
	{
		uiExposure = MAX_EXPOSURE;
	}

	
	uiDigitalGain = ( expgain * 2/uiExposure +1 ) / 2;
    //RETAILMSG(1,(L"[CAM]:[Ov3640]:regs_setexposure_qxga():  expgain<%d>  ; REAL Gain<%d>\r\n",expgain,uiDigitalGain));

	uiDigitalGain = sub_gain_real2reg ( uiDigitalGain );
    //RETAILMSG(1,(L"[CAM]:[Ov3640]:regs_setexposure_qxga():  exposure<%d>; REG Gain<0x%08x>\r\n",uiExposure,uiDigitalGain));
		
	/*** Set digital gain value ***/
	ucRegValue = (unsigned char)((uiDigitalGain>>8) & 0xFF);
	ov5640_i2c_write(ov5640_client->addr,0x3000, ucRegValue,BYTE_LEN);
	
	ucRegValue = (unsigned char)(uiDigitalGain & 0xFF);
	ov5640_i2c_write(ov5640_client->addr,0x3001, ucRegValue,BYTE_LEN);
	
	/*** Set Exposure value ***/
	ucRegValue = (unsigned char)((uiExposure>>8) & 0xFF);
	ov5640_i2c_write(ov5640_client->addr,0x3002, ucRegValue,BYTE_LEN);
	
	ucRegValue = (unsigned char)(uiExposure & 0xFF);
	ov5640_i2c_write(ov5640_client->addr,0x3003, ucRegValue,BYTE_LEN);	
		
 	return 0;
}

/*** Get exposure gain ***/
static unsigned long regs_getexposure_xga( void )
{
	unsigned char reg;
	unsigned int  exposure;
	unsigned int  digitalgain;
	unsigned int  uiNumerator,uiDenominator;
	unsigned long expgain;
			
	/*** exposure ***/
	ov5640_i2c_read(ov5640_client->addr,0x3002,&reg, BYTE_LEN);		
	exposure = reg;
	exposure <<= 8;
	ov5640_i2c_read(ov5640_client->addr,0x3003,&reg, BYTE_LEN);	
	exposure += reg;
	
	
	/*** gain ***/
	ov5640_i2c_read(ov5640_client->addr,0x3001,&reg, BYTE_LEN);	
	digitalgain = sub_gain_reg2real(reg);
    //RETAILMSG(1,(L"[CAM]:[Ov3640]:regs_getexposure_xga():  exposure<%d>  digitalgain<%d>  reg<0x%08x> ; \r\n",exposure,digitalgain,reg));
	
	/*** Get PCLK ***/
	getpclkgene(&uiNumerator, &uiDenominator);
	
	expgain= exposure*digitalgain*uiDenominator*2376;
	expgain += (uiNumerator/2);
	expgain /= uiNumerator;

	return expgain;
}

static long ov5640_set_sensor_mode(int mode)
{
	//uint16_t clock;
	long rc = 0;
  	unsigned long expgain = 0;

	CDBG("ov5640_set_sensor_mode:mode[%d]\n", mode);


	switch (mode) {
	case SENSOR_PREVIEW_MODE:
#ifdef OV5642
        // soft reset
    	ov5640_i2c_write(ov5640_client->addr,0x3103,0x93,BYTE_LEN);
    	ov5640_i2c_write(ov5640_client->addr,0x3008,0x82,BYTE_LEN);
    	msleep(30);        
        // init data
        rc = ov5640_i2c_write_table(ov5640_regs.init_settings,
                                ov5640_regs.init_settings_size);
        //set_AF_furthest
		msleep(100);
    	ov5640_i2c_write(ov5640_client->addr,0x3024, 0x08,BYTE_LEN);

        
#else
        rc = ov5640_i2c_write_table(ov5640_regs.qxga_xga_settings,
                                ov5640_regs.qxga_xga_settings_size);
#endif
    	if (rc < 0)
    		return rc;
     
		msleep(100);
		break;

	case SENSOR_SNAPSHOT_MODE:
#ifdef OV5642
        rc = ov5640_i2c_write_table(ov5640_regs.xga_qxga_settings,
                                ov5640_regs.xga_qxga_settings_size);
    	if (rc < 0)
    		return rc;

        camsensor_ov5642AF_capture_gain();        

#else
   		expgain = regs_getexposure_xga();

        rc = ov5640_i2c_write_table(ov5640_regs.xga_qxga_settings,
                                ov5640_regs.xga_qxga_settings_size);
    	if (rc < 0)
    		return rc;

        expgain += expgain/10;

   		regs_setexposure_qxga(expgain);


#endif

		msleep(200);


		break;

	case SENSOR_RAW_SNAPSHOT_MODE:
        rc = ov5640_i2c_write_table(ov5640_regs.xga_qxga_settings,
                                ov5640_regs.xga_qxga_settings_size);
    	if (rc < 0)
    		return rc;

		msleep(200);
		break;

	default:
		return -EINVAL;
	}
    


	return 0;
}

static int ov5640_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	uint16_t model_id = 0;
	int rc = 0;

	//CDBG("init entry \n");
	CDBG("ov5640_sensor_init_probe \n");

	rc = ov5640_reset(data);
	if (rc < 0) 
		goto init_probe_fail;

	/* Read the Model ID of the sensor */
	rc = ov5640_i2c_read(ov5640_client->addr,
		REG_OV5640_MODEL_ID, &model_id, BYTE_LEN);
	if (rc < 0)
		goto init_probe_fail;

	CDBG("ov5640_sensor_init_probe:ov5640 model_id = 0x%x\n", model_id);

	/* Check if it matches it with the value in Datasheet */
	if (model_id != OV5640_MODEL_ID) {
		rc = -EINVAL;
		goto init_probe_fail;
	}


	rc = ov5640_reg_init();

	if (rc < 0)
		goto init_probe_fail;

	return rc;

init_probe_fail:
    
	CDBG("ov5640_sensor_init_probe:failed\n");
	return rc;
}

int ov5640_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	CDBG("ov5640_sensor_init\n");

	ov5640_ctrl = kzalloc(sizeof(struct ov5640_ctrl), GFP_KERNEL);
	if (!ov5640_ctrl) {
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		ov5640_ctrl->sensordata = data;

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);
	msleep(5);

	msm_camio_camif_pad_reg_reset();

	rc = ov5640_sensor_init_probe(data);

	if (rc < 0) {
		goto init_fail;
	}

init_done:
	return rc;

init_fail:
	kfree(ov5640_ctrl);
	return rc;
}

static int ov5640_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&ov5640_wait_queue);
	return 0;
}

int ov5640_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	if (copy_from_user(&cfg_data,
			(void *)argp,
			sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	CDBG("ov5640_sensor_config : cfgtype [%d], mode[%d]\n",cfg_data.cfgtype, cfg_data.mode);

		switch (cfg_data.cfgtype) {
		case CFG_SET_MODE:
			rc = ov5640_set_sensor_mode(
						cfg_data.mode);
			break;

		case CFG_SET_EFFECT:
			rc = ov5640_set_effect(cfg_data.cfg.effect);
			break;

		case CFG_SET_WB:
			rc = ov5640_set_wb(cfg_data.cfg.wb);
			break;

		case CFG_SET_SCENE:
			rc = ov5640_set_scene(cfg_data.cfg.scene);
			break;

		case CFG_SET_BRIGHTNESS:
			rc = ov5640_set_brightness(cfg_data.cfg.brightness);
			break;

#ifdef OV5642
		case CFG_SET_DEFAULT_FOCUS:
			rc = ov5640_set_default_focus();
			break;
#endif
		case CFG_GET_AF_MAX_STEPS:
		default:
			rc = -EINVAL;
			break;
		}

	return rc;
}

int ov5640_sensor_release(void)
{
	int rc = 0;

	CDBG("ov5640_sensor_release!\n");

    kfree(ov5640_ctrl);

	return rc;
}

static int ov5640_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	ov5640_sensorw =
		kzalloc(sizeof(struct ov5640_work), GFP_KERNEL);

	if (!ov5640_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, ov5640_sensorw);
	ov5640_init_client(client);
	ov5640_client = client;

	CDBG("ov5640_i2c_probe succeeded!\n");

	return 0;

probe_failure:
	kfree(ov5640_sensorw);
	ov5640_sensorw = NULL;
	CDBG("ov5640_i2c_probe failed!\n");
	return rc;
}

static const struct i2c_device_id ov5640_i2c_id[] = {
	{ "ov5640", 0},
	{ },
};

static struct i2c_driver ov5640_i2c_driver = {
	.id_table = ov5640_i2c_id,
	.probe  = ov5640_i2c_probe,
	.remove = __exit_p(ov5640_i2c_remove),
	.driver = {
		.name = "ov5640",
	},
};

static int ov5640_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{


	int rc = i2c_add_driver(&ov5640_i2c_driver);
	if (rc < 0 || ov5640_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_done;
	}

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);
	msleep(5);

	rc = ov5640_sensor_init_probe(info);

	if (rc < 0){
		i2c_del_driver(&ov5640_i2c_driver);     
		goto probe_done;
	}

	s->s_init = ov5640_sensor_init;
	s->s_release = ov5640_sensor_release;
	s->s_config  = ov5640_sensor_config;

probe_done:
	//CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;
}

static int __ov5640_probe(struct platform_device *pdev)
{

	return msm_camera_drv_start(pdev, ov5640_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __ov5640_probe,
	.driver = {
		.name = "msm_camera_ov5640",
		.owner = THIS_MODULE,
	},
};

static int __init ov5640_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(ov5640_init);
