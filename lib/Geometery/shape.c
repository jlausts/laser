#include "shape.h"

void rotate_point(uint16_t *x, uint16_t *y, float angle, float cx, float cy)
{
    const float dx = (float)(*x) - cx;
    const float dy = (float)(*y) - cy;
    const float sin_ = sinf(angle);
    const float cos_ = cosf(angle);
    const float x_rot = cx + dx * cos_ - dy * sin_;
    const float y_rot = cy + dx * sin_ + dy * cos_;

    *x = (uint16_t)(x_rot + 0.5f);
    *y = (uint16_t)(y_rot + 0.5f);
}

void send_to_laser(const int len, const float *const arr, const uint8_t *const types, bool first_one)
{

    static int t = 0;
    static Data data_array[256] = {0};
    static uint8_t packed[256] = {0};
    float amp, p;
    memset(data_array, 0, sizeof(data_array));

    if (first_one)
        t = 0;

    for (int i = 0, type = 0; type < len; ++i, type++)
    {
        switch (types[type])
        {
        case XHZ:
            amp = 4095/2 * arr[i+1];
            p = 2 * 3.1415926 * arr[i] / ISR_HZ;
            for (int j = 0, k = t; j < 256; ++j, ++k)
                data_array[j].laser_x += (sinf(k * p) + 1) * amp + 0.5f;
            ++i;
            break;

        case YHZ:
            amp = 4095/2 * arr[i+1];
            p = 2 * 3.1415926 *  arr[i] / ISR_HZ;
            for (int j = 0, k = t; j < 256; ++j, ++k)
                data_array[j].laser_y += (sinf(k * p) + 1) * amp + 0.5f;
            ++i;
            break;

        case RED:
            for (int j = 0; j < 256; ++j)
                data_array[j].r = (int)(arr[i] + 0.5f);
            break;

        case GREEN:
            for (int j = 0; j < 256; ++j)
                data_array[j].g = (int)(arr[i] + 0.5f);
            break;

        case BLUE:
            for (int j = 0; j < 256; ++j)
                data_array[j].b = (int)(arr[i] + 0.5f);
            break;

        case XOFF:
            for (int j = 0, k = t; j < 256; ++j, ++k)
                data_array[j].laser_x += arr[i];
            break;

        case YOFF:
            for (int j = 0, k = t; j < 256; ++j, ++k)
                data_array[j].laser_y += arr[i];
            break;

        case ROTATE:
            for (int j = 0, k = t; j < 256; ++j, ++k)
                rotate_point(&data_array[j].laser_x, &data_array[j].laser_y, arr[i], arr[i+1], arr[i+2]);
            i += 2;
            break;

        case COLOR_ANGLE:
            break;

        default:
            break;
        }
    }

    for (int j = 0; j < 256; ++j)
    {
        if (data_array[j].laser_y > 4095)
            data_array[j].laser_y = 4095;
        else if (data_array[j].laser_y < 0)
            data_array[j].laser_y = 0;

        if (data_array[j].laser_x > 4095)
            data_array[j].laser_x = 4095;
        else if (data_array[j].laser_x < 0)
            data_array[j].laser_x = 0;
    }
    
    t += 255;
}