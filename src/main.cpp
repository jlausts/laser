#include <Arduino.h>
#include "wiring_private.h"          // pinPeripheral(), etc.
#include <analogWrite.h>
#include <sin_table.h>
#include <motion.h>
// #include <motion2.h>
#include <audio.h>
#include <shapes.h>


#define DAC_PIN0 A0                   // only A0 has the DAC
#define DAC_PIN1 A1                   // only A0 has the DAC

// channel numbers inside TCC0 that correspond to our pins
#define RGB_CH_RED     2
#define RGB_CH_GREEN   3
#define RGB_CH_BLUE    4

#define pin8on PORT->Group[PORTB].OUTSET.reg = (1 << 10);
#define pin8off PORT->Group[PORTB].OUTCLR.reg = (1 << 10);  // Pin 8 is PB10


#define COLOR_OFF pwmWriteAll(0, 0, 0)

#define pin10on {digitalWrite(10, HIGH);}//PORT->Group[PORTA].OUTSET.reg = PORT_PA20
#define pin10of {digitalWrite(10, LOW);}//PORT->Group[PORTA].OUTCLR.reg = PORT_PA20


#define COLOR_ON pwmWriteAll(color.r, color.g, color.b)

typedef struct color {
    int r;                    
    int g;                    
    int b;    
    int radd;
    int gadd;
    int badd;         
    const int minr;  // minimum red value
    const int ming; // minimum green value
    const int minb;  // minimum blue value       
} Color;

// Color color = { 255, 255, 0 , 1, 1, 1, 66, 92, 50}; // initial color values
Color color = { 90, 130, 70 , 1, 1, 1, 66, 92, 50}; // initial color values









#include "SAMDTimerInterrupt.h"

#define USING_TIMER_TC3         true      // Only TC3 can be used for SAMD51
#define USING_TIMER_TC4         false     // Not to use with Servo library
#define USING_TIMER_TC5         false
#define USING_TIMER_TCC         false
#define USING_TIMER_TCC1        false
#define USING_TIMER_TCC2        false     // Don't use this, can crash on some boards

#if USING_TIMER_TC3
  #define SELECTED_TIMER      TIMER_TC3
#elif USING_TIMER_TC4
  #define SELECTED_TIMER      TIMER_TC4
#elif USING_TIMER_TC5
  #define SELECTED_TIMER      TIMER_TC5
#elif USING_TIMER_TCC
  #define SELECTED_TIMER      TIMER_TCC
#elif USING_TIMER_TCC1
  #define SELECTED_TIMER      TIMER_TCC1
#elif USING_TIMER_TCC2
  #define SELECTED_TIMER      TIMER_TCC
#else
  #error You have to select 1 Timer  
#endif

// Init selected SAMD timer
SAMDTimer ITimer(SELECTED_TIMER);

#define TIMER_INTERVAL_US        20








/*********************************************************************
  Enable the DWT cycle counter once at boot.
*********************************************************************/
void enableCycleCounter()
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  // Enable trace
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;             // Start the counter
}

bool on;
void TimerHandler()
{
    on = !on;
    if (on)
        {pin10on;}
    else
        {pin10of;}
}

void setup()
{
    analogWriteResolution(12);         // 0-4095
    analogWrite(DAC_PIN0, 0);          // prime hardware
    analogWrite(DAC_PIN1, 0);          // prime hardware

    pwmSetup(RGB_CH_RED, color.r);
    pwmSetup(RGB_CH_GREEN, color.g);
    pwmSetup(RGB_CH_BLUE, color.b);
    pinMode(10, OUTPUT);

    Serial.begin(115200);
    Serial.println("Starting up...");

    if (ITimer.attachInterruptInterval(TIMER_INTERVAL_US, TimerHandler))
    {
        Serial.print(F("Starting  ITimer OK"));
    }
    else
        Serial.println(F("Can't set ITimer. Select another freq. or timer"));

    // enableCycleCounter();
}

uint16_t wave(uint16_t t, const uint16_t max_val = 4095) {
    if (t < max_val)          return t;
    else if (t < 2 * max_val) return 2 * max_val - t;
    else if (t < 3 * max_val) return t - 2 * max_val;
    else                      return 4 * max_val - t;
}



