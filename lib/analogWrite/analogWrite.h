#include <Arduino.h>
#include "wiring_private.h"         


#define CLIPX(x) (x < 0 ? 0 : (x > 4095 ? 4095 : (int)x));
#define CLIPY(y) (y < 0 ? 0 : (y > 4095 ? 4095 : (int)y));

// max values -> 4096
#define WRITE_BOTH(a, b) \
    { \
        DAC->DATA[0].reg = CLIPX(a); \
        while (DAC->SYNCBUSY.bit.DATA0); \
        DAC->DATA[1].reg = CLIPY(b); \
        while (DAC->SYNCBUSY.bit.DATA1); \
    } 


void pwmSetup(uint32_t pin, uint32_t value);
// void pwmWrite(uint32_t pin, uint32_t value);
// void pwmDisable();
// void pwmEnable();
void pwmWriteAll(uint32_t r, uint32_t g, uint32_t b);