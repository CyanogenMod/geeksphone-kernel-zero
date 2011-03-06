
#include <linux/delay.h>
#include <linux/charge_pump.h>
#include <mach/gpio.h>
#include "msm_fb.h"




extern int lcdc_9418_get_panel_info( bool islastone);
extern int lcdc_4573_get_panel_info( bool islastone);

#define INITFUNS_NUM (sizeof(lcdc_initfn_list)/sizeof(int*))
int (*lcdc_initfn_list[])(bool) = {
#ifdef CONFIG_FB_MSM_LCDC_MULTI_4573_PANEL
	lcdc_4573_get_panel_info,
#endif
#ifdef CONFIG_FB_MSM_LCDC_MULTI_9418_PANEL
    lcdc_9418_get_panel_info,
#endif
};

struct disp_state_type lcdc_disp_state = { 0 };
struct msm_panel_common_pdata *lcdc_disp_pdata;

struct platform_driver *pthis_driver;
struct platform_device *pthis_device;

bool public_poweron_reset_ok = FALSE;

static int check_panel(void)
{
	bool ret = FALSE;
	int i;
	bool islastone = FALSE;

	//do public poweron & reset here
	
	for (i = 0; i < INITFUNS_NUM; i++)
	{
		if (i == INITFUNS_NUM - 1)
			islastone = TRUE;
		
		if (lcdc_initfn_list[i](islastone))
		{
			ret = TRUE;
			break;
		}
	}

	return ret;
}

static int __init lcdc_multi_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		lcdc_disp_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);
	return 0;
}

static struct platform_driver lcdc_multi_driver = {
	.probe  = lcdc_multi_probe,
	.driver = {
		.name   = "ili9325sim_qvga",
	},
};

static int __init lcdc_multi_panel_init(void)
{
	int ret;
	printk("lcdc_multi_panel_init++\n");

	ret = platform_driver_register(&lcdc_multi_driver);
	if (ret)
		return ret;

	//here get real driver
	check_panel();

	ret = platform_device_register(pthis_device);
	if (ret)
		platform_driver_unregister(&lcdc_multi_driver);
	printk("lcdc_multi_panel_init--\n");

	return ret;
}

module_init(lcdc_multi_panel_init);

