/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/delay.h>
#include <linux/charge_pump.h>
#include <mach/gpio.h>
#include "msm_fb.h"

#ifdef CONFIG_FB_MSM_LCDC_LG4573_WVGA_PANEL
#include <linux/syscalls.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#endif

#define LCD_MSG 0
#define PARA_NUM(x) (sizeof(x)/sizeof(uint16)/2)

#define VCOM_PATH		"persist/lcdvcom"
#define VCOM_REG		0xC5
#define VCOM_DEFAULT	0x6E

static int lcdc_ili9325sim_panel_off(struct platform_device *pdev);

#ifdef CONFIG_FB_MSM_LCDC_LG4573_WVGA_PANEL
static int lcdc_lg4573_get_vcom(unsigned int *pvcom);
static int lcdc_lg4573_set_vcom(unsigned int *pvcom);
static int lcdc_lg4573_init_vcom(unsigned int *pvcom);
static unsigned char vcom8 = VCOM_DEFAULT;
#endif

static int spi_cs;
static int spi_sclk;
static int spi_sdo;
static int spi_sdi;
static int spi_rst;

struct ili9325sim_state_type{
	boolean disp_initialized;
	boolean display_on;
	boolean disp_powered_up;
};

static struct ili9325sim_state_type ili9325sim_state = { 0 };
static struct msm_panel_common_pdata *lcdc_ili9325sim_pdata;
int lcdc_ili9325sim_panel_on(struct platform_device *pdev);


/*===========================================================================

                            FUNCTION PROTOTYPES

===========================================================================*/
/*===========================================================================

FUNCTION      ILI9325SIM_BUSY_WAIT

DESCRIPTION
  Busy wait for specified no. of milliseconds

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void ili9325sim_busy_wait(unsigned long ms) 
{
	//ms >>= 1;	
	mdelay(ms);
}

/*===========================================================================

FUNCTION      SPI_DELAY

DESCRIPTION
  Delay for 33 us

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
static void spi_delay(unsigned long delay) 
{
	//delay /= 5;	
	udelay(delay);
}

static __inline void LCD_CS(unsigned char Lv) 
{
	gpio_set_value(spi_cs, Lv);
}

static __inline void LCD_SCL(unsigned char Lv) 
{
	gpio_set_value(spi_sclk, Lv);
}

static __inline unsigned long LCD_SO(void)
{
	unsigned long level;
	level = gpio_get_value(spi_sdo);

	return level;
}

static __inline void LCD_SI(unsigned char Lv) 
{
	gpio_set_value(spi_sdi, Lv);
}

static __inline void LCD_RST(unsigned char Lv) 
{
	gpio_set_value(spi_rst, Lv);
}

static __inline void SendByte( unsigned char Byte)
{
    unsigned long   Shift;
    unsigned long   Cnt;

    Shift = 0x80;

    for (Cnt = 0; Cnt < 8; Cnt++)
    {
        LCD_SCL( 0);
        if (Byte&Shift)
        {
            LCD_SI( 1);
        }
        else
        {
            LCD_SI( 0);
        }
        Shift = Shift >> 1;
        spi_delay( 1);
        LCD_SCL( 1);
        spi_delay( 1);
    }
}

static void SendStartByte( unsigned char RW, unsigned char RS)
{
#define LCD_DEV_ID(v) (v<<2)
#define LCD_RS(v)     (v<<1)
#define LCD_RW(v)     (v<<0)

    unsigned char StartByte = 0x70;
    StartByte |= (LCD_DEV_ID(0) | LCD_RS(RS) | LCD_RW(RW)); 
    SendByte( StartByte);
    
}

static void SendIndex( unsigned short Index)
{
    SendStartByte( 0, 0);
    SendByte( (Index>>8));
    SendByte( (Index&0xFF));
}

static void SendData( unsigned short Data)
{
    SendStartByte( 0, 1);
    SendByte( (Data>>8));
    SendByte( (Data&0xFF));
}

static void SetIndex( unsigned char Index)
{
    LCD_CS( 0);
    spi_delay( 1 );
    SendStartByte( 0, 0);
    SendByte( Index);
    spi_delay( 1 );
    LCD_CS( 1);
}

static void SetData( unsigned char Data)
{
    LCD_CS( 0);
    spi_delay( 1);
    SendStartByte( 0, 1);
    SendByte( Data);
    spi_delay( 1);
    LCD_CS( 1);
}

static void WriteByte( unsigned short Value)
{
	unsigned long	Shift;
	unsigned long	Cnt;
	unsigned char	Byte;
	unsigned char	A0;

	spi_delay( 5);

	A0 = Value >> 8;
	Byte = Value & 0xFF;

	LCD_SCL( 0);
	if (A0)
	{
		LCD_SI( 1);
	}
	else
	{
		LCD_SI(0);
	}
	spi_delay( 5);
	LCD_SCL( 1);
	spi_delay( 5);
	
	Shift = 0x80;
	for (Cnt = 0; Cnt < 8; Cnt++)
	{
		LCD_SCL(0);
		if (Byte&Shift)
		{
			LCD_SI(1);
		}
		else
		{
			LCD_SI( 0);
		}
		Shift = Shift >> 1;
		spi_delay( 5);
		LCD_SCL(1);
		spi_delay(5);
	}
	LCD_SCL(0);	  
	spi_delay( 5);
}


static void LcdSend(unsigned short Value)
{
  	
    LCD_CS(0);
    spi_delay(5);
    WriteByte( Value);
    spi_delay( 5);
    LCD_CS( 1);
    spi_delay(5);
}

static __inline void GetByte(unsigned char *pByte)
{
    unsigned long   Shift;
    unsigned long   Cnt;
    unsigned char    Byte = 0;
    
    Shift = 0x80;

    for (Cnt = 0; Cnt < 8; Cnt++)
    {
        LCD_SCL( 0);
        spi_delay( 1);
        if (LCD_SO())
        {
            Byte |= Shift;
        }
        LCD_SCL( 1 );
        spi_delay( 1);
        Shift = Shift >> 1;
    }
    
    *pByte = Byte;
}


static void GetData( unsigned char *pData)
{
    LCD_CS( 0);
    spi_delay( 1);

    SendStartByte(1, 1);
    GetByte(pData); //dumy byte
    GetByte( pData);

    spi_delay(1);
    LCD_CS(1);
}


/*===========================================================================

FUNCTION      SPI_INIT

DESCRIPTION
  Initialize SPI interface

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void spi_init(void)
{
	spi_sclk = 132;
	spi_cs = 131;
	spi_sdo = 103;
	spi_sdi = 102;
	spi_rst = 88;
}

static void ili9325sim_disp_powerup(void)
{
	if (!ili9325sim_state.disp_powered_up && !ili9325sim_state.display_on) {
		/* Reset the hardware first */
		/* Include DAC power up implementation here */
	      ili9325sim_state.disp_powered_up = TRUE;
	}
}