// 1.358 us or 163 clock cycles
void change_color()
{
    // pin10on;
    pwmWriteAll(color.r += color.radd, color.g += color.gadd, color.b += color.badd);

    if (color.r == 255) color.radd = -1;  
    if (color.r == color.minr) color.radd = 1;     
    if (color.g == 255) color.gadd = -1;  
    if (color.g == color.ming) color.gadd = 1;     
    if (color.b == 255) color.badd = -1;  
    if (color.b == color.minb) color.badd = 1;   
    // pin10of;
}


void square()
{
    const uint16_t max_val = 4095;
    const uint16_t quarter_cycle = 2048;
    const int step = 1;

    for (uint16_t t = 0; t < 4 * max_val; t+=step) 
    {
        uint16_t a = wave(t);
        uint16_t b = wave((t + quarter_cycle) % (4 * max_val));
        WRITE_BOTH(a, b);
    }
    change_color();
}

void randLines()
{
    for (int i = 0; i < 100; ++i)
        line(random(0, 4096), random(0, 4096), random(0, 4096), random(0, 4096), 100000); 
}

void circles()
{
    for (int x = 0; x < 4096; x += 200)
    {   
        line(0, 0, 4096, 0, 10000);
        line(4096, 0, 4096, 4096, 10000);
        line(4096, 4096, 0, 4096, 10000);
        line(0, 4096, 0, 0, 10000);
        circle(900, x, 2048, 10000);
    }
}

void move(int x1, int y1, int x2, int y2, int steps)
{
    COLOR_OFF;
    for (int i = 0; i <= steps; ++i)
    {
        const float t = 1 - ((float)i / steps);       
        const float ease = 1 - t * t; 

        const int x = x1 + (x2 - x1) * ease;
        const int y = y1 + (y2 - y1) * ease;

        WRITE_BOTH(x, y);
    }
    COLOR_ON;
}

void randArcs()
{
    for (int i = 0; i < 4096 - 100; i += 400)
    {
        line_decel(0, i, 4096, i, 3000);
        if (i > 3500)
            move(4096, i, 0, 0, 1000);
        else
            move(4096, i, 0, i+400, 320);
    }
}

void randArcsold()
{
    WRITE_BOTH(4096, 0);
    delayMicroseconds(1000);

    for (int i = 0; i < 4096 - 100; i += 500)
    {
        WRITE_BOTH(4096, i);
        delayMicroseconds(500);
        COLOR_ON;
        line(0, i, 4096, i, 3000);
        COLOR_OFF;
    }
}





void loop()
{

    audio[0] = new SAMD_PWM(5, 100000, 50);
    audio[0]->setPWM();
    audio[1] = new SAMD_PWM(7, 100000, 25);
    audio[1]->setPWM();


    // classd_pwm_init(400000);   /* 400 kHz carrier (e.g. 120 MHz / 300) */
    // while (1)
    // {
    //     /* Example: simple 1 kHz triangle on both channels */
    //     for (uint16_t d = 0; d <= _top_ticks; d += 10)
    //     {
    //         classd_pwm_update(d, d);
    //         delayMicroseconds(25);  /* audio update rate: ~40 kHz */
    //     }
    //     for (int32_t d = _top_ticks; d >= 0; d -= 10)
    //     {
    //         classd_pwm_update(d, d);
    //         delayMicroseconds(25);
    //     }
    // }



    while(1){square();}

    
    // while(1) draw_quad(2000, 0,
    //                     4095, 1000,
    //                     4095, 4095,
    //                     0, 4095,
    //                     1000000);
    for (int x = 0; x < 4000; x += 10)
    {
        ellipse_arc(x, 2048, 1000, 500, 0, 4, 8000);
    }
    loop();


    circle(0.01f, 2048, 2048, 100);
    COLOR_ON;
    for (float rad = 0.05; rad <= 1.0f; rad += 0.05f)
    {
        circle(rad, 2048, 2048, 100);
    }
    COLOR_OFF;
    delayMicroseconds(10);
}



























