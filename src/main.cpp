

#include "main.h"

#define USE_SERIAL1_NOT
volatile uint16_t a5;

void blink()
{
    constexpr int del = 200;
    digitalWrite(LED_BUILTIN, HIGH);
    delay(del);
    digitalWrite(LED_BUILTIN, LOW);
    delay(del);
}

void blink(int count, int del)
{
    digitalWrite(LED_BUILTIN, HIGH);
    delay(del);
    digitalWrite(LED_BUILTIN, LOW);
    delay(del);
}

void blink(const int count)
{
    for (int i = 0; i < count; ++i)
        blink();
}

inline void pwmSetup(uint32_t pin, uint32_t value)
{
    static bool tcEnabled[TCC_INST_NUM + TC_INST_NUM];
    PinDescription pinDesc = g_APinDescription[pin];
    uint32_t attr = pinDesc.ulPinAttribute;

    if (!(attr & (PIN_ATTR_PWM_E | PIN_ATTR_PWM_F | PIN_ATTR_PWM_G))) return;

    uint32_t tcNum = GetTCNumber(pinDesc.ulPWMChannel);
    uint8_t tcChannel = GetTCChannelNumber(pinDesc.ulPWMChannel);

    tcNums    [pin] = tcNum;
    tcChannels[pin] = tcChannel;
    pinDescs  [pin] = pinDesc;

    if      (attr & PIN_ATTR_PWM_E) pinPeripheral(pin, PIO_TIMER);
    else if (attr & PIN_ATTR_PWM_F) pinPeripheral(pin, PIO_TIMER_ALT);
    else if (attr & PIN_ATTR_PWM_G) pinPeripheral(pin, PIO_TCC_PDEC);

    if (tcEnabled[tcNum]) return;  // Already setup
    tcEnabled[tcNum] = true;

    GCLK->PCHCTRL[GCLK_CLKCTRL_IDs[tcNum]].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | (1 << GCLK_PCHCTRL_CHEN_Pos);

    Tcc *TCCx = (Tcc *)GetTC(pinDesc.ulPWMChannel);
    TCCx->CTRLA.bit.SWRST = 1;
    while (TCCx->SYNCBUSY.bit.SWRST);
    TCCx->CTRLA.bit.ENABLE = 0;
    while (TCCx->SYNCBUSY.bit.ENABLE);


    // TCC_CTRLA_PRESCALER_DIV2 234 khz. no broken lines, but limited colors.
    // TCC_CTRLA_PRESCALER_DIV2 117 khz. slightly broken lines, Perhaps slightly more colors?
    // TCC_CTRLA_PRESCALER_DIV2 58 khz. has BROKEN LINES, DON'T USE


    TCCx->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV2 | TCC_CTRLA_PRESCSYNC_GCLK; // divider
    TCCx->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    while (TCCx->SYNCBUSY.bit.WAVE);
    while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1);
    TCCx->CC[tcChannel].reg = (uint32_t)value;
    while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1);
    TCCx->PER.reg = 0xFF; // counter
    while (TCCx->SYNCBUSY.bit.PER);
    TCCx->CTRLA.bit.ENABLE = 1;
    while (TCCx->SYNCBUSY.bit.ENABLE);
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

inline void write_laser(volatile const Data *const info)
{
    DAC->DATA[0].reg = info->laser_x; 
    DAC->DATA[1].reg = info->laser_y; 
}

inline void unpack(volatile Data *i, uint8_t *tmp)
{
    // rgb is left shifted by 3 because it is 5 bit and we only use the most significant bits.
    i->r =   tmp[0] << 3;
    i->g = ((tmp[0] & 0b11100000) >> 2) | (tmp[1] << 6);
    i->b =   tmp[1] & 0b11111000;

    // pull out laser position
    i->laser_x = (uint16_t)tmp[2] | ((uint16_t)(tmp[3] & 0x0F) << 8);
    i->laser_y = (uint16_t)(tmp[3] >> 4) | (((uint16_t)tmp[4]) << 4);
    i->empty = false;
}

inline void wait_for_empty_array()
{
    // wait for the interrupts to use up the data.
    volatile bool *const empty = &data[!array_reading][0].empty;
    while (!(*empty));
}

