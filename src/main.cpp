#include "main.h"

void pulse10(const int num_pulses)
{
    for (int i = 0 ; i < num_pulses; ++i)
    {
        pin10on;
        delayMicroseconds(10);
        pin10of;
        delayMicroseconds(10);
    }
}

void p10(const int num_pulses)
{
    for (int i = 0 ; i < num_pulses; ++i)
    {
        pin10on;
        pin10of;
    }
}

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

    int counter = pin > 5 ? 1199: 0xFF; // audio counts to 1199, rgb counts to 255


    GCLK->PCHCTRL[GCLK_CLKCTRL_IDs[tcNum]].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | (1 << GCLK_PCHCTRL_CHEN_Pos);

    if (tcNum >= TCC_INST_NUM) {
        int divider = pin > 5 ? TC_CTRLA_MODE_COUNT8 | TC_CTRLA_PRESCALER_DIV1 : TC_CTRLA_MODE_COUNT8 | TC_CTRLA_PRESCALER_DIV8;

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
        int divider = pin > 5 ? TCC_CTRLA_PRESCALER_DIV1 | TCC_CTRLA_PRESCSYNC_GCLK : TCC_CTRLA_PRESCALER_DIV8 | TCC_CTRLA_PRESCSYNC_GCLK;
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

inline void write_color(volatile const Data *const info)
{
    // should wait 10 us after this to write a new color. takes 1.083 us or 130 clock cycles.
    Tcc *const TCCx1 = (Tcc *)GetTC(pinDescs[2].ulPWMChannel);
    Tcc *const TCCx2 = (Tcc *)GetTC(pinDescs[3].ulPWMChannel);
    Tcc *const TCCx3 = (Tcc *)GetTC(pinDescs[4].ulPWMChannel);
    TCCx1->CCBUF[tcChannels[2]].reg = info->r;
    TCCx2->CCBUF[tcChannels[3]].reg = info->g;
    TCCx3->CCBUF[tcChannels[4]].reg = info->b;
    TCCx1->CTRLBCLR.bit.LUPD = 1;
    TCCx2->CTRLBCLR.bit.LUPD = 1;
    TCCx3->CTRLBCLR.bit.LUPD = 1;
}

inline void write_audio(volatile const Data *const info)
{
    Tcc *const TCCx1 = (Tcc *)GetTC(pinDescs[6].ulPWMChannel);
    Tcc *const TCCx2 = (Tcc *)GetTC(pinDescs[12].ulPWMChannel);
    TCCx1->CCBUF[tcChannels[6]].reg = info->audio_l;
    TCCx2->CCBUF[tcChannels[12]].reg = info->audio_r;
    TCCx1->CTRLBCLR.bit.LUPD = 1;
    TCCx2->CTRLBCLR.bit.LUPD = 1;
}

inline void write_laser(volatile const Data *const info)
{
    DAC->DATA[0].reg = info->laser_x; 
    DAC->DATA[1].reg = info->laser_y; 
}

inline void unpack(volatile Data *i, uint8_t *tmp)
{
    // rgb is left shifted by 3 because it is 5 bit and we only use the most significant bits.
    i->r = ( tmp[0] & 0b00011111) << 3 | 0b111;
    i->g = ((tmp[0] & 0b11100000) >> 2) | ((tmp[1] & 0b00000011) << 6) | 0b111;
    i->b = ( tmp[1] & 0b01111100) << 1 | 0b111;

    // pull out the audio and laser position
    i->laser_x = (uint16_t)tmp[2] | ((uint16_t)(tmp[3] & 0x0F) << 8);
    i->laser_y = (uint16_t)(tmp[3] >> 4) | (((uint16_t)tmp[4]) << 4);
    i->audio_l = (uint16_t)tmp[5] | ((uint16_t)(tmp[6] & 0x0F) << 8);
    i->audio_r = (uint16_t)(tmp[6] >> 4) | (((uint16_t)tmp[7]) << 4);
    i->empty = false;

    // pull out the timestamp
    // i->t = tmp[8] | 
    //         ((uint16_t)tmp[9 ] << 8 ) | 
    //         ((uint32_t)tmp[10] << 16);// | 
            // ((uint32_t)tmp[11] << 24) | 
            // ((uint64_t)tmp[12] << 32) | 
            // ((uint64_t)tmp[13] << 40) | 
            // ((uint64_t)tmp[14] << 48) | 
            // ((uint64_t)tmp[15] << 56);
}

inline void wait_for_empty_array()
{
    // wait for the interrupts to use up the data.
    {
        volatile bool *const empty = &data[!array_reading][0].empty;
        while (!(*empty));
    }
}

inline void get_from_serial()
{
    static uint8_t serial_data[256];
    volatile Data *tmp_data = data[!array_reading];

    // 16 iterations
    for (uint16_t c = 0; c < 256;)
    {
        pin10on;
        // wait for data from the serial port
        while (!Serial.usb.available(CDC_ENDPOINT_OUT));
        pin10of;

        // pull 250 bytes from Serial (maxes out at 256)
        epHandlers[CDC_ENDPOINT_OUT]->recv(serial_data, 256);

        // 25 iterations
        for (uint16_t chunk = 0; chunk < 256; chunk += 8, ++c)
            unpack(&tmp_data[c], &serial_data[chunk]);
    }  
}

inline void pull_from_serial_to_array()
{
    wait_for_empty_array();
    get_from_serial();
}



void TimerHandler()
{
    // when this gets to 256, it will switch to the other data array, and the empty one will get filled
    static uint8_t array_count = 0;

    // when the uint8 rolls over, switch the arrays
    if (!array_count)
    {
        array_count = 0;
        array_reading = !array_reading;
        digitalWrite(9, array_reading);
    }

    // adjust pointer
    volatile Data *const info = &data[array_reading][array_count];

    // return if the timestamp has not matched the requirement
    // return if the info has already been used
    if (info->empty)
        return;
    
    // write to the ports
    write_color(info);
    write_laser(info);
    write_audio(info);

    // the current array address is nolonger valid
    info->empty = true;
    array_count++;
}

void setup()
{
    // prime DAC
    analogWriteResolution(12); 
    analogWrite(DAC_PIN0, 0);  
    analogWrite(DAC_PIN1, 0);  

    // setup RGB PWM
    pwmSetup(RGB_CH_RED, 0);
    pwmSetup(RGB_CH_GREEN, 0);
    pwmSetup(RGB_CH_BLUE, 0);

    // setup audio PWM
    pwmSetup(6, 150);  
    pwmSetup(12, 150); 

    Serial.begin(1000000);

    pinMode(9 , OUTPUT); // debugging pin
    pinMode(10, OUTPUT); // debugging pin
    pinMode(11, OUTPUT); // debugging pin
    pinMode(13, OUTPUT); // debugging pin

    get_from_serial(); 
    array_reading = !array_reading;
    get_from_serial(); 

    // setup ISR
    ITimer.attachInterruptInterval(TIMER_INTERVAL_US, TimerHandler);
}

void loop()
{
    pull_from_serial_to_array(); 
}



























