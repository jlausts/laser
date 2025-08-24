#include "shape.h"

#define LEN 255
#define MAX_POSITION 4095
#define ISR_HZ 40000

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

constexpr float TAU = 6.28318530717958647f;
constexpr float HZ_MULT = TAU / (double)ISR_HZ;
constexpr float AMP_MULT = 4095.0 / 2.0;

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
    const uint64_t t, const float *const xhz, const float *const yhz, 
    const float *const xamp, const float *const yamp,
    const float rotate_angle, const float cx, const float cy)
{
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

void fill_array(const int len, const float *const arr, 
    const uint8_t *const types, const Data *const input_data, bool first_one)
{
    volatile Data *const data_array = data[!array_reading];
    static float xhz[5], yhz[5], xamp[5], yamp[5];
    float rotate_angle = 0, cx = 0, cy = 0;
    uint8_t x_count = 0, y_count = 0;
    static uint64_t t = 0;

    // unpack types and their variables
    for (int i = 0, type = 0; type < len; i++, type++)
    {
        switch (types[type])
        {
        case XHZ:
            xhz[x_count] = arr[i++] * HZ_MULT;
            xamp[x_count++] = AMP_MULT * arr[i];
            break;

        case YHZ:
            yhz[y_count] = arr[i++] * HZ_MULT;
            yamp[y_count++] = AMP_MULT * arr[i];
            break;

        case ROTATE:
            rotate_angle = arr[i++];
            cx = arr[i++];
            cy = arr[i++];
            break;

        default:
            break;
        }
    }

    if (first_one) t = 0;

    // calculate the position for each timestamp for any combination of xhz and yhz.
    all_combinations(data_array, input_data, x_count, y_count, 
        t, xhz, yhz, xamp, yamp, rotate_angle, cx, cy);

    t += LEN;
}


/*
period: 6,375 us (25us * 255)
2 hz = 268  us (4.2%)
5 hz = 517  us
6 hz = 665  us
10hz = 1000 us
offset += 30 us
rotate += 925 us

max time possible: 1945 us (30 %)
*/
void test_make_shapeOLD(const bool first)
{
    pin10on;
    // float arr[] = {4.1891910e+02, 2.5290594e-01, 2.5134238e+02, 1.2733901e-01, 4.1886707e+02, 3.2696864e-01, 2.5140036e+02, 2.8470719e-01, 2.5125056e+02, 1.7530885e-01, 8.3829834e+01, 2.4719754e-01, 6.0000000e+02, 6.0000000e+02, 255, 255, 255};
    // uint8_t types[] = {0, 0, 0, 1, 1, 1, 5, 6, 3, 2, 4};
    const float mul = 1;
    float arr[] = { 1050.6808 / mul, 0.3179864, 955.0732 / mul, 0.42970395, 
        1050.593 / mul, 0.25230965, 955.0194 / mul, 0.40079733, 1050.5621 / mul, 0.39920267,
        1050.9 / mul, 0.2};
    uint8_t types[] = { 0, 0, 0, 1, 1, 1 };
    Data tmp = {.r = 152, .g = 152, .b = 152, .laser_x = 0, .laser_y = 0};
        
    fill_array(sizeof(types), arr, types, &tmp, first);
    pin10of;
}