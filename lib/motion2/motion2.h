#include <Arduino.h>
#include <sin_table.h>
#include <analogWrite.h>
#include <math.h>

// radius 0 -> 2048
void circle(const int radius, const int x, const int y, const int time_us);

void line(const int x1, const int y1, const int x2, const int y2, const int time_us);

void arc_three(uint16_t x0, uint16_t y0,
               uint16_t x1, uint16_t y1,
               uint16_t x2, uint16_t y2,
               uint32_t time_us);

void ellipse(uint16_t rx, uint16_t ry,
             uint16_t cx, uint16_t cy,
             uint32_t time_us);

void ellipse_arc(uint16_t cx, uint16_t cy,
            uint16_t rx, uint16_t ry,
            float start_angle, float end_angle,
            uint32_t time_us);

void line_decel(const int x1, const int y1, const int x2, const int y2, const int time_us);

void arc_three_decel(uint16_t x0, uint16_t y0,
                     uint16_t x1, uint16_t y1,
                     uint16_t x2, uint16_t y2,
                     uint32_t time_us);

                     
void circle_decel(const int radius, int x, int y, const int time_us);