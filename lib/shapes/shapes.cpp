#include "motion.h"



void horizontal_square()
{
    int max = 4095;
    int steps = 2000;
    line_decel(0, 0, max, 0, steps);
    line_decel(max, 0, max, max, steps);
    line_decel(max, max, 0, max, steps);
    line_decel(0, max, 0, 0, steps);
}



float dist(int x1, int y1, int x2, int y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy);
}

void draw_quad(int x1, int y1,
               int x2, int y2,
               int x3, int y3,
               int x4, int y4,
               uint32_t time_us)
{
    float d0 = dist(x1, y1, x2, y2);
    float d1 = dist(x2, y2, x3, y3);
    float d2 = dist(x3, y3, x4, y4);
    float d3 = dist(x4, y4, x1, y1);
    float total = d0 + d1 + d2 + d3;

    uint32_t t0 = (time_us * d0) / total;
    uint32_t t1 = (time_us * d1) / total;
    uint32_t t2 = (time_us * d2) / total;
    uint32_t t3 = (time_us * d3) / total;

    line_decel(x1, y1, x2, y2, t0);
    line_decel(x2, y2, x3, y3, t1);
    line_decel(x3, y3, x4, y4, t2);
    line_decel(x4, y4, x1, y1, t3);
}
