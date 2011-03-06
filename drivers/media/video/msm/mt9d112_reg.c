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

#include "mt9d112.h"

static const struct mt9d112_i2c_reg_conf const init_tbl[] = {

    //[Step2-PLL_Timing]
   // {0x0010, 0x0340, WORD_LEN, 0},	//PLL Dividers = 832
    {0x0010, 0x0340, WORD_LEN, 0},	//PLL Dividers = 808
    {0x0012, 0x00F0, WORD_LEN, 0},	//PLL P Dividers = 112
    {0x0014, 0x2025, WORD_LEN, 0},	//PLL Control: TEST_BYPASS off = 8229
    {0x001E, 0x0775, WORD_LEN, 0},	//Pad Slew Pad Config = 1911
    {0x0022, 0x01E0, WORD_LEN, 0},	//VDD_DIS counter delay
    {0x002A, 0x7F7F, WORD_LEN, 0},	//PLL P Dividers 4-5-6 = 32327
    {0x002C, 0x0000, WORD_LEN, 0},	//PLL P Dividers 7 = 0
    {0x002E, 0x0000, WORD_LEN, 0}, //Sensor Clock Divider = 0
    {0x0018, 0x4008, WORD_LEN, 10},	//Standby Control and Status: Out of standby
    //{0x30D4, 0x9080, WORD_LEN, 0},	//Disable Double Samplings
    
    {0x098E, 0x483A, WORD_LEN, 0}, 	// LOGICAL_ADDRESS_ACCESS [CAM_CORE_A_Y_ADDR_START]
    {0xC83A, 0x000C, WORD_LEN, 0}, 	// CAM_CORE_A_Y_ADDR_START
    {0xC83C, 0x0018, WORD_LEN, 0}, 	// CAM_CORE_A_X_ADDR_START
    {0xC83E, 0x07B1, WORD_LEN, 0}, 	// CAM_CORE_A_Y_ADDR_END
    {0xC840, 0x0A45, WORD_LEN, 0}, 	// CAM_CORE_A_X_ADDR_END
    {0xC842, 0x0001, WORD_LEN, 0}, 	// CAM_CORE_A_ROW_SPEED
    {0xC844, 0x0103, WORD_LEN, 0}, 	// CAM_CORE_A_SKIP_X_CORE
    {0xC846, 0x0103, WORD_LEN, 0}, 	// CAM_CORE_A_SKIP_Y_CORE
    {0xC848, 0x0103, WORD_LEN, 0}, 	// CAM_CORE_A_SKIP_X_PIPE
    {0xC84A, 0x0103, WORD_LEN, 0}, 	// CAM_CORE_A_SKIP_Y_PIPE
    {0xC84C, 0x00F6, WORD_LEN, 0},   	// CAM_CORE_A_POWER_MODE
    {0xC84E, 0x0001, WORD_LEN, 0}, 	// CAM_CORE_A_BIN_MODE
    {0xC850, 0x00,   BYTE_LEN, 0}, 	// CAM_CORE_A_ORIENTATION
    {0xC851, 0x00,   BYTE_LEN, 0}, 	// CAM_CORE_A_PIXEL_ORDER
    {0xC852, 0x019C, WORD_LEN, 0}, 	// CAM_CORE_A_FINE_CORRECTION
    {0xC854, 0x0732, WORD_LEN, 0}, 	// CAM_CORE_A_FINE_ITMIN
    {0xC858, 0x0000, WORD_LEN, 0}, 	// CAM_CORE_A_COARSE_ITMIN
    {0xC85A, 0x0001, WORD_LEN, 0}, 	// CAM_CORE_A_COARSE_ITMAX_MARGIN
    {0xC85C, 0x0423, WORD_LEN, 0},   	// CAM_CORE_A_MIN_FRAME_LENGTH_LINES
    {0xC85E, 0xFFFF, WORD_LEN, 0},   	// CAM_CORE_A_MAX_FRAME_LENGTH_LINES
    {0xC860, 0x0423, WORD_LEN, 0},   	// CAM_CORE_A_BASE_FRAME_LENGTH_LINES
    {0xC862, 0x1194, WORD_LEN, 0},   	// CAM_CORE_A_MIN_LINE_LENGTH_PCLK
    {0xC864, 0xFFFE, WORD_LEN, 0},   	// CAM_CORE_A_MAX_LINE_LENGTH_PCLK
    {0xC866, 0x7F7F, WORD_LEN, 0},   	// CAM_CORE_A_P4_5_6_DIVIDER
    {0xC868, 0x0423, WORD_LEN, 0},   	// CAM_CORE_A_FRAME_LENGTH_LINES
    {0xC86A, 0x1194, WORD_LEN, 0},   	// CAM_CORE_A_LINE_LENGTH_PCK
    {0xC86C, 0x0518, WORD_LEN, 0},   	// CAM_CORE_A_OUTPUT_SIZE_WIDTH
    {0xC86E, 0x03D4, WORD_LEN, 0},   	// CAM_CORE_A_OUTPUT_SIZE_HEIGHT
    {0xC870, 0x0014, WORD_LEN, 0},   	// CAM_CORE_A_RX_FIFO_TRIGGER_MARK
    {0xC858, 0x0002, WORD_LEN, 0},   	// CAM_CORE_A_COARSE_ITMIN
    {0xC8B8, 0x0004, WORD_LEN, 0},   	// CAM_OUTPUT_0_JPEG_CONTROL
    {0xC8AE, 0x0001, WORD_LEN, 0},   	// CAM_OUTPUT_0_OUTPUT_FORMAT
    {0xC8AA, 0x0320, WORD_LEN, 0}, 	// CAM_OUTPUT_0_IMAGE_WIDTH
    {0xC8AC, 0x0258, WORD_LEN, 0}, 	// CAM_OUTPUT_0_IMAGE_HEIGHT
    {0xC872, 0x0010, WORD_LEN, 0},   	// CAM_CORE_B_Y_ADDR_START
    {0xC874, 0x001C, WORD_LEN, 0},   	// CAM_CORE_B_X_ADDR_START
    {0xC876, 0x07AF, WORD_LEN, 0},   	// CAM_CORE_B_Y_ADDR_END
    {0xC878, 0x0A43, WORD_LEN, 0},   	// CAM_CORE_B_X_ADDR_END
    {0xC87A, 0x0001, WORD_LEN, 0},   	// CAM_CORE_B_ROW_SPEED
    {0xC87C, 0x0101, WORD_LEN, 0},   	// CAM_CORE_B_SKIP_X_CORE
    {0xC87E, 0x0101, WORD_LEN, 0},   	// CAM_CORE_B_SKIP_Y_CORE
    {0xC880, 0x0101, WORD_LEN, 0},   	// CAM_CORE_B_SKIP_X_PIPE
    {0xC882, 0x0101, WORD_LEN, 0},   	// CAM_CORE_B_SKIP_Y_PIPE
    {0xC884, 0x00F2, WORD_LEN, 0},   	// CAM_CORE_B_POWER_MODE
    {0xC886, 0x0000, WORD_LEN, 0},   	// CAM_CORE_B_BIN_MODE
    {0xC888, 0x00,   BYTE_LEN, 0}, 	// CAM_CORE_B_ORIENTATION
    {0xC889, 0x00,   BYTE_LEN, 0}, 	// CAM_CORE_B_PIXEL_ORDER
    {0xC88A, 0x009C, WORD_LEN, 0},   	// CAM_CORE_B_FINE_CORRECTION
    {0xC88C, 0x034A, WORD_LEN, 0},   	// CAM_CORE_B_FINE_ITMIN
    {0xC890, 0x0000, WORD_LEN, 0},   	// CAM_CORE_B_COARSE_ITMIN
    {0xC892, 0x0001, WORD_LEN, 0},   	// CAM_CORE_B_COARSE_ITMAX_MARGIN
    {0xC894, 0x07EF, WORD_LEN, 0},   	// CAM_CORE_B_MIN_FRAME_LENGTH_LINES
    {0xC896, 0xFFFF, WORD_LEN, 0},   	// CAM_CORE_B_MAX_FRAME_LENGTH_LINES
    {0xC898, 0x082F, WORD_LEN, 0},   	// CAM_CORE_B_BASE_FRAME_LENGTH_LINES
    {0xC89A, 0x28A0, WORD_LEN, 0},   	// CAM_CORE_B_MIN_LINE_LENGTH_PCLK
    {0xC89C, 0xFFFE, WORD_LEN, 0},   	// CAM_CORE_B_MAX_LINE_LENGTH_PCLK
    {0xC89E, 0x7F7F, WORD_LEN, 0},   	// CAM_CORE_B_P4_5_6_DIVIDER
    {0xC8A0, 0x07EF, WORD_LEN, 0},   	// CAM_CORE_B_FRAME_LENGTH_LINES
    {0xC8A2, 0x28A0, WORD_LEN, 0},   	// CAM_CORE_B_LINE_LENGTH_PCK
    {0xC8A4, 0x0A28, WORD_LEN, 0},   	// CAM_CORE_B_OUTPUT_SIZE_WIDTH
    {0xC8A6, 0x07A0, WORD_LEN, 0},   	// CAM_CORE_B_OUTPUT_SIZE_HEIGHT
    {0xC8A8, 0x0124, WORD_LEN, 0},   	// CAM_CORE_B_RX_FIFO_TRIGGER_MARK
    {0xC890, 0x0002, WORD_LEN, 0},   	// CAM_CORE_B_COARSE_ITMIN
    {0xC8C0, 0x0A20, WORD_LEN, 0},   	// CAM_OUTPUT_1_IMAGE_WIDTH
    {0xC8C2, 0x0798, WORD_LEN, 0},   	// CAM_OUTPUT_1_IMAGE_HEIGHT
    {0xC89A, 0x28A0, WORD_LEN, 0},   	// CAM_CORE_B_MIN_LINE_LENGTH_PCLK
    {0xC8A2, 0x28A0, WORD_LEN, 0},   	// CAM_CORE_B_LINE_LENGTH_PCK
    {0xC8C4, 0x0001, WORD_LEN, 0},   	// CAM_OUTPUT_1_OUTPUT_FORMAT
    {0xC8C6, 0x0000, WORD_LEN, 0},   	// CAM_OUTPUT_1_OUTPUT_FORMAT_ORDER
    {0xC8CE, 0x0014, WORD_LEN, 0},   	// CAM_OUTPUT_1_JPEG_CONTROL
    {0xD822, 0x4610, WORD_LEN, 0},   	// JPEG_JPSS_CTRL_VAR
    {0x3330, 0x0000, WORD_LEN, 0},   	// OUTPUT_FORMAT_TEST
    {0xA018, 0x00D5, WORD_LEN, 0},	//fd_expected50hz_flicker_period (A) 
    {0xA01A, 0x005C, WORD_LEN, 0},	//fd_expected50hz_flicker_period (B) 
    {0xA01C, 0x00B1, WORD_LEN, 0},	//fd_expected60hz_flicker_period (A) 
    {0xA01E, 0x004C, WORD_LEN, 0},	//fd_expected60hz_flicker_period (B)
		
    //[Step3-Recommended]
    {0x0982, 0x0000, WORD_LEN, 0},  // ACCESS_CTL_STAT
    {0x098A, 0x0000, WORD_LEN, 0},  // PHYSICAL_ADDRESS_ACCESS
    {0x886C, 0xC0F1, WORD_LEN, 0},  
    {0x886E, 0xC5E1, WORD_LEN, 0},  
    {0x8870, 0x246A, WORD_LEN, 0},  
    {0x8872, 0x1280, WORD_LEN, 0},  
    {0x8874, 0xC4E1, WORD_LEN, 0},  
    {0x8876, 0xD20F, WORD_LEN, 0},  
    {0x8878, 0x2069, WORD_LEN, 0},  
    {0x887A, 0x0000, WORD_LEN, 0},  
    {0x887C, 0x6A62, WORD_LEN, 0},  
    {0x887E, 0x1303, WORD_LEN, 0},  
    {0x8880, 0x0084, WORD_LEN, 0},  
    {0x8882, 0x1734, WORD_LEN, 0},  
    {0x8884, 0x7005, WORD_LEN, 0},  
    {0x8886, 0xD801, WORD_LEN, 0},  
    {0x8888, 0x8A41, WORD_LEN, 0},  
    {0x888A, 0xD900, WORD_LEN, 0},  
    {0x888C, 0x0D5A, WORD_LEN, 0},  
    {0x888E, 0x0664, WORD_LEN, 0},  
    {0x8890, 0x8B61, WORD_LEN, 0},  
    {0x8892, 0xE80B, WORD_LEN, 0},  
    {0x8894, 0x000D, WORD_LEN, 0},  
    {0x8896, 0x0020, WORD_LEN, 0},  
    {0x8898, 0xD508, WORD_LEN, 0},  
    {0x889A, 0x1504, WORD_LEN, 0},  
    {0x889C, 0x1400, WORD_LEN, 0},  
    {0x889E, 0x7840, WORD_LEN, 0},  
    {0x88A0, 0xD007, WORD_LEN, 0},  
    {0x88A2, 0x0DFB, WORD_LEN, 0},  
    {0x88A4, 0x9004, WORD_LEN, 0},  
    {0x88A6, 0xC4C1, WORD_LEN, 0},  
    {0x88A8, 0x2029, WORD_LEN, 0},  
    {0x88AA, 0x0300, WORD_LEN, 0},  
    {0x88AC, 0x0219, WORD_LEN, 0},  
    {0x88AE, 0x06C4, WORD_LEN, 0},  
    {0x88B0, 0xFF80, WORD_LEN, 0},  
    {0x88B2, 0x08C4, WORD_LEN, 0},  
    {0x88B4, 0xFF80, WORD_LEN, 0},  
    {0x88B6, 0x086C, WORD_LEN, 0},  
    {0x88B8, 0xFF80, WORD_LEN, 0},  
    {0x88BA, 0x08C0, WORD_LEN, 0},  
    {0x88BC, 0xFF80, WORD_LEN, 0},  
    {0x88BE, 0x08C4, WORD_LEN, 0},  
    {0x88C0, 0xFF80, WORD_LEN, 0},  
    {0x88C2, 0x097C, WORD_LEN, 0},  
    {0x88C4, 0x0001, WORD_LEN, 0},  
    {0x88C6, 0x0005, WORD_LEN, 0},  
    {0x88C8, 0x0000, WORD_LEN, 0},  
    {0x88CA, 0x0000, WORD_LEN, 0},  
    {0x88CC, 0xC0F1, WORD_LEN, 0},  
    {0x88CE, 0x0976, WORD_LEN, 0},  
    {0x88D0, 0x06C4, WORD_LEN, 0},  
    {0x88D2, 0xD639, WORD_LEN, 0},  
    {0x88D4, 0x7708, WORD_LEN, 0},  
    {0x88D6, 0x8E01, WORD_LEN, 0},  
    {0x88D8, 0x1604, WORD_LEN, 0},  
    {0x88DA, 0x1091, WORD_LEN, 0},  
    {0x88DC, 0x2046, WORD_LEN, 0},  
    {0x88DE, 0x00C1, WORD_LEN, 0},  
    {0x88E0, 0x202F, WORD_LEN, 0},  
    {0x88E2, 0x2047, WORD_LEN, 0},  
    {0x88E4, 0xAE21, WORD_LEN, 0},  
    {0x88E6, 0x0F8F, WORD_LEN, 0},  
    {0x88E8, 0x1440, WORD_LEN, 0},  
    {0x88EA, 0x8EAA, WORD_LEN, 0},  
    {0x88EC, 0x8E0B, WORD_LEN, 0},  
    {0x88EE, 0x224A, WORD_LEN, 0},  
    {0x88F0, 0x2040, WORD_LEN, 0},  
    {0x88F2, 0x8E2D, WORD_LEN, 0},  
    {0x88F4, 0xBD08, WORD_LEN, 0},  
    {0x88F6, 0x7D05, WORD_LEN, 0},  
    {0x88F8, 0x8E0C, WORD_LEN, 0},  
    {0x88FA, 0xB808, WORD_LEN, 0},  
    {0x88FC, 0x7825, WORD_LEN, 0},  
    {0x88FE, 0x7510, WORD_LEN, 0},  
    {0x8900, 0x22C2, WORD_LEN, 0},  
    {0x8902, 0x248C, WORD_LEN, 0},  
    {0x8904, 0x081D, WORD_LEN, 0},  
    {0x8906, 0x0363, WORD_LEN, 0},  
    {0x8908, 0xD9FF, WORD_LEN, 0},  
    {0x890A, 0x2502, WORD_LEN, 0},  
    {0x890C, 0x1002, WORD_LEN, 0},  
    {0x890E, 0x2A05, WORD_LEN, 0},  
    {0x8910, 0x03FE, WORD_LEN, 0},  
    {0x8912, 0x0A16, WORD_LEN, 0},  
    {0x8914, 0x06E4, WORD_LEN, 0},  
    {0x8916, 0x702F, WORD_LEN, 0},  
    {0x8918, 0x7810, WORD_LEN, 0},  
    {0x891A, 0x7D02, WORD_LEN, 0},  
    {0x891C, 0x7DB0, WORD_LEN, 0},  
    {0x891E, 0xF00B, WORD_LEN, 0},  
    {0x8920, 0x78A2, WORD_LEN, 0},  
    {0x8922, 0x2805, WORD_LEN, 0},  
    {0x8924, 0x03FE, WORD_LEN, 0},  
    {0x8926, 0x0A02, WORD_LEN, 0},  
    {0x8928, 0x06E4, WORD_LEN, 0},  
    {0x892A, 0x702F, WORD_LEN, 0},  
    {0x892C, 0x7810, WORD_LEN, 0},  
    {0x892E, 0x651D, WORD_LEN, 0},  
    {0x8930, 0x7DB0, WORD_LEN, 0},  
    {0x8932, 0x7DAF, WORD_LEN, 0},  
    {0x8934, 0x8E08, WORD_LEN, 0},  
    {0x8936, 0xBD06, WORD_LEN, 0},  
    {0x8938, 0xD120, WORD_LEN, 0},  
    {0x893A, 0xB8C3, WORD_LEN, 0},  
    {0x893C, 0x78A5, WORD_LEN, 0},  
    {0x893E, 0xB88F, WORD_LEN, 0},  
    {0x8940, 0x1908, WORD_LEN, 0},  
    {0x8942, 0x0024, WORD_LEN, 0},  
    {0x8944, 0x2841, WORD_LEN, 0},  
    {0x8946, 0x0201, WORD_LEN, 0},  
    {0x8948, 0x1E26, WORD_LEN, 0},  
    {0x894A, 0x1042, WORD_LEN, 0},  
    {0x894C, 0x0F15, WORD_LEN, 0},  
    {0x894E, 0x1463, WORD_LEN, 0},  
    {0x8950, 0x1E27, WORD_LEN, 0},  
    {0x8952, 0x1002, WORD_LEN, 0},  
    {0x8954, 0x224C, WORD_LEN, 0},  
    {0x8956, 0xA000, WORD_LEN, 0},  
    {0x8958, 0x224A, WORD_LEN, 0},  
    {0x895A, 0x2040, WORD_LEN, 0},  
    {0x895C, 0x22C2, WORD_LEN, 0},  
    {0x895E, 0x2482, WORD_LEN, 0},  
    {0x8960, 0x204F, WORD_LEN, 0},  
    {0x8962, 0x2040, WORD_LEN, 0},  
    {0x8964, 0x224C, WORD_LEN, 0},  
    {0x8966, 0xA000, WORD_LEN, 0},  
    {0x8968, 0xB8A2, WORD_LEN, 0},  
    {0x896A, 0xF204, WORD_LEN, 0},  
    {0x896C, 0x2045, WORD_LEN, 0},  
    {0x896E, 0x2180, WORD_LEN, 0},  
    {0x8970, 0xAE01, WORD_LEN, 0},  
    {0x8972, 0x0D9E, WORD_LEN, 0},  
    {0x8974, 0xFFE3, WORD_LEN, 0},  
    {0x8976, 0x70E9, WORD_LEN, 0},  
    {0x8978, 0x0125, WORD_LEN, 0},  
    {0x897A, 0x06C4, WORD_LEN, 0},  
    {0x897C, 0xC0F1, WORD_LEN, 0},  
    {0x897E, 0xD010, WORD_LEN, 0},  
    {0x8980, 0xD110, WORD_LEN, 0},  
    {0x8982, 0xD20D, WORD_LEN, 0},  
    {0x8984, 0xA020, WORD_LEN, 0},  
    {0x8986, 0x8A00, WORD_LEN, 0},  
    {0x8988, 0x0809, WORD_LEN, 0},  
    {0x898A, 0x01DE, WORD_LEN, 0},  
    {0x898C, 0xB8A7, WORD_LEN, 0},  
    {0x898E, 0xAA00, WORD_LEN, 0},  
    {0x8990, 0xDBFF, WORD_LEN, 0},  
    {0x8992, 0x2B41, WORD_LEN, 0},  
    {0x8994, 0x0200, WORD_LEN, 0},  
    {0x8996, 0xAA0C, WORD_LEN, 0},  
    {0x8998, 0x1228, WORD_LEN, 0},  
    {0x899A, 0x0080, WORD_LEN, 0},  
    {0x899C, 0xAA6D, WORD_LEN, 0},  
    {0x899E, 0x0815, WORD_LEN, 0},  
    {0x89A0, 0x01DE, WORD_LEN, 0},  
    {0x89A2, 0xB8A7, WORD_LEN, 0},  
    {0x89A4, 0x1A28, WORD_LEN, 0},  
    {0x89A6, 0x0002, WORD_LEN, 0},  
    {0x89A8, 0x8123, WORD_LEN, 0},  
    {0x89AA, 0x7960, WORD_LEN, 0},  
    {0x89AC, 0x1228, WORD_LEN, 0},  
    {0x89AE, 0x0080, WORD_LEN, 0},  
    {0x89B0, 0xC0D1, WORD_LEN, 0},  
    {0x89B2, 0x7EE0, WORD_LEN, 0},  
    {0x89B4, 0xFF80, WORD_LEN, 0},  
    {0x89B6, 0x0158, WORD_LEN, 0},  
    {0x89B8, 0xFF00, WORD_LEN, 0},  
    {0x89BA, 0x0618, WORD_LEN, 0},  
    {0x89BC, 0x8000, WORD_LEN, 0},  
    {0x89BE, 0x0008, WORD_LEN, 0},  
    {0x89C0, 0xFF80, WORD_LEN, 0},  
    {0x89C2, 0x0A08, WORD_LEN, 0},  
    {0x89C4, 0xE280, WORD_LEN, 0},  
    {0x89C6, 0x24CA, WORD_LEN, 0},  
    {0x89C8, 0x7082, WORD_LEN, 0},  
    {0x89CA, 0x78E0, WORD_LEN, 0},  
    {0x89CC, 0x20E8, WORD_LEN, 0},  
    {0x89CE, 0x01A2, WORD_LEN, 0},  
    {0x89D0, 0x1002, WORD_LEN, 0},  
    {0x89D2, 0x0D02, WORD_LEN, 0},  
    {0x89D4, 0x1902, WORD_LEN, 0},  
    {0x89D6, 0x0094, WORD_LEN, 0},  
    {0x89D8, 0x7FE0, WORD_LEN, 0},  
    {0x89DA, 0x7028, WORD_LEN, 0},  
    {0x89DC, 0x7308, WORD_LEN, 0},  
    {0x89DE, 0x1000, WORD_LEN, 0},  
    {0x89E0, 0x0900, WORD_LEN, 0},  
    {0x89E2, 0x7904, WORD_LEN, 0},  
    {0x89E4, 0x7947, WORD_LEN, 0},  
    {0x89E6, 0x1B00, WORD_LEN, 0},  
    {0x89E8, 0x0064, WORD_LEN, 0},  
    {0x89EA, 0x7EE0, WORD_LEN, 0},  
    {0x89EC, 0xE280, WORD_LEN, 0},  
    {0x89EE, 0x24CA, WORD_LEN, 0},  
    {0x89F0, 0x7082, WORD_LEN, 0},  
    {0x89F2, 0x78E0, WORD_LEN, 0},  
    {0x89F4, 0x20E8, WORD_LEN, 0},  
    {0x89F6, 0x01A2, WORD_LEN, 0},  
    {0x89F8, 0x1102, WORD_LEN, 0},  
    {0x89FA, 0x0502, WORD_LEN, 0},  
    {0x89FC, 0x1802, WORD_LEN, 0},  
    {0x89FE, 0x00B4, WORD_LEN, 0},  
    {0x8A00, 0x7FE0, WORD_LEN, 0},  
    {0x8A02, 0x7028, WORD_LEN, 0},  
    {0x8A04, 0x0000, WORD_LEN, 0},  
    {0x8A06, 0x0000, WORD_LEN, 0},  
    {0x8A08, 0xFF80, WORD_LEN, 0},  
    {0x8A0A, 0x097C, WORD_LEN, 0},  
    {0x8A0C, 0xFF80, WORD_LEN, 0},  
    {0x8A0E, 0x08CC, WORD_LEN, 0},  
    {0x8A10, 0x0000, WORD_LEN, 0},  
    {0x8A12, 0x08DC, WORD_LEN, 0},  
    {0x8A14, 0x0000, WORD_LEN, 0},  
    {0x8A16, 0x0998, WORD_LEN, 0},  
    {0x098E, 0x0016, WORD_LEN, 0},  // LOGICAL_ADDRESS_ACCESS [MON_ADDRESS_LO]
    {0x8016, 0x086C, WORD_LEN, 0},  // MON_ADDRESS_LO
    //{0x8002, 0x0001, WORD_LEN, 250},	// MON_CMD    
    {0x8002, 0x0001, WORD_LEN, 80},	// MON_CMD    
    
   	{0x098E, 0xC40C, WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS
   	{0xC40C, 0x00FF, WORD_LEN, 0},	// AFM_POS_MAX
   	{0xC40A, 0x0000, WORD_LEN, 0},	// AFM_POS_MIN
   	{0x30D4, 0x9080, WORD_LEN, 0},    // LOGICAL_ADDRESS_ACCESS
   	{0x316E, 0xCAFF, WORD_LEN, 0},    // AFM_POS_MAX
   	{0x305E, 0x10A0, WORD_LEN, 0},    // AFM_POS_MIN
   	{0x3E00, 0x0010, WORD_LEN, 0},	// COLUMN_CORRECTION
   	{0x3E02, 0xED02, WORD_LEN, 0},	// DAC_ECL
   	{0x3E04, 0xC88C, WORD_LEN, 0},	// GLOBAL_GAIN
   	{0x3E06, 0xC88C, WORD_LEN, 0},	// SAMP_CONTROL
   	{0x3E08, 0x700A, WORD_LEN, 0},	// SAMP_ADDR_EN
   	{0x3E0A, 0x701E, WORD_LEN, 0},	// SAMP_RD1_SIG
   	{0x3E0C, 0x00FF, WORD_LEN, 0},	// SAMP_RD1_SIG_BOOST
   	{0x3E0E, 0x00FF, WORD_LEN, 0},	// SAMP_RD1_RST
   	{0x3E10, 0x00FF, WORD_LEN, 0},	// SAMP_RD1_RST_BOOST
   	{0x3E12, 0x0000, WORD_LEN, 0},	// SAMP_RST1_EN
   {0x3E14, 0xC78C, WORD_LEN, 0},	// SAMP_RST1_BOOST
   {0x3E16, 0x6E06, WORD_LEN, 0},	// SAMP_RST1_CLOOP_SH
   {0x3E18, 0xA58C, WORD_LEN, 0},	// SAMP_RST_BOOST_SEQ
   {0x3E1A, 0xA58E, WORD_LEN, 0},	// SAMP_SAMP1_SIG
   {0x3E1C, 0xA58E, WORD_LEN, 0},	// SAMP_SAMP1_RST
   {0x3E1E, 0xC0D0, WORD_LEN, 0},	// SAMP_TX_EN
   {0x3E20, 0xEB00, WORD_LEN, 0},	// SAMP_TX_BOOST
   {0x3E22, 0x00FF, WORD_LEN, 0},	// SAMP_TX_CLOOP_SH
   {0x3E24, 0xEB02, WORD_LEN, 0},	// SAMP_TX_BOOST_SEQ
   {0x3E26, 0xEA02, WORD_LEN, 0},	// SAMP_VLN_EN
   {0x3E28, 0xEB0A, WORD_LEN, 0},	// SAMP_VLN_HOLD
   {0x3E2A, 0xEC01, WORD_LEN, 0},	// SAMP_VCL_EN
   {0x3E2C, 0xEB01, WORD_LEN, 0},	// SAMP_COLCLAMP
   {0x3E2E, 0x00FF, WORD_LEN, 0},	// SAMP_SH_VCL
   {0x3E30, 0x00F3, WORD_LEN, 0},	// SAMP_SH_VREF
   {0x3E32, 0x3DFA, WORD_LEN, 0},	// SAMP_SH_VBST
   {0x3E34, 0x00FF, WORD_LEN, 0},	// SAMP_SPARE
   {0x3E36, 0x00F3, WORD_LEN, 0},	// SAMP_READOUT
   {0x3E38, 0x0000, WORD_LEN, 0},	// SAMP_RESET_DONE
   {0x3E3A, 0xF802, WORD_LEN, 0},	// SAMP_VLN_CLAMP
   {0x3E3C, 0x0FFF, WORD_LEN, 0},	// SAMP_ASC_INT
   {0x3E3E, 0xEA10, WORD_LEN, 0},	// SAMP_RS_CLOOP_SH_R
   {0x3E40, 0xEB05, WORD_LEN, 0},	// SAMP_RS_CLOOP_SH
   {0x3E42, 0xE5C8, WORD_LEN, 0},	// SAMP_RS_BOOST_SEQ
   {0x3E44, 0xE5C8, WORD_LEN, 0},	// SAMP_TXLO_GND
   {0x3E46, 0x8C70, WORD_LEN, 0},	// SAMP_VLN_PER_COL
   {0x3E48, 0x8C71, WORD_LEN, 0},	// SAMP_RD2_SIG
   {0x3E4A, 0x00FF, WORD_LEN, 0},	// SAMP_RD2_SIG_BOOST
   {0x3E4C, 0x00FF, WORD_LEN, 0},	// SAMP_RD2_RST
   {0x3E4E, 0x00FF, WORD_LEN, 0},	// SAMP_RD2_RST_BOOST
   {0x3E50, 0xE38D, WORD_LEN, 0},	// SAMP_RST2_EN
   {0x3E52, 0x8B0A, WORD_LEN, 0},	// SAMP_RST2_BOOST
   {0x3E58, 0xEB0A, WORD_LEN, 0},	// SAMP_RST2_CLOOP_SH
   {0x3E5C, 0x0A00, WORD_LEN, 0},	// SAMP_SAMP2_SIG
   {0x3E5E, 0x00FF, WORD_LEN, 0},	// SAMP_SAMP2_RST
   {0x3E60, 0x00FF, WORD_LEN, 0},	// SAMP_PIX_CLAMP_EN
   {0x3E90, 0x3C01, WORD_LEN, 0},	// SAMP_PIX_PULLUP_EN
   {0x3E92, 0x00FF, WORD_LEN, 0},	// SAMP_PIX_PULLDOWN_EN_R
   {0x3E94, 0x00FF, WORD_LEN, 0},	// SAMP_PIX_PULLDOWN_EN_S
   {0x3E96, 0x3C00, WORD_LEN, 0},	// RST_ADDR_EN
   {0x3E98, 0x3C00, WORD_LEN, 0},	// RST_RST_EN
   {0x3E9A, 0x3C00, WORD_LEN, 0},	// RST_RST_BOOST
   {0x3E9C, 0xC0E0, WORD_LEN, 0},	// RST_TX_EN
   {0x3E9E, 0x00FF, WORD_LEN, 0},	// RST_TX_BOOST
   {0x3EA0, 0x0000, WORD_LEN, 0},	// RST_TX_CLOOP_SH
   {0x3EA6, 0x3C00, WORD_LEN, 0},	// RST_TX_BOOST_SEQ
   {0x3ED8, 0x3057, WORD_LEN, 0},	// RST_RST_CLOOP_SH
   {0x316C, 0xB44F, WORD_LEN, 0},	// RST_RST_BOOST_SEQ
   {0x316E, 0xCAFF, WORD_LEN, 0},	// RST_PIX_PULLUP_EN
   {0x3ED2, 0xEA0A, WORD_LEN, 0},	// DAC_LD_12_13
   {0x3ED4, 0x00A3, WORD_LEN, 0},	// DAC_TXLO
   {0x3EDC, 0x6020, WORD_LEN, 0},	// DAC_ECL
   {0x3EE6, 0xA541, WORD_LEN, 0},	// DAC_LD_6_7
   {0x31E0, 0x0000, WORD_LEN, 0},	// DAC_LD_8_9
   {0x3ED0, 0x2409, WORD_LEN, 0},	// DAC_LD_16_17
   {0x3EDE, 0x0A49, WORD_LEN, 0},	// DAC_LD_26_27
   {0x3EE0, 0x4910, WORD_LEN, 0},	// PIX_DEF_ID
   {0x3EE2, 0x09D2, WORD_LEN, 0},	// DAC_LD_4_5
   {0x30B6, 0x0006, WORD_LEN, 0},	// DAC_LD_18_19
   //{0x098E, 0x8404, WORD_LEN, 0},	// DAC_LD_20_21
   //{0x8404, 0x06, BYTE_LEN, 100},                                   
   {0x337C, 0x0006, WORD_LEN, 0},                                                   
                                         
    //[Step4-PGA]
	 {0x3640, 0x0510, WORD_LEN, 0},
	 {0x3642, 0x044E, WORD_LEN, 0},
	 {0x3644, 0x51F1, WORD_LEN, 0},
	 {0x3646, 0xE9CD, WORD_LEN, 0},
 	 {0x3648, 0xF7D0, WORD_LEN, 0},
	 {0x364A, 0x01B0, WORD_LEN, 0},
	 {0x364C, 0xE64D, WORD_LEN, 0},
	 {0x364E, 0x2AB1, WORD_LEN, 0},
	 {0x3650, 0x178F, WORD_LEN, 0},
 	 {0x3652, 0xA931, WORD_LEN, 0},
	 {0x3654, 0x0150, WORD_LEN, 0},
 	 {0x3656, 0x3CCD, WORD_LEN, 0},
	 {0x3658, 0x0D51, WORD_LEN, 0},
 	 {0x365A, 0x8B4D, WORD_LEN, 0},
	 {0x365C, 0xACF0, WORD_LEN, 0},
	 {0x365E, 0x0110, WORD_LEN, 0},
	 {0x3660, 0xB0AD, WORD_LEN, 0},
	 {0x3662, 0x4D11, WORD_LEN, 0},
	 {0x3664, 0x806D, WORD_LEN, 0},
	 {0x3666, 0x85B1, WORD_LEN, 0},
	 {0x3680, 0xB02C, WORD_LEN, 0},
	 {0x3682, 0xAC8E, WORD_LEN, 0},
	 {0x3684, 0xCDCD, WORD_LEN, 0},
	 {0x3686, 0x200D, WORD_LEN, 0},
	 {0x3688, 0x30B0, WORD_LEN, 0},
	 {0x368A, 0x0CED, WORD_LEN, 0},
	 {0x368C, 0x1E0E, WORD_LEN, 0},
	 {0x368E, 0x0C2E, WORD_LEN, 0},
	 {0x3690, 0x956F, WORD_LEN, 0},
	 {0x3692, 0xD28F, WORD_LEN, 0},
	 {0x3694, 0x966C, WORD_LEN, 0},
	 {0x3696, 0x454E, WORD_LEN, 0},
	 {0x3698, 0x7E6E, WORD_LEN, 0},
	 {0x369A, 0xDA4F, WORD_LEN, 0},
	 {0x369C, 0xE78F, WORD_LEN, 0},
	 {0x369E, 0x8CA7, WORD_LEN, 0},
	 {0x36A0, 0xC48E, WORD_LEN, 0},
	 {0x36A2, 0x400F, WORD_LEN, 0},
	 {0x36A4, 0x78ED, WORD_LEN, 0},
	 {0x36A6, 0xBD0F, WORD_LEN, 0},
	 {0x36C0, 0x6D91, WORD_LEN, 0},
	 {0x36C2, 0x79AD, WORD_LEN, 0},
	 {0x36C4, 0x44AB, WORD_LEN, 0},
	 {0x36C6, 0x35EE, WORD_LEN, 0},
	 {0x36C8, 0x8BD3, WORD_LEN, 0},
	 {0x36CA, 0x5051, WORD_LEN, 0},
	 {0x36CC, 0x8BAF, WORD_LEN, 0},
	 {0x36CE, 0xF510, WORD_LEN, 0},
	 {0x36D0, 0x248D, WORD_LEN, 0},
	 {0x36D2, 0xE6D1, WORD_LEN, 0},
	 {0x36D4, 0x37B1, WORD_LEN, 0},
	 {0x36D6, 0x1E70, WORD_LEN, 0},
	 {0x36D8, 0x2091, WORD_LEN, 0},
	 {0x36DA, 0x9F11, WORD_LEN, 0},
	 {0x36DC, 0xD493, WORD_LEN, 0},
	 {0x36DE, 0x7671, WORD_LEN, 0},
	 {0x36E0, 0x9290, WORD_LEN, 0},
	 {0x36E2, 0xA84E, WORD_LEN, 0},
	 {0x36E4, 0x2D30, WORD_LEN, 0},
	 {0x36E6, 0xD7B2, WORD_LEN, 0},
	 {0x3700, 0x25EF, WORD_LEN, 0},
	 {0x3702, 0x1FAD, WORD_LEN, 0},
	 {0x3704, 0xCEEF, WORD_LEN, 0},
	 {0x3706, 0x06F1, WORD_LEN, 0},
	 {0x3708, 0xC392, WORD_LEN, 0},
	 {0x370A, 0x26AF, WORD_LEN, 0},
	 {0x370C, 0xC84F, WORD_LEN, 0},
	 {0x370E, 0xD7F1, WORD_LEN, 0},
	 {0x3710, 0x5291, WORD_LEN, 0},
	 {0x3712, 0x0E71, WORD_LEN, 0},
	 {0x3714, 0xB76F, WORD_LEN, 0},
	 {0x3716, 0xFDCE, WORD_LEN, 0},
	 {0x3718, 0x0610, WORD_LEN, 0},
	 {0x371A, 0x0CB1, WORD_LEN, 0},
	 {0x371C, 0xAB91, WORD_LEN, 0},
	 {0x371E, 0xE928, WORD_LEN, 0},
	 {0x3720, 0xB06E, WORD_LEN, 0},
	 {0x3722, 0xB6B0, WORD_LEN, 0},
	 {0x3724, 0x3CD1, WORD_LEN, 0},
	 {0x3726, 0x9452, WORD_LEN, 0},
	 {0x3740, 0xEEEF, WORD_LEN, 0},
	 {0x3742, 0x984A, WORD_LEN, 0},
	 {0x3744, 0xCF34, WORD_LEN, 0},
	 {0x3746, 0x5190, WORD_LEN, 0},
	 {0x3748, 0x0E56, WORD_LEN, 0},
	 {0x374A, 0xD570, WORD_LEN, 0},
	 {0x374C, 0x376F, WORD_LEN, 0},
	 {0x374E, 0xC1B4, WORD_LEN, 0},
	 {0x3750, 0x2EB1, WORD_LEN, 0},
	 {0x3752, 0x2F96, WORD_LEN, 0},
	 {0x3754, 0xF76D, WORD_LEN, 0},
	 {0x3756, 0xE2F1, WORD_LEN, 0},
	 {0x3758, 0xFD34, WORD_LEN, 0},
	 {0x375A, 0x2893, WORD_LEN, 0},
	 {0x375C, 0x3516, WORD_LEN, 0},
	 {0x375E, 0x95B0, WORD_LEN, 0},
	 {0x3760, 0x2C0E, WORD_LEN, 0},
	 {0x3762, 0xB2D4, WORD_LEN, 0},
	 {0x3764, 0x02F3, WORD_LEN, 0},
	 {0x3766, 0x7635, WORD_LEN, 0},
	 {0x3782, 0x03C8, WORD_LEN, 0},
	 {0x3784, 0x0508, WORD_LEN, 0},
	 {0x3210, 0x49B8, WORD_LEN, 10},// COLOR_PIPELINE_CONTROL 

    //[Step5-AWB_CCM]
    {0x098E, 0xAC01 , WORD_LEN, 0},// LOGICAL_ADDRESS_ACCESS [AWB_MODE]
    {0xAC01, 0xAB 	, BYTE_LEN, 0},     // AWB_MODE
    {0xAC46, 0x01BC , WORD_LEN, 0},// AWB_LEFT_CCM_0
    {0xAC48, 0xFF18 , WORD_LEN, 0},// AWB_LEFT_CCM_1
    {0xAC4A, 0x002D , WORD_LEN, 0},// AWB_LEFT_CCM_2
    {0xAC4C, 0xFFE2 , WORD_LEN, 0},// AWB_LEFT_CCM_3
    {0xAC4E, 0x012B , WORD_LEN, 0},// AWB_LEFT_CCM_4   13924596670 wang
	{0xAC50, 0xFFF3 , WORD_LEN, 0},// AWB_LEFT_CCM_5
	{0xAC52, 0xFFD3 , WORD_LEN, 0},// AWB_LEFT_CCM_6
	{0xAC54, 0xFF2A , WORD_LEN, 0},// AWB_LEFT_CCM_7
	{0xAC56, 0x0204 , WORD_LEN, 0},// AWB_LEFT_CCM_8
    {0xAC58, 0x0130 , WORD_LEN, 0},// AWB_LEFT_CCM_R2BRATIO
	{0xAC56, 0x01F4 , WORD_LEN, 0},// AWB_LEFT_CCM_8
	{0xAC5E, 0xFF44 , WORD_LEN, 0},// AWB_RIGHT_CCM_1
	{0xAC60, 0xFFC8 , WORD_LEN, 0},// AWB_RIGHT_CCM_2
	{0xAC62, 0xFFC0 , WORD_LEN, 0},// AWB_RIGHT_CCM_3
	{0xAC64, 0x014C , WORD_LEN, 0},// AWB_RIGHT_CCM_4
	{0xAC66, 0xFFF4 , WORD_LEN, 0},// AWB_RIGHT_CCM_5
	{0xAC68, 0xFFF3 , WORD_LEN, 0},// AWB_RIGHT_CCM_6
	{0xAC6A, 0xFF5B , WORD_LEN, 0},// AWB_RIGHT_CCM_7
	{0xAC6C, 0x01B2 , WORD_LEN, 0},// AWB_RIGHT_CCM_8
    {0xAC6E, 0x0055 , WORD_LEN, 0},// AWB_RIGHT_CCM_R2BRATIO
    {0xB842, 0x0037 , WORD_LEN, 0},// STAT_AWB_GRAY_CHECKER_OFFSET_X
    {0xB844, 0x0044 , WORD_LEN, 0},// STAT_AWB_GRAY_CHECKER_OFFSET_Y
    {0x3240, 0x0025 , WORD_LEN, 0},// AWB_XY_SCALE
    {0x3242, 0x0000 , WORD_LEN, 0},// AWB_WEIGHT_R0
    {0x3244, 0x0000 , WORD_LEN, 0},// AWB_WEIGHT_R1
    {0x3246, 0x0000 , WORD_LEN, 0},// AWB_WEIGHT_R2
    {0x3248, 0x7F00 , WORD_LEN, 0},// AWB_WEIGHT_R3
    {0x324A, 0xA500 , WORD_LEN, 0},// AWB_WEIGHT_R4
    {0x324C, 0x1540 , WORD_LEN, 0},// AWB_WEIGHT_R5
    {0x324E, 0x01AC , WORD_LEN, 0},// AWB_WEIGHT_R6
    {0x3250, 0x003E , WORD_LEN, 0},// AWB_WEIGHT_R7
    //{0x098E, 0x8404 , WORD_LEN, 0},},// SEQ_CMD
    //{0x8404, 0x06 	, BYTE_LEN, 100// AWB_MIN_ACCEPTED_PRE_AWB_R2G_RATIO
    {0x098E, 0xAC3C , WORD_LEN, 0},
    {0xAC3C, 0x2E   , BYTE_LEN, 0},// AWB_MAX_ACCEPTED_PRE_AWB_R2G_RATIO
    {0xAC3D, 0x84 	, BYTE_LEN, 0},// AWB_MIN_ACCEPTED_PRE_AWB_B2G_RATIO
    {0xAC3E, 0x11 	, BYTE_LEN, 0},// AWB_MAX_ACCEPTED_PRE_AWB_B2G_RATIO
    {0xAC3F, 0x63 	, BYTE_LEN, 0},// AWB_RG_MIN
    {0xACB0, 0x2B 	, BYTE_LEN, 0},// AWB_RG_MAX
    {0xACB1, 0x84 	, BYTE_LEN, 0},// AWB_BG_MIN
    {0xACB4, 0x11 	, BYTE_LEN, 0},// AWB_BG_MAX
    {0xACB5, 0x63 	, BYTE_LEN, 0},
    //[Step6-CPIPE_Calibration]
    {0x098E, 0xD80F, WORD_LEN, 0},// LOGICAL_ADDRESS_ACCESS [JPEG_QSCALE_0]                          
    {0xD80F, 0x04 	, BYTE_LEN, 0},// JPEG_QSCALE_0                                                  
    {0xD810, 0x08 	, BYTE_LEN, 0},// JPEG_QSCALE_1                                                  
    {0xC8D2, 0x04 	, BYTE_LEN, 0},// CAM_OUTPUT_1_JPEG_QSCALE_0                                     
    {0xC8D3, 0x08 	, BYTE_LEN, 0},// CAM_OUTPUT_1_JPEG_QSCALE_1                                     
    {0xC8BC, 0x04 	, BYTE_LEN, 0},// CAM_OUTPUT_0_JPEG_QSCALE_0                                     
    {0xC8BD, 0x08 	, BYTE_LEN, 0},// CAM_OUTPUT_0_JPEG_QSCALE_1                                     
    {0x301A, 0x10F4, WORD_LEN, 0},// RESET_REGISTER                                                  
    {0x301E, 0x0000, WORD_LEN, 0},// DATA_PEDESTAL                                                   
    {0x301A, 0x10FC, WORD_LEN, 0},// RESET_REGISTER                                                  
    {0x098E, 0xDC33, WORD_LEN, 0},                                                                   
    {0xDC33, 0x00 	, BYTE_LEN, 0},// SYS_FIRST_BLACK_LEVEL                                          
    {0xDC35, 0x04 	, BYTE_LEN, 0},// SYS_UV_COLOR_BOOST                                             
    {0x326E, 0x0006, WORD_LEN, 0},// LOW_PASS_YUV_FILTER                                             
    {0x098E, 0xDC37, WORD_LEN, 0},                                                                   
    {0xDC37, 0x62 	, BYTE_LEN, 0},// SYS_BRIGHT_COLORKILL                                            
    {0x35A4, 0x0596, WORD_LEN, 0},// BRIGHT_COLOR_KILL_CONTROLS                                      
    {0x35A2, 0x0094, WORD_LEN, 0},// DARK_COLOR_KILL_CONTROLS                                        
    {0x098E, 0xDC36, WORD_LEN, 0},                                                                   
    {0xDC36, 0x23 	, BYTE_LEN, 0},// SYS_DARK_COLOR_KILL                                            
    //{0x098E, 0x8404, WORD_LEN, 0},                                                                   
    //{0x8404, 0x06 	, BYTE_LEN, 300},// SEQ_CMD                                                      
    {0x098E, 0xBC18, WORD_LEN, 0},                                                                   
    {0xBC18, 0x00 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_0                                      
    {0xBC19, 0x11 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_1                                      
    {0xBC1A, 0x23 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_2                                      
    {0xBC1B, 0x3F 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_3                                      
    {0xBC1C, 0x67 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_4                                      
    {0xBC1D, 0x85 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_5                                      
    {0xBC1E, 0x9B 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_6                                      
    {0xBC1F, 0xAD 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_7                                      
    {0xBC20, 0xBB 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_8                                      
    {0xBC21, 0xC7 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_9                                      
    {0xBC22, 0xD1 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_10                                     
    {0xBC23, 0xDA 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_11                                     
    {0xBC24, 0xE1 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_12                                     
    {0xBC25, 0xE8 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_13                                     
    {0xBC26, 0xEE 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_14                                     
    {0xBC27, 0xF3 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_15                                     
    {0xBC28, 0xF7 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_16                                     
    {0xBC29, 0xFB 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_17                                     
    {0xBC2A, 0xFF 	, BYTE_LEN, 0},// LL_GAMMA_CONTRAST_CURVE_18                                     
    {0xBC2B, 0x00 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_0                                       
    {0xBC2C, 0x11 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_1                                       
    {0xBC2D, 0x23 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_2                                       
    {0xBC2E, 0x3F 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_3                                       
    {0xBC2F, 0x67 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_4                                       
    {0xBC30, 0x85 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_5                                       
    {0xBC31, 0x9B 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_6                                       
    {0xBC32, 0xAD 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_7                                       
    {0xBC33, 0xBB 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_8                                       
    {0xBC34, 0xC7 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_9                                       
    {0xBC35, 0xD1 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_10                                      
    {0xBC36, 0xDA 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_11                                      
    {0xBC37, 0xE1 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_12                                      
    {0xBC38, 0xE8 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_13                                      
    {0xBC39, 0xEE 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_14                                      
    {0xBC3A, 0xF3 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_15                                      
    {0xBC3B, 0xF7 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_16                                      
    {0xBC3C, 0xFB 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_17                                      
    {0xBC3D, 0xFF 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_18                                      
    {0xBC3E, 0x00 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_0                                            
    {0xBC3F, 0x18 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_1                                            
    {0xBC40, 0x25 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_2                                            
    {0xBC41, 0x3A 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_3                                            
    {0xBC42, 0x59 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_4                                            
    {0xBC43, 0x70 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_5                                            
    {0xBC44, 0x81 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_6                                            
    {0xBC45, 0x90 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_7                                            
    {0xBC46, 0x9E 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_8                                            
    {0xBC47, 0xAB 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_9                                            
    {0xBC48, 0xB6 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_10                                           
    {0xBC49, 0xC1 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_11                                           
    {0xBC4A, 0xCB 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_12                                           
    {0xBC4B, 0xD5 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_13                                           
    {0xBC4C, 0xDE 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_14                                           
    {0xBC4D, 0xE7 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_15                                           
    {0xBC4E, 0xEF 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_16                                           
    {0xBC4F, 0xF7 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_17                                           
    {0xBC50, 0xFF 	, BYTE_LEN, 0},// LL_GAMMA_NR_CURVE_18                                           
    //{0x8404, 0x06 	, BYTE_LEN, 100},// SEQ_CMD                                                      
    {0xB801, 0xE0 	, BYTE_LEN, 0},// STAT_MODE                                                      
    {0xB862, 0x04 	, BYTE_LEN, 0},// STAT_BMTRACKING_SPEED                                          
    {0xB829, 0x02 	, BYTE_LEN, 0},// STAT_LL_BRIGHTNESS_METRIC_DIVISOR                              
    {0xB863, 0x02 	, BYTE_LEN, 0},// STAT_BM_MUL                                                    
    {0xB827, 0x0F 	, BYTE_LEN, 0},// STAT_AE_EV_SHIFT                                               
    {0xA409, 0x37 	, BYTE_LEN, 0},// AE_RULE_BASE_TARGET                                            
    {0xBC52, 0x00C8, WORD_LEN, 0},	// LL_START_BRIGHTNESS_METRIC                                    
    {0xBC54, 0x0A28, WORD_LEN, 0},	// LL_END_BRIGHTNESS_METRIC                                      
    {0xBC58, 0x00C8, WORD_LEN, 0},	// LL_START_GAIN_METRIC                                          
    {0xBC5A, 0x12C0, WORD_LEN, 0},	// LL_END_GAIN_METRIC                                            
    {0xBC5E, 0x00FA, WORD_LEN, 0},	// LL_START_APERTURE_GAIN_BM                                     
    {0xBC60, 0x0258, WORD_LEN, 0},	// LL_END_APERTURE_GAIN_BM                                       
    {0xBC66, 0x00FA, WORD_LEN, 0},	// LL_START_APERTURE_GM                                          
    {0xBC68, 0x0258, WORD_LEN, 0},	// LL_END_APERTURE_GM                                            
    {0xBC86, 0x00C8, WORD_LEN, 0},	// LL_START_FFNR_GM                                              
    {0xBC88, 0x0640, WORD_LEN, 0},	// LL_END_FFNR_GM                                                
    {0xBCBC, 0x0040, WORD_LEN, 0},	// LL_SFFB_START_GAIN                                            
    {0xBCBE, 0x01FC, WORD_LEN, 0},	// LL_SFFB_END_GAIN                                              
    {0xBCCC, 0x00C8, WORD_LEN, 0},	// LL_SFFB_START_MAX_GM                                          
    {0xBCCE, 0x0640, WORD_LEN, 0},	// LL_SFFB_END_MAX_GM                                            
    {0xBC90, 0x00C8, WORD_LEN, 0},	// LL_START_GRB_GM                                               
    {0xBC92, 0x0640, WORD_LEN, 0},	// LL_END_GRB_GM                                                 
    {0xBC0E, 0x0001, WORD_LEN, 0},	// LL_GAMMA_CURVE_ADJ_START_POS                                  
    {0xBC10, 0x0002, WORD_LEN, 0},	// LL_GAMMA_CURVE_ADJ_MID_POS                                    
    {0xBC12, 0x02BC, WORD_LEN, 0},	// LL_GAMMA_CURVE_ADJ_END_POS                                    
    {0xBCAA, 0x044C, WORD_LEN, 0},	// LL_CDC_THR_ADJ_START_POS                                      
    {0xBCAC, 0x00AF, WORD_LEN, 0},	// LL_CDC_THR_ADJ_MID_POS                                        
    {0xBCAE, 0x0009, WORD_LEN, 0},	// LL_CDC_THR_ADJ_END_POS                                        
    {0xBCD8, 0x00C8, WORD_LEN, 0},	// LL_PCR_START_BM                                               
    {0xBCDA, 0x0A28, WORD_LEN, 0},	// LL_PCR_END_BM                                                 
    {0x3380, 0x0504, WORD_LEN, 0},	// KERNEL_CONFIG                                                 
    {0x098E, 0xBC94, WORD_LEN, 0},                                                                   
    {0xBC94, 0x0C 	, BYTE_LEN, 0},// LL_GB_START_THRESHOLD_0                                        
    {0xBC95, 0x08 	, BYTE_LEN, 0},// LL_GB_START_THRESHOLD_1                                        
    {0xBC9C, 0x3C 	, BYTE_LEN, 0},// LL_GB_END_THRESHOLD_0                                          
    {0xBC9D, 0x28 	, BYTE_LEN, 0},// LL_GB_END_THRESHOLD_1                                          
    {0x33B0, 0x2A16, WORD_LEN, 0},	// FFNR_ALPHA_BET                                                
    {0x098E, 0xBC8A, WORD_LEN, 0},                                                                   
    {0xBC8A, 0x02 	, BYTE_LEN, 0},// LL_START_FF_MIX_THRESH_Y                                       
    {0xBC8B, 0x0F 	, BYTE_LEN, 0},// LL_END_FF_MIX_THRESH_Y                                         
    {0xBC8C, 0xFF 	, BYTE_LEN, 0},// LL_START_FF_MIX_THRESH_YGAIN                                   
    {0xBC8D, 0xFF 	, BYTE_LEN, 0},// LL_END_FF_MIX_THRESH_YGAIN                                     
    {0xBC8E, 0xFF 	, BYTE_LEN, 0},// LL_START_FF_MIX_THRESH_GAIN                                    
    {0xBC8F, 0x00 	, BYTE_LEN, 0},// LL_END_FF_MIX_THRESH_GAIN                                      
    {0xBCB2, 0x20 	, BYTE_LEN, 0},// LL_CDC_DARK_CLUS_SLOPE                                         
    {0xBCB3, 0x3A 	, BYTE_LEN, 0},// LL_CDC_DARK_CLUS_SATUR                                         
    {0xBCB4, 0x39 	, BYTE_LEN, 0},// LL_CDC_BRIGHT_CLUS_LO_LIGHT_SLOPE                              
    {0xBCB7, 0x39 	, BYTE_LEN, 0},// LL_CDC_BRIGHT_CLUS_LO_LIGHT_SATUR                              
    {0xBCB5, 0x20 	, BYTE_LEN, 0},// LL_CDC_BRIGHT_CLUS_MID_LIGHT_SLOPE                             
    {0xBCB8, 0x3A 	, BYTE_LEN, 0},// LL_CDC_BRIGHT_CLUS_MID_LIGHT_SATUR                             
    {0xBCB6, 0x80 	, BYTE_LEN, 0},// LL_CDC_BRIGHT_CLUS_HI_LIGHT_SLOPE                              
    {0xBCB9, 0x24 	, BYTE_LEN, 0},// LL_CDC_BRIGHT_CLUS_HI_LIGHT_SATUR                              
    {0xBCAA, 0x03E8, WORD_LEN, 0},	// LL_CDC_THR_ADJ_START_POS                                      
    {0xBCAC, 0x012C, WORD_LEN, 0},	// LL_CDC_THR_ADJ_MID_POS                                        
    {0xBCAE, 0x0009, WORD_LEN, 0},	// LL_CDC_THR_ADJ_END_POS                                        
    {0x33BA, 0x0084, WORD_LEN, 0},	// APEDGE_CONTROL                                                
    {0x33BE, 0x0000, WORD_LEN, 0},	// UA_KNEE_L                                                     
    {0x33C2, 0x8800, WORD_LEN, 0},	// UA_WEIGHTS                                                    
    {0x098E, 0xBC5E, WORD_LEN, 0},                                                                   
    {0xBC5E, 0x0154, WORD_LEN, 0},	// LL_START_APERTURE_GAIN_BM                                     
    {0xBC60, 0x0640, WORD_LEN, 0},	// LL_END_APERTURE_GAIN_BM                                       
    {0xBC62, 0x0E 	, BYTE_LEN, 0},// LL_START_APERTURE_KPGAIN                                       
    {0xBC63, 0x14 	, BYTE_LEN, 0},// LL_END_APERTURE_KPGAIN                                         
    {0xBC64, 0x0E 	, BYTE_LEN, 0},// LL_START_APERTURE_KNGAIN                                       
    {0xBC65, 0x14 	, BYTE_LEN, 0},// LL_END_APERTURE_KNGAIN                                         
    {0xBCE2, 0x0A 	, BYTE_LEN, 0},// LL_START_POS_KNEE                                              
    {0xBCE3, 0x2B 	, BYTE_LEN, 0},// LL_END_POS_KNEE                                                
    {0xBCE4, 0x0A 	, BYTE_LEN, 0},// LL_START_NEG_KNEE                                              
    {0xBCE5, 0x2B 	, BYTE_LEN, 0},// LL_END_NEG_KNEE                                                
    {0x3210, 0x49B8, WORD_LEN, 0},	// COLOR_PIPELINE_CONTROL                                        
    {0x098E, 0xBCC0, WORD_LEN, 0},                                                                   
    {0xBCC0, 0x1F 	, BYTE_LEN, 0},// LL_SFFB_RAMP_START                                             
    {0xBCC1, 0x03 	, BYTE_LEN, 0},// LL_SFFB_RAMP_STOP                                              
    {0xBCC2, 0x2C 	, BYTE_LEN, 0},// LL_SFFB_SLOPE_START                                            
    {0xBCC3, 0x10 	, BYTE_LEN, 0},// LL_SFFB_SLOPE_STOP                                             
    {0xBCC4, 0x07 	, BYTE_LEN, 0},// LL_SFFB_THSTART                                                
    {0xBCC5, 0x0B 	, BYTE_LEN, 0},// LL_SFFB_THSTOP                                                 
    {0xBCBA, 0x0009, WORD_LEN, 0},	// LL_SFFB_CONFIG                                                
    //{0x8404, 0x06 	, BYTE_LEN, 100},// SEQ_CMD
    //[Step7-CPIPE_Preference]
    {0x098E, 0x3C14, WORD_LEN, 0},  // LOGICAL_ADDRESS_ACCESS [LL_GAMMA_FADE_TO_BLACK_START_POS]
	{0xBC14, 0xFFFE, WORD_LEN, 0},  // LL_GAMMA_FADE_TO_BLACK_START_POS
	{0xBC16, 0xFFFE, WORD_LEN, 0},  // LL_GAMMA_FADE_TO_BLACK_END_POS
	{0xBC66, 0x0154, WORD_LEN, 0},  // LL_START_APERTURE_GM
	{0xBC68, 0x07D0, WORD_LEN, 0},// LL_END_APERTURE_GM
	{0xBC6A, 0x04  , BYTE_LEN, 0},// LL_START_APERTURE_INTEGER_GAIN
	{0xBC6B, 0x00  , BYTE_LEN, 0},// LL_END_APERTURE_INTEGER_GAIN
	{0xBC6C, 0x00  , BYTE_LEN, 0},// LL_START_APERTURE_EXP_GAIN
	{0xBC6D, 0x00  , BYTE_LEN, 0},// LL_END_APERTURE_EXP_GAIN
	{0xA81C, 0x0040, WORD_LEN, 0},// AE_TRACK_MIN_AGAIN
	{0xA81E, 0x012C, WORD_LEN, 0},// AE_TRACK_MAX_AGAIN
	{0xA820, 0x01FC, WORD_LEN, 0},// AE_TRACK_MAX_AGAIN
	{0xA822, 0x0080, WORD_LEN, 0},// AE_TRACK_MIN_DGAIN
	{0xA824, 0x0100, WORD_LEN, 0},// AE_TRACK_MAX_DGAIN
	{0xBC56, 0x64  , BYTE_LEN, 0},// LL_START_CCM_SATURATION
	{0xBC57, 0x1E  , BYTE_LEN, 0},// LL_END_CCM_SATURATION
	{0xBCDE, 0x03  , BYTE_LEN, 0},// LL_START_SYS_THRESHOLD
	{0xBCDF, 0x50  , BYTE_LEN, 0},// LL_STOP_SYS_THRESHOLD
	{0xBCE0, 0x08  , BYTE_LEN, 0},// LL_START_SYS_GAIN
	{0xBCE1, 0x03  , BYTE_LEN, 0},// LL_STOP_SYS_GAIN
	{0xBCD0, 0x000A, WORD_LEN, 0},// LL_SFFB_SOBEL_FLAT_START
	{0xBCD2, 0x00FE, WORD_LEN, 0},// LL_SFFB_SOBEL_FLAT_STOP
	{0xBCD4, 0x001E, WORD_LEN, 0},// LL_SFFB_SOBEL_SHARP_START
	{0xBCD6, 0x00FF, WORD_LEN, 0},// LL_SFFB_SOBEL_SHARP_STOP
	{0xBCC6, 0x00  , BYTE_LEN, 0},// LL_SFFB_SHARPENING_START
	{0xBCC7, 0x00  , BYTE_LEN, 0},// LL_SFFB_SHARPENING_STOP
	{0xBCC8, 0x20  , BYTE_LEN, 0},// LL_SFFB_FLATNESS_START
	{0xBCC9, 0x40  , BYTE_LEN, 0},// LL_SFFB_FLATNESS_STOP
	{0xBCCA, 0x04  , BYTE_LEN, 0},// LL_SFFB_TRANSITION_START
	{0xBCCB, 0x00  , BYTE_LEN, 0},// LL_SFFB_TRANSITION_STOP
	{0xBCE6, 0x03  , BYTE_LEN, 0},// LL_SFFB_ZERO_ENABLE
	{0xBCE6, 0x03  , BYTE_LEN, 0},// LL_SFFB_ZERO_ENABLE
	{0xA410, 0x04  , BYTE_LEN, 0},// AE_RULE_TARGET_AE_6
	{0xA411, 0x06  , BYTE_LEN, 0},// AE_RULE_TARGET_AE_7
	//{0x8404, 0x06  , BYTE_LEN, 100},// SEQ_CMD
	    //[Step8-Features]
	    {0x098E, 0xC8BC, WORD_LEN, 0},// LOGICAL_ADDRESS_ACCESS [CAM_OUTPUT_0_JPEG_QSCALE_0]
	{0xC8BC, 0x04  , BYTE_LEN, 0}, // CAM_OUTPUT_0_JPEG_QSCALE_0
	{0xC8BD, 0x0A  , BYTE_LEN, 0}, // CAM_OUTPUT_0_JPEG_QSCALE_1
	{0xC8D2, 0x04  , BYTE_LEN, 0}, // CAM_OUTPUT_1_JPEG_QSCALE_0
	{0xC8D3, 0x0A  , BYTE_LEN, 0}, // CAM_OUTPUT_1_JPEG_QSCALE_1
	{0xDC3A, 0x23  , BYTE_LEN, 0}, // SYS_SEPIA_CR
	{0xDC3B, 0xB2  , BYTE_LEN, 0}, // SYS_SEPIA_CB
    //end
    //{0x0018, 0x2008, WORD_LEN, 200},	// STANDBY_CONTROL_AND_STATUS
    {0x0018, 0x2008, WORD_LEN, 50},	// STANDBY_CONTROL_AND_STATUS
    // Frame rate control   Set Normal mode fixed 15fps
    {0x098E, 0xA818, WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS
    {0xA818, 0x04FE, WORD_LEN, 0},  //15fps
    {0xA81A, 0x04FE, WORD_LEN, 0},	

    //50HZ
    {0x098E, 0x8417, WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS
    {0x8417, 0x01 	, BYTE_LEN, 0},// LL_START_CCM_SATURATION
    {0xA004, 0x32 	, BYTE_LEN, 0},// LL_END_CCM_SATURATION


    // Set the AE target
    {0x098E, 0xA409, WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS
    //{0xA409, 0x50  , BYTE_LEN, 0},// AE_RULE_BASE_TARGET
    {0xA409, 0x48  , BYTE_LEN, 0},// AE_RULE_BASE_TARGET
    {0xA81E, 0x0080, WORD_LEN, 0},// AE_TRACK_MAX_AGAIN
    {0xA820, 0x0080, WORD_LEN, 0},// AE_TRACK_MAX_AGAIN
    // Set the saturation
    {0x098E, 0xBC56, WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS
    //{0xBC56, 0xB4  , BYTE_LEN, 0},// LL_START_CCM_SATURATION
    {0xBC56, 0xA8  , BYTE_LEN, 0},// LL_START_CCM_SATURATION
    {0xBC57, 0x32  , BYTE_LEN, 0},// LL_END_CCM_SATURATION
    //{0x8404, 0x06  , BYTE_LEN, 100}//Refresh Sequencer Mode = 6
    {0x8404, 0x06  , BYTE_LEN, 30}//Refresh Sequencer Mode = 6


};

static const struct mt9d112_i2c_reg_conf const preview_tbl[] = {

    {0x098E, 0x843C, WORD_LEN, 0},
    {0x843C, 0x01  , BYTE_LEN, 0},	
    {0x8404, 0x01  , BYTE_LEN, 30},
    {0x0016, 0x0447, WORD_LEN, 0}
};

static const struct mt9d112_i2c_reg_conf const snapshot_tbl[] = {

    {0x098E, 0x843C, WORD_LEN, 0},
    {0x843C, 0xFF  , BYTE_LEN, 0},	
    {0x8404, 0x02  , BYTE_LEN, 30}
};
static const struct mt9d112_i2c_reg_conf const awb_tbl[] = {

    {0x098E, 0x8410, WORD_LEN, 0},
    {0x8410, 0x02  , BYTE_LEN, 0},	
    {0x8418, 0x02  , BYTE_LEN, 0},    
    {0x8420, 0x02  , BYTE_LEN, 0}, 
    {0xAC44, 0x00  , BYTE_LEN, 0}, 
    {0xAC45, 0x7F  , BYTE_LEN, 0}, 
    {0x8404, 0x06  , BYTE_LEN, 30}
};

static const struct mt9d112_i2c_reg_conf const MWB_Cloudy_tbl[] = {
    {0x098E, 0x8410, WORD_LEN, 0},
    {0x8410, 0x01  , BYTE_LEN, 0},	
    {0x8418, 0x01  , BYTE_LEN, 0},    
    {0x8420, 0x01  , BYTE_LEN, 0}, 
    {0xAC44, 0x7F  , BYTE_LEN, 0}, 
    {0xAC45, 0x7F  , BYTE_LEN, 0}, 
    {0x8404, 0x06  , BYTE_LEN, 30},
    {0xAC04, 0x3D  , BYTE_LEN, 0},
    {0xAC05, 0x47  , BYTE_LEN, 0}
};
static const struct mt9d112_i2c_reg_conf const MWB_Day_light_tbl[] = {
	{0x098E, 0x8410, WORD_LEN, 0},
	{0x8410, 0x01  , BYTE_LEN, 0},	
	{0x8418, 0x01  , BYTE_LEN, 0},	  
	{0x8420, 0x01  , BYTE_LEN, 0}, 
	{0xAC44, 0x56  , BYTE_LEN, 0}, 
	{0xAC45, 0x56  , BYTE_LEN, 0}, 
	{0x8404, 0x06  , BYTE_LEN, 30},
	{0xAC04, 0x4C  , BYTE_LEN, 0},
	{0xAC05, 0x31  , BYTE_LEN, 0}
};
static const struct mt9d112_i2c_reg_conf const MWB_FLUORESCENT_tbl[] = {
    {0x098E, 0x8410, WORD_LEN, 0},
    {0x8410, 0x01  , BYTE_LEN, 0},	
    {0x8418, 0x01  , BYTE_LEN, 0},    
    {0x8420, 0x01  , BYTE_LEN, 0}, 
    {0xAC44, 0x79  , BYTE_LEN, 0}, 
    {0xAC45, 0x79  , BYTE_LEN, 0}, 
    {0x8404, 0x06  , BYTE_LEN, 30},
    {0xAC04, 0x41  , BYTE_LEN, 0},
    {0xAC05, 0x44  , BYTE_LEN, 0}
};
static const struct mt9d112_i2c_reg_conf const MWB_INCANDESCENT_tbl[] = {
	{0x098E, 0x8410, WORD_LEN, 0},
	{0x8410, 0x01  , BYTE_LEN, 0},	
	{0x8418, 0x01  , BYTE_LEN, 0},	  
	{0x8420, 0x01  , BYTE_LEN, 0}, 
	{0xAC44, 0x00  , BYTE_LEN, 0}, 
	{0xAC45, 0x00  , BYTE_LEN, 0}, 
	{0x8404, 0x06  , BYTE_LEN, 30},
	{0xAC04, 0x55  , BYTE_LEN, 0},
	{0xAC05, 0x2A  , BYTE_LEN, 0}
};
//effect
static const struct mt9d112_i2c_reg_conf const EFFECT_OFF_tbl[] = {
	{0x098E, 0xDC38, WORD_LEN, 0},
	{0xDC38, 0x00  , BYTE_LEN, 0},	
	{0xDC02, 0x302E, WORD_LEN, 0},	  
	{0x8404, 0x06  , BYTE_LEN, 30}, 
};

static const struct mt9d112_i2c_reg_conf const EFFECT_MONO_tbl[] = {
	{0x098E, 0xDC38, WORD_LEN, 0},
	{0xDC38, 0x01  , BYTE_LEN, 0},	
	{0xDC02, 0x306E, WORD_LEN, 0},	  
	{0x8404, 0x06  , BYTE_LEN, 30}, 
};

static const struct mt9d112_i2c_reg_conf const EFFECT_SEPIA_tbl[] = {
	{0x098E, 0xDC38, WORD_LEN, 0},
	{0xDC38, 0x02  , BYTE_LEN, 0},	
	{0xDC02, 0x306E, WORD_LEN, 0},	  
	{0x8404, 0x06  , BYTE_LEN, 30}, 
};

static const struct mt9d112_i2c_reg_conf const EFFECT_NEGATIVE_tbl[] = {
	{0x098E, 0xDC38, WORD_LEN, 0},
	{0xDC38, 0x03  , BYTE_LEN, 0},	
	{0xDC02, 0x306E, WORD_LEN, 0},	  
	{0x8404, 0x06  , BYTE_LEN, 30}, 
};

static const struct mt9d112_i2c_reg_conf const EFFECT_SOLARIZE_tbl[] = {
	{0x098E, 0xDC38, WORD_LEN, 0},
	{0xDC38, 0x04  , BYTE_LEN, 0},	
	{0xDC02, 0x306E, WORD_LEN, 0},	  
	{0x8404, 0x06  , BYTE_LEN, 30}, 
};

//AF
static const struct mt9d112_i2c_reg_conf const VCM_Enable_full_scan_tbl[] = {
	{0x098E, 0xC400, WORD_LEN, 0},
	{0xC400, 0x88  , BYTE_LEN, 0},	
	{0x8419, 0x05  , BYTE_LEN, 0},	  
	{0xC400, 0x08  , BYTE_LEN, 0}, 
	{0xB002, 0x0347, WORD_LEN, 0}, 
	{0xB004, 0x0042, WORD_LEN, 0}, 
	{0xC40C, 0x00F0, WORD_LEN, 0}, 
	{0xC40A, 0x0010, WORD_LEN, 0}, 
	{0xB018, 0x20  , BYTE_LEN, 0}, 
	{0xB019, 0x40  , BYTE_LEN, 0}, 
	{0xB01A, 0x5E  , BYTE_LEN, 0}, 
	{0xB01B, 0x7C  , BYTE_LEN, 0}, 
	{0xB01C, 0x98  , BYTE_LEN, 0}, 
	{0xB01D, 0xB3  , BYTE_LEN, 0}, 
	{0xB01E, 0xCD  , BYTE_LEN, 0}, 
	{0xB01F, 0xE5  , BYTE_LEN, 0}, 
	{0xB020, 0xFB  , BYTE_LEN, 0}, 
	{0xB012, 0x09  , BYTE_LEN, 0}, 
	{0xB013, 0x77  , BYTE_LEN, 0}, 
	{0xB014, 0x05  , BYTE_LEN, 0}, 
	{0x098E, 0x8404, WORD_LEN, 0}, 
	{0x8404, 0x05  , BYTE_LEN, 30}, 
};

static const struct mt9d112_i2c_reg_conf const AF_Trigger_tbl[] = {
	{0x098E, 0xB006, WORD_LEN, 0},
	{0xB006, 0x01  , BYTE_LEN, 0},	
};

static const struct mt9d112_i2c_reg_conf const VCM_Enable_Continue_scan_tbl[] = {
	{0x098E, 0xC400, WORD_LEN, 0},
	{0xC400, 0x88  , BYTE_LEN, 0},	
	{0x8419, 0x03  , BYTE_LEN, 0},	  
	{0xC400, 0x08  , BYTE_LEN, 0}, 
	{0xB002, 0x0347, WORD_LEN, 0}, 
	{0xB004, 0x0042, WORD_LEN, 0}, 
	{0xC40C, 0x00F0, WORD_LEN, 0}, 
	{0xC40A, 0x0010, WORD_LEN, 0}, 
	{0xB018, 0x20  , BYTE_LEN, 0}, 
	{0xB019, 0x40  , BYTE_LEN, 0}, 
	{0xB01A, 0x5E  , BYTE_LEN, 0}, 
	{0xB01B, 0x7C  , BYTE_LEN, 0}, 
	{0xB01C, 0x98  , BYTE_LEN, 0}, 
	{0xB01D, 0xB3  , BYTE_LEN, 0}, 
	{0xB01E, 0xCD  , BYTE_LEN, 0}, 
	{0xB01F, 0xE5  , BYTE_LEN, 0}, 
	{0xB020, 0xFB  , BYTE_LEN, 0}, 
	{0xB012, 0x09  , BYTE_LEN, 0}, 
	{0xB013, 0x77  , BYTE_LEN, 0}, 
	{0xB014, 0x05  , BYTE_LEN, 0}, 
	{0x098E, 0x8404, WORD_LEN, 0}, 
	{0x8404, 0x05  , BYTE_LEN, 30}, 
};

//scene
static const struct mt9d112_i2c_reg_conf const SCENE_AUTO_tbl[] = {
	{0x098E, 0xA818, WORD_LEN, 0},                           
	{0xA818, 0x04FE, WORD_LEN, 0},  //15fps                     
	{0xA81A, 0x04FE, WORD_LEN, 0},	                          
	{0x8404, 0x06  , BYTE_LEN, 30},	                          

};

static const struct mt9d112_i2c_reg_conf const SCENE_NIGHT_tbl[] = {
	{0x098E, 0xA818, WORD_LEN, 0}, 
	{0xA818, 0x10A4, WORD_LEN, 0},
	{0xA81A, 0x10A4, WORD_LEN, 0},	
	{0x8404, 0x06  , BYTE_LEN, 30},	
};

struct mt9d112_reg mt9d112_regs = {
	.init_tbl = init_tbl,
	.init_tbl_size = ARRAY_SIZE(init_tbl),
	.preview_tbl = preview_tbl,
	.preview_tbl_size = ARRAY_SIZE(preview_tbl),
	.snapshot_tbl = snapshot_tbl,
	.snapshot_tbl_size = ARRAY_SIZE(snapshot_tbl),
	.awb_tbl = awb_tbl,//add by lijiankun 2010-9-3 auto whitebalance
	.awb_tbl_size = ARRAY_SIZE(awb_tbl),
	.MWB_Cloudy_tbl = MWB_Cloudy_tbl,//add by lijiankun 2010-9-3 cloudy
	.MWB_Cloudy_tbl_size = ARRAY_SIZE(MWB_Cloudy_tbl),
	.MWB_Day_light_tbl = MWB_Day_light_tbl,//add by lijiankun 2010-9-3 Day_light
	.MWB_Day_light_tbl_size = ARRAY_SIZE(MWB_Day_light_tbl),
	.MWB_FLUORESCENT_tbl = MWB_FLUORESCENT_tbl,//add by lijiankun 2010-9-3 FLUORESCENT
	.MWB_FLUORESCENT_tbl_size = ARRAY_SIZE(MWB_FLUORESCENT_tbl),
	.MWB_INCANDESCENT_tbl = MWB_INCANDESCENT_tbl,//add by lijiankun 2010-9-3 INCANDESCENT
	.MWB_INCANDESCENT_tbl_size = ARRAY_SIZE(MWB_INCANDESCENT_tbl),

	.EFFECT_OFF_tbl = EFFECT_OFF_tbl,//add by lijiankun 2010-9-7 EFFECT_OFF
	.EFFECT_OFF_tbl_size = ARRAY_SIZE(EFFECT_OFF_tbl),
	.EFFECT_MONO_tbl = EFFECT_MONO_tbl,//add by lijiankun 2010-9-7 EFFECT_MONO
	.EFFECT_MONO_tbl_size = ARRAY_SIZE(EFFECT_MONO_tbl),
	.EFFECT_SEPIA_tbl = EFFECT_SEPIA_tbl,//add by lijiankun 2010-9-7 EFFECT_SEPIA
	.EFFECT_SEPIA_tbl_size = ARRAY_SIZE(EFFECT_SEPIA_tbl),
	.EFFECT_NEGATIVE_tbl = EFFECT_NEGATIVE_tbl,//add by lijiankun 2010-9-7 EFFECT_NEGATIVE
	.EFFECT_NEGATIVE_tbl_size = ARRAY_SIZE(EFFECT_NEGATIVE_tbl),
	.EFFECT_SOLARIZE_tbl = EFFECT_SOLARIZE_tbl,//add by lijiankun 2010-9-7 EFFECT_SOLARIZE
	.EFFECT_SOLARIZE_tbl_size = ARRAY_SIZE(EFFECT_SOLARIZE_tbl),

	.VCM_Enable_full_scan_tbl = VCM_Enable_full_scan_tbl,//add by lijiankun 2010-9-9 AF
	.VCM_Enable_full_scan_tbl_size = ARRAY_SIZE(VCM_Enable_full_scan_tbl),
	.AF_Trigger_tbl = AF_Trigger_tbl,//add by lijiankun 2010-9-9 AF
	.AF_Trigger_tbl_size = ARRAY_SIZE(AF_Trigger_tbl),
	.VCM_Enable_Continue_scan_tbl = VCM_Enable_Continue_scan_tbl,//add by lijiankun 2010-9-9 AF
	.VCM_Enable_Continue_scan_tbl_size = ARRAY_SIZE(VCM_Enable_Continue_scan_tbl),

	.SCENE_AUTO_tbl = SCENE_AUTO_tbl,//add by lijiankun 2010-9-9 scene
	.SCENE_AUTO_tbl_size = ARRAY_SIZE(SCENE_AUTO_tbl),
	.SCENE_NIGHT_tbl = SCENE_NIGHT_tbl,//add by lijiankun 2010-9-9 scene
	.SCENE_NIGHT_tbl_size = ARRAY_SIZE(SCENE_NIGHT_tbl),


};



