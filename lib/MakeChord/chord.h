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
    uint16_t x_offset_start, x_offset_stop, x_offset_step, y_offset_start, y_offset_stop, y_offset_step;
    int t;
    float rotate_angle, rotate_angle_start, rotate_angle_stop, rotate_angle_step;
    float xamp_start, xamp_stop, xamp_step, yamp_start, yamp_stop, yamp_step;
    float xhz[5], yhz[5], xamp1[5], yamp1[5], xamp[5], yamp[5];
    float other_hz[16];
} 
ChordInfo;

void printChordInfo(const ChordInfo *c);

void make_chord(ChordInfo *info);

#endif