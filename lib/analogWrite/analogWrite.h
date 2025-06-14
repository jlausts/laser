#include <Arduino.h>
#include "wiring_private.h"         



// max values -> 4096
#define WRITE_BOTH(a, b) \
    { \
        DAC->DATA[0].reg = a; \
        while (DAC->SYNCBUSY.bit.DATA0); \
        DAC->DATA[1].reg = b; \
        while (DAC->SYNCBUSY.bit.DATA1); \
    } 


void pwmSetup(uint32_t pin, uint32_t value);
void pwmWrite(uint32_t pin, uint32_t value);
void pwmDisable();
void pwmEnable();