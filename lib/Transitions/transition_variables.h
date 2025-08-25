#include "transitions.h"

#define LEN 255
#define MAX_POSITION 4095

#define SET_Y5 data_array[j].laser_y = (uint16_t)((sine(k * info->yhz[0])) * info->yamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * info->yhz[1])) * info->yamp[1] + 0.5f)\
                                     + (uint16_t)((sine(k * info->yhz[2])) * info->yamp[2] + 0.5f)\
                                     + (uint16_t)((sine(k * info->yhz[3])) * info->yamp[3] + 0.5f)\
                                     + (uint16_t)((sine(k * info->yhz[3])) * info->yamp[4] + 0.5f);

#define SET_Y4 data_array[j].laser_y = (uint16_t)((sine(k * info->yhz[0])) * info->yamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * info->yhz[1])) * info->yamp[1] + 0.5f)\
                                     + (uint16_t)((sine(k * info->yhz[2])) * info->yamp[2] + 0.5f)\
                                     + (uint16_t)((sine(k * info->yhz[3])) * info->yamp[3] + 0.5f);
                                     
#define SET_Y3 data_array[j].laser_y = (uint16_t)((sine(k * info->yhz[0])) * info->yamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * info->yhz[1])) * info->yamp[1] + 0.5f)\
                                     + (uint16_t)((sine(k * info->yhz[2])) * info->yamp[2] + 0.5f);
                                     
#define SET_Y2 data_array[j].laser_y = (uint16_t)((sine(k * info->yhz[0])) * info->yamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * info->yhz[1])) * info->yamp[1] + 0.5f);
                                     
#define SET_Y1 data_array[j].laser_y = (uint16_t)((sine(k * info->yhz[0])) * info->yamp[0] + 0.5f);



#define SET_X5 data_array[j].laser_x = (uint16_t)((sine(k * info->xhz[0])) * info->xamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * info->xhz[1])) * info->xamp[1] + 0.5f)\
                                     + (uint16_t)((sine(k * info->xhz[2])) * info->xamp[2] + 0.5f)\
                                     + (uint16_t)((sine(k * info->xhz[3])) * info->xamp[3] + 0.5f)\
                                     + (uint16_t)((sine(k * info->xhz[3])) * info->xamp[4] + 0.5f);

#define SET_X4 data_array[j].laser_x = (uint16_t)((sine(k * info->xhz[0])) * info->xamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * info->xhz[1])) * info->xamp[1] + 0.5f)\
                                     + (uint16_t)((sine(k * info->xhz[2])) * info->xamp[2] + 0.5f)\
                                     + (uint16_t)((sine(k * info->xhz[3])) * info->xamp[3] + 0.5f);
                                     
#define SET_X3 data_array[j].laser_x = (uint16_t)((sine(k * info->xhz[0])) * info->xamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * info->xhz[1])) * info->xamp[1] + 0.5f)\
                                     + (uint16_t)((sine(k * info->xhz[2])) * info->xamp[2] + 0.5f);
                                     
#define SET_X2 data_array[j].laser_x = (uint16_t)((sine(k * info->xhz[0])) * info->xamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * info->xhz[1])) * info->xamp[1] + 0.5f);
                                     
#define SET_X1 data_array[j].laser_x = (uint16_t)((sine(k * info->xhz[0])) * info->xamp[0] + 0.5f);



#define SET_COLOR data_array[j].empty = false; \
                  data_array[j].r  = info->r; \
                  data_array[j].g  = info->g; \
                  data_array[j].b  = info->b;

#define OFFSET data_array[j].laser_x += info->x_offset;\
               data_array[j].laser_y += info->y_offset;

#define CLAMP if (data_array[j].laser_y > MAX_POSITION) data_array[j].laser_y = MAX_POSITION;\
              if (data_array[j].laser_x > MAX_POSITION) data_array[j].laser_x = MAX_POSITION;

#define ROTATE_CLAMP rotate_point_and_clamp(&data_array[j], info->rotate_angle, info->center_x, info->center_y);

#define pin10on {digitalWrite(10, HIGH);}//PORT->Group[PORTA].OUTSET.reg = PORT_PA20
#define pin10of {digitalWrite(10, LOW);}//PORT->Group[PORTA].OUTCLR.reg = PORT_PA20


extern volatile Data data[2][256];
extern volatile bool array_reading;

enum {XHZ, YHZ, RED, GREEN, BLUE, XOFF, YOFF, ROTATE, COLOR_ANGLE};