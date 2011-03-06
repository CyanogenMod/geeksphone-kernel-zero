

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


static void LcdSend(unsigned char Value)
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
static void spi_init(void)
{
	spi_sclk = 132;
	spi_cs = 131;
	spi_sdo = 103;
	spi_sdi = 102;
	spi_rst = 88;
}

static void ili4573sim_disp_powerup(void)
{
	if (!lcdc_disp_state.disp_powered_up && !lcdc_disp_state.display_on) {
		/* Reset the hardware first */
		/* Include DAC power up implementation here */
	      lcdc_disp_state.disp_powered_up = TRUE;
	}
}

static void ili4573sim_init(void)
{
	unsigned int i;
	unsigned char pid;
	// has been called from checkid

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
	printk("+4573_init\r\n");

	//Display Mode 
	SetIndex(0xc0);
	SetData(0x01);
	SetData(0x18);//18

	SetIndex(0x20);//exit_invert_mode
	SetIndex(0x29);//set_display_on
	SetIndex(0x3A);//set_pixel_format 
	SetData(0x60);//70   0X60 26k

	SetIndex(0xB1);//RGB Interface Setting
	SetData(0x06);
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
	SetData(0x14);//VRH1  Vreg1out=1.533xVCI(10)
	SetData(0x14);//VRH2  Vreg2out=-1.533xVCI(10)

	SetData(0x01);

	SetData(0x6D);// 20100413 changed from 3F to 6D by LGE

	//Sleep(200);       // 20100413 deleted by LGE

	SetIndex(0xC5);
	SetData(0x6E);

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

	printk("-ili4573sim_init\r\n");
}


static bool lcdc_4573_checkID(void)
{
	unsigned char pid;
	printk("+lcdc_4573_checkID\r\n");

	lcdc_disp_pdata->panel_config_gpio(1);
	spi_init();	/* LCD needs SPI */

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

	if (pid == 0x10)
		return TRUE;
	else
		return FALSE;			
}

static void ili4573sim_disp_on(void)
{
	if (lcdc_disp_state.disp_powered_up && !lcdc_disp_state.display_on) {
		ili4573sim_init();
		lcdc_disp_state.display_on = TRUE;
	}
}

int lcdc_4573_panel_on(struct platform_device *pdev)
{
	printk("lcdc_4573_panel_on\n");

	if (!lcdc_disp_state.disp_initialized) {
		/* Configure reset GPIO that drives DAC */

		lcdc_disp_pdata->panel_config_gpio(1);
		spi_init();	/* LCD needs SPI */

		ili4573sim_disp_powerup();
		ili4573sim_disp_on();
		lcdc_disp_state.disp_initialized = TRUE;
	}
	return 0;
}

static int lcdc_4573_panel_off(struct platform_device *pdev)
{
	printk("lcdc_4573_panel_off\n");

	if (lcdc_disp_state.disp_powered_up && lcdc_disp_state.display_on) {
		SetIndex(0x10);
		mdelay(120);
		SetIndex(0xC1);
		SetData(0x01);
		mdelay(120);
		LCD_RST(0);

		lcdc_disp_pdata->panel_config_gpio(0);
		lcdc_disp_state.display_on = FALSE;
		lcdc_disp_state.disp_initialized = FALSE;
	}
	return 0;
}

static void lcdc_4573_set_backlight(struct msm_fb_data_type *mfd)
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

/*
static int __init lcdc_4573_probe(struct platform_device *pdev)
{
	printk("lcdc_4573_probe\n");

	if (pdev->id == 0) {
		lcdc_disp_pdata = pdev->dev.platform_data;
		return 0;
	}
	printk("lcdc_4573_probe1\n");

	msm_fb_add_device(pdev);
	printk("lcdc_4573_probe2\n");
	return 0;
}

static struct platform_driver lcdc_4573_driver = {
	.probe  = lcdc_4573_probe,
	.driver = {
		.name   = "lcd_multi",
	},
};
*/

static struct msm_fb_panel_data lcdc_4573_panel_data = {
	.on = lcdc_4573_panel_on,
	.off = lcdc_4573_panel_off,
	.set_backlight = lcdc_4573_set_backlight,
};


static struct platform_device lcdc_4573_device = {
	.name   = "ili9325sim_qvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcdc_4573_panel_data,
	}
};

int lcdc_4573_get_panel_info( bool islastone)
{
	struct msm_panel_info *pinfo;
	
	printk("lcdc_4573_get_panel_info\n");

	if (public_poweron_reset_ok)
	{
		// if not power on already, power on first
		// if we need special power on sequance, do it here
	}

	//check id first
	// if not supported return FALSE,(0)
	if(!lcdc_4573_checkID())
	{
		if (!islastone)
			return FALSE;
	}
	
	pinfo = &lcdc_4573_panel_data.panel_info;
	pinfo->xres = 480;
	pinfo->yres = 800;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
#ifdef CONFIG_FB_MSM_LCDC_MULTI_4573_PANEL_18BIT
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


	//pthis_driver = &lcdc_4573_driver;
	pthis_device = &lcdc_4573_device;

	return TRUE;
}


