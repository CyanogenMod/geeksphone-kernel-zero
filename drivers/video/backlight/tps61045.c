#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/charge_pump.h>

#define CLIP_VALUE(g,l,h)  {                        \
                                if ((g) > (h))      \
                                {                   \
                                    (g) = (h);      \
                                }                   \
                                else if ((g) < (l)) \
                                {                   \
                                    (g) = (l);      \
                                }                   \
                            }

#define TPS_HW_MAX_LV (25)

typedef struct {
    uint Id;
    uint MaxLv;
    uint DesLv;
    uint CurLv;
    struct platform_device *Device;
} TpsPumpRes_t;

static TpsPumpRes_t TpsRes = {
    .MaxLv = TPS_HW_MAX_LV,
};

static DEFINE_SPINLOCK(atom_lock);

extern struct gpio_chip *gpio2chip(unsigned int gpio);
/*extern void *msm_gpio_get_regs(struct gpio_chip *chip);
extern void msm_gpio_set_bit(unsigned n, void *regs);
extern void msm_gpio_clr_bit(unsigned n, void *regs);*/

static void Tpsc(TpsPumpRes_t *pstRes, u32 n, bool Dir) {
    //void *regs;
    struct gpio_chip * chip;
    unsigned long irq_flags;
    u32 i, offset, delay, loops;
    
    chip = gpio2chip(pstRes->Id);
    //regs = msm_gpio_get_regs(chip);
    offset = pstRes->Id - chip->base;
    delay = Dir ? 20 : 200;
    
    //printk("Tpsc n:%d, d:%d\n", n, Dir);

    spin_lock_irqsave(&atom_lock, irq_flags);
    loops = loops_per_jiffy/(1000000/HZ);
    loops *= delay;
    for (i = 0; i < n; i++) {
        gpio_direction_output(pstRes->Id, 1);
        //msm_gpio_set_bit(offset, regs);
        // delay 1us
        gpio_direction_output(pstRes->Id, 0);
        //msm_gpio_clr_bit(offset, regs);
        __delay(loops);
    }
    gpio_direction_output(pstRes->Id, 1);
    //msm_gpio_set_bit(offset, regs);
    spin_unlock_irqrestore(&atom_lock, irq_flags);

    udelay(1000);
}

uint Tps61045Lv(uint Lv) {    
    uint Bank;

    CLIP_VALUE(Lv, 0, 100);
    Bank = Lv * TpsRes.MaxLv / 100;
    CLIP_VALUE(Bank, 1, TpsRes.MaxLv);
    return Bank;
}

void Tps61045Set(TpsPumpRes_t *pstRes) {
    uint Delta;

    printk("CurLv:%d, DesLv:%d\n", pstRes->CurLv, pstRes->DesLv);

    if (pstRes->DesLv > pstRes->CurLv) {
        Delta = pstRes->DesLv - pstRes->CurLv;
        Tpsc(pstRes, Delta, true);        
    }
    else if (pstRes->DesLv < pstRes->CurLv) {
        Delta = pstRes->CurLv - pstRes->DesLv;
        Tpsc(pstRes, Delta, false);        
    }
    else {
        gpio_set_value(pstRes->Id, 1);
    }
    pstRes->CurLv = pstRes->DesLv;
}

void Tps61045PowerOn(TpsPumpRes_t *pstRes) {
    gpio_set_value(pstRes->Id, 1);
    pstRes->CurLv = pstRes->DesLv = TPS_HW_MAX_LV;
}

void Tps61045PowerOff(TpsPumpRes_t *pstRes) {
    
    gpio_set_value(pstRes->Id, 0);
    udelay(600);
}

static void _ChargePumpPowerOn(void) {
    Tps61045PowerOn(&TpsRes);
}

static void _ChargePumpPowerOff(void) {
    Tps61045PowerOff(&TpsRes);
}

static void _ChargePumpSetDispLightLv(uint Lv) {
    TpsRes.DesLv = Tps61045Lv(Lv);
    Tps61045Set(&TpsRes);
}

static void _ChargePumpSetKeyLightLv(uint Lv) {
}

static ChargePumpIf_t _ChargePumpIf = {
    .PowerOn = &_ChargePumpPowerOn,
    .PowerOff = &_ChargePumpPowerOff,
    .SetDispLightLv = &_ChargePumpSetDispLightLv,
    .SetKeyLightLv = &_ChargePumpSetKeyLightLv,
};

static int __init Tps61045Probe(struct platform_device *pdev) {

    int rc;
    struct resource *res;
    
    TpsRes.Device = pdev;

    do {
        res = platform_get_resource_byname(TpsRes.Device, IORESOURCE_IO, "ctrl");
        if (!res) {
            printk(KERN_ERR "%s:%d get ctrl pin fail\n",
                    __func__, __LINE__);
            rc = -EIO;
            break;
        }
        TpsRes.Id = res->start;
        printk(KERN_INFO "%s: get ctrl pin %d\n", __func__, TpsRes.Id);
        
        rc = gpio_request(TpsRes.Id, "ctrl_pin");
        if (rc)
            break;
        gpio_direction_output(TpsRes.Id, 0);
        Tps61045PowerOff(&TpsRes);

        res = platform_get_resource_byname(TpsRes.Device, IORESOURCE_IO, "lvs");
        if (res) {
            TpsRes.MaxLv = res->start;
        }
        else {
            printk(KERN_WARNING "%s:%d get levels fail\n",
                    __func__, __LINE__);
        }
        if (TpsRes.MaxLv > TPS_HW_MAX_LV) {
            TpsRes.MaxLv = TPS_HW_MAX_LV;
        }
        printk(KERN_INFO "%s: get max level %d\n", __func__, TpsRes.MaxLv);
        TpsRes.CurLv = TpsRes.DesLv = TPS_HW_MAX_LV;
        
        ChargePumpRegIf(&_ChargePumpIf);
        
    } while(0);

    return rc;
}

static struct platform_driver __refdata Tps61045River = {
    .probe  = Tps61045Probe,
    .driver = {
        .name   = "tps61045",
    },
};

int __init Tps61045Init(void) 
{
    int ret;

    ret = platform_driver_register(&Tps61045River);
    
    return ret;
}

void __exit Tps61045Exit(void) 
{
    platform_driver_unregister(&Tps61045River);
}

module_init(Tps61045Init);
module_exit(Tps61045Exit);

void ChargePumpTest(void) {
    Tpsc(&TpsRes, 2, false);
}
EXPORT_SYMBOL(ChargePumpTest);