static void ili9325sim_init(void)
{
	unsigned char pid;
	printk("+ili9325sim_init\r\n");

	LCD_CS(1);
	LCD_SCL(0);
	LCD_SI(1);
	LCD_RST(1);

	// Reset LCD
	LCD_RST( 0);
	mdelay(15);
	LCD_RST(1);
	mdelay(50);

	SetIndex(0xa1);//read id
	GetData(&pid);
	printk("+InitLcd:ID[%x]\r\n",pid);

	//lcdc_lg4573_init_vcom(&vcom);

	//Display Mode 
	SetIndex(0xc0);
	SetData(0x01);
	SetData(0x18);//18

	SetIndex(0x20);//exit_invert_mode
	SetIndex(0x29);//set_display_on
	SetIndex(0x3A);//set_pixel_format 
	SetData(0x60);//70   0X60 26k

	SetIndex(0xB1);//RGB Interface Setting
	//SetData(0x06);
	SetData(0x0E);
	SetData(0x1E);
	SetData(0x0C);

	SetIndex(0xB2);//Panel Characteristics Setting
	SetData(0x10);//480 pixels
	SetData(0xC8);//800 pixels

	SetIndex(0xB3);//Panel Drive Setting    Set the inversion mode
	//SetData(pGpio,0x02);//1-dot inversion 0x01
	SetData(0x00);//1-dot inversion 0x01

	SetIndex(0xB4);//Display Mode Control
	SetData(0x04);//Dither disable.

	SetIndex(0xB5);//Display Mode and Frame Memory Write Mode Setting
	SetData(0x10);
	SetData(0x30);
	SetData(0x30);
	SetData(0x00);
	SetData(0x00);

	SetIndex(0xB6);//Display Control 2 ( GIP Specific )
	SetData(0x01);
	SetData(0x18);
	SetData(0x02);
	SetData(0x40);
	SetData(0x10);
	SetData(0x40);


	SetIndex(0xC3); 
	SetData(0x03);
	SetData(0x04);
	SetData(0x02);
	SetData(0x02);
	SetData(0x03);

	//Sleep(40);        // 20100413 deleted by LGE

	SetIndex(0xC4);//VDD Regulator setting
	SetData(0x11);
	SetData(0x23);//GDC AP
	SetData(0x0c);//VRH1  Vreg1out=1.533xVCI(10)
	SetData(0x0c);//VRH2  Vreg2out=-1.533xVCI(10)

	SetData(0x01);

	SetData(0x6D);// 20100413 changed from 3F to 6D by LGE

	//Sleep(200);       // 20100413 deleted by LGE

	SetIndex(0xC5);
	SetData(vcom8);

	//Sleep(100);       // 20100413 deleted by LGE

	SetIndex(0xC6);
	SetData(0x25);//VCI 23
	SetData(0x50);//RESET RCO 53
	SetData(0x00);//SBC GBC

	//Sleep(100);       // 20100413 deleted by LGE

	//GAMMA SETTING
	SetIndex(0xD0);
	SetData(0x00);
	SetData(0x44);
	SetData(0x44);
	SetData(0x16);
	SetData(0x15);
	SetData(0x03);
	SetData(0x61);
	SetData(0x16);
	SetData(0x33);

	SetIndex(0xD1);
	SetData(0x00);
	SetData(0x44);
	SetData(0x44);
	SetData(0x16);
	SetData(0x15);
	SetData(0x03);
	SetData(0x61);
	SetData(0x16);
	SetData(0x33);

	SetIndex(0xD2);
	SetData(0x00);
	SetData(0x44);
	SetData(0x44);
	SetData(0x16);
	SetData(0x15);
	SetData(0x03);
	SetData(0x61);
	SetData(0x16);
	SetData(0x33);

	SetIndex(0xD3);
	SetData(0x00);
	SetData(0x44);
	SetData(0x44);
	SetData(0x16);
	SetData(0x15);
	SetData(0x03);
	SetData(0x61);
	SetData(0x16);
	SetData(0x33);

	SetIndex(0xD4);
	SetData(0x00);
	SetData(0x44);
	SetData(0x44);
	SetData(0x16);
	SetData(0x15);
	SetData(0x03);
	SetData(0x61);
	SetData(0x16);
	SetData(0x33);

	SetIndex(0xD5);
	SetData(0x00);
	SetData(0x44);
	SetData(0x44);
	SetData(0x16);
	SetData(0x15);
	SetData(0x03);
	SetData(0x61);
	SetData(0x16);
	SetData(0x33);
	//DISPLAY ON
	SetIndex(0x11);

	mdelay(200);             // 20100413 added by LGE

	printk("-lg4573_init\r\n");
}

