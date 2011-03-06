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

#include "ov7690.h"

/*
static struct register_address_value_pair const
init_settings_array[] = {

    {0x0c, 0x06},
    {0x42, 0x0d},                                                   //DOVDD = 2.8V
    {0x80, 0x7f},                                                   //bit0:Lens Correction;bit1:AWB;bit2:AGC;bit3:Gamma;
    {0x81, 0xff},                                                   //bit0:Color Matrix;bit1:UV Average;bit2:Flip horizintal;bit3:Flip vertical;
    {0x81, 0xf3},                                                   //bit0:Color Matrix;bit1:UV Average;bit2:Flip horizintal;bit3:Flip vertical;
    {0x12, 0x40},
    {0x82, 0x03},
    {0xd0, 0x26},
    {0x3e, 0x30},
    {0x16, 0x03},
    {0x17, 0x69},
    {0x18, 0xa4},
    {0x19, 0x0c},
    {0x1a, 0xf6},
    {0xc8, 0x02},
    {0xc9, 0x90},                                                   //ISP input hsize (640)                                                   //80
    {0xca, 0x01},
    {0xcb, 0xec},                                                   //ISP input vsize (480)                                                   //e0
    {0xcc, 0x02},
    {0xcd, 0x90},                                                   //ISP output hsize (640)                                                   //80
    {0xce, 0x01},
    {0xcf, 0xec},                                                   //ISP output vsize (480)                                                   //e0
    {0x85, 0x90},                                                   //bit0 bit1:reserve;bit2:H jump;bit3:V jump;
    {0x86, 0x00},
    {0x87, 0xb0},
    {0x88, 0xA0},
    {0x89, 0x32},
    {0x8a, 0x2c},
    {0x8b, 0x30},
    {0xbb, 0xac},
    {0xbc, 0xae},
    {0xbd, 0x02},
    {0xbe, 0x1f},
    {0xbf, 0x93},
    {0xc0, 0xb1},
    {0xc1, 0x1a},
    {0xb4, 0x16},
    {0xb5, 0x05},
    {0xb7, 0x06},
    {0xb8, 0x06},
    {0xb9, 0x02},
    {0xba, 0x08},
    {0x24, 0x94},
    {0x25, 0x80},
    {0x26, 0xb4},
    {0x5a, 0x10},                                                   //30 ?
    {0x5b, 0xa1},                                                   //a5 ?
    {0x5c, 0x30},
    {0x5d, 0x20},
    {0xa3, 0x05},
    {0xa4, 0x10},
    {0xa5, 0x25},
    {0xa6, 0x46},
    {0xa7, 0x57},
    {0xa8, 0x64},
    {0xa9, 0x70},
    {0xaa, 0x7c},
    {0xab, 0x87},
    {0xac, 0x90},
    {0xad, 0x9f},
    {0xae, 0xac},
    {0xaf, 0xc1},
    {0xb0, 0xd5},
    {0xb1, 0xe7},
    {0xb2, 0x21},
    {0x8c, 0x5e},
    {0x8d, 0x11},
    {0x8e, 0x12},
    {0x8f, 0x19},
    {0x90, 0x50},
    {0x91, 0x20},
    {0x92, 0x99},
    {0x93, 0x8b},
    {0x94, 0x13},
    {0x95, 0x14},
    {0x96, 0xf0},
    {0x97, 0x10},
    {0x98, 0x34},
    {0x99, 0x32},
    {0x9a, 0x53},
    {0x9b, 0x41},
    {0x9c, 0xf0},
    {0x9d, 0xf0},
    {0x9e, 0xf0},
    {0x9f, 0xff},
    {0xa0, 0x66},
    {0xa1, 0x52},
    {0xa2, 0x11},
    {0x13, 0xff},
    {0x14, 0x21},                                                   //Select 50Hz banding filter
    {0x21, 0x68},                                                   //6 step for 50hz, 8 step for 60hz
    {0x50, 0x53},                                                   //50Hz banding filter
    {0x51, 0x45},                                                   //60Hz banding filter

    {0x11, 0x00},
    {0x29, 0x50},
    {0x2a, 0x30},
    {0x2b, 0x08},
    {0x2c, 0x00},
    {0x15, 0x00},
    {0x2d, 0x00},
    {0x2e, 0x00},

};

static struct register_address_value_pair const
qxga_xga_settings_array[] = {

    {0x12, 0x00},                         
    {0x16, 0x03},                         
    {0x17, 0x69},                         
    {0x18, 0xa4},                         
    {0x19, 0x0c},                         
    {0x1a, 0xf6},	                      
    {0xc8, 0x02},                         
    {0xc9, 0x80},//ISP input hsize (640)  
    {0xca, 0x01},                         
    {0xcb, 0xe0},//ISP input vsize (480)  
    {0xcc, 0x02},                         
    {0xcd, 0x80},//ISP output hsize (640) 
    {0xce, 0x01},                         
    {0xcf, 0xe0},//ISP output vsize (480)}
    
};
*/


