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

#define LCD_MSG 0
#define PARA_NUM(x) (sizeof(x)/sizeof(uint16)/2)

static int lcdc_ili9325sim_panel_off(struct platform_device *pdev);

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

static void SetData( unsigned short Data)
{
    LCD_CS( 0);
    spi_delay( 1);
    SendStartByte( 0, 1);
    SendByte( Data);
    spi_delay( 1);
    LCD_CS( 1);
}

static void WriteByte( unsigned char Value)
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


static void LcdSend(unsigned char Value)
{
  	
    LCD_CS(0);
    spi_delay(5);
    WriteByte( Value);
    spi_delay( 5);
    LCD_CS( 1);
    spi_delay(5);
}


static void LCD_ILI9481_CMD(unsigned char cmd)
{
	LCD_CS(0);

	LCD_SCL(0);
	spi_delay(1);
	LCD_SI(0);
	LCD_SCL(1);

	spi_delay(1);
	SendByte(cmd);
	spi_delay(1);

	LCD_CS( 1);

}

static void LCD_ILI9481_INDEX(unsigned char index)
{

	LCD_CS(0);

	LCD_SCL(0);
	spi_delay(1);
	LCD_SI(1);
	LCD_SCL(1);

	spi_delay(1);
	SendByte(index);
	spi_delay(1);

	LCD_CS( 1);

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
	unsigned int i;
	unsigned char pid;
	printk("+ili9481_init\r\n");

	LCD_CS(1);
	LCD_SCL(1);
	LCD_SI(1);
	LCD_RST(1);
	
	mdelay(1);
	// Reset LCD
	LCD_RST(0);
	mdelay(10);
	LCD_RST(1);
	mdelay(100);

	//************* Start Initial Sequence **********//
	LCD_ILI9481_CMD(0x11);

	LCD_ILI9481_CMD(0xD0);
	LCD_ILI9481_INDEX(0x07);
	LCD_ILI9481_INDEX(0x41);
	LCD_ILI9481_INDEX(0x15);

	LCD_ILI9481_CMD(0xD1);
	LCD_ILI9481_INDEX(0x00);
	LCD_ILI9481_INDEX(0x13);
	LCD_ILI9481_INDEX(0x11);

	LCD_ILI9481_CMD(0xD2);
	LCD_ILI9481_INDEX(0x01);
	LCD_ILI9481_INDEX(0x11);

	LCD_ILI9481_CMD(0xC0);
	LCD_ILI9481_INDEX(0x10);
	LCD_ILI9481_INDEX(0x3B);
	LCD_ILI9481_INDEX(0x00);
	LCD_ILI9481_INDEX(0x02);
	LCD_ILI9481_INDEX(0x11);
	LCD_ILI9481_INDEX(0x00);
	
	LCD_ILI9481_CMD(0xC5);
	LCD_ILI9481_INDEX(0x03);
	
	LCD_ILI9481_CMD(0xB4);
	LCD_ILI9481_INDEX(0x10);//rgb interface

	LCD_ILI9481_CMD(0xB3);
	LCD_ILI9481_INDEX(0x02);
	LCD_ILI9481_INDEX(0x00);
	LCD_ILI9481_INDEX(0x00);
	LCD_ILI9481_INDEX(0x20);

//	LCD_ILI9481_CMD(0xC6);
//	LCD_ILI9481_INDEX(0x80);


	LCD_ILI9481_CMD(0x3A);
	LCD_ILI9481_INDEX(0x66);


	LCD_ILI9481_CMD(0xC8);
	LCD_ILI9481_INDEX(0x00);
	LCD_ILI9481_INDEX(0x66);
	LCD_ILI9481_INDEX(0x15);
	LCD_ILI9481_INDEX(0x24);
	LCD_ILI9481_INDEX(0x00);
	LCD_ILI9481_INDEX(0x08);
	LCD_ILI9481_INDEX(0x26);
	LCD_ILI9481_INDEX(0x11);
	LCD_ILI9481_INDEX(0x77);
	LCD_ILI9481_INDEX(0x42);
	LCD_ILI9481_INDEX(0x08);
	LCD_ILI9481_INDEX(0x00);

	LCD_ILI9481_CMD(0x0B);
	LCD_ILI9481_INDEX(0x00);
	LCD_ILI9481_INDEX(0x80);

	

	LCD_ILI9481_CMD(0xF0);
	LCD_ILI9481_INDEX(0x08);

	LCD_ILI9481_CMD(0xF6);
	LCD_ILI9481_INDEX(0x84);
	
	LCD_ILI9481_CMD(0xF3);
	LCD_ILI9481_INDEX(0x00);
	LCD_ILI9481_INDEX(0x2A);
	

	LCD_ILI9481_CMD(0x36);
	LCD_ILI9481_INDEX(0x0A);

	mdelay(10);

	LCD_ILI9481_CMD(0x29);
	LCD_ILI9481_CMD(0x2C);

	printk("-ili9481_init\r\n");
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
	if (ili9325sim_state.disp_powered_up && ili9325sim_state.display_on) 
	{
		LCD_ILI9481_CMD(0x10);  
		mdelay(10);
		LCD_ILI9481_CMD(0x2c);

		lcdc_ili9325sim_pdata->panel_config_gpio(0);
		ili9325sim_state.display_on = FALSE;
		ili9325sim_state.disp_initialized = FALSE;
	}
	printk("lcdc_ili9325sim_panel_off ***\r\n");
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
	uint32 lcdc_ns ;
	unsigned char *msm_clock_base;


	ret = platform_driver_register(&this_driver);
	if (ret)
		return ret;

	pinfo = &ili9325sim_panel_data.panel_info;
	pinfo->xres = 320;
	pinfo->yres = 480;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 16;
	pinfo->fb_num = 2;
	pinfo->clk_rate = 8000000;
	pinfo->bl_max = 100;
	pinfo->bl_min = 1;

	pinfo->lcdc.h_back_porch = 2;
	pinfo->lcdc.h_front_porch = 2;
	pinfo->lcdc.h_pulse_width = 3;
	pinfo->lcdc.v_back_porch = 1;
	pinfo->lcdc.v_front_porch = 3;
	pinfo->lcdc.v_pulse_width = 2;
	pinfo->lcdc.border_clr = 0;     /* blk */
	pinfo->lcdc.underflow_clr = 0;       /* blk */
	pinfo->lcdc.hsync_skew = 0;
	
	#define MSM_CLK_CTL_PHYS      0xA8600000
	msm_clock_base = ioremap(MSM_CLK_CTL_PHYS, 0x3bf);

	lcdc_ns = readl(msm_clock_base + 0x3b0);
	lcdc_ns |= (1<<13);
	writel(lcdc_ns, msm_clock_base + 0x3b0);

	ret = platform_device_register(&this_device);
	if (ret)
		platform_driver_unregister(&this_driver);

	return ret;
}

module_init(lcdc_ili9325sim_panel_init);

