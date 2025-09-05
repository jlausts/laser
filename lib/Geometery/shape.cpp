#include "shape.h"

#define LEN 255
#define MAX_POSITION 4095

#define SET_COLOR data_array[j].empty = false; \
                  data_array[j].r  = info->r; \
                  data_array[j].g  = info->g; \
                  data_array[j].b  = info->b;

#define OFFSET data_array[j].laser_x += info->x_offset;\
               data_array[j].laser_y += info->y_offset;

#define CLAMP if (data_array[j].laser_y > MAX_POSITION) data_array[j].laser_y = MAX_POSITION;\
              if (data_array[j].laser_x > MAX_POSITION) data_array[j].laser_x = MAX_POSITION;

#define ROTATE_CLAMP rotate_point_and_clamp(&data_array[j], info->rotate_angle, info->center_x, info->center_y);

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


#define pin10on {digitalWrite(10, HIGH);}//PORT->Group[PORTA].OUTSET.reg = PORT_PA20
#define pin10of {digitalWrite(10, LOW);}//PORT->Group[PORTA].OUTCLR.reg = PORT_PA20



extern volatile Data data[2][256];
extern volatile bool array_reading;

enum {XHZ, YHZ, RED, GREEN, BLUE, XOFF, YOFF, ROTATE, COLOR_ANGLE};


// ensures the color is always bright enough and different from the last one.
void random_color(ChordInfo *const info)
{
    uint8_t r = info->r << 3, g = info->g << 3, b = info->b << 3;
    uint8_t difference;
    do
    {
        info->r = random(5, 32);
        info->g = random(14, 32);
        info->b = random(7, 32);
        difference = (info->r > r ? info->r - r : r - info->r) + 
                     (info->g > g ? info->g - g : g - info->g) + 
                     (info->b > b ? info->b - b : b - info->b);

    } while ((((info->r - 5) + (info->g - 14) + (info->b - 7)) <= 6) && difference > 9);
    
    info->r <<= 3;
    info->g <<= 3;
    info->b <<= 3;
    // info->r = 255;
    // info->g = 255;
    // info->b = 255;
}

void rotate_point_and_clamp(volatile Data *const data_array, const float angle, const float cx, const float cy)
{
    const float dx = (float)(data_array->laser_x) - cx;
    const float dy = (float)(data_array->laser_y) - cy;
    const float sin_ = sine(angle) - 1.0f;
    const float cos_ = cosine(angle) - 1.0f;
    // Serial.println(String((uint16_t)((cx + dx * cos_ - dy * sin_) + 0.5f)) + " " + String(data_array->laser_y));
    data_array->laser_x = (uint16_t)((cx + dx * cos_ - dy * sin_) + 0.5f);
    data_array->laser_y = (uint16_t)((cy + dx * sin_ + dy * cos_) + 0.5f);

    if (data_array->laser_x > MAX_POSITION) data_array->laser_x = MAX_POSITION;
    if (data_array->laser_y > MAX_POSITION) data_array->laser_y = MAX_POSITION;
}