static void ili9325sim_disp_on(void)
{
	if (ili9325sim_state.disp_powered_up && !ili9325sim_state.display_on) {
		ili9325sim_init();
		ili9325sim_state.display_on = TRUE;
	}
}

int lcdc_ili9325sim_panel_on(struct platform_device *pdev)
{

	
	if (!ili9325sim_state.disp_initialized) {
		/* Configure reset GPIO that drives DAC */
		lcdc_ili9325sim_pdata->panel_config_gpio(1);
		spi_init();	/* LCD needs SPI */
		ili9325sim_disp_powerup();
		ili9325sim_disp_on();
		ili9325sim_state.disp_initialized = TRUE;
	}
	return 0;
}

static int lcdc_ili9325sim_panel_off(struct platform_device *pdev)
{
	if (ili9325sim_state.disp_powered_up && ili9325sim_state.display_on) {
		SetIndex(0x10);
		mdelay(120);
		SetIndex(0xC1);
		SetData(0x01);
		mdelay(120);
		LCD_RST(0);

		lcdc_ili9325sim_pdata->panel_config_gpio(0);
		ili9325sim_state.display_on = FALSE;
		ili9325sim_state.disp_initialized = FALSE;
	}
	return 0;
}

static void lcdc_ili9325sim_set_backlight(struct msm_fb_data_type *mfd)
{
	static bool bl_on = false;
	bool panel_on = mfd->panel_power_on;
	int bl_lv = mfd->bl_level;

	printk("bl_lv %d bl_on %d panel_on %d\n", bl_lv, bl_on, panel_on);
	if (bl_lv && panel_on) {
		if (!bl_on) {
			msleep(200);
			ChargePumpPowerOn();
			bl_on = true;
		}
		ChargePumpSetDispLightLv(bl_lv);
		// ChargePumpTest();
	} else {
		ChargePumpPowerOff();
		bl_on = false;
	}
}