static struct ov7690_i2c_reg_conf const init_settings_array[] = {
	{0x12, 0x80,BYTE_LEN, 2}, //reset
//	
	{0x0c, 0x46,BYTE_LEN, 0}, 
	{0x42, 0x0d,BYTE_LEN, 0}, 
	{0x80, 0x7f,BYTE_LEN, 0}, 
	{0x81, 0xff,BYTE_LEN, 0}, 
	{0x81, 0xf3,BYTE_LEN, 0}, 
	{0x12, 0x40,BYTE_LEN, 0}, 
	{0x82, 0x03,BYTE_LEN, 0}, 
	{0xd0, 0x26,BYTE_LEN, 0}, 
	{0x3e, 0x30,BYTE_LEN, 0}, 
	{0x16, 0x03,BYTE_LEN, 0}, 
	{0x17, 0x69,BYTE_LEN, 0}, 
	{0x18, 0xa4,BYTE_LEN, 0}, 
	{0x19, 0x0c,BYTE_LEN, 0}, 
	{0x1a, 0xf6,BYTE_LEN, 0}, 
	{0xc8, 0x02,BYTE_LEN, 0}, 
	{0xc9, 0x90,BYTE_LEN, 0}, 
	{0xca, 0x01,BYTE_LEN, 0}, 
	{0xcb, 0xec,BYTE_LEN, 0}, 
	{0xcc, 0x02,BYTE_LEN, 0}, 
	{0xcd, 0x90,BYTE_LEN, 0}, 
	{0xce, 0x01,BYTE_LEN, 0}, 
	{0xcf, 0xec,BYTE_LEN, 0}, 
	{0x85, 0x90,BYTE_LEN, 0}, 
	{0x86, 0x00,BYTE_LEN, 0}, 
	{0x87, 0xb0,BYTE_LEN, 0}, 
	{0x88, 0xA0,BYTE_LEN, 0}, 
	{0x89, 0x32,BYTE_LEN, 0}, 
	{0x8a, 0x2c,BYTE_LEN, 0}, 
	{0x8b, 0x30,BYTE_LEN, 0}, 
	{0xbb, 0xac,BYTE_LEN, 0}, 
	{0xbc, 0xae,BYTE_LEN, 0}, 
	{0xbd, 0x02,BYTE_LEN, 0}, 
	{0xbe, 0x1f,BYTE_LEN, 0}, 
	{0xbf, 0x93,BYTE_LEN, 0}, 
	{0xc0, 0xb1,BYTE_LEN, 0}, 
	{0xc1, 0x1a,BYTE_LEN, 0}, 
	{0xb4, 0x16,BYTE_LEN, 0}, 
	{0xb5, 0x05,BYTE_LEN, 0}, 
	{0xb7, 0x06,BYTE_LEN, 0}, 
	{0xb8, 0x06,BYTE_LEN, 0}, 
	{0xb9, 0x02,BYTE_LEN, 0}, 
	{0xba, 0x08,BYTE_LEN, 0}, 
	{0x24, 0x94,BYTE_LEN, 0}, 
	{0x25, 0x80,BYTE_LEN, 0}, 
	{0x26, 0xb4,BYTE_LEN, 0}, 
	{0x5a, 0x10,BYTE_LEN, 0}, 
	{0x5b, 0xa1,BYTE_LEN, 0}, 
	{0x5c, 0x30,BYTE_LEN, 0}, 
	{0x5d, 0x20,BYTE_LEN, 0}, 
	{0xa3, 0x05,BYTE_LEN, 0}, 
	{0xa4, 0x10,BYTE_LEN, 0}, 
	{0xa5, 0x25,BYTE_LEN, 0}, 
	{0xa6, 0x46,BYTE_LEN, 0}, 
	{0xa7, 0x57,BYTE_LEN, 0}, 
	{0xa8, 0x64,BYTE_LEN, 0}, 
	{0xa9, 0x70,BYTE_LEN, 0}, 
	{0xaa, 0x7c,BYTE_LEN, 0}, 
	{0xab, 0x87,BYTE_LEN, 0}, 
	{0xac, 0x90,BYTE_LEN, 0}, 
	{0xad, 0x9f,BYTE_LEN, 0}, 
	{0xae, 0xac,BYTE_LEN, 0}, 
	{0xaf, 0xc1,BYTE_LEN, 0}, 
	{0xb0, 0xd5,BYTE_LEN, 0}, 
	{0xb1, 0xe7,BYTE_LEN, 0}, 
	{0xb2, 0x21,BYTE_LEN, 0}, 
	{0x8c, 0x5e,BYTE_LEN, 0}, 
	{0x8d, 0x11,BYTE_LEN, 0}, 
	{0x8e, 0x12,BYTE_LEN, 0}, 
	{0x8f, 0x19,BYTE_LEN, 0}, 
	{0x90, 0x50,BYTE_LEN, 0}, 
	{0x91, 0x20,BYTE_LEN, 0}, 
	{0x92, 0x99,BYTE_LEN, 0}, 
	{0x93, 0x8b,BYTE_LEN, 0}, 
	{0x94, 0x13,BYTE_LEN, 0}, 
	{0x95, 0x14,BYTE_LEN, 0}, 
	{0x96, 0xf0,BYTE_LEN, 0}, 
	{0x97, 0x10,BYTE_LEN, 0}, 
	{0x98, 0x34,BYTE_LEN, 0}, 
	{0x99, 0x32,BYTE_LEN, 0}, 
	{0x9a, 0x53,BYTE_LEN, 0}, 
	{0x9b, 0x41,BYTE_LEN, 0}, 
	{0x9c, 0xf0,BYTE_LEN, 0}, 
	{0x9d, 0xf0,BYTE_LEN, 0}, 
	{0x9e, 0xf0,BYTE_LEN, 0}, 
	{0x9f, 0xff,BYTE_LEN, 0}, 
	{0xa0, 0x66,BYTE_LEN, 0}, 
	{0xa1, 0x52,BYTE_LEN, 0}, 
	{0xa2, 0x11,BYTE_LEN, 0}, 
	{0x13, 0xff,BYTE_LEN, 0}, 
	{0x14, 0x21,BYTE_LEN, 0}, 
	{0x21, 0x68,BYTE_LEN, 0}, 
	{0x50, 0x53,BYTE_LEN, 0}, 
	{0x51, 0x45,BYTE_LEN, 0}, 					  
	{0x11, 0x01,BYTE_LEN, 0}, 
	{0x29, 0x50,BYTE_LEN, 0}, 
	{0x2a, 0x30,BYTE_LEN, 0}, 
	{0x2b, 0x38,BYTE_LEN, 0}, //08-38
	{0x2c, 0x00,BYTE_LEN, 0}, 
	{0x15, 0x00,BYTE_LEN, 0}, 
	{0x2d, 0x00,BYTE_LEN, 0}, 
	{0x2e, 0x00,BYTE_LEN, 0}, 
};

