#include "analogWrite.h"


uint32_t tcNums[20];
uint8_t tcChannels[20];
PinDescription pinDescs[20];


void pwmSetup(uint32_t pin, uint32_t value)
{
    static bool tcEnabled[TCC_INST_NUM + TC_INST_NUM];
    PinDescription pinDesc = g_APinDescription[pin];
    uint32_t attr = pinDesc.ulPinAttribute;

    if (!(attr & (PIN_ATTR_PWM_E | PIN_ATTR_PWM_F | PIN_ATTR_PWM_G))) return;

    uint32_t tcNum = GetTCNumber(pinDesc.ulPWMChannel);
    uint8_t tcChannel = GetTCChannelNumber(pinDesc.ulPWMChannel);

    tcNums[pin] = tcNum;
    tcChannels[pin] = tcChannel;
    pinDescs[pin] = pinDesc;

    if (attr & PIN_ATTR_PWM_E)
        pinPeripheral(pin, PIO_TIMER);
    else if (attr & PIN_ATTR_PWM_F)
        pinPeripheral(pin, PIO_TIMER_ALT);
    else if (attr & PIN_ATTR_PWM_G)
        pinPeripheral(pin, PIO_TCC_PDEC);

    if (tcEnabled[tcNum]) return;  // Already setup
    tcEnabled[tcNum] = true;

    GCLK->PCHCTRL[GCLK_CLKCTRL_IDs[tcNum]].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | (1 << GCLK_PCHCTRL_CHEN_Pos);

    if (tcNum >= TCC_INST_NUM) {
        Tc *TCx = (Tc *)GetTC(pinDesc.ulPWMChannel);
        TCx->COUNT8.CTRLA.bit.SWRST = 1;
        while (TCx->COUNT8.SYNCBUSY.bit.SWRST);
        TCx->COUNT8.CTRLA.bit.ENABLE = 0;
        while (TCx->COUNT8.SYNCBUSY.bit.ENABLE);
        TCx->COUNT8.CTRLA.reg = TC_CTRLA_MODE_COUNT8 | TC_CTRLA_PRESCALER_DIV8;
        TCx->COUNT8.WAVE.reg = TC_WAVE_WAVEGEN_NPWM;
        while (TCx->COUNT8.SYNCBUSY.bit.CC0);
        TCx->COUNT8.CC[tcChannel].reg = (uint8_t)value;
        while (TCx->COUNT8.SYNCBUSY.bit.CC0);
        TCx->COUNT8.PER.reg = 0xFF;
        while (TCx->COUNT8.SYNCBUSY.bit.PER);
        TCx->COUNT8.CTRLA.bit.ENABLE = 1;
        while (TCx->COUNT8.SYNCBUSY.bit.ENABLE);
    } 
    else {
        Tcc *TCCx = (Tcc *)GetTC(pinDesc.ulPWMChannel);
        TCCx->CTRLA.bit.SWRST = 1;
        while (TCCx->SYNCBUSY.bit.SWRST);
        TCCx->CTRLA.bit.ENABLE = 0;
        while (TCCx->SYNCBUSY.bit.ENABLE);
        TCCx->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV8 | TCC_CTRLA_PRESCSYNC_GCLK;
        TCCx->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
        while (TCCx->SYNCBUSY.bit.WAVE);
        while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1);
        TCCx->CC[tcChannel].reg = (uint32_t)value;
        while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1);
        TCCx->PER.reg = 0xFF;
        while (TCCx->SYNCBUSY.bit.PER);
        TCCx->CTRLA.bit.ENABLE = 1;
        while (TCCx->SYNCBUSY.bit.ENABLE);
    }
}

void pwmWrite(uint32_t pin, uint32_t value)
{
    if (tcNums[pin] >= TCC_INST_NUM) 
    {
        Tc *TCx = (Tc *)GetTC(pinDescs[pin].ulPWMChannel);
        TCx->COUNT8.CC[tcChannels[pin]].reg = (uint8_t)value;
        while (TCx->COUNT8.SYNCBUSY.bit.CC0 || TCx->COUNT8.SYNCBUSY.bit.CC1);
    } 
    else 
    {
        Tcc *TCCx = (Tcc *)GetTC(pinDescs[pin].ulPWMChannel);
        while (TCCx->SYNCBUSY.bit.CTRLB);
        while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1);
        TCCx->CCBUF[tcChannels[pin]].reg = (uint32_t)value;
        while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1);
        TCCx->CTRLBCLR.bit.LUPD = 1;
        while (TCCx->SYNCBUSY.bit.CTRLB);
    }
}

void pwmDisable()
{
    for (uint8_t pin = 2; pin < 5; ++pin)
    {
        if (tcNums[pin] >= TCC_INST_NUM)
        {
            Tc *TCx = (Tc *)GetTC(pinDescs[pin].ulPWMChannel);
            TCx->COUNT8.CTRLA.bit.ENABLE = 0;
            while (TCx->COUNT8.SYNCBUSY.bit.ENABLE);
        }
        else
        {
            Tcc *TCCx = (Tcc *)GetTC(pinDescs[pin].ulPWMChannel);
            TCCx->CTRLA.bit.ENABLE = 0;
            while (TCCx->SYNCBUSY.bit.ENABLE);
        }
    }
}

void pwmEnable()
{
    for (uint8_t pin = 2; pin < 5; ++pin)
    {
        if (tcNums[pin] >= TCC_INST_NUM)
        {
            Tc *TCx = (Tc *)GetTC(pinDescs[pin].ulPWMChannel);
            TCx->COUNT8.CTRLA.bit.ENABLE = 1;
            while (TCx->COUNT8.SYNCBUSY.bit.ENABLE);
        }
        else
        {
            Tcc *TCCx = (Tcc *)GetTC(pinDescs[pin].ulPWMChannel);
            TCCx->CTRLA.bit.ENABLE = 1;
            while (TCCx->SYNCBUSY.bit.ENABLE);
        }
    }
}