#ifdef CONFIG_FB_MSM_LCDC_LG4573_WVGA_PANEL
static int lcdc_lg4573_init_vcom(unsigned int *pvcom)
{
	int fd;
	int rc = -1;
	unsigned int vcom;
	mm_segment_t old_fs;
	char *filename = VCOM_PATH;

	old_fs = get_fs();
	printk("lcdc_lg4573_init_vcom\n");

	set_fs (KERNEL_DS);
	
	fd = sys_open(filename, O_RDONLY , 0);
	if (fd < 0) {
		printk("Can not open %s fd = 0x%x\n", filename, fd);
		rc = -ENOENT;
		goto error1;
	}
	
	sys_read(fd, &vcom, sizeof(vcom));
	printk("lcdc_lg4573_init_vcom = 0x%x\n",vcom);
	vcom8 = vcom & 0xFF;
	
	if(vcom8 > 0x7F || vcom8< 0x30)
	{
		printk("lcdc_lg4573_init_vcom failed 0x%x\n", sys_unlink(filename));
	}
	else
	{
		SetIndex(VCOM_REG);
		SetData(vcom8);
		*pvcom = vcom8;
	}

	sys_close(fd);
error1:
	set_fs (old_fs);
	return rc;
}

static int lcdc_lg4573_set_vcom(unsigned int *pvcom)
{
	printk("lcdc_lg4573_set_vcom %d\n",*pvcom);
   	vcom8 = *pvcom;
	if(vcom8 > 0x7F || vcom8< 0x30)
	{
		printk("lcdc_lg4573_set_vcom invalid parameter\n ");
	}
	else
	{
		SetIndex(VCOM_REG);
		SetData(vcom8);
	}
	return 0;
}

static int lcdc_lg4573_get_vcom(unsigned int *pvcom)
{
	SetIndex(VCOM_REG);
	GetData(&vcom8);
	*pvcom = vcom8;
	printk("lcdc_lg4573_get_vcom 0x%x\n",*pvcom);
	return 0;
}
#endif

static int __init ili9325sim_probe(struct platform_device *pdev)
{	
	

	if (pdev->id == 0) {
		lcdc_ili9325sim_pdata = pdev->dev.platform_data;
		return 0;
	}
	msm_fb_add_device(pdev);
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = ili9325sim_probe,
	.driver = {
		.name   = "ili9325sim_qvga",
	},
};


static struct msm_fb_panel_data ili9325sim_panel_data = {
	.on = lcdc_ili9325sim_panel_on,
	.off = lcdc_ili9325sim_panel_off,
	.set_backlight = lcdc_ili9325sim_set_backlight,
#ifdef CONFIG_FB_MSM_LCDC_LG4573_WVGA_PANEL
	.set_vcom = lcdc_lg4573_set_vcom,
	.get_vcom = lcdc_lg4573_get_vcom,
	.init_vcom = lcdc_lg4573_init_vcom,
#endif
};

static struct platform_device this_device = {
	.name   = "ili9325sim_qvga",
	.id	= 1,
	.dev	= {
		.platform_data = &ili9325sim_panel_data,
	}
};

static int __init lcdc_ili9325sim_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

	ret = platform_driver_register(&this_driver);
	if (ret)
		return ret;
	printk("lcdc_ili9325sim_panel_init++\n");

	pinfo = &ili9325sim_panel_data.panel_info;
	pinfo->xres = 480;
	pinfo->yres = 800;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
#ifdef CONFIG_FB_MSM_LCDC_LG4573_WVGA_PANEL_18BIT
	pinfo->bpp = 18;
#else
	pinfo->bpp = 16;
#endif
	pinfo->fb_num = 2;
	pinfo->clk_rate = 18000000;
	pinfo->bl_max = 100;
	pinfo->bl_min = 1;

	pinfo->lcdc.h_back_porch = 27;
	pinfo->lcdc.h_front_porch = 8;
	pinfo->lcdc.h_pulse_width = 1;
	pinfo->lcdc.v_back_porch = 9;
	pinfo->lcdc.v_front_porch = 4;
	pinfo->lcdc.v_pulse_width = 1;
	pinfo->lcdc.border_clr = 0;     /* blk */
	pinfo->lcdc.underflow_clr = 0;       /* blk */
	pinfo->lcdc.hsync_skew = 0;
	printk("lcdc_ili9325sim_panel_init--\n");

	ret = platform_device_register(&this_device);
	if (ret)
		platform_driver_unregister(&this_driver);

	return ret;
}

module_init(lcdc_ili9325sim_panel_init);

