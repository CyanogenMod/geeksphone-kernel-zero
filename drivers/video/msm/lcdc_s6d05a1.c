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


static const unsigned short InitParaList[][2] =
{
    {0x00F0, 0},
    {0x015A, 0},
    {0x015A, 0},
#if 0
    {0x00F2, 0},
    {0x013B, 0},
    {0x0153, 0},
    {0x0103, 0},
    {0x0108, 0},
    {0x0108, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0112, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0153, 0},
    {0x0108, 0},
    {0x0108, 0},
    {0x0108, 0},
    {0x0108, 0},
#endif
    {0x00F4, 0},
    {0x0107, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0104, 0},
    {0x016B, 0},
    {0x0104, 0},
    {0x0104, 0},
    {0x016B, 0},
    {0x0104, 0},

    {0x00F5, 0},
    {0x0100, 0},
    {0x0150, 0},
    {0x015D, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0102, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0104, 0},
    {0x0100, 0},
    {0x0150, 0},
    {0x015D, 0},

    {0x00F6, 0},
    {0x0102, 0},
    {0x0100, 0},
    {0x0104, 0},
    {0x0103, 0},
    {0x0101, 0},
    {0x0100, 0},
    {0x0101, 0},
    {0x0100, 0},
    {0x0100, 0},

    {0x00F7, 0},
    {0x0100, 0},
    {0x0181, 0},
    {0x0130, 0},
    {0x0102, 0},
    {0x0100, 100},

    {0x00F9, 0},
    {0x0117, 0},

    {0x00FA, 0},
    {0x0100, 0},
    {0x010C, 0},
    {0x0103, 0},
    {0x011D, 0},
    {0x011E, 0},
    {0x0121, 0},
    {0x0124, 0},
    {0x0122, 0},
    {0x012F, 0},
    {0x0135, 0},
    {0x013F, 0},
    {0x013F, 0},
    {0x0134, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0100, 0},

    {0x00FB, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0130, 0},
    {0x013E, 0},
    {0x013F, 0},
    {0x0137, 0},
    {0x0132, 0},
    {0x0126, 0},
    {0x011F, 0},
    {0x011E, 0},
    {0x011B, 0},
    {0x011A, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0100, 0},
    {0x0100, 0},

    {0x0044, 0},
    {0x0100, 0},
    {0x0101, 0},

    {0x0035, 0},
    {0x0101, 0},

    {0x0036, 0},
    {0x0180, 0},

    {0x0011, 120},

    {0x002C, 0},

    {0x0029, 0},
};

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
void spi_delay(unsigned long delay) 
{
	//delay /= 5;	
	udelay(delay*100);
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
	unsigned long Level;

	return Level;
}

static __inline void LCD_SI(unsigned char Lv) 
{
	gpio_set_value(spi_sdi, Lv);
}

static __inline void LCD_RST(unsigned char Lv) 
{
	gpio_set_value(spi_rst, Lv);
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
	printk("+s6d05a1_init\r\n");

	LCD_CS(1);
	LCD_SCL(1);
	LCD_SI(1);
	LCD_RST(1);

	// Reset LCD
	LCD_RST(0);
	mdelay(150);
	LCD_RST(1);
	mdelay(500);

while(0) {
    LcdSend(0x0055); // cmd
    udelay(200);
    LcdSend(0x01AA); // cmd
    udelay(200);
}

	// Set LCD Parameters
    for (i = 0; i < PARA_NUM(InitParaList); i++)
    {
        LcdSend(InitParaList[i][0]); // cmd
        ili9325sim_busy_wait(InitParaList[i][1]);
        //printk("data:0x%x, delay:%d\r\n", InitParaList[i][0], InitParaList[i][1]);
	}

	printk("-s6d05a1_init\r\n");
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
	pinfo->xres = 320;
	pinfo->yres = 480;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 18;
	pinfo->fb_num = 2;
	pinfo->clk_rate = 18000000;
	pinfo->bl_max = 100;
	pinfo->bl_min = 1;

	pinfo->lcdc.h_back_porch = 16;
	pinfo->lcdc.h_front_porch = 16;
	pinfo->lcdc.h_pulse_width = 2;
	pinfo->lcdc.v_back_porch = 8;
	pinfo->lcdc.v_front_porch = 8;
	pinfo->lcdc.v_pulse_width = 2;
	pinfo->lcdc.border_clr = 0;     /* blk */
	pinfo->lcdc.underflow_clr = 0;       /* blk */
	pinfo->lcdc.hsync_skew = 0;

	ret = platform_device_register(&this_device);
	if (ret)
		platform_driver_unregister(&this_driver);

	return ret;
}

module_init(lcdc_ili9325sim_panel_init);

