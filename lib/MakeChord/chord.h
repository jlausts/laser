
#ifndef CHORDS_
#define CHORDS_
#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include "sam.h"

typedef struct
{
    uint8_t x_count, y_count, r, g, b, other_hz_count;
    uint16_t x_offset, y_offset, center_x, center_y;
    float x_offset_start, x_offset_step, y_offset_start, y_offset_step;
    float alpha_angle, alpha_angle_step;
    int t;
    float base_hz;
    float rotate_angle, rotate_angle_start, rotate_angle_step;
    float xamp_start, xamp_step, yamp_start, yamp_step;
    float xhz[5], yhz[5], xamp1[5], yamp1[5], xamp[5], yamp[5];
    float other_hz[16];
} 
ChordInfo;

void printChordInfo(const ChordInfo *c);

uint8_t hz_count_to_num_hz(const int x_count, const int y_count);

void make_chord(ChordInfo *info, const bool one_hz=false, const float base_hz=0, const uint8_t hz_using=0);

#endif