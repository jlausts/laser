#include "motion2.h"


constexpr float TAU = 6.28318530718f;
constexpr uint16_t SIN_MAX = 65535;





class Motion{

public:

    const int width = 4095;
    const int height = 4095;
    int x = width / 2;
    int y = height / 2;
    int dx = 0;
    int dy = 0;

    Motion()
    {

    }

    void linear(const int x2, const int y2, const float final_speed, const int time_us)
    {
        constexpr int steps = 100;
        constexpr float mult = 1.0f / (float)steps;
        const int wait_time = time_us / steps - 5;
        const int x1_ = width - this->x;
        const int x2_ = width - x2;
        
        for (int i = 0; i <= steps; ++i)
        {   
            const float t = 1.0f - (float)i * mult;       
            const float ease = 1 - t * t; 

            const int x = x1_ + (x2_ - x1_) * ease;
            const int y = this->y + (y2 - this->y) * ease;
            WRITE_BOTH(x, y);
            delayMicroseconds(wait_time);
        }
    }
    void linear(int x2, int y2, int time_us)
    {
        constexpr int steps = 100;
        const float x_inc = (x2 - this->x) / (float)steps;
        const float y_inc = (y2 - this->y) / (float)steps;
        float x1 = width - this->x;
        float y1 = (float)this->y;
        const int wait_time = time_us / steps - 5;

        for (int i = 0; i <= steps; ++i)
        {
            WRITE_BOTH(x1, y1);
            x1 += x_inc;
            y1 += y_inc;
            delayMicroseconds(wait_time);
        }
        this->x = x1;
        this->y = y1;
    }


    void ellipse(uint16_t rx, uint16_t ry,
                uint16_t cx, uint16_t cy,
                uint32_t time_us)
    {
        const uint16_t STEP_PHASE = 100;      // phase increment (smaller = smoother)
        const uint16_t QUARTER    = 65536 / 4;
        const int steps           = 65536 / STEP_PHASE;
        int wait_us               = time_us / steps - 5;   // rough μs per step

        // pre-offset so fast sin in [-1..+1] maps to DAC range 0..4095
        int16_t x_offset = 4095 - (cx - rx);
        int16_t y_offset = cy - ry;

        for (uint32_t phase = 0; phase < 65536; phase += STEP_PHASE)
        {
            // sine and cosine via 90° phase shift
            float sx = fast_sin_u16(phase);              // sin
            float sy = fast_sin_u16((phase + QUARTER) & 0xFFFF);  // cos

            // scale to radii and shift to center, then clip to 12-bit DAC
            uint16_t x = constrain((int)((sx + 1.0f) * rx) + x_offset, 0, 4095);
            uint16_t y = constrain((int)((sy + 1.0f) * ry) + y_offset, 0, 4095);

            WRITE_BOTH(x, y);
            delayMicroseconds(wait_us);
        }
    }


    void ellipse_arc(uint16_t cx, uint16_t cy,
                    uint16_t rx, uint16_t ry,
                    float start_angle, float end_angle,
                    uint32_t time_us)
    {
        const uint16_t STEPS = 256;
        const uint16_t PHASE_MAX = 65535;
        const uint16_t PHASE_MASK = PHASE_MAX - 1;

        // Convert angles (radians) to 16-bit phase (0..65535 maps to 0..2pi)
        uint16_t phase_start = (uint16_t)(start_angle * PHASE_MAX / (2.0f * PI)) & PHASE_MASK;
        uint16_t phase_end   = (uint16_t)(end_angle * PHASE_MAX / (2.0f * PI)) & PHASE_MASK;

        // Calculate phase span (correcting for wrap-around)
        int32_t span = (int32_t)phase_end - (int32_t)phase_start;
        if (span < 0) span += PHASE_MAX;

        uint16_t phase_step = span / STEPS;
        uint16_t wait_time = (time_us / STEPS) - 5;
        cx = 4095 - cx;

        for (uint16_t i = 0; i <= STEPS; ++i)
        {
            uint16_t phase = (phase_start + i * phase_step) & PHASE_MASK;

            float s = fast_sin_u16(phase);         // range: -1 to 1
            float c = fast_sin_u16((phase + PHASE_MAX / 4) & PHASE_MASK); // cos = sin(phase + pi/2)
            
            int x = (int)(cx + c * rx);
            int y = (int)(cy + s * ry);

            x = (cx - x) + cx; // flip x around center

            x = x < 0 ? 0 : (x > 4095 ? 4095 : x);
            y = y < 0 ? 0 : (y > 4095 ? 4095 : y);

            WRITE_BOTH(x, y);
            delayMicroseconds(wait_time);
        }
    }


