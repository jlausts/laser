#include <Arduino.h>
#include "sine_table.h"


typedef struct __attribute__((packed))
{
    bool empty;
    uint8_t r, g, b;
    uint16_t laser_x, laser_y;
} Data;

void test_make_shapeOLD(const bool first);
void fill_array(const int len, const float *const arr, 
    const uint8_t *const types, const Data *const input_data, bool first_one);