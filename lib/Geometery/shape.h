#include <Arduino.h>
#include "sine_table.h"


typedef struct __attribute__((packed))
{
    bool empty;
    uint8_t r, g, b;
    uint16_t laser_x, laser_y;
} Data;



void all_combinations(volatile Data *const data_array, const Data *const input_data, 
    const uint8_t x_count, const uint8_t y_count, 
    const int t, const float *const xhz, const float *const yhz, 
    const float *const xamp, const float *const yamp,
    const float rotate_angle, const float cx, const float cy);