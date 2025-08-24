#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include "sam.h"

#ifndef CHORDS_
#define CHORDS_

typedef struct
{
    uint8_t x_count, y_count, r, g, b;
    uint16_t x_offset, y_offset, center_x, center_y;
    int t;
    float rotate_angle, xhz[5], yhz[5], xamp1[5], yamp1[5], xamp[5], yamp[5];
} 
ChordInfo;

void printChordInfo(const ChordInfo *c);

void make_chord(ChordInfo *info);

#endif