    void circle_decel(const int radius, int x, int y, const int time_us)
    {
        constexpr uint16_t STEPS = 100;
        constexpr uint16_t QUARTER = 65536 / 4;
        const int wait_time = time_us / STEPS - 5;

        x = 4095 - x;
        x -= radius;
        y -= radius;

        for (int i = 0; i <= STEPS; ++i)
        {
            float t = (float)i / STEPS;                 // 0.0 to 1.0
            float ease = 1.0f - (1.0f - t) * (1.0f - t); // quadratic decel
            uint16_t phase = static_cast<uint16_t>(ease * 65535.0f) & 0xFFFF;

            float s1 = fast_sin_u16(phase);
            float s2 = fast_sin_u16((phase + QUARTER) & 0xFFFF);

            int a = static_cast<int>((s1 + 1.0f) * radius) + x;
            int b = static_cast<int>((s2 + 1.0f) * radius) + y;

            a = a < 0 ? 0 : (a > 4095 ? 4095 : a);
            b = b < 0 ? 0 : (b > 4095 ? 4095 : b);

            WRITE_BOTH(a, b);
            delayMicroseconds(wait_time);
        }
    }



    // radius 0 -> 2048
    void circle(const int radius, int x, int y, const int time_us)
    {
        const uint16_t steps = 100;                 // phase increment
        const uint16_t QUARTER = 65536 / 4;       // 90° phase shift
        int wait_time = time_us / steps - 5;
        x = 4095 - x;
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
        float x = 4095 - x1;
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

    void line_decel(const int x1, const int y1, const int x2, const int y2, const int time_us)
    {
        constexpr int steps = 100;
        const int wait_time = time_us / steps - 5;
        const int x1_ = 4095 - x1;
        const int x2_ = 4095 - x2;
        for (int i = 0; i <= steps; ++i)
        {
            const float t = 1 - ((float)i / steps);       
            const float ease = 1 - t * t; 

            const int x = x1_ + (x2_ - x1_) * ease;
            const int y = y1 + (y2 - y1) * ease;
            WRITE_BOTH(x, y);
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
        const int STEPS = 100;
        const int delay_us = ((time_us - 6) / STEPS) - 4;

        float x0f = 4095 - x0, y0f = y0;
        float x1f = 4095 - x1, y1f = y1;
        float x2f = 4095 - x2, y2f = y2;

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
            const float a = a0 + step * i;

            const uint16_t x = cx + cos_(a) * r;
            const uint16_t y = cy + sin_(a) * r;
            WRITE_BOTH((x > 4095 ? 4095 : x), (y > 4095 ? 4095 : y));
            delayMicroseconds(delay_us);
        }
    }





    void arc_three_decel(uint16_t x0, uint16_t y0,
                        uint16_t x1, uint16_t y1,
                        uint16_t x2, uint16_t y2,
                        uint32_t time_us)
    {
        constexpr int STEPS = 100;
        const int wait_us = time_us / STEPS - 5;          // room for WRITE + delay

        /* ---------- mirror X (same convention as your other code) ---------- */
        float x0f = x0, y0f = y0;
        float x1f = x1, y1f = y1;
        float x2f = x2, y2f = y2;

        /* ---------- solve circle through three points ---------- */
        float a = x0f*(y1f-y2f) - y0f*(x1f-x2f) + x1f*y2f - x2f*y1f;
        if (fabsf(a) < 1e-6f) {           // collinear → fall back to line
            line_decel(x0, y0, x2, y2, time_us);
            return;
        }

        float b = (x0f*x0f+y0f*y0f)*(y2f-y1f) +
                (x1f*x1f+y1f*y1f)*(y0f-y2f) +
                (x2f*x2f+y2f*y2f)*(y1f-y0f);

        float c = (x0f*x0f+y0f*y0f)*(x1f-x2f) +
                (x1f*x1f+y1f*y1f)*(x2f-x0f) +
                (x2f*x2f+y2f*y2f)*(x0f-x1f);

        float cx = -b / (2 * a);
        float cy = -c / (2 * a);
        float r  = hypotf(x0f - cx, y0f - cy);

        float a0 = atan2f(y0f - cy, x0f - cx);
        float a1 = atan2f(y2f - cy, x2f - cx);
        float a2 = atan2f(y1f - cy, x1f - cx);

        bool ccw = sinf(a1 - a0) * sinf(a2 - a0) >= 0;
        float span = ccw ? (a1 - a0) : (a1 - a0 - 2 * PI);

        /* ---------- Draw with quadratic ease-out on the angle ---------- */
        for (int i = 0; i <= STEPS; ++i)
        {
            float t = static_cast<float>(i) / STEPS;     // 0 … 1
            float ease = 1.0f - (1.0f - t) * (1.0f - t); // quadratic decel
            float a_cur = a0 + span * ease;              // eased angle

            uint16_t x = static_cast<uint16_t>(cx + cosf(a_cur) * r);
            uint16_t y = static_cast<uint16_t>(cy + sinf(a_cur) * r);

            /* clip and un-mirror X back to DAC space */
            x = 4095 - (x > 4095 ? 4095 : x);
            y = (y > 4095 ? 4095 : y);

            WRITE_BOTH(x, y);
            delayMicroseconds(wait_us);
        }
    }


};