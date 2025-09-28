

#include "main.h"

#define USE_SERIAL1_NOT
#define USE_SERIALd
volatile uint16_t a5;
#define BLINK_PIN 13


void set_seed() 
{
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_TRNG;
    TRNG->CTRLA.reg = TRNG_CTRLA_ENABLE;
    while ((TRNG->INTFLAG.reg & TRNG_INTFLAG_DATARDY) == 0) { }
    randomSeed(TRNG->DATA.reg);
}

void blink(int del)
{
    digitalWrite(BLINK_PIN, HIGH);
    delayMicroseconds(del);
    digitalWrite(BLINK_PIN, LOW);
    delayMicroseconds(del);
}

void blink(const int count, const int del)
{
    for (int i = 0; i < count; ++i)
        blink(del);
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

void make_shape(const bool new_shape, const float amp_mult, const uint16_t x_offset, const uint16_t y_offset, const float rotate_angle)
{
    static ChordInfo info = {.r = 152, .g = 152, .b = 152, .center_x = 2048, .center_y = 2048, .rotate_angle = 0};
    info.x_offset = x_offset;
    info.y_offset = y_offset;
    info.rotate_angle = rotate_angle;

    if (new_shape)
    {    
        info.t = 0;
        make_chord(&info);
        random_color(&info);
    }

    for (uint8_t i = 0; i < info.x_count; ++i)
        info.xamp[i] = info.xamp1[i] * amp_mult;
    for (uint8_t i = 0; i < info.y_count; ++i)
        info.yamp[i] = info.yamp1[i] * amp_mult;

    all_combinations(data[!array_reading], &info);
    info.t += 255;
}

void wait_then_make(const bool new_shape, const float amp_mult, const uint16_t x_offset, const uint16_t y_offset, const float rotate_angle)
{
    wait_for_empty_array();
    make_shape(new_shape, amp_mult, x_offset, y_offset, rotate_angle);
}

void make_shapes()
{
    while(1)
    {
        wait_then_make(true, 0, 2048, 2048, 0);

        const float max_amp = .8;
        const float start_grow_speed = 0.005f;
        const float end_shrink_speed = 0.005f;
        const int stable_time = 512;

        // random angle -.25 -> +.25
        const float total_angle = ((float)(rand() & 1023) / 2048.0f - 0.25f) * TAU;
        const float total_steps = (1.0f / start_grow_speed) + (1.0f / end_shrink_speed) + stable_time;
        const float angle_step = total_angle / total_steps;
        float angle = 3 * TAU;

        float offset_add = (1 - max_amp) * 0.5f * 4096;
        for (float amp_mult = 0; amp_mult < 1; amp_mult += start_grow_speed, angle += angle_step)
        {
            const float new_amp = sine(((amp_mult + 1.5f) * 0.5f) * TAU) * 0.5f;
            const float offset = (1.0f - new_amp) * 2048;
            wait_then_make(false, new_amp * max_amp, offset * max_amp + offset_add, offset * max_amp + offset_add, angle);
        }
        
        for (int i = 0; i < stable_time; ++i, angle += angle_step)
            wait_then_make(false, max_amp, offset_add, offset_add, angle);
        
        for (float amp_mult = 0; amp_mult < 1; amp_mult += end_shrink_speed, angle += angle_step)
        {
            const float new_amp = sine(((amp_mult + 0.5f) * 0.5f) * TAU) * 0.5f;
            const float offset = (1.0f - new_amp) * 2048;
            wait_then_make(false, new_amp * max_amp, offset * max_amp + offset_add, offset * max_amp + offset_add, angle);
        }

        wait_then_make(false, 0, 2048, 2048, angle);
    }
}

// run this before the ISR starts up
void calibration_square(const int delay_time = 1000, const uint8_t light_intensity = 255)
{
    volatile Data info = {.r=light_intensity, .g=light_intensity, .b=light_intensity};
    write_color(&info);
    while(1)
    {
        info.laser_x = 0;
        info.laser_y = 0;
        write_laser(&info);
        delayMicroseconds(delay_time);
        
        info.laser_x = 4095;
        info.laser_y = 0;
        write_laser(&info);
        delayMicroseconds(delay_time);

        info.laser_x = 4095;
        info.laser_y = 4095;
        write_laser(&info);
        delayMicroseconds(delay_time);

        info.laser_x = 0;
        info.laser_y = 4095;
        write_laser(&info);
        delayMicroseconds(delay_time);
    }
}

// takes 3.211us
// runs 12.8% of the time.
void TimerHandler()
{
    // pin10on;
    // when this gets to 255, it will switch to the other data array, and the empty one will get filled
    static uint8_t array_count = 0;

    startReadA5();

    // when the uint8 rolls over, switch the arrays
    if (array_count == 255)
    {
        array_count = 0;
        array_reading = !array_reading;
        // pin10on;
        // pin10of;
    }

    // adjust pointer
    volatile Data *const info = &data[array_reading][array_count];

    // return if the timestamp has not matched the requirement
    // return if the info has already been used
    if (info->empty)
        return;
    
    // write to the ports
    write_color(&data[array_reading][0]);
    write_laser(info);

    // the current array address is nolonger valid
    info->empty = true;
    array_count++;
    finishReadA5(&a5);
    // pin10of;
}

void setup()
{
    set_seed();
    setupADC_A5();

    // prime DAC
    analogWriteResolution(12); 
    analogWrite(DAC_PIN0, 0);  
    analogWrite(DAC_PIN1, 0);  

    // setup RGB PWM
    pwmSetup(RGB_CH_RED, 0);
    pwmSetup(RGB_CH_GREEN, 0);
    pwmSetup(RGB_CH_BLUE, 0);

    // calibration_square();

#ifdef USE_SERIAL1
    Serial1.begin(2000000);    // 2 Mbps to match ESP32-S3
    Serial1.setTimeout(10); // 10 ms timeout
#else
    Serial.begin(1000000);
    // while(!Serial);
    // while(!Serial.available());
#endif

    pinMode(9 , OUTPUT); // debugging pin
    pinMode(10, OUTPUT); // debugging pin
    // pinMode(11, OUTPUT); // debugging pin
    // pinMode(13, OUTPUT); // debugging pin

#ifdef USE_SERIAL
    get_from_serial(); 
    array_reading = !array_reading;
    get_from_serial(); 
#else
    make_shape(true, 0, 0, 0, 0);
    array_reading = !array_reading;
    make_shape(false, 0, 0, 0, 0);

#endif
    // setup ISR
    ITimer.attachInterruptInterval(TIMER_INTERVAL_US, TimerHandler);
}

void loop()
{





#ifdef USE_SERIAL
    pull_from_serial_to_array(); 
#else
    flow();
    // make_shapes();
#endif
}



/*
TODO
set up a double rotation: shape spins, and orbits the center. like a planet.
MAYBE: smoothe out the amplitude changer by calculating it for every position?
think of a better formula to ensure only the cool shapes are created.
transistion one HZ out of the shape, and bring a new one into the shape.
    the new shape will have new aplitudes for all the other HZ's but not by much.
    cosine interpolate between the old AMPS and the new ones.
*/