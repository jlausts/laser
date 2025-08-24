#include "shape.h"

#define LEN 255
#define MAX_POSITION 4095

#define SET_COLOR data_array[j].empty = false; \
                  data_array[j].r  = input_data->r; \
                  data_array[j].g  = input_data->g; \
                  data_array[j].b  = input_data->b;

#define OFFSET data_array[j].laser_x += input_data->laser_x;\
               data_array[j].laser_y += input_data->laser_y;

#define CLAMP if (data_array[j].laser_y > MAX_POSITION) data_array[j].laser_y = MAX_POSITION;\
              if (data_array[j].laser_x > MAX_POSITION) data_array[j].laser_x = MAX_POSITION;

#define ROTATE_CLAMP rotate_point_and_clamp(&data_array[j], rotate_angle, cx, cy);

#define SET_Y5 data_array[j].laser_y = (uint16_t)((sine(k * yhz[0])) * yamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * yhz[1])) * yamp[1] + 0.5f)\
                                     + (uint16_t)((sine(k * yhz[2])) * yamp[2] + 0.5f)\
                                     + (uint16_t)((sine(k * yhz[3])) * yamp[3] + 0.5f)\
                                     + (uint16_t)((sine(k * yhz[3])) * yamp[4] + 0.5f);

#define SET_Y4 data_array[j].laser_y = (uint16_t)((sine(k * yhz[0])) * yamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * yhz[1])) * yamp[1] + 0.5f)\
                                     + (uint16_t)((sine(k * yhz[2])) * yamp[2] + 0.5f)\
                                     + (uint16_t)((sine(k * yhz[3])) * yamp[3] + 0.5f);
                                     
#define SET_Y3 data_array[j].laser_y = (uint16_t)((sine(k * yhz[0])) * yamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * yhz[1])) * yamp[1] + 0.5f)\
                                     + (uint16_t)((sine(k * yhz[2])) * yamp[2] + 0.5f);
                                     
#define SET_Y2 data_array[j].laser_y = (uint16_t)((sine(k * yhz[0])) * yamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * yhz[1])) * yamp[1] + 0.5f);
                                     
#define SET_Y1 data_array[j].laser_y = (uint16_t)((sine(k * yhz[0])) * yamp[0] + 0.5f);



#define SET_X5 data_array[j].laser_x = (uint16_t)((sine(k * xhz[0])) * xamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * xhz[1])) * xamp[1] + 0.5f)\
                                     + (uint16_t)((sine(k * xhz[2])) * xamp[2] + 0.5f)\
                                     + (uint16_t)((sine(k * xhz[3])) * xamp[3] + 0.5f)\
                                     + (uint16_t)((sine(k * xhz[3])) * xamp[4] + 0.5f);

#define SET_X4 data_array[j].laser_x = (uint16_t)((sine(k * xhz[0])) * xamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * xhz[1])) * xamp[1] + 0.5f)\
                                     + (uint16_t)((sine(k * xhz[2])) * xamp[2] + 0.5f)\
                                     + (uint16_t)((sine(k * xhz[3])) * xamp[3] + 0.5f);
                                     
#define SET_X3 data_array[j].laser_x = (uint16_t)((sine(k * xhz[0])) * xamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * xhz[1])) * xamp[1] + 0.5f)\
                                     + (uint16_t)((sine(k * xhz[2])) * xamp[2] + 0.5f);
                                     
#define SET_X2 data_array[j].laser_x = (uint16_t)((sine(k * xhz[0])) * xamp[0] + 0.5f)\
                                     + (uint16_t)((sine(k * xhz[1])) * xamp[1] + 0.5f);
                                     
#define SET_X1 data_array[j].laser_x = (uint16_t)((sine(k * xhz[0])) * xamp[0] + 0.5f);


#define pin10on {digitalWrite(10, HIGH);}//PORT->Group[PORTA].OUTSET.reg = PORT_PA20
#define pin10of {digitalWrite(10, LOW);}//PORT->Group[PORTA].OUTCLR.reg = PORT_PA20



extern volatile Data data[2][256];
extern volatile bool array_reading;

enum {XHZ, YHZ, RED, GREEN, BLUE, XOFF, YOFF, ROTATE, COLOR_ANGLE};


void rotate_point_and_clamp(volatile Data *const data_array, const float angle, const float cx, const float cy)
{
    const float dx = (float)(data_array->laser_x) - cx;
    const float dy = (float)(data_array->laser_y) - cy;
    const float sin_ = sine(angle);
    const float cos_ = cosine(angle);
    data_array->laser_x = (uint16_t)((cx + dx * cos_ - dy * sin_) + 0.5f);
    data_array->laser_y = (uint16_t)((cy + dx * sin_ + dy * cos_) + 0.5f);

    if (data_array->laser_x > MAX_POSITION) data_array->laser_x = MAX_POSITION;
    if (data_array->laser_y > MAX_POSITION) data_array->laser_y = MAX_POSITION;
}