static struct ov7690_i2c_reg_conf  const qxga_xga_settings_array[] = {
    {0x12, 0x00,BYTE_LEN, 0},                           
    {0x16, 0x03,BYTE_LEN, 0},                           
    {0x17, 0x69,BYTE_LEN, 0},                           
    {0x18, 0xa4,BYTE_LEN, 0},                           
    {0x19, 0x0c,BYTE_LEN, 0},                           
    {0x1a, 0xf6,BYTE_LEN, 0},	                        
    {0xc8, 0x02,BYTE_LEN, 0},                           
    {0xc9, 0x80,BYTE_LEN, 0},//ISP input hsize (640)    
    {0xca, 0x01,BYTE_LEN, 0},                           
    {0xcb, 0xe0,BYTE_LEN, 0},//ISP input vsize (480)    
    {0xcc, 0x02,BYTE_LEN, 0},                           
    {0xcd, 0x80,BYTE_LEN, 0},//ISP output hsize (640)   
    {0xce, 0x01,BYTE_LEN, 0},                           
    {0xcf, 0xe0,BYTE_LEN, 0},//ISP output vsize (480)}  
	{0x0e, 0x03,BYTE_LEN, 0},	
};


static struct ov7690_i2c_reg_conf const awb_tbl[] = {
    {0x13, 0xff, BYTE_LEN, 0},
    {0x01, 0x43, BYTE_LEN, 0},  
	{0x02, 0x54, BYTE_LEN, 0},						   
    {0x15, 0x00, BYTE_LEN, 0}                        
};