inline void get_from_serial()
{
    static uint8_t serial_data[5][255];
    volatile Data *const tmp_data = data[!array_reading];

    // grab all the data from serial all at once
    for (uint8_t b = 0; b < 5; ++b)
    {
        // pin10on;
        
#ifdef USE_SERIAL1
        while (!Serial1.available());
        Serial1.readBytes(serial_data, 255);
#else
        // wait for data from the serial port
        while (!Serial.usb.available(CDC_ENDPOINT_OUT)); 

        // pull 255 bytes from Serial (maxes out at 256)
        epHandlers[CDC_ENDPOINT_OUT]->recv(serial_data[b], 255); 
#endif
        // pin10of;
    }  

    // unpack the serial data
    for (uint8_t c = 0, b = 0; b < 5; ++b)
        for (uint8_t chunk = 0; chunk < 255; chunk += 5, ++c)
            unpack(&tmp_data[c], &serial_data[b][chunk]);
}

inline void pull_from_serial_to_array()
{
    wait_for_empty_array();
    get_from_serial();
}

static inline void setupADC_A5(void)
{
    Adc *const adc = ADC0;

    // Route the pin to the ADC peripheral once
    pinPeripheral(A5, PIO_ANALOG);

    // Disable before reconfig
    while (adc->SYNCBUSY.bit.ENABLE);
    adc->CTRLA.bit.ENABLE = 0;
    while (adc->SYNCBUSY.bit.ENABLE);

    // One sample, no averaging
    adc->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1 | ADC_AVGCTRL_ADJRES(0);

    // Minimal sample time (increase if your source impedance is high)
    adc->SAMPCTRL.reg = 0;

    // Fastest prescaler that’s stable on your board (start with DIV2, relax to DIV4 if needed)
    adc->CTRLA.bit.PRESCALER = ADC_CTRLA_PRESCALER_DIV8_Val;

    // Resolution (use _10BIT or _8BIT for even faster conversions)
    adc->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_12BIT_Val;

    // Free-run off (single-shot mode)
    adc->CTRLB.bit.FREERUN = 0;

    // Single-ended A5, negative to GND (set ONCE; do not rewrite in reads)
    adc->INPUTCTRL.reg =
        ADC_INPUTCTRL_MUXPOS(g_APinDescription[A5].ulADCChannelNumber) |
        ADC_INPUTCTRL_MUXNEG_GND;

    // Enable once
    while (adc->SYNCBUSY.reg);
    adc->CTRLA.bit.ENABLE = 1;
    while (adc->SYNCBUSY.reg);

    // One dummy conversion to settle after mux/ref changes
    adc->INTFLAG.reg = ADC_INTFLAG_RESRDY;
    adc->SWTRIG.bit.START = 1;
    while (adc->INTFLAG.bit.RESRDY == 0);
    (void)adc->RESULT.reg;
}

// about 2.75us per read.
static inline void readA5(volatile uint16_t *value)
{
    // Clear ready flag BEFORE starting
    (ADC0)->INTFLAG.reg = ADC_INTFLAG_RESRDY;

    // Start one conversion
    (ADC0)->SWTRIG.bit.START = 1;

    // Wait for completion
    while (!(ADC0)->INTFLAG.bit.RESRDY);

    // Read result
    *value = (ADC0)->RESULT.reg;
}

static inline void startReadA5()
{
    // Clear ready flag BEFORE starting
    (ADC0)->INTFLAG.reg = ADC_INTFLAG_RESRDY;

    // Start one conversion
    (ADC0)->SWTRIG.bit.START = 1;
}

static inline void finishReadA5(volatile uint16_t *value)
{
    // Wait for completion
    while (!(ADC0)->INTFLAG.bit.RESRDY);

    // Read result
    *value = (ADC0)->RESULT.reg;
}

// takes 1.2us
void TimerHandler()
{
    // when this gets to 255, it will switch to the other data array, and the empty one will get filled
    static uint8_t array_count = 0;

    startReadA5();

    // when the uint8 rolls over, switch the arrays
    if (array_count == 255)
    {
        array_count = 0;
        array_reading = !array_reading;
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

    // the current array address is nolonger valid
    info->empty = true;
    array_count++;
    finishReadA5(&a5);

}

void setup()
{
    setupADC_A5();

    // prime DAC
    analogWriteResolution(12); 
    analogWrite(DAC_PIN0, 0);  
    analogWrite(DAC_PIN1, 0);  

    // setup RGB PWM
    pwmSetup(RGB_CH_RED, 0);
    pwmSetup(RGB_CH_GREEN, 0);
    pwmSetup(RGB_CH_BLUE, 0);

#ifdef USE_SERIAL1
    Serial1.begin(2000000);    // 2 Mbps to match ESP32-S3
    Serial1.setTimeout(10); // 10 ms timeout
#else
    Serial.begin(1000000);
    while(!Serial);
    while(!Serial.available());
#endif

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
    // while (!a5);
    // Serial.println(a5);
    // a5 = 0;
    Serial.println(sine(4));
    pull_from_serial_to_array(); 
}