void all_combinations(volatile Data *const data_array, const ChordInfo *const info)
{
    if (info->rotate_angle > 0.000001f)
    {
        // no offset
        if (info->x_offset == 0 && info->y_offset == 0)
        {
            switch (info->x_count << 4 | info->y_count)
            {
            case 1 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y1; ROTATE_CLAMP; } break;
            case 1 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y2; ROTATE_CLAMP; } break;
            case 1 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y3; ROTATE_CLAMP; } break;
            case 1 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y4; ROTATE_CLAMP; } break;
            case 1 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y5; ROTATE_CLAMP; } break;

            case 2 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y1; ROTATE_CLAMP; } break;
            case 2 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y2; ROTATE_CLAMP; } break;
            case 2 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y3; ROTATE_CLAMP; } break;
            case 2 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y4; ROTATE_CLAMP; } break;
            case 2 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y5; ROTATE_CLAMP; } break;

            case 3 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y1; ROTATE_CLAMP; } break;
            case 3 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y2; ROTATE_CLAMP; } break;
            case 3 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y3; ROTATE_CLAMP; } break;
            case 3 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y4; ROTATE_CLAMP; } break;
            case 3 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y5; ROTATE_CLAMP; } break;

            case 4 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y1; ROTATE_CLAMP; } break;
            case 4 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y2; ROTATE_CLAMP; } break;
            case 4 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y3; ROTATE_CLAMP; } break;
            case 4 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y4; ROTATE_CLAMP; } break;
            case 4 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y5; ROTATE_CLAMP; } break;

            case 5 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y1; ROTATE_CLAMP; } break;
            case 5 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y2; ROTATE_CLAMP; } break;
            case 5 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y3; ROTATE_CLAMP; } break;
            case 5 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y4; ROTATE_CLAMP; } break;
            case 5 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y5; ROTATE_CLAMP; } break;

            default: break;
            }
        }
        
        // contains offset. (stored in laser_x&y)
        else
        {
            switch (info->x_count << 4 | info->y_count)
            {
            case 1 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 1 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 1 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 1 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 1 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            case 2 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 2 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 2 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 2 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 2 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            case 3 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 3 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 3 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 3 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 3 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            case 4 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 4 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 4 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 4 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 4 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            case 5 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 5 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 5 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 5 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 5 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            default: break;
            }
        }
    }
    else
    {
        // no offset.
        if (info->x_offset == 0 && info->y_offset == 0)
        {
            switch (info->x_count << 4 | info->y_count)
            {
            case 1 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y1; CLAMP; } break;
            case 1 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y2; CLAMP; } break;
            case 1 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y3; CLAMP; } break;
            case 1 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y4; CLAMP; } break;
            case 1 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y5; CLAMP; } break;

            case 2 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y1; CLAMP; } break;
            case 2 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y2; CLAMP; } break;
            case 2 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y3; CLAMP; } break;
            case 2 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y4; CLAMP; } break;
            case 2 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y5; CLAMP; } break;

            case 3 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y1; CLAMP; } break;
            case 3 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y2; CLAMP; } break;
            case 3 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y3; CLAMP; } break;
            case 3 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y4; CLAMP; } break;
            case 3 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y5; CLAMP; } break;

            case 4 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y1; CLAMP; } break;
            case 4 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y2; CLAMP; } break;
            case 4 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y3; CLAMP; } break;
            case 4 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y4; CLAMP; } break;
            case 4 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y5; CLAMP; } break;

            case 5 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y1; CLAMP; } break;
            case 5 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y2; CLAMP; } break;
            case 5 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y3; CLAMP; } break;
            case 5 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y4; CLAMP; } break;
            case 5 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y5; CLAMP; } break;

            default: break;
            }
        }
        
        // contains offset. (stored in laser_x&y)
        else
        {
            switch (info->x_count << 4 | info->y_count)
            {
            case 1 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y1; OFFSET; CLAMP; } break;
            case 1 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y2; OFFSET; CLAMP; } break;
            case 1 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y3; OFFSET; CLAMP; } break;
            case 1 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y4; OFFSET; CLAMP; } break;
            case 1 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X1; SET_Y5; OFFSET; CLAMP; } break;

            case 2 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y1; OFFSET; CLAMP; } break;
            case 2 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y2; OFFSET; CLAMP; } break;
            case 2 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y3; OFFSET; CLAMP; } break;
            case 2 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y4; OFFSET; CLAMP; } break;
            case 2 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X2; SET_Y5; OFFSET; CLAMP; } break;

            case 3 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y1; OFFSET; CLAMP; } break;
            case 3 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y2; OFFSET; CLAMP; } break;
            case 3 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y3; OFFSET; CLAMP; } break;
            case 3 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y4; OFFSET; CLAMP; } break;
            case 3 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X3; SET_Y5; OFFSET; CLAMP; } break;

            case 4 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y1; OFFSET; CLAMP; } break;
            case 4 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y2; OFFSET; CLAMP; } break;
            case 4 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y3; OFFSET; CLAMP; } break;
            case 4 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y4; OFFSET; CLAMP; } break;
            case 4 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X4; SET_Y5; OFFSET; CLAMP; } break;

            case 5 << 4 | 1:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y1; OFFSET; CLAMP; } break;
            case 5 << 4 | 2:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y2; OFFSET; CLAMP; } break;
            case 5 << 4 | 3:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y3; OFFSET; CLAMP; } break;
            case 5 << 4 | 4:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y4; OFFSET; CLAMP; } break;
            case 5 << 4 | 5:
                for (int j = 0, k = info->t; j < LEN; ++j, ++k) { SET_COLOR; SET_X5; SET_Y5; OFFSET; CLAMP; } break;

            default: break;
            }
        }
    }
}

