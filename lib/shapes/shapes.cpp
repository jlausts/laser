#include "shapes.h"


constexpr float TAU = 6.28318530718f;
constexpr uint16_t SIN_MAX = 65535;


// radius 0 -> 2048
void circle(const int radius, int x, int y, const int time_us)
{
    const uint16_t steps = 100;                 // phase increment
    const uint16_t QUARTER = 65536 / 4;       // 90° phase shift
    int wait_time = time_us / steps - 5;
    x -= radius;
    y -= radius;
    for (uint32_t t = 0; t < 65536; t += steps)
    {
        // raw sine in –1.0 … +1.0
        float s1 = fast_sin_u16(t);
        float s2 = fast_sin_u16( (t + QUARTER) & 0xFFFF );

        // map to 0 … 4095 for 12-bit DAC
        int a = (int)((s1 + 1.0f) * radius) + x;   
        int b = (int)((s2 + 1.0f) * radius) + y;
        a = a < 0 ? 0 : (a > 4095 ? 4095 : a); 
        b = b < 0 ? 0 : (b > 4095 ? 4095 : b); 
        
        WRITE_BOTH(a, b);   
        delayMicroseconds(wait_time);
    }
}

void line(const int x1, const int y1, const int x2, const int y2, const int time_us)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = 100;
    float x_inc = dx / (float)steps;
    float y_inc = dy / (float)steps;
    float x = x1;
    float y = y1;
    int wait_time = time_us / steps - 5;

    for (int i = 0; i <= steps; ++i)
    {
        WRITE_BOTH(
            x < 0 ? 0 : (x > 4095 ? 4095 : (int)x), 
            y < 0 ? 0 : (y > 4095 ? 4095 : (int)y)
        );
        x += x_inc;
        y += y_inc;
        delayMicroseconds(wait_time);
    }
}

uint16_t angleToIndex(float angle_rad) {
    while (angle_rad < 0) angle_rad += TAU;
    while (angle_rad >= TAU) angle_rad -= TAU;
    return (uint16_t)((angle_rad / TAU) * SIN_MAX);
}

float sin_(float angle_rad) {
    return fast_sin_u16(angleToIndex(angle_rad));
}

float cos_(float angle_rad) {
    return sin(angle_rad + TAU / 4.0f);
}


float atan2_(float y, float x)
{
    const float c1 = PI / 4;
    const float c2 = 3 * PI / 4;

    float abs_y = fabsf(y) + 1e-10f;  // prevent divide by 0
    float r, angle;

    if (x >= 0)
    {
        r = (x - abs_y) / (x + abs_y);
        angle = c1 - c1 * r;
    }
    else
    {
        r = (x + abs_y) / (abs_y - x);
        angle = c2 - c1 * r;
    }

    return (y < 0) ? -angle : angle;
}

void arc_three(uint16_t x0, uint16_t y0,
               uint16_t x1, uint16_t y1,
               uint16_t x2, uint16_t y2,
               uint32_t time_us)
{
    const int STEPS = 256;
    const int delay_us = time_us / STEPS;

    float x0f = x0, y0f = y0;
    float x1f = x1, y1f = y1;
    float x2f = x2, y2f = y2;

    float a = x0f * (y1f - y2f) - y0f * (x1f - x2f) + x1f * y2f - x2f * y1f;

    if (fabs(a) < 1e-6) 
    {
        line(x0, y0, x2, y2, time_us);
        return;
    }

    float b = (x0f * x0f + y0f * y0f) * (y2f - y1f) +
              (x1f * x1f + y1f * y1f) * (y0f - y2f) +
              (x2f * x2f + y2f * y2f) * (y1f - y0f);

    float c = (x0f * x0f + y0f * y0f) * (x1f - x2f) +
              (x1f * x1f + y1f * y1f) * (x2f - x0f) +
              (x2f * x2f + y2f * y2f) * (x0f - x1f);

    float cx = -b / (2 * a);
    float cy = -c / (2 * a);

    float dx = x0f - cx;
    float dy = y0f - cy;
    float r = sqrtf(dx * dx + dy * dy);

    float a0 = atan2_(y0f - cy, x0f - cx);
    float a1 = atan2_(y2f - cy, x2f - cx);
    float a2 = atan2_(y1f - cy, x1f - cx);

    float ccw_total = sin_(a1 - a0);
    float ccw_middle = sin_(a2 - a0);
    bool ccw = ccw_total * ccw_middle >= 0;

    float span = ccw ? (a1 - a0) : (a1 - a0 - 2 * PI);
    float step = span / STEPS;

    for (int i = 0; i <= STEPS; ++i)
    {
        float a = a0 + step * i;

        uint16_t x = constrain((uint16_t)(cx + cos_(a) * r), 0u, 4095u);
        uint16_t y = constrain((uint16_t)(cy + sin_(a) * r), 0u, 4095u);

        WRITE_BOTH(x, y);
        delayMicroseconds(delay_us);
    }
}





