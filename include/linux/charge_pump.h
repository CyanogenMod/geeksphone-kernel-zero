#ifndef _CHARGE_PUMP_H
#define _CHARGE_PUMP_H

typedef struct {
    void (*PowerOn)(void);
    void (*PowerOff)(void);
    void (*SetDispLightLv)(uint Lv);
    void (*SetKeyLightLv)(uint Lv);
} ChargePumpIf_t;

void ChargePumpRegIf(ChargePumpIf_t *If);
void ChargePumpPowerOn(void);
void ChargePumpPowerOff(void);
void ChargePumpSetDispLightLv(uint Lv);
void ChargePumpSetKeyLightLv(uint Lv);
void ChargePumpTest(void);

#endif //_CHARGE_PUMP_H
