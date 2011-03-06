

#include <linux/delay.h>
#include <linux/charge_pump.h>
#include <mach/gpio.h>
#include "msm_fb.h"



extern bool public_poweron_reset_ok;
extern struct platform_driver *pthis_driver;
extern struct platform_device *pthis_device;
extern struct disp_state_type lcdc_disp_state;
extern struct msm_panel_common_pdata *lcdc_disp_pdata;

#define LCD_MSG 0
#define PARA_NUM(x) (sizeof(x)/sizeof(uint16)/2)

static int spi_cs;
static int spi_sclk;
static int spi_sdo;
static int spi_sdi;
static int spi_rst;



/*===========================================================================
                            FUNCTION PROTOTYPES
===========================================================================*/

static void busy_wait(unsigned long ms) 
{
	//ms >>= 1;	
	mdelay(ms);
}

/*===========================================================================
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

static __inline void GetByte(unsigned int *pByte)
{
    unsigned long   Shift;
    unsigned long   Cnt;
    unsigned int    Byte = 0;
    
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

static void GetData( unsigned int *pData)
{
    LCD_CS(0);
    spi_delay( 1);

    GetByte(pData);

    spi_delay(1);
    LCD_CS(1);
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
===========================================================================*/
static void spi_init(void)
{
	spi_sclk = 132;
	spi_cs = 131;
	spi_sdo = 103;
	spi_sdi = 102;
	spi_rst = 88;
	//gpio_set_value(bkl_on, 1);
}
static void lcdc_9418_disp_powerup(void)
{
	if (!lcdc_disp_state.disp_powered_up && !lcdc_disp_state.display_on) {
		/* Reset the hardware first */
		/* Include DAC power up implementation here */
	      lcdc_disp_state.disp_powered_up = TRUE;
	}
}

static void lcdc_9418_init(void)
{
	unsigned int i;
	unsigned int pid;
	// has been called from checkid
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

	LCD_ILI9481_CMD(0xBF);
	GetData(&pid);
	printk("lcd chip id = 0x%x \r\n",pid);

	printk("+ili9481_init\r\n");

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

static bool lcdc_9418_checkID(void)
{
	unsigned int id;

	lcdc_disp_pdata->panel_config_gpio(1);
	spi_init();	/* LCD needs SPI */

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
	LCD_ILI9481_CMD(0xBF);
	GetData(&id);
	printk("lcd chip id = 0x%x \r\n",id);

	//0x1024a40
	if (id == 0x1)
		return TRUE;
	else
		return FALSE;	

}

static void lcdc_9418_disp_on(void)
{
	if (lcdc_disp_state.disp_powered_up && !lcdc_disp_state.display_on) {
		lcdc_9418_init();
		lcdc_disp_state.display_on = TRUE;
	}
}

static int lcdc_9418_panel_on(struct platform_device *pdev)
{
	//printk("lcdc_9418_panel_on\n");
	if (!lcdc_disp_state.disp_initialized) {
		/* Configure reset GPIO that drives DAC */

		lcdc_disp_pdata->panel_config_gpio(1);
		spi_init();	/* LCD needs SPI */

		lcdc_9418_disp_powerup();
		lcdc_9418_disp_on();
		lcdc_disp_state.disp_initialized = TRUE;
	}
	return 0;
}

static int lcdc_9418_panel_off(struct platform_device *pdev)
{
	if (lcdc_disp_state.disp_powered_up && lcdc_disp_state.display_on) 
	{
		LCD_ILI9481_CMD(0x10);  
		mdelay(10);
		LCD_ILI9481_CMD(0x2c);
		lcdc_disp_pdata->panel_config_gpio(0);
		lcdc_disp_state.display_on = FALSE;
		lcdc_disp_state.disp_initialized = FALSE;
	}
	//printk("lcdc_9418_panel_off\n");
	return 0;
}

static void lcdc_9418_set_backlight(struct msm_fb_data_type *mfd)
{
	static bool bl_on = false;
	bool panel_on = mfd->panel_power_on;
	int bl_lv = mfd->bl_level;

	//printk("lcdc_9418_set_backlight bl_lv %d bl_on %d panel_on %d\n", bl_lv, bl_on, panel_on);
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

static struct msm_fb_panel_data lcdc_9418_panel_data = {
	.on = lcdc_9418_panel_on,
	.off = lcdc_9418_panel_off,
	.set_backlight = lcdc_9418_set_backlight,
};

static struct platform_device lcdc_9418_device = {
	.name   = "ili9325sim_qvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcdc_9418_panel_data,
	}
};

int lcdc_9418_get_panel_info( bool islastone)
{
	struct msm_panel_info *pinfo;
	uint32 lcdc_ns ;
	unsigned char *msm_clock_base;
	
	printk("lcdc_9418_get_panel_info\n");

	if (public_poweron_reset_ok)
	{
		// if not power on already, power on first
		// if we need special power on sequance, do it here
	}

	//check id first
	// if not supported return FALSE,(0)
	if (!lcdc_9418_checkID())
	{
		if (!islastone)
			return FALSE;
	}

	pinfo = &lcdc_9418_panel_data.panel_info;
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

	//pthis_driver = &lcdc_9418_driver;
	pthis_device = &lcdc_9418_device;

	return TRUE;
}


