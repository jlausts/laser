#include <Arduino.h>
#include <sin_table.h>
#include <analogWrite.h>
#include <math.h>

// radius 0 -> 1
void circle(const int radius, const int x, const int y, const int time_us);

void line(const int x1, const int y1, const int x2, const int y2, const int time_us);

void arc_three(uint16_t x0, uint16_t y0,
               uint16_t x1, uint16_t y1,
               uint16_t x2, uint16_t y2,
               uint32_t time_us);