static struct ov7690_i2c_reg_conf const mwb_cloudy_tbl[] = {
    {0x13, 0xf5, BYTE_LEN, 0},                         
    {0x01, 0x58, BYTE_LEN, 0},  
	{0x02, 0x56, BYTE_LEN, 0},						   
	{0x15, 0x00, BYTE_LEN, 0}					   
};



static struct ov7690_i2c_reg_conf const mwb_fluorescent_tbl[] = {
    {0x13, 0xf5, BYTE_LEN, 0},                         
    {0x01, 0x58, BYTE_LEN, 0},  
	{0x02, 0x60, BYTE_LEN, 0},						   
	{0x15, 0x00, BYTE_LEN, 0}					   
};


static struct ov7690_i2c_reg_conf const mwb_day_light_tbl[] = {
    {0x13, 0xf5, BYTE_LEN, 0},                         
    {0x01, 0x58, BYTE_LEN, 0},  
	{0x02, 0x56, BYTE_LEN, 0},						   
	{0x15, 0x00, BYTE_LEN, 0}					   
};

static struct ov7690_i2c_reg_conf const mwb_incandescent_tbl[] = {
	{0x13, 0xf5, BYTE_LEN, 0},						   
	{0x01, 0x67, BYTE_LEN, 0},	
	{0x02, 0x40, BYTE_LEN, 0},						   
	{0x15, 0x00, BYTE_LEN, 0}					   
};
 




static struct ov7690_i2c_reg_conf const effect_off_tbl[] = {
    	{0x28, 0x00, BYTE_LEN, 0},   
        {0xbb, 0xac,BYTE_LEN, 0}, 
	{0xbc, 0xae,BYTE_LEN, 0}, 
	{0xbd, 0x02,BYTE_LEN, 0}, 
	{0xbe, 0x1f,BYTE_LEN, 0}, 
	{0xbf, 0x93,BYTE_LEN, 0}, 
	{0xc0, 0xb1,BYTE_LEN, 0}, 
	{0xc1, 0x1a,BYTE_LEN, 0},                  
};



static struct ov7690_i2c_reg_conf const effect_mono_tbl[] = {
	{0x28, 0x00, BYTE_LEN, 0},  	
	{0xbb, 0x00, BYTE_LEN, 0},					   
	{0xbc, 0x00, BYTE_LEN, 0},	  
	{0xbd, 0x00, BYTE_LEN, 0},				  
	{0xbe, 0x00, BYTE_LEN, 0},	
	{0xbf, 0x00, BYTE_LEN, 0},	
	{0xc0, 0x00, BYTE_LEN, 0},	
	{0xc1, 0x00, BYTE_LEN, 0}	
 				  
};

static struct ov7690_i2c_reg_conf const effect_sepia_tbl[] = {
	{0x28, 0x00, BYTE_LEN, 0},  	
	{0xbb, 0x20, BYTE_LEN, 0},					   
	{0xbc, 0x20, BYTE_LEN, 0},	  
	{0xbd, 0x20, BYTE_LEN, 0},				  
	{0xbe, 0x30, BYTE_LEN, 0},	
	{0xbf, 0x30, BYTE_LEN, 0},	
	{0xc0, 0x30, BYTE_LEN, 0},	
	{0xc1, 0x1c, BYTE_LEN, 0}	
				  
};

static struct ov7690_i2c_reg_conf const effect_negative_tbl[] = {
	{0x28, 0x80, BYTE_LEN, 0},
        {0xbb, 0xac,BYTE_LEN, 0}, 
	{0xbc, 0xae,BYTE_LEN, 0}, 
	{0xbd, 0x02,BYTE_LEN, 0}, 
	{0xbe, 0x1f,BYTE_LEN, 0}, 
	{0xbf, 0x93,BYTE_LEN, 0}, 
	{0xc0, 0xb1,BYTE_LEN, 0}, 
	{0xc1, 0x1a,BYTE_LEN, 0},             
			  
};

static struct ov7690_i2c_reg_conf const effect_bluish_tbl[] = {
	{0x81, 0x20, BYTE_LEN, 0},						   	
	{0x28, 0x00, BYTE_LEN, 0},
	{0xd2, 0x18, BYTE_LEN, 0},	  
	{0xda, 0xa0, BYTE_LEN, 0},				  
	{0xdb, 0x40, BYTE_LEN, 0}					  
};

