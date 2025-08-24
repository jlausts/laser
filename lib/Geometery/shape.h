#include <Arduino.h>
#include "sine_table.h"
#include "chord.h"

typedef struct __attribute__((packed))
{
    bool empty;
    uint8_t r, g, b;
    uint16_t laser_x, laser_y;
} Data;


void random_color(ChordInfo *const info);

void all_combinations(volatile Data *const data_array, const ChordInfo *const info);