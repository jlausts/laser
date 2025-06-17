#include <Arduino.h>
#include <sin_table.h>
#include <analogWrite.h>
#include <math.h>

void horizontal_square();

void draw_quad(int x1, int y1,
               int x2, int y2,
               int x3, int y3,
               int x4, int y4,
               uint32_t time_us);