static struct ov7690_i2c_reg_conf const effect_greenish_tbl[] = {
	{0x81, 0x20, BYTE_LEN, 0},	
	{0x28, 0x00, BYTE_LEN, 0},						   
	{0xd2, 0x18, BYTE_LEN, 0},	  
	{0xda, 0x60, BYTE_LEN, 0},				  
	{0xdb, 0x60, BYTE_LEN, 0}					  
};

static struct ov7690_i2c_reg_conf const effect_reddish_tbl[] = {
	{0x81, 0x20, BYTE_LEN, 0},
	{0x28, 0x00, BYTE_LEN, 0},						   
	{0xd2, 0x18, BYTE_LEN, 0},	  
	{0xda, 0x80, BYTE_LEN, 0},				  
	{0xdb, 0xc0, BYTE_LEN, 0}					  
};

static struct ov7690_i2c_reg_conf const scene_auto_tbl[] = {    
    {0x11, 0x01, BYTE_LEN, 0},   //devf                      
    {0x29, 0x50, BYTE_LEN, 0}                     		 
};


static struct ov7690_i2c_reg_conf const scene_night_tbl[] = {
    {0x11, 0x03, BYTE_LEN, 0},                         
    {0x29, 0x50, BYTE_LEN, 0}                     

};






struct ov7690_reg ov7690_regs = {
	.init_settings = &init_settings_array[0],
	.init_settings_size = ARRAY_SIZE(init_settings_array),
	.qxga_xga_settings = &qxga_xga_settings_array[0],
	.qxga_xga_settings_size = ARRAY_SIZE(qxga_xga_settings_array),
	
	.awb_tbl = awb_tbl,//add by lijiankun 2010-9-3 auto whitebalance
	.awb_tbl_size = ARRAY_SIZE(awb_tbl),
	.mwb_cloudy_tbl = mwb_cloudy_tbl,//add by lijiankun 2010-9-3 cloudy
	.mwb_cloudy_tbl_size = ARRAY_SIZE(mwb_cloudy_tbl),
	.mwb_day_light_tbl = mwb_day_light_tbl,//add by lijiankun 2010-9-3 Day_light
	.mwb_day_light_tbl_size = ARRAY_SIZE(mwb_day_light_tbl),
	.mwb_fluorescent_tbl = mwb_fluorescent_tbl,//add by lijiankun 2010-9-3 FLUORESCENT
	.mwb_fluorescent_tbl_size = ARRAY_SIZE(mwb_fluorescent_tbl),
	.mwb_incandescent_tbl = mwb_incandescent_tbl,//add by lijiankun 2010-9-3 INCANDESCENT
	.mwb_incandescent_tbl_size = ARRAY_SIZE(mwb_incandescent_tbl),
	
	.effect_off_tbl = effect_off_tbl,//add by lijiankun 2010-9-7 EFFECT_OFF
	.effect_off_tbl_size = ARRAY_SIZE(effect_off_tbl),
	.effect_mono_tbl = effect_mono_tbl,//add by lijiankun 2010-9-7 EFFECT_MONO
	.effect_mono_tbl_size = ARRAY_SIZE(effect_mono_tbl),
	.effect_sepia_tbl = effect_sepia_tbl,//add by lijiankun 2010-9-7 EFFECT_SEPIA
	.effect_sepia_tbl_size = ARRAY_SIZE(effect_sepia_tbl),
	.effect_negative_tbl = effect_negative_tbl,//add by lijiankun 2010-9-7 EFFECT_NEGATIVE
	.effect_negative_tbl_size = ARRAY_SIZE(effect_negative_tbl),
	.effect_bluish_tbl = effect_bluish_tbl,//add by lijiankun 2010-9-7 EFFECT_SOLARIZE
	.effect_bluish_tbl_size = ARRAY_SIZE(effect_bluish_tbl),
	.effect_greenish_tbl = effect_greenish_tbl,//add by lijiankun 2010-9-7 EFFECT_SOLARIZE
	.effect_greenish_tbl_size = ARRAY_SIZE(effect_greenish_tbl),
	.effect_reddish_tbl = effect_reddish_tbl,//add by lijiankun 2010-9-7 EFFECT_SOLARIZE
	.effect_reddish_tbl_size = ARRAY_SIZE(effect_reddish_tbl),
		
	.scene_auto_tbl = scene_auto_tbl,//add by lijiankun 2010-9-15 scene
	.scene_auto_tbl_size = ARRAY_SIZE(scene_auto_tbl),
	.scene_night_tbl = scene_night_tbl,//add by lijiankun 2010-9-15 scene
	.scene_night_tbl_size = ARRAY_SIZE(scene_night_tbl)
};

