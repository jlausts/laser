#include "analogWrite.h"


uint32_t tcNums[20];
uint8_t tcChannels[20];
PinDescription pinDescs[20];

#define pin10on digitalWrite(10, HIGH);//PORT->Group[PORTA].OUTSET.reg = PORT_PA20
#define pin10of digitalWrite(10, LOW);//PORT->Group[PORTA].OUTCLR.reg = PORT_PA20


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

    int counter = pin == 6 || pin == 12 ? 1199: 0xFF;


    GCLK->PCHCTRL[GCLK_CLKCTRL_IDs[tcNum]].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | (1 << GCLK_PCHCTRL_CHEN_Pos);

    if (tcNum >= TCC_INST_NUM) {
        int divider = pin == 6 || pin == 12 ? TC_CTRLA_MODE_COUNT8 | TC_CTRLA_PRESCALER_DIV1 : TC_CTRLA_MODE_COUNT8 | TC_CTRLA_PRESCALER_DIV8;

        Tc *TCx = (Tc *)GetTC(pinDesc.ulPWMChannel);
        TCx->COUNT8.CTRLA.bit.SWRST = 1;
        while (TCx->COUNT8.SYNCBUSY.bit.SWRST);
        TCx->COUNT8.CTRLA.bit.ENABLE = 0;
        while (TCx->COUNT8.SYNCBUSY.bit.ENABLE);
        TCx->COUNT8.CTRLA.reg = divider;
        TCx->COUNT8.WAVE.reg = TC_WAVE_WAVEGEN_NPWM;
        while (TCx->COUNT8.SYNCBUSY.bit.CC0);
        TCx->COUNT8.CC[tcChannel].reg = (uint8_t)value;
        while (TCx->COUNT8.SYNCBUSY.bit.CC0);
        TCx->COUNT8.PER.reg = counter;
        while (TCx->COUNT8.SYNCBUSY.bit.PER);
        TCx->COUNT8.CTRLA.bit.ENABLE = 1;
        while (TCx->COUNT8.SYNCBUSY.bit.ENABLE);
    } 
    else {
        int divider = pin == 6 || pin == 12 ? TCC_CTRLA_PRESCALER_DIV1 | TCC_CTRLA_PRESCSYNC_GCLK : TCC_CTRLA_PRESCALER_DIV8 | TCC_CTRLA_PRESCSYNC_GCLK;
        Tcc *TCCx = (Tcc *)GetTC(pinDesc.ulPWMChannel);
        TCCx->CTRLA.bit.SWRST = 1;
        while (TCCx->SYNCBUSY.bit.SWRST);
        TCCx->CTRLA.bit.ENABLE = 0;
        while (TCCx->SYNCBUSY.bit.ENABLE);
        TCCx->CTRLA.reg = divider;
        TCCx->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
        while (TCCx->SYNCBUSY.bit.WAVE);
        while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1);
        TCCx->CC[tcChannel].reg = (uint32_t)value;
        while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1);
        TCCx->PER.reg = counter;
        while (TCCx->SYNCBUSY.bit.PER);
        TCCx->CTRLA.bit.ENABLE = 1;
        while (TCCx->SYNCBUSY.bit.ENABLE);
    }
}

void pwmWrite(uint32_t pin, uint32_t value)
{
    // if (tcNums[pin] >= TCC_INST_NUM) 
    // {
    //     Tc *TCx = (Tc *)GetTC(pinDescs[pin].ulPWMChannel);
    //     TCx->COUNT8.CC[tcChannels[pin]].reg = (uint8_t)value;
    //     while (TCx->COUNT8.SYNCBUSY.bit.CC0 || TCx->COUNT8.SYNCBUSY.bit.CC1);
    // } 
    // else 
    // {
        // pin10on;
        Tcc *TCCx = (Tcc *)GetTC(pinDescs[pin].ulPWMChannel);
        while (TCCx->SYNCBUSY.bit.CTRLB);
        while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1);
        TCCx->CCBUF[tcChannels[pin]].reg = (uint32_t)value;
        while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1);
        TCCx->CTRLBCLR.bit.LUPD = 1;
        while (TCCx->SYNCBUSY.bit.CTRLB);
        // pin10of;
    // }
}


// shoudl wait 10 us after this to write a new color
// takes 1.083 us or 130 clock cycles.
// very reliable time
void pwmWriteAll(const uint32_t r, const uint32_t g, const uint32_t b)
{
        Tcc *const TCCx1 = (Tcc *)GetTC(pinDescs[2].ulPWMChannel);
        Tcc *const TCCx2 = (Tcc *)GetTC(pinDescs[3].ulPWMChannel);
        Tcc *const TCCx3 = (Tcc *)GetTC(pinDescs[4].ulPWMChannel);
        // while (TCCx->SYNCBUSY.bit.CTRLB && TCCx2->SYNCBUSY.bit.CTRLB && TCCx3->SYNCBUSY.bit.CTRLB);
        // while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1 || TCCx2->SYNCBUSY.bit.CC0 || TCCx2->SYNCBUSY.bit.CC1 || TCCx3->SYNCBUSY.bit.CC0 || TCCx3->SYNCBUSY.bit.CC1);
        TCCx1->CCBUF[tcChannels[2]].reg = r;
        TCCx2->CCBUF[tcChannels[3]].reg = g;
        TCCx3->CCBUF[tcChannels[4]].reg = b;
        // while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1 || TCCx2->SYNCBUSY.bit.CC0 || TCCx2->SYNCBUSY.bit.CC1 || TCCx3->SYNCBUSY.bit.CC0 || TCCx3->SYNCBUSY.bit.CC1);
        TCCx1->CTRLBCLR.bit.LUPD = 1;
        TCCx2->CTRLBCLR.bit.LUPD = 1;
        TCCx3->CTRLBCLR.bit.LUPD = 1;
        // while (TCCx->SYNCBUSY.bit.CTRLB || TCCx2->SYNCBUSY.bit.CTRLB || TCCx3->SYNCBUSY.bit.CTRLB);
}

void pwmWriteAll(const uint32_t l, const uint32_t r)
{
        Tcc *const TCCx1 = (Tcc *)GetTC(pinDescs[6].ulPWMChannel);
        Tcc *const TCCx2 = (Tcc *)GetTC(pinDescs[12].ulPWMChannel);
        // while (TCCx->SYNCBUSY.bit.CTRLB && TCCx2->SYNCBUSY.bit.CTRLB && TCCx3->SYNCBUSY.bit.CTRLB);
        // while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1 || TCCx2->SYNCBUSY.bit.CC0 || TCCx2->SYNCBUSY.bit.CC1 || TCCx3->SYNCBUSY.bit.CC0 || TCCx3->SYNCBUSY.bit.CC1);
        TCCx1->CCBUF[tcChannels[6]].reg = l;
        TCCx2->CCBUF[tcChannels[12]].reg = r;
        // while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1 || TCCx2->SYNCBUSY.bit.CC0 || TCCx2->SYNCBUSY.bit.CC1 || TCCx3->SYNCBUSY.bit.CC0 || TCCx3->SYNCBUSY.bit.CC1);
        TCCx1->CTRLBCLR.bit.LUPD = 1;
        TCCx2->CTRLBCLR.bit.LUPD = 1;
        // while (TCCx->SYNCBUSY.bit.CTRLB || TCCx2->SYNCBUSY.bit.CTRLB || TCCx3->SYNCBUSY.bit.CTRLB);
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
