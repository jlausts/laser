#include "transitions.h"

#define LEN 255
#define MAX_POSITION 4095

#define Y_AMPLITUDE(i) info->yamp[i] * yamp + 0.5f
#define X_AMPLITUDE(i) info->xamp[i] * xamp + 0.5f

#define SET_Y5 data_array[j].laser_y = (uint16_t)((sine(k * info->yhz[0])) * Y_AMPLITUDE(0))\
                                     + (uint16_t)((sine(k * info->yhz[1])) * Y_AMPLITUDE(1))\
                                     + (uint16_t)((sine(k * info->yhz[2])) * Y_AMPLITUDE(2))\
                                     + (uint16_t)((sine(k * info->yhz[3])) * Y_AMPLITUDE(3))\
                                     + (uint16_t)((sine(k * info->yhz[3])) * Y_AMPLITUDE(4));

#define SET_Y4 data_array[j].laser_y = (uint16_t)((sine(k * info->yhz[0])) * Y_AMPLITUDE(0))\
                                     + (uint16_t)((sine(k * info->yhz[1])) * Y_AMPLITUDE(1))\
                                     + (uint16_t)((sine(k * info->yhz[2])) * Y_AMPLITUDE(2))\
                                     + (uint16_t)((sine(k * info->yhz[3])) * Y_AMPLITUDE(3));
                                     
#define SET_Y3 data_array[j].laser_y = (uint16_t)((sine(k * info->yhz[0])) * Y_AMPLITUDE(0))\
                                     + (uint16_t)((sine(k * info->yhz[1])) * Y_AMPLITUDE(1))\
                                     + (uint16_t)((sine(k * info->yhz[2])) * Y_AMPLITUDE(2));
                                     
#define SET_Y2 data_array[j].laser_y = (uint16_t)((sine(k * info->yhz[0])) * Y_AMPLITUDE(0))\
                                     + (uint16_t)((sine(k * info->yhz[1])) * Y_AMPLITUDE(1));
                                     
#define SET_Y1 data_array[j].laser_y = (uint16_t)((sine(k * info->yhz[0])) * Y_AMPLITUDE(0));



#define SET_X5 data_array[j].laser_x = (uint16_t)((sine(k * info->xhz[0])) * X_AMPLITUDE(0))\
                                     + (uint16_t)((sine(k * info->xhz[1])) * X_AMPLITUDE(1))\
                                     + (uint16_t)((sine(k * info->xhz[2])) * X_AMPLITUDE(2))\
                                     + (uint16_t)((sine(k * info->xhz[3])) * X_AMPLITUDE(3))\
                                     + (uint16_t)((sine(k * info->xhz[3])) * X_AMPLITUDE(4));

#define SET_X4 data_array[j].laser_x = (uint16_t)((sine(k * info->xhz[0])) * X_AMPLITUDE(0))\
                                     + (uint16_t)((sine(k * info->xhz[1])) * X_AMPLITUDE(1))\
                                     + (uint16_t)((sine(k * info->xhz[2])) * X_AMPLITUDE(2))\
                                     + (uint16_t)((sine(k * info->xhz[3])) * X_AMPLITUDE(3));
                                     
#define SET_X3 data_array[j].laser_x = (uint16_t)((sine(k * info->xhz[0])) * X_AMPLITUDE(0))\
                                     + (uint16_t)((sine(k * info->xhz[1])) * X_AMPLITUDE(1))\
                                     + (uint16_t)((sine(k * info->xhz[2])) * X_AMPLITUDE(2));
                                     
#define SET_X2 data_array[j].laser_x = (uint16_t)((sine(k * info->xhz[0])) * X_AMPLITUDE(0))\
                                     + (uint16_t)((sine(k * info->xhz[1])) * X_AMPLITUDE(1));
                                     
#define SET_X1 data_array[j].laser_x = (uint16_t)((sine(k * info->xhz[0])) * X_AMPLITUDE(0));



#define SET_COLOR data_array[j].empty = false; \
                  data_array[j].r  = info->r; \
                  data_array[j].g  = info->g; \
                  data_array[j].b  = info->b;


#define CLAMP if (data_array[j].laser_y > MAX_POSITION) data_array[j].laser_y = MAX_POSITION;\
              if (data_array[j].laser_x > MAX_POSITION) data_array[j].laser_x = MAX_POSITION;


#define pin10on {digitalWrite(10, HIGH);}//PORT->Group[PORTA].OUTSET.reg = PORT_PA20
#define pin10of {digitalWrite(10, LOW);}//PORT->Group[PORTA].OUTCLR.reg = PORT_PA20


extern volatile Data data[2][256];
extern volatile bool array_reading;

enum {XHZ, YHZ, RED, GREEN, BLUE, XOFF, YOFF, ROTATE, COLOR_ANGLE};