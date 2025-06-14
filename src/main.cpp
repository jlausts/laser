#include <Arduino.h>
#include "wiring_private.h"          // pinPeripheral(), etc.
#include <analogWrite.h>
#include <sin_table.h>
#include <shapes.h>


#define DAC_PIN0 A0                   // only A0 has the DAC
#define DAC_PIN1 A1                   // only A0 has the DAC

// channel numbers inside TCC0 that correspond to our pins
#define RGB_CH_RED     2
#define RGB_CH_GREEN   3
#define RGB_CH_BLUE    4

#define COLOR_OFF \
{\
    pwmWrite(RGB_CH_RED, 0);\
    pwmWrite(RGB_CH_GREEN, 0);\
    pwmWrite(RGB_CH_BLUE, 0);\
}

#define COLOR_ON \
{\
    pwmWrite(RGB_CH_RED, color.r);\
    pwmWrite(RGB_CH_GREEN, color.g);\
    pwmWrite(RGB_CH_BLUE, color.b);\
}

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
Color color = { 100, 100, 100 , 1, 1, 1, 66, 92, 50}; // initial color values




void setup()
{
    analogWriteResolution(12);         // 0-4095
    analogWrite(DAC_PIN0, 0);          // prime hardware
    analogWrite(DAC_PIN1, 0);          // prime hardware

    pwmSetup(RGB_CH_RED, color.r);
    pwmSetup(RGB_CH_GREEN, color.g);
    pwmSetup(RGB_CH_BLUE, color.b);

    Serial.begin(115200);
    Serial.println("Starting up...");
}

uint16_t wave(uint16_t t, const uint16_t max_val = 4095) {
    if (t < max_val)          return t;
    else if (t < 2 * max_val) return 2 * max_val - t;
    else if (t < 3 * max_val) return t - 2 * max_val;
    else                      return 4 * max_val - t;
}

void change_color()
{
    pwmWrite(RGB_CH_RED, color.r += color.radd);
    pwmWrite(RGB_CH_GREEN, color.g += color.gadd);
    pwmWrite(RGB_CH_BLUE, color.b += color.badd);

    if (color.r == 255) color.radd = -1;  
    if (color.r == color.minr) color.radd = 1;     
    if (color.g == 255) color.gadd = -1;  
    if (color.g == color.ming) color.gadd = 1;     
    if (color.b == 255) color.badd = -1;  
    if (color.b == color.minb) color.badd = 1;   
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


void randArcs()
{
    for (int i = 200; i < 3500; i += 200)
    {
        
        line(0, 0, 4096, 0, 10000);
        line(4096, 0, 4096, 4096, 10000);
        line(4096, 4096, 0, 4096, 10000);
        line(0, 4096, 0, 0, 10000);

        arc_three(
            0, 2048, 
            2048, i, 
            3900, 2048, 
            300000); 
    }
}

void loop()
{

    while(1) randArcs();


    circle(0.01f, 2048, 2048, 100);
    COLOR_ON;
    for (float rad = 0.05; rad <= 1.0f; rad += 0.05f)
    {
        circle(rad, 2048, 2048, 100);
    }
    COLOR_OFF;
    delayMicroseconds(10);
}



