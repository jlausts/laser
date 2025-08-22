#include <Arduino.h>


#define ISR_HZ 40000

typedef struct 
{
    uint8_t r, g, b;
    uint16_t laser_x, laser_y;
} Data;


enum {XHZ, YHZ, RED, GREEN, BLUE, XOFF, YOFF, ROTATE, COLOR_ANGLE}TYPES;