void all_combinations(volatile Data *const data_array, const Data *const input_data, 
    const uint8_t x_count, const uint8_t y_count, 
    const int t, const float *const xhz, const float *const yhz, 
    const float *const xamp, const float *const yamp,
    const float rotate_angle, const float cx, const float cy)
{
    // Serial.println(input_data->laser_x);
    // Serial.println(input_data->laser_y);
    if (rotate_angle > 0.001f)
    {
        // no offset
        if (input_data->laser_x == 0 && input_data->laser_y == 0)
        {
            switch (x_count << 4 | y_count)
            {
            case 1 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y1; ROTATE_CLAMP; } break;
            case 1 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y2; ROTATE_CLAMP; } break;
            case 1 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y3; ROTATE_CLAMP; } break;
            case 1 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y4; ROTATE_CLAMP; } break;
            case 1 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y5; ROTATE_CLAMP; } break;

            case 2 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y1; ROTATE_CLAMP; } break;
            case 2 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y2; ROTATE_CLAMP; } break;
            case 2 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y3; ROTATE_CLAMP; } break;
            case 2 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y4; ROTATE_CLAMP; } break;
            case 2 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y5; ROTATE_CLAMP; } break;

            case 3 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y1; ROTATE_CLAMP; } break;
            case 3 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y2; ROTATE_CLAMP; } break;
            case 3 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y3; ROTATE_CLAMP; } break;
            case 3 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y4; ROTATE_CLAMP; } break;
            case 3 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y5; ROTATE_CLAMP; } break;

            case 4 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y1; ROTATE_CLAMP; } break;
            case 4 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y2; ROTATE_CLAMP; } break;
            case 4 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y3; ROTATE_CLAMP; } break;
            case 4 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y4; ROTATE_CLAMP; } break;
            case 4 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y5; ROTATE_CLAMP; } break;

            case 5 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y1; ROTATE_CLAMP; } break;
            case 5 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y2; ROTATE_CLAMP; } break;
            case 5 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y3; ROTATE_CLAMP; } break;
            case 5 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y4; ROTATE_CLAMP; } break;
            case 5 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y5; ROTATE_CLAMP; } break;

            default: break;
            }
        }
        
        // contains offset. (stored in laser_x&y)
        else
        {
            switch (x_count << 4 | y_count)
            {
            case 1 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 1 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 1 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 1 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 1 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            case 2 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 2 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 2 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 2 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 2 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            case 3 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 3 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 3 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 3 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 3 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            case 4 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 4 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 4 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 4 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 4 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            case 5 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 5 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 5 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 5 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 5 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            default: break;
            }
        }
    }
    else
    {
        // no offset.
        if (input_data->laser_x == 0 && input_data->laser_y == 0)
        {
            switch (x_count << 4 | y_count)
            {
            case 1 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y1; CLAMP; } break;
            case 1 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y2; CLAMP; } break;
            case 1 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y3; CLAMP; } break;
            case 1 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y4; CLAMP; } break;
            case 1 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y5; CLAMP; } break;

            case 2 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y1; CLAMP; } break;
            case 2 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y2; CLAMP; } break;
            case 2 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y3; CLAMP; } break;
            case 2 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y4; CLAMP; } break;
            case 2 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y5; CLAMP; } break;

            case 3 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y1; CLAMP; } break;
            case 3 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y2; CLAMP; } break;
            case 3 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y3; CLAMP; } break;
            case 3 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y4; CLAMP; } break;
            case 3 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y5; CLAMP; } break;

            case 4 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y1; CLAMP; } break;
            case 4 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y2; CLAMP; } break;
            case 4 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y3; CLAMP; } break;
            case 4 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y4; CLAMP; } break;
            case 4 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y5; CLAMP; } break;

            case 5 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y1; CLAMP; } break;
            case 5 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y2; CLAMP; } break;
            case 5 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y3; CLAMP; } break;
            case 5 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y4; CLAMP; } break;
            case 5 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y5; CLAMP; } break;

            default: break;
            }
        }
        
        // contains offset. (stored in laser_x&y)
        else
        {
            switch (x_count << 4 | y_count)
            {
            case 1 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y1; OFFSET; CLAMP; } break;
            case 1 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y2; OFFSET; CLAMP; } break;
            case 1 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y3; OFFSET; CLAMP; } break;
            case 1 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y4; OFFSET; CLAMP; } break;
            case 1 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y5; OFFSET; CLAMP; } break;

            case 2 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y1; OFFSET; CLAMP; } break;
            case 2 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y2; OFFSET; CLAMP; } break;
            case 2 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y3; OFFSET; CLAMP; } break;
            case 2 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y4; OFFSET; CLAMP; } break;
            case 2 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y5; OFFSET; CLAMP; } break;

            case 3 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y1; OFFSET; CLAMP; } break;
            case 3 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y2; OFFSET; CLAMP; } break;
            case 3 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y3; OFFSET; CLAMP; } break;
            case 3 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y4; OFFSET; CLAMP; } break;
            case 3 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y5; OFFSET; CLAMP; } break;

            case 4 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y1; OFFSET; CLAMP; } break;
            case 4 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y2; OFFSET; CLAMP; } break;
            case 4 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y3; OFFSET; CLAMP; } break;
            case 4 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y4; OFFSET; CLAMP; } break;
            case 4 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y5; OFFSET; CLAMP; } break;

            case 5 << 4 | 1:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y1; OFFSET; CLAMP; } break;
            case 5 << 4 | 2:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y2; OFFSET; CLAMP; } break;
            case 5 << 4 | 3:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y3; OFFSET; CLAMP; } break;
            case 5 << 4 | 4:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y4; OFFSET; CLAMP; } break;
            case 5 << 4 | 5:
                for (int j = 0, k = t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y5; OFFSET; CLAMP; } break;

            default: break;
            }
        }
    }
}
