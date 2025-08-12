#include <Arduino.h>
#include "wiring_private.h"          
#include "SAMDTimerInterrupt.h"


#define DAC_PIN0 A0                   // only A0 has the DAC
#define DAC_PIN1 A1                   // only A0 has the DAC

// channel numbers inside TCC0 that correspond to our pins
#define RGB_CH_RED     2
#define RGB_CH_GREEN   3
#define RGB_CH_BLUE    4

#define pin10on {digitalWrite(10, HIGH);}//PORT->Group[PORTA].OUTSET.reg = PORT_PA20
#define pin10of {digitalWrite(10, LOW);}//PORT->Group[PORTA].OUTCLR.reg = PORT_PA20

#define pin11on {digitalWrite(11, HIGH);}//PORT->Group[PORTA].OUTSET.reg = PORT_PA20
#define pin11of {digitalWrite(11, LOW);}//PORT->Group[PORTA].OUTCLR.reg = PORT_PA20

#define pin13on {digitalWrite(13, HIGH);}//PORT->Group[PORTA].OUTSET.reg = PORT_PA20
#define pin13of {digitalWrite(13, LOW);}//PORT->Group[PORTA].OUTCLR.reg = PORT_PA20

#define pin9on {digitalWrite(9, HIGH);}//PORT->Group[PORTA].OUTSET.reg = PORT_PA20
#define pin9of {digitalWrite(9, LOW);}//PORT->Group[PORTA].OUTCLR.reg = PORT_PA20

#define SERCOM_PORT SERCOM5  // You can use any of the 6 available SERCOMs
extern EPHandler *epHandlers[7];

typedef struct color {
    int r;                    
    int g;                    
    int b;    
    int radd;
    int gadd;
    int badd;         
    const int minr;  // minimum red value
    const int ming;  // minimum green value
    const int minb;  // minimum blue value       
} Color;

Color color = { 90, 130, 70 , 1, 1, 1, 66, 92, 50}; // initial color values


// TCC OK => pin 4, 5, 6, 8, 9, 10, 11, 16/A2, 17/A3
// TC OK  => pin 12
// For ITSYBITSY_M4
// 16-bit Higher accuracy, Lower Frequency, PWM Pin OK: TCCx: 0-2, 4, 5, 7, 9-13
//  8-bit Lower  accuracy, Hi Frequency,    PWM Pin OK: TCx: 18-20, 24-25






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


#define TIMER_INTERVAL_US 25

// ISR
SAMDTimer ITimer(SELECTED_TIMER);

// for PWM ooptimization
uint32_t tcNums[20];
uint8_t tcChannels[20];
PinDescription pinDescs[20];

// one array gets filled as the other gets read from
volatile bool array_reading = true;

// increments every time the ISR gets triggered
volatile uint32_t i_count = 0;


typedef struct __attribute__((packed))
{
    bool empty;
    uint8_t r, g, b;
    uint16_t laser_x, laser_y;
} Data;

// one array is read while the other is filled from Serial
volatile Data data[2][256] = {0};