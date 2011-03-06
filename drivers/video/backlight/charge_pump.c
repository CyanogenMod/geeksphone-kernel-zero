#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/charge_pump.h>

typedef struct {
    uint Id;
    ChargePumpIf_t *If;
    struct platform_device *Device;
} ChargePumpRes_t;

static ChargePumpRes_t ChargePumpRes;

void ChargePumpPowerOn(void) {
    if (ChargePumpRes.If) {
        ChargePumpRes.If->PowerOn();
    }
    else {
        if (0 == gpio_request(ChargePumpRes.Id, "ctrl_pin")) {
            gpio_direction_output(ChargePumpRes.Id, 1);
            gpio_free(ChargePumpRes.Id);
        }
    }
}
EXPORT_SYMBOL(ChargePumpPowerOn);

void ChargePumpPowerOff(void) {
    if (ChargePumpRes.If) {
        ChargePumpRes.If->PowerOff();
    }
    else {
        if (0 == gpio_request(ChargePumpRes.Id, "ctrl_pin")) {
            gpio_direction_output(ChargePumpRes.Id, 0);
            gpio_free(ChargePumpRes.Id);
        }
    }
}
EXPORT_SYMBOL(ChargePumpPowerOff);

void ChargePumpSetDispLightLv(uint Lv) {
    if (ChargePumpRes.If) {
        ChargePumpRes.If->SetDispLightLv(Lv);
    }
}
EXPORT_SYMBOL(ChargePumpSetDispLightLv);

void ChargePumpSetKeyLightLv(uint Lv) {
    if (ChargePumpRes.If) {
        ChargePumpRes.If->SetKeyLightLv(Lv);
    }
}
EXPORT_SYMBOL(ChargePumpSetKeyLightLv);

void ChargePumpRegIf(ChargePumpIf_t *If) {
    ChargePumpRes.If = If;
}
EXPORT_SYMBOL(ChargePumpRegIf);

static int __init ChargePumpProbe(struct platform_device *pdev) {

    int rc = 0;
    struct resource *res;
    
    ChargePumpRes.Device = pdev;

    do {
        res = platform_get_resource_byname(ChargePumpRes.Device, IORESOURCE_IO, "ctrl");
        if (!res) {
            printk(KERN_ERR "%s:%d get ctrl pin fail\n",
                    __func__, __LINE__);
            rc = -EIO;
            break;
        }
        ChargePumpRes.Id = res->start;
        ChargePumpPowerOff();
    } while(0);

    return rc;
}

static struct platform_driver __refdata ChargePumpRiver = {
    .probe  = ChargePumpProbe,
    .driver = {
        .name   = "charge_pump",
    },
};

int __init ChargePumpInit(void) 
{
    int ret;

    ret = platform_driver_register(&ChargePumpRiver);
    
    return ret;
}

void __exit ChargePumpExit(void) 
{
    platform_driver_unregister(&ChargePumpRiver);
}

module_init(ChargePumpInit);
module_exit(ChargePumpExit);

