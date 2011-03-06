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


static void LCDSPI_InitCMD(unsigned char Byte)
{
    unsigned long   Shift;
    unsigned long   Cnt;

    Shift = 0x80;

    LCD_CS(0);
    spi_delay(1);
    LCD_SCL(0);
    LCD_SI(0);
    spi_delay(1);
    LCD_SCL(1);
    spi_delay(1);

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

    spi_delay(1);
    LCD_CS(1);
}


static void LCDSPI_InitDAT(unsigned char Byte)
{
    unsigned long   Shift;
    unsigned long   Cnt;

    Shift = 0x80;

    LCD_CS(0);
    spi_delay(1);

    //DNC =1
    LCD_SCL(0);
    LCD_SI(1);
    spi_delay(1);
    LCD_SCL(1);
    spi_delay(1);

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


    spi_delay(1);
    LCD_CS(1);
}

static void GetData( unsigned char *pData)
{
    LCD_CS(0);
    spi_delay( 1);

    LCDSPI_InitCMD(0xDA);
    GetByte(pData);

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
	unsigned int i;
	unsigned char pid;
	printk("+ili9325sim_init\r\n");

	LCD_CS(1);
	LCD_SCL(0);
	LCD_SI(1);
	LCD_RST(1);

	// Reset LCD
	LCD_RST( 0);
	mdelay(50);
	LCD_RST(1);
	mdelay(50);

	GetData(&pid);
	printk("lcd chip id = 0x%x \r\n",pid);

	LCDSPI_InitCMD(0xB9); //Set_EXTC
	LCDSPI_InitDAT(0xFF); //
	LCDSPI_InitDAT(0x83); //
	LCDSPI_InitDAT(0x63); //
	LCDSPI_InitCMD(0xB1); //Set_POWER
	LCDSPI_InitDAT(0x81); //
	LCDSPI_InitDAT(0x30); //
	LCDSPI_InitDAT(0x07); //
	LCDSPI_InitDAT(0x34); //
	LCDSPI_InitDAT(0x02); //
	LCDSPI_InitDAT(0x13); //
	LCDSPI_InitDAT(0x11); //
	LCDSPI_InitDAT(0x00); //
	LCDSPI_InitDAT(0x3A); //
	LCDSPI_InitDAT(0x42); //
	LCDSPI_InitDAT(0x3F); //
	LCDSPI_InitDAT(0x3F); //
	LCDSPI_InitCMD(0x11);
	mdelay(120);
	
	LCDSPI_InitCMD(0x36); //Set_PANEL
	LCDSPI_InitDAT(0x02); //SS=1

	LCDSPI_InitCMD(0x3A); //COLMOD
	LCDSPI_InitDAT(0x66); //
	LCDSPI_InitCMD(0xB3); //Set_RGBIF
	LCDSPI_InitDAT(0x09); //01
	LCDSPI_InitCMD(0xB4); //Set_CYC
	LCDSPI_InitDAT(0x08); //
	LCDSPI_InitDAT(0x12); //
	LCDSPI_InitDAT(0x72); //
	LCDSPI_InitDAT(0x12); //
	LCDSPI_InitDAT(0x06); //
	LCDSPI_InitDAT(0x03); //
	LCDSPI_InitDAT(0x54); //
	LCDSPI_InitDAT(0x03); //
	LCDSPI_InitDAT(0x4E); //
	LCDSPI_InitDAT(0x00); //
	LCDSPI_InitDAT(0x00); //
	LCDSPI_InitCMD(0xB6); //Set_VCOM
	LCDSPI_InitDAT(0x00); //
	LCDSPI_InitCMD(0xCC); //Set_PANEL
	LCDSPI_InitDAT(0x03); //01

	mdelay(5);
	LCDSPI_InitCMD(0xE0); //Gamma 2.2
	LCDSPI_InitDAT(0x00); //
	LCDSPI_InitDAT(0x40); //
	LCDSPI_InitDAT(0x00); //
	LCDSPI_InitDAT(0x4C); //
	LCDSPI_InitDAT(0x13); //

	LCDSPI_InitDAT(0x67); //
	LCDSPI_InitDAT(0x02); //
	LCDSPI_InitDAT(0x0A); //
	LCDSPI_InitDAT(0x0F); //
	LCDSPI_InitDAT(0x15); //
	LCDSPI_InitDAT(0x1A); //
	LCDSPI_InitDAT(0x56); //
	LCDSPI_InitDAT(0x17); //
	LCDSPI_InitDAT(0x09); //
	LCDSPI_InitDAT(0x04); //
	LCDSPI_InitDAT(0x00); //
	LCDSPI_InitDAT(0x40); //
	LCDSPI_InitDAT(0x00); //
	LCDSPI_InitDAT(0x4C); //
	LCDSPI_InitDAT(0x13); //
	LCDSPI_InitDAT(0x67); //
	LCDSPI_InitDAT(0x04); //
	LCDSPI_InitDAT(0x0A); //
	LCDSPI_InitDAT(0x0F); //
	LCDSPI_InitDAT(0x15); //
	LCDSPI_InitDAT(0x16); //
	LCDSPI_InitDAT(0x56); //
	LCDSPI_InitDAT(0x17); //
	LCDSPI_InitDAT(0x09); //
	LCDSPI_InitDAT(0x04); //
	mdelay(5);
	LCDSPI_InitCMD(0xC1); //DGC
	LCDSPI_InitDAT(0x01);
	LCDSPI_InitDAT(0x04 );
	LCDSPI_InitDAT(0x0A );
	LCDSPI_InitDAT(0x0E );
	LCDSPI_InitDAT(0x1A );
	LCDSPI_InitDAT(0x22 );
	LCDSPI_InitDAT(0x2A );
	LCDSPI_InitDAT(0x30 );
	LCDSPI_InitDAT(0x36 );
	LCDSPI_InitDAT(0x3E );
	LCDSPI_InitDAT(0x47 );
	LCDSPI_InitDAT(0x50 );
	LCDSPI_InitDAT(0x58 );
	LCDSPI_InitDAT(0x60 );
	LCDSPI_InitDAT(0x68 );
	LCDSPI_InitDAT(0x70 );
	LCDSPI_InitDAT(0x78 );
	LCDSPI_InitDAT(0x80 );
	LCDSPI_InitDAT(0x88 );
	LCDSPI_InitDAT(0x90 );
	LCDSPI_InitDAT(0x99 );
	LCDSPI_InitDAT(0xA0 );
	LCDSPI_InitDAT(0xA7 );
	LCDSPI_InitDAT(0xAE );
	LCDSPI_InitDAT(0xB7 );
	LCDSPI_InitDAT(0xC0 );
	LCDSPI_InitDAT(0xC9 );
	LCDSPI_InitDAT(0xCE );
	LCDSPI_InitDAT(0xD7 );
	LCDSPI_InitDAT(0xE0 );

	LCDSPI_InitDAT(0xE7 );
	LCDSPI_InitDAT(0xF0 );
	LCDSPI_InitDAT(0xF8 );
	LCDSPI_InitDAT(0xFF );
	LCDSPI_InitDAT(0xA7 );
	LCDSPI_InitDAT(0x98 );
	LCDSPI_InitDAT(0x79 );
	LCDSPI_InitDAT(0x66 );
	LCDSPI_InitDAT(0x9D );
	LCDSPI_InitDAT(0xAD );
	LCDSPI_InitDAT(0xC9 );
	LCDSPI_InitDAT(0x13 );
	LCDSPI_InitDAT(0x02 );
	LCDSPI_InitDAT(0x04 );
	LCDSPI_InitDAT(0x0A );
	LCDSPI_InitDAT(0x0E );
	LCDSPI_InitDAT(0x1A );
	LCDSPI_InitDAT(0x22 );
	LCDSPI_InitDAT(0x2A );
	LCDSPI_InitDAT(0x30 );
	LCDSPI_InitDAT(0x36 );
	LCDSPI_InitDAT(0x3E );
	LCDSPI_InitDAT(0x47 );
	LCDSPI_InitDAT(0x50 );
	LCDSPI_InitDAT(0x58 );
	LCDSPI_InitDAT(0x60 );
	LCDSPI_InitDAT(0x68 );
	LCDSPI_InitDAT(0x70 );
	LCDSPI_InitDAT(0x78 );
	LCDSPI_InitDAT(0x80 );
	LCDSPI_InitDAT(0x88 );
	LCDSPI_InitDAT(0x90 );
	LCDSPI_InitDAT(0x99 );
	LCDSPI_InitDAT(0xA0 );
	LCDSPI_InitDAT(0xA7 );
	LCDSPI_InitDAT(0xAE );
	LCDSPI_InitDAT(0xB7 );
	LCDSPI_InitDAT(0xC0 );
	LCDSPI_InitDAT(0xC9 );
	LCDSPI_InitDAT(0xCE );
	LCDSPI_InitDAT(0xD7 );
	LCDSPI_InitDAT(0xE0 );
	LCDSPI_InitDAT(0xE7 );
	LCDSPI_InitDAT(0xF0 );
	LCDSPI_InitDAT(0xF8 );
	LCDSPI_InitDAT(0xFF );
	LCDSPI_InitDAT(0xA7 );
	LCDSPI_InitDAT(0x98 );
	LCDSPI_InitDAT(0x79 );
	LCDSPI_InitDAT(0x66 );
	LCDSPI_InitDAT(0x9D );
	LCDSPI_InitDAT(0xAD );
	LCDSPI_InitDAT(0xC9 );
	LCDSPI_InitDAT(0x13 );
	LCDSPI_InitDAT(0x02 );
	LCDSPI_InitDAT(0x04 );
	LCDSPI_InitDAT(0x0A );
	LCDSPI_InitDAT(0x0E );
	LCDSPI_InitDAT(0x1A );
	LCDSPI_InitDAT(0x22 );
	LCDSPI_InitDAT(0x2A );

	LCDSPI_InitDAT(0x30 );
	LCDSPI_InitDAT(0x36 );
	LCDSPI_InitDAT(0x3E );
	LCDSPI_InitDAT(0x47 );
	LCDSPI_InitDAT(0x50 );
	LCDSPI_InitDAT(0x58 );
	LCDSPI_InitDAT(0x60 );
	LCDSPI_InitDAT(0x68 );
	LCDSPI_InitDAT(0x70 );
	LCDSPI_InitDAT(0x78 );
	LCDSPI_InitDAT(0x80 );
	LCDSPI_InitDAT(0x88 );
	LCDSPI_InitDAT(0x90 );
	LCDSPI_InitDAT(0x99 );
	LCDSPI_InitDAT(0xA0 );
	LCDSPI_InitDAT(0xA7 );
	LCDSPI_InitDAT(0xAE );
	LCDSPI_InitDAT(0xB7 );
	LCDSPI_InitDAT(0xC0 );
	LCDSPI_InitDAT(0xC9 );
	LCDSPI_InitDAT(0xCE );
	LCDSPI_InitDAT(0xD7 );
	LCDSPI_InitDAT(0xE0 );
	LCDSPI_InitDAT(0xE7 );
	LCDSPI_InitDAT(0xF0 );
	LCDSPI_InitDAT(0xF8 );
	LCDSPI_InitDAT(0xFF );
	LCDSPI_InitDAT(0xA7 );
	LCDSPI_InitDAT(0x98 );
	LCDSPI_InitDAT(0x79 );
	LCDSPI_InitDAT(0x66 );
	LCDSPI_InitDAT(0x9D );
	LCDSPI_InitDAT(0xAD );
	LCDSPI_InitDAT(0xC9 );
	LCDSPI_InitDAT(0x13 );
	LCDSPI_InitDAT(0x02 );
	mdelay(5);
	LCDSPI_InitCMD(0x29);

	printk("-ili9325sim_init\r\n");
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
		
		ili9325sim_state.display_on = FALSE;
		ili9325sim_state.disp_initialized = FALSE;
		printk("lcdc_ili9325sim_panel_off\r\n");
		//SetIndex(0x28);
		LCDSPI_InitCMD(0x10);
		mdelay(120);
		gpio_set_value(88, 0);
		lcdc_ili9325sim_pdata->panel_config_gpio(0);
		//SetIndex(0xC1);
		//SetData(0x01);
	
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

	ret = platform_driver_register(&this_driver);
	if (ret)
		return ret;

	pinfo = &ili9325sim_panel_data.panel_info;
	pinfo->xres = 480;
	pinfo->yres = 800;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 16;
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

	ret = platform_device_register(&this_device);
	if (ret)
		platform_driver_unregister(&this_driver);

	return ret;
}

module_init(lcdc_ili9325sim_panel_init);
