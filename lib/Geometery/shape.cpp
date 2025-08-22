#include "shape.h"

#define LEN 255
#define MAX_POSITION 4095
#define ISR_HZ 40000

typedef struct __attribute__((packed))
{
    bool empty;
    uint8_t r, g, b;
    uint16_t laser_x, laser_y;
} Data;

extern volatile Data data[2][256];
extern volatile bool array_reading;

enum {XHZ, YHZ, RED, GREEN, BLUE, XOFF, YOFF, ROTATE, COLOR_ANGLE};

constexpr float TAU = 6.28318530717958647f;
constexpr float HZ_MULT = TAU / (double)ISR_HZ;
constexpr float AMP_MULT = 4095.0 / 2.0;

void rotate_point(volatile Data *const data_array, const float angle, const float cx, const float cy)
{
    const float dx = (float)(data_array->laser_x) - cx;
    const float dy = (float)(data_array->laser_y) - cy;
    const float sin_ = sine(angle);
    const float cos_ = cosine(angle);
    data_array->laser_x = (uint16_t)((cx + dx * cos_ - dy * sin_) + 0.5f);
    data_array->laser_y = (uint16_t)((cy + dx * sin_ + dy * cos_) + 0.5f);
}

void fill_array(const int len, const float *const arr, const uint8_t *const types, bool first_one)
{
    static uint64_t t = 0;
    float amp, p;
    volatile Data *const data_array = data[!array_reading];
    memset((void*)data_array, 0, sizeof(data[0]));

    if (first_one)
        t = 0;

    for (int i = 0, type = 0; type < len; ++i, type++)
    {
        switch (types[type])
        {
        case XHZ:
            amp = AMP_MULT * arr[i+1];
            p = arr[i] * HZ_MULT;
            for (int j = 0, k = t; j < LEN; ++j, ++k)
                data_array[j].laser_x += (uint16_t)((sine(k * p) + 1) * amp + 0.5f);
            ++i;
            break;

        case YHZ:
            amp = AMP_MULT * arr[i+1];
            p = arr[i] * HZ_MULT;
            for (int j = 0, k = t; j < LEN; ++j, ++k)
                data_array[j].laser_y += (uint16_t)((sine(k * p) + 1) * amp + 0.5f);
            ++i;
            break;

        case RED:
            for (int j = 0; j < LEN; ++j)
                data_array[j].r = (uint8_t)(arr[i] + 0.5f);
            break;

        case GREEN:
            for (int j = 0; j < LEN; ++j)
                data_array[j].g = (uint8_t)(arr[i] + 0.5f);
            break;

        case BLUE:
            for (int j = 0; j < LEN; ++j)
                data_array[j].b = (uint8_t)(arr[i] + 0.5f);
            break;

        case XOFF:
            for (int j = 0, k = t; j < LEN; ++j, ++k)
                data_array[j].laser_x += arr[i];
            break;

        case YOFF:
            for (int j = 0, k = t; j < LEN; ++j, ++k)
                data_array[j].laser_y += arr[i];
            break;

        case ROTATE:
            for (int j = 0, k = t; j < LEN; ++j, ++k)
                rotate_point(&data_array[j], arr[i], arr[i+1], arr[i+2]);
            i += 2;
            break;

        case COLOR_ANGLE:
            break;

        default:
            break;
        }
    }

    for (int i = 0; i < LEN; ++i)
    {
        if (data_array[i].laser_y > MAX_POSITION)
            data_array[i].laser_y = MAX_POSITION;

        if (data_array[i].laser_x > MAX_POSITION)
            data_array[i].laser_x = MAX_POSITION;
    }
    
    t += LEN;
}

void test_make_shape(const bool first)
{
    float arr[] = {4.1891910e+02, 2.5290594e-01, 2.5134238e+02, 1.2733901e-01,
       4.1886707e+02, 3.2696864e-01, 2.5140036e+02, 2.8470719e-01,
       2.5125056e+02, 1.7530885e-01, 8.3829834e+01, 2.4719754e-01,
       6.0000000e+02, 6.0000000e+02, 255, 255, 255};

    uint8_t types[] = {0, 0, 0, 1, 1, 1, 5, 6, 3, 2, 4};

    fill_array(sizeof(types), arr, types, first);
}