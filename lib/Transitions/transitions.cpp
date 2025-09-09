#include "transitions.h"
#include "transition_variables.h"
#include <math.h>
#include "print.h"
#include "sigmoid.h"
#include <string>

#define TRANSITION_FOR_LOOP for (int j = 0, k = info->t; j < LEN; \
    ++j, ++k, angle+=info->rotate_angle_step, \
    xamp+=info->xamp_step, yamp+=info->yamp_step, \
    x_offset+=info->x_offset_step, y_offset+=info->y_offset_step)

extern volatile Data data[2][256];
static constexpr double TAU = 6.283185307179586;
uint8_t hz_using_arr[8] = {3, 6, 9, 11, 12, 13, 14, 15};

#define ROTATE_CLAMP rotate_point_and_clamp2(&data_array[j], angle, info->center_x, info->center_y);

#define OFFSET data_array[j].laser_x += x_offset; data_array[j].laser_y += y_offset;

#define RANDOM_ROTATION_ANGLE 0.5f


extern inline void write_color(volatile const Data *const info);


void println(){}

// --- Base cases ---

// Generic type: uses normal Serial.println
template <typename T>
void println(const T &value) {
    Serial.println(value);
}

// float specialization
template <>
void println<float>(const float &value) {
    Serial.println(value, 6);   // 6 decimal places
}

// double specialization
template <>
void println<double>(const double &value) {
    Serial.println(value, 10);  // 10 decimal places
}

template <typename T, typename... Args>
void println(const T &first, const Args&... rest) {
    // Generic type
    Serial.print(first);
    Serial.print(' ');
    println(rest...);
}

template <typename... Args>
void println(const float &first, const Args&... rest) {
    Serial.print(first, 6);
    Serial.print(' ');
    println(rest...);
}

template <typename... Args>
void println(const double &first, const Args&... rest) {
    Serial.print(first, 10);
    Serial.print(' ');
    println(rest...);
}

void ftoa4(char *buf, float val)
{
    // Handle sign
    if (val < 0.0f) {
        *buf++ = '-';
        val = -val;
    }

    // Integer part
    int32_t int_part = (int32_t)val;
    float frac = val - (float)int_part;

    // Convert integer part
    char tmp[16];
    int i = 0;
    do {
        tmp[i++] = '0' + (int_part % 10);
        int_part /= 10;
    } while (int_part > 0);
    while (i > 0) {
        *buf++ = tmp[--i];
    }

    // Decimal point
    *buf++ = '.';

    // Fractional part (4 digits)
    for (int j = 0; j < 4; j++) {
        frac *= 10.0f;
        int d = (int)frac;
        *buf++ = '0' + d;
        frac -= d;
    }
    *buf = '\0';
}

void show_hz(ChordInfo *info, const char *end="\n")
{
    char buf[512];
    char tmp[32];
    buf[0] = '\0';

    // Helper lambda to append arrays
    auto append_array = [&](float *arr, int c) {
        for (int i = 0; i < c; i++) {
            ftoa4(tmp, arr[i]);
            strcat(buf, tmp);
            if (i < 4) strcat(buf, " ");
        }
        strcat(buf, "\n");
    };

    append_array(info->xhz, info->x_count);
    append_array(info->yhz, info->y_count);
    append_array(info->xamp1, info->x_count);
    append_array(info->yamp1, info->y_count);
    strcat(buf, end);

    Serial.println(buf);
}

void rotate_point_and_clamp2(volatile Data *const data_array, const float angle, const float cx, const float cy)
{
    const float dx = (float)(data_array->laser_x) - cx;
    const float dy = (float)(data_array->laser_y) - cy;
    const float sin_ = sine(angle) - 1.0f;
    const float cos_ = cosine(angle) - 1.0f;
    data_array->laser_x = (uint16_t)((cx + dx * cos_ - dy * sin_) + 0.5f);
    data_array->laser_y = (uint16_t)((cy + dx * sin_ + dy * cos_) + 0.5f);

    if (data_array->laser_x > MAX_POSITION) data_array->laser_x = MAX_POSITION;
    if (data_array->laser_y > MAX_POSITION) data_array->laser_y = MAX_POSITION;
}

inline void wait_for_empty_array()
{
    // wait for the interrupts to use up the data.
    volatile bool *const empty = &data[!array_reading][0].empty;
    while (!(*empty));
}



void transitioner(volatile Data *const data_array, const ChordInfo *const info)
{
    float angle = info->rotate_angle_start;
    float xamp = info->xamp_start;
    float yamp = info->yamp_start;
    float x_offset = info->x_offset_start;
    float y_offset = info->y_offset_start;
    // println(angle, info->rotate_angle_step*1e6, xamp*1e3, info->xamp_step * 1e6, "\n");
    // println((xamp+yamp) * 0.5f);
    // no offset

    if (x_offset == 0 && y_offset == 0)
    {
        switch (info->x_count << 4 | info->y_count)
        {
        case 1 << 4 | 1: TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y1; ROTATE_CLAMP; } break;
        case 1 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y2; ROTATE_CLAMP; } break;
        case 1 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y3; ROTATE_CLAMP; } break;
        case 1 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y4; ROTATE_CLAMP; } break;
        case 1 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y5; ROTATE_CLAMP; } break;

        case 2 << 4 | 1: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y1; ROTATE_CLAMP; } break;
        case 2 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y2; ROTATE_CLAMP; } break;
        case 2 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y3; ROTATE_CLAMP; } break;
        case 2 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y4; ROTATE_CLAMP; } break;
        case 2 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y5; ROTATE_CLAMP; } break;

        case 3 << 4 | 1: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y1; ROTATE_CLAMP; } break;
        case 3 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y2; ROTATE_CLAMP; } break;
        case 3 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y3; ROTATE_CLAMP; } break;
        case 3 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y4; ROTATE_CLAMP; } break;
        case 3 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y5; ROTATE_CLAMP; } break;

        case 4 << 4 | 1: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y1; ROTATE_CLAMP; } break;
        case 4 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y2; ROTATE_CLAMP; } break;
        case 4 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y3; ROTATE_CLAMP; } break;
        case 4 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y4; ROTATE_CLAMP; } break;
        case 4 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y5; ROTATE_CLAMP; } break;

        case 5 << 4 | 1: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y1; ROTATE_CLAMP; } break;
        case 5 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y2; ROTATE_CLAMP; } break;
        case 5 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y3; ROTATE_CLAMP; } break;
        case 5 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y4; ROTATE_CLAMP; } break;
        case 5 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y5; ROTATE_CLAMP; } break;

        default: break;
        }
    }

    // contains offset. (stored in laser_x&y)
    else
    {
        switch (info->x_count << 4 | info->y_count)
        {
        case 1 << 4 | 1: TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
        case 1 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
        case 1 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
        case 1 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
        case 1 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

        case 2 << 4 | 1: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
        case 2 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
        case 2 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
        case 2 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
        case 2 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

        case 3 << 4 | 1: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
        case 3 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
        case 3 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
        case 3 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
        case 3 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

        case 4 << 4 | 1: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
        case 4 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
        case 4 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
        case 4 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
        case 4 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

        case 5 << 4 | 1: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
        case 5 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
        case 5 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
        case 5 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
        case 5 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

        default: break;
        }
    }
}

void make_shape(const bool new_shape, ChordInfo *info, bool new_color = true)
{
    if (new_shape)
    {
        info->t = 0;
        make_chord(info);
        if (new_color)
            random_color(info);
    }

    for (uint8_t i = 0; i < info->x_count; ++i)
        info->xamp[i] = info->xamp1[i];// * amp_mult;
    for (uint8_t i = 0; i < info->y_count; ++i)
        info->yamp[i] = info->yamp1[i];// * amp_mult;

    transitioner(data[!array_reading], info);
    info->t += 255;
}

void wait_then_make(const bool new_shape, ChordInfo *info, bool new_color=true)
{
    pin10of;
    wait_for_empty_array();
    pin10on;
    make_shape(new_shape, info, new_color);
}

void grow_shape(ChordInfo *const info, const float amp_mult, const float start_grow_speed, const float max_amp, const float offset_add)
{
    const float new_amp = sine(((amp_mult + 1.5f) * 0.5f) * TAU) * 0.5f;
    const float offset = (1.0f - new_amp) * 2048;
    const float new_amp2 = sine(((amp_mult + 1.5f + start_grow_speed) * 0.5f) * TAU) * 0.5f;
    const float offset2 = (1.0f - new_amp2) * 2048;

    info->rotate_angle_start = info->alpha_angle;
    info->rotate_angle_step = info->alpha_angle_step / (float)LEN;

    info->x_offset_start = offset * max_amp + offset_add;
    info->x_offset_step = (offset2 - offset) / (float)LEN;

    info->y_offset_start = info->x_offset_start;
    info->y_offset_step = info->x_offset_step;

    info->xamp_start = new_amp * max_amp;
    info->xamp_step = (new_amp2 - new_amp) / (float)LEN;
    info->yamp_start = info->xamp_start;
    info->yamp_step = info->xamp_step;

    wait_then_make(false, info);
}

void cosine_transistion(ChordInfo *const info)
{
    const float max_amp = .8;
    const float start_grow_speed = 0.005f;
    const float end_shrink_speed = 0.005f;
    const int stable_time = 512;
    const float offset_add = (1 - max_amp) * 0.5f * 4096;

    for (float amp_mult = 1; amp_mult > end_shrink_speed; amp_mult -= end_shrink_speed, info->alpha_angle += info->alpha_angle_step)
        grow_shape(info, amp_mult, start_grow_speed, max_amp, offset_add);

    // random angle -.3 -> +.3
    const float total_angle = ((float)(rand() & 1023) / (1023/RANDOM_ROTATION_ANGLE/2) - RANDOM_ROTATION_ANGLE) * TAU;
    const float total_steps = (1.0f / start_grow_speed) + (1.0f / end_shrink_speed) + stable_time - 2.0f;
    info->alpha_angle_step = total_angle / total_steps;
    info->alpha_angle = RANDOM_ROTATION_ANGLE * 5 * TAU;

    wait_then_make(true, info);

    for (float amp_mult = start_grow_speed; amp_mult < 1; amp_mult += start_grow_speed, info->alpha_angle += info->alpha_angle_step)
        grow_shape(info, amp_mult, start_grow_speed, max_amp, offset_add);

    info->x_offset_start += info->x_offset_step;
    info->y_offset_start += info->y_offset_step;
    info->xamp_start += info->yamp_step;
    info->yamp_start += info->xamp_step;

    info->x_offset_step = 0;
    info->y_offset_step = 0;
    info->yamp_step = 0;
    info->xamp_step = 0;
}

void maintain_shape(int stable_time, ChordInfo *info)
{
    for (int i = 0; i < stable_time; ++i, info->alpha_angle += info->alpha_angle_step)
    {
        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;
        wait_then_make(false, info);
    }
}

void cosine_twister_iterations(const ChordInfo *const info, int *first, float *next_alpha_angle, float *next_alpha_angle_step, float *solved_x, float *y_adder)
{
    const float start_grow_speed = 0.005f;
    const float end_shrink_speed = 0.005f;
    const float angle_step = 0.01f;
    const int stable_time = 512;
    const float twist_count = 2.5;
    const float angle_mult = 1.0f;
    *solved_x = cbrtf(info->alpha_angle_step / angle_step / 4.0f / angle_mult);
    *y_adder = *solved_x * *solved_x * *solved_x * *solved_x;
    const float stop_slope = 4.0f * twist_count * twist_count * twist_count * angle_mult*angle_step;

    for (float deg = 0, deg2 = 0, i = 0, j = angle_step;
        deg2-deg < stop_slope;
        i += angle_step, j += angle_step, *first = *first+1)
    {
        float ii = (i + *solved_x);
        float jj = (j + *solved_x);
        deg = ii * ii * ii * ii * angle_mult + info->alpha_angle - *y_adder;
        deg2 = jj * jj * jj * jj * angle_mult + info->alpha_angle - *y_adder;
    }

    // random angle -.3 -> +.3
    const float total_angle = ((float)(rand() & 1023) / (1023/RANDOM_ROTATION_ANGLE)) * TAU;
    const float total_steps = (1.0f / start_grow_speed) + (1.0f / end_shrink_speed) + stable_time - 2.0f;
    *next_alpha_angle_step = total_angle / total_steps;
    *next_alpha_angle = RANDOM_ROTATION_ANGLE * 7 * TAU;
}

void cosine_twister(ChordInfo *const info)
{
    int first_iterations = 0;
    float next_alpha_angle = 0, next_alph_angle_step = 0, solved_x, y_adder;
    cosine_twister_iterations(info, &first_iterations, &next_alpha_angle, &next_alph_angle_step, &solved_x, &y_adder);
    const float max_amp = 0.8f;
    const float angle_step = 0.01f;
    const float twist_count = 2.5;
    const float angle_mult = 1.0f;
    const float amp_step = TAU * 0.45f / first_iterations;
    float deg2 = 0;
    const float offset_add = (1 - max_amp) * 0.5f * 4096;
    // const float first_amp = info->xamp_start;

    for (float deg = 0, i = 0, j = angle_step, count = 0, amp = 0;
        count < first_iterations;
        i += angle_step, j += angle_step, ++count, amp += amp_step)
    {
        float ii = (i + solved_x);
        float jj = (j + solved_x);
        deg = ii * ii * ii * ii * angle_mult + info->alpha_angle - y_adder;
        deg2 = jj * jj * jj * jj * angle_mult + info->alpha_angle - y_adder;
        info->rotate_angle_start = deg;
        info->rotate_angle_step = (deg2 - deg) / (float)LEN;

        const float new_amp = cosine(amp) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        const float new_amp2 = cosine(amp + amp_step) * 0.5f;
        const float offset2 = (1.0f - new_amp2) * 2048;

        info->x_offset_start = offset * max_amp + offset_add;
        info->x_offset_step = (offset2 - offset) / (float)LEN;

        info->y_offset_start = info->x_offset_start;
        info->y_offset_step = info->x_offset_step;

        info->xamp_start = new_amp * max_amp;
        info->xamp_step = (new_amp2 - new_amp) / (float)LEN;

        info->yamp_start = info->xamp_start;
        info->yamp_step = info->xamp_step;

        wait_then_make(false, info);
    }
    info->rotate_angle_start = deg2; 
    info->xamp_start += info->xamp_step * LEN;
    info->yamp_start = info->xamp_start;

    info->alpha_angle_step = next_alph_angle_step;
    info->alpha_angle = next_alpha_angle;
    float end_x = twist_count - cbrtf(info->alpha_angle_step / angle_step / 4.0f / angle_mult);
    const float amp_step2 = TAU * 0.45f / (end_x / angle_step);
    wait_then_make(true, info);
    for (float deg = info->alpha_angle, deg2 = info->alpha_angle, i = 0, j = angle_step, amp = TAU * 0.55f;
        i < end_x;
        i += angle_step, j += angle_step, amp += amp_step2)
    {
        float ii = (twist_count - i);
        float jj = (twist_count - j);
        deg = -ii * ii * ii * ii * angle_mult + info->alpha_angle + TAU * 5;
        deg2 = -jj * jj * jj * jj * angle_mult + info->alpha_angle + TAU * 5;
        info->rotate_angle_start = deg;
        info->rotate_angle_step = (deg2 - deg) / (float)LEN;

        const float new_amp = cosine(amp) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        const float new_amp2 = cosine(amp + amp_step) * 0.5f;
        const float offset2 = (1.0f - new_amp2) * 2048;

        info->x_offset_start = offset * max_amp + offset_add;
        info->x_offset_step = (offset2 - offset) / (float)LEN;

        info->y_offset_start = info->x_offset_start;
        info->y_offset_step = info->x_offset_step;

        info->xamp_start = new_amp * max_amp;
        info->xamp_step = (new_amp2 - new_amp) / (float)LEN;

        info->yamp_start = info->xamp_start;
        info->yamp_step = info->xamp_step;

        wait_then_make(false, info);
    }

    info->alpha_angle = info->rotate_angle_start + info->rotate_angle_step;
    info->x_offset_start += info->x_offset_step;
    info->y_offset_start += info->y_offset_step;
    info->xamp_start += info->yamp_step;
    info->yamp_start += info->xamp_step;

    info->x_offset_step = 0;
    info->y_offset_step = 0;
    info->yamp_step = 0;
    info->xamp_step = 0;
}
// would be nice to make the sudden transition from one shape to another when shaking to be less sudden.
void shaker(ChordInfo *const info)
{
    const float mult = 25 * PI;
    int i = 100;
    const float shake_mult = 40;
    for (; i < 500; ++i, info->alpha_angle += info->alpha_angle_step)
    {
        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;

        float ex = e_to_x(i);
        float ex_plus1 = 1 + ex;
        float der1 = (cosine(mult / ex_plus1) - 1) * mult * ex / (ex_plus1 * ex_plus1) * shake_mult;
        info->x_offset_start = der1 + 409; //sine(e_to_x(i) * PI * 20) * 0.5f * 100 + 409-50;

        ex = e_to_x(i+1);
        ex_plus1 = 1 + ex;
        float der2 = (cosine(mult / ex_plus1) - 1) * mult * ex / ex_plus1 / ex_plus1 * shake_mult;

        info->x_offset_step = (der2 - der1) / LEN;
        wait_then_make(false, info);
    }

    info->rotate_angle_start = info->alpha_angle;
    info->rotate_angle_step = info->alpha_angle_step / (float)LEN;

    float ex = e_to_x(i++);
    float ex_plus1 = 1 + ex;
    float der1 = (cosine(mult / ex_plus1) - 1) * mult * ex / (ex_plus1 * ex_plus1) * shake_mult;
    info->x_offset_start = der1 + 409; //sine(e_to_x(i) * PI * 20) * 0.5f * 100 + 409-50;

    ex = e_to_x(i);
    ex_plus1 = 1 + ex;
    float der2 = (cosine(mult / ex_plus1) - 1) * mult * ex / ex_plus1 / ex_plus1 * shake_mult;
    info->x_offset_step = (der2 - der1) / LEN;
    wait_then_make(true, info, false);

    for (; i < 900; ++i, info->alpha_angle += info->alpha_angle_step)
    {
        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;

        float ex = e_to_x(i);
        float ex_plus1 = 1 + ex;
        float der1 = (cosine(mult / ex_plus1) - 1) * mult * ex / (ex_plus1 * ex_plus1) * shake_mult;
        info->x_offset_start = der1 + 409; //sine(e_to_x(i) * PI * 20) * 0.5f * 100 + 409-50;

        ex = e_to_x(i+1);
        ex_plus1 = 1 + ex;
        float der2 = (cosine(mult / ex_plus1) - 1) * mult * ex / ex_plus1 / ex_plus1 * shake_mult;

        info->x_offset_step = (der2 - der1) / LEN;
        wait_then_make(false, info);
    }
}

void tornado_twist_power2(ChordInfo *const info)
{
    const float start_grow_speed = 0.005f;
    const float end_shrink_speed = 0.005f;
    const float angle_step = 0.01f;
    const int stable_time = 512;
    const float twist_count = 4;
    const float angle_mult = 4.0f;

    float solved_x = info->alpha_angle_step / angle_step / 2.0f / angle_mult;
    float y_adder = solved_x * solved_x;
    const float stop_slope = 2.0f * twist_count * angle_mult*angle_step;//_deg2 - _deg;

    for (float deg = 0, deg2 = 0, i = 0, j = angle_step;
        deg2-deg<stop_slope;
        i += angle_step, j += angle_step)
    {
        float ii = i + solved_x;
        float jj = j + solved_x;
        deg  = ii * ii * angle_mult + info->alpha_angle - y_adder * angle_mult;
        deg2 = jj * jj * angle_mult + info->alpha_angle - y_adder * angle_mult;
        info->rotate_angle_start = deg;
        info->rotate_angle_step = (deg2 - deg) / (float)LEN;

        wait_then_make(false, info);
    }

    // random angle -.3 -> +.3
    const float total_angle = ((float)(rand() & 1023) / (1023/RANDOM_ROTATION_ANGLE/2) - RANDOM_ROTATION_ANGLE) * TAU;
    // const float total_angle = ((float)(rand() & 1023) / (1023/RANDOM_ROTATION_ANGLE)) * TAU;
    const float total_steps = (1.0f / start_grow_speed) + (1.0f / end_shrink_speed) + stable_time - 2.0f;
    info->alpha_angle_step = total_angle / total_steps;
    info->alpha_angle = RANDOM_ROTATION_ANGLE * 12 * TAU;
    float end_x = twist_count - info->alpha_angle_step / angle_step / 2.0f / angle_mult;

    info->rotate_angle_start += info->rotate_angle_step * LEN;
    wait_then_make(true, info);
    for (float deg = 0, deg2 = 0, i = 0, j = angle_step;
        i < end_x;
        i += angle_step, j += angle_step)
    {
        float ii = twist_count - i;
        float jj = twist_count - j;
        deg  = -ii * ii * angle_mult + info->alpha_angle + TAU * 5;
        deg2 = -jj * jj * angle_mult + info->alpha_angle + TAU * 5;
        info->rotate_angle_start = deg;
        info->rotate_angle_step = (deg2 - deg) / (float)LEN;
        wait_then_make(false, info);
    }

    info->alpha_angle = info->rotate_angle_start + info->alpha_angle_step;
    info->x_offset_start += info->x_offset_step;
    info->y_offset_start += info->y_offset_step;
    info->xamp_start += info->yamp_step;
    info->yamp_start += info->xamp_step;

    info->x_offset_step = 0;
    info->y_offset_step = 0;
    info->yamp_step = 0;
    info->xamp_step = 0;
}

void tornado_twist_power4(ChordInfo *const info)
{
    const float start_grow_speed = 0.005f;
    const float end_shrink_speed = 0.005f;
    const float angle_step = 0.01f;
    const int stable_time = 512;
    const float twist_count = 2.5;
    const float angle_mult = 1.0f;

    float solved_x = cbrtf(info->alpha_angle_step / angle_step / 4.0f / angle_mult);
    float y_adder = solved_x * solved_x * solved_x * solved_x;
    const float stop_slope = 4.0f * twist_count * twist_count * twist_count * angle_mult*angle_step;
    float deg2 = 0;
    
    for (float deg = 0, i = 0, j = angle_step;
        deg2-deg < stop_slope;
        i += angle_step, j += angle_step)
    {
        float ii = (i + solved_x);
        float jj = (j + solved_x);
        deg = ii * ii * ii * ii * angle_mult + info->alpha_angle - y_adder;
        deg2 = jj * jj * jj * jj * angle_mult + info->alpha_angle - y_adder;
        info->rotate_angle_start = deg;
        info->rotate_angle_step = (deg2 - deg) / (float)LEN;

        wait_then_make(false, info);
    }
    info->rotate_angle_start = deg2; 

    // random angle -.3 -> +.3
    const float total_angle = ((float)(rand() & 1023) / (1023/RANDOM_ROTATION_ANGLE)) * TAU;
    const float total_steps = (1.0f / start_grow_speed) + (1.0f / end_shrink_speed) + stable_time - 2.0f;
    info->alpha_angle_step = total_angle / total_steps;
    info->alpha_angle = RANDOM_ROTATION_ANGLE * 7 * TAU;
    float end_x = twist_count - cbrtf(info->alpha_angle_step / angle_step / 4.0f / angle_mult);
    wait_then_make(true, info, false);
    for (float deg = 0, deg2 = 0, i = 0, j = angle_step;
        i < end_x;
        i += angle_step, j += angle_step)
    {
        float ii = (twist_count - i);
        float jj = (twist_count - j);
        deg = -ii * ii * ii * ii * angle_mult + info->alpha_angle + TAU * 5;
        deg2 = -jj * jj * jj * jj * angle_mult + info->alpha_angle + TAU * 5;
        info->rotate_angle_start = deg;
        info->rotate_angle_step = (deg2 - deg) / (float)LEN;
        wait_then_make(false, info);
    }
// fix the final alpha angle step after this for loop.
    info->alpha_angle = info->rotate_angle_start + info->rotate_angle_step;
    info->x_offset_start += info->x_offset_step;
    info->y_offset_start += info->y_offset_step;
    info->xamp_start += info->yamp_step;
    info->yamp_start += info->xamp_step;

    info->x_offset_step = 0;
    info->y_offset_step = 0;
    info->yamp_step = 0;
    info->xamp_step = 0;
}

bool reborn(ChordInfo *const info, const int time=256)
{
    if (info->x_count == 5 || info->y_count == 5)
        return false;

    static ChordInfo new_info;
    for (uint8_t i = 0; i < info->other_hz_count; ++i)
        new_info.other_hz[i] = info->other_hz[i];
    new_info.other_hz_count = info->other_hz_count;

    make_chord(&new_info, true);

    const float count_inv = 0.5f / (float)time * TAU;

    const int xi = random(info->x_count);
    const int yi = random(info->y_count);
    const float original_xamp = info->xamp1[xi];
    const float original_yamp = info->yamp1[yi];
    const int xc = info->x_count;
    const int yc = info->y_count;
    info->xhz[xc] = new_info.xhz[0];
    info->yhz[yc] = new_info.yhz[0];

    info->x_count++;
    info->y_count++;
    
    for (float j = 0, k = 0; j < time; j += 1, k += count_inv)
    {
        const float amp_mult = cosine(k) * 0.5f;
        info->xamp1[xi] = original_xamp * amp_mult;
        info->yamp1[yi] = original_yamp * amp_mult;
        info->xamp1[xc] = original_xamp - info->xamp1[xi];
        info->yamp1[yc] = original_yamp - info->yamp1[yi];

        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;
        info->alpha_angle += info->alpha_angle_step;
        wait_then_make(false, info);
    }

    info->xhz[xi] = info->xhz[xc];
    info->yhz[yi] = info->yhz[yc];
    info->xamp1[xi] = info->xamp1[xc];
    info->yamp1[yi] = info->yamp1[yc];
    info->x_count--;
    info->y_count--;

    return true;
}

void one_twist_in(ChordInfo *const info)
{
    const float max_amp = .8;
    const float start_grow_speed = 0.006f;
    const float end_shrink_speed = 0.006f;

    const float offset_add = (1 - max_amp) * 0.5f * 4096;
    const float deg_step = end_shrink_speed * PI;
    const float original_angle = info->rotate_angle_start;
    for (float deg = 0, amp_mult = 1; amp_mult > end_shrink_speed; deg += deg_step, amp_mult -= end_shrink_speed, info->alpha_angle += info->alpha_angle_step)
    {
        const float cosine_angle = (1.0f - cosine(deg) * 0.5f) * TAU;
        info->rotate_angle_start = cosine_angle + original_angle;
        info->rotate_angle_step = (((1.0f - cosine(deg + deg_step) * 0.5f) * TAU) - cosine_angle) / (float)LEN;  

        const float new_amp = sine(((amp_mult + 1.5f) * 0.5f) * TAU) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        const float new_amp2 = sine(((amp_mult + 1.5f + start_grow_speed) * 0.5f) * TAU) * 0.5f;
        const float offset2 = (1.0f - new_amp2) * 2048;

        info->x_offset_start = offset * max_amp + offset_add;
        info->x_offset_step = (offset2 - offset) / (float)LEN;

        info->y_offset_start = info->x_offset_start;
        info->y_offset_step = info->x_offset_step;

        info->xamp_start = new_amp * max_amp;
        info->xamp_step = (new_amp2 - new_amp) / (float)LEN;
        info->yamp_start = info->xamp_start;
        info->yamp_step = info->xamp_step;
        wait_then_make(false, info);
    }

    int first_iterations = 0;
    float next_alpha_angle = 0, next_alph_angle_step = 0, solved_x, y_adder;
    cosine_twister_iterations(info, &first_iterations, &next_alpha_angle, &next_alph_angle_step, &solved_x, &y_adder);
    const float angle_step = 0.01f;
    const float twist_count = 2.5;
    const float angle_mult = 1.0f;
    const float amp_step = TAU * 0.45f / first_iterations;
    float deg2 = 0;
    // const float first_amp = info->xamp_start;

    info->rotate_angle_start = deg2; 
    info->xamp_start += info->xamp_step * LEN;
    info->yamp_start = info->xamp_start;

    info->alpha_angle_step = next_alph_angle_step;
    info->alpha_angle = next_alpha_angle;
    float end_x = twist_count - cbrtf(info->alpha_angle_step / angle_step / 4.0f / angle_mult);
    const float amp_step2 = TAU * 0.45f / (end_x / angle_step);
    wait_then_make(true, info);
    for (float deg = info->alpha_angle, deg2 = info->alpha_angle, i = 0, j = angle_step, amp = TAU * 0.55f;
        i < end_x;
        i += angle_step, j += angle_step, amp += amp_step2)
    {
        float ii = (twist_count - i);
        float jj = (twist_count - j);
        deg = -ii * ii * ii * ii * angle_mult + info->alpha_angle + TAU * 5;
        deg2 = -jj * jj * jj * jj * angle_mult + info->alpha_angle + TAU * 5;
        info->rotate_angle_start = deg;
        info->rotate_angle_step = (deg2 - deg) / (float)LEN;

        const float new_amp = cosine(amp) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        const float new_amp2 = cosine(amp + amp_step) * 0.5f;
        const float offset2 = (1.0f - new_amp2) * 2048;

        info->x_offset_start = offset * max_amp + offset_add;
        info->x_offset_step = (offset2 - offset) / (float)LEN;

        info->y_offset_start = info->x_offset_start;
        info->y_offset_step = info->x_offset_step;

        info->xamp_start = new_amp * max_amp;
        info->xamp_step = (new_amp2 - new_amp) / (float)LEN;

        info->yamp_start = info->xamp_start;
        info->yamp_step = info->xamp_step;

        wait_then_make(false, info);
    }

    info->alpha_angle = info->rotate_angle_start + info->rotate_angle_step;
    info->x_offset_start += info->x_offset_step;
    info->y_offset_start += info->y_offset_step;
    info->xamp_start += info->yamp_step;
    info->yamp_start += info->xamp_step;

    info->x_offset_step = 0;
    info->y_offset_step = 0;
    info->yamp_step = 0;
    info->xamp_step = 0;
}

void one_twist_out(ChordInfo *const info)
{
    int first_iterations = 0;
    float next_alpha_angle = 0, next_alph_angle_step = 0, solved_x, y_adder;
    cosine_twister_iterations(info, &first_iterations, &next_alpha_angle, &next_alph_angle_step, &solved_x, &y_adder);
    const float max_amp = 0.8f;
    const float angle_step = 0.01f;
    const float angle_mult = 1.0f;
    const float amp_step = TAU * 0.45f / first_iterations;
    float deg2 = 0;
    const float offset_add = (1 - max_amp) * 0.5f * 4096;
    // const float first_amp = info->xamp_start;

    for (float deg = 0, i = 0, j = angle_step, count = 0, amp = 0;
        count < first_iterations;
        i += angle_step, j += angle_step, ++count, amp += amp_step)
    {
        float ii = (i + solved_x);
        float jj = (j + solved_x);
        deg = ii * ii * ii * ii * angle_mult + info->alpha_angle - y_adder;
        deg2 = jj * jj * jj * jj * angle_mult + info->alpha_angle - y_adder;
        info->rotate_angle_start = deg;
        info->rotate_angle_step = (deg2 - deg) / (float)LEN;

        const float new_amp = cosine(amp) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        const float new_amp2 = cosine(amp + amp_step) * 0.5f;
        const float offset2 = (1.0f - new_amp2) * 2048;

        info->x_offset_start = offset * max_amp + offset_add;
        info->x_offset_step = (offset2 - offset) / (float)LEN;

        info->y_offset_start = info->x_offset_start;
        info->y_offset_step = info->x_offset_step;

        info->xamp_start = new_amp * max_amp;
        info->xamp_step = (new_amp2 - new_amp) / (float)LEN;

        info->yamp_start = info->xamp_start;
        info->yamp_step = info->xamp_step;

        wait_then_make(false, info);
    }
    info->rotate_angle_start = deg2; 
    info->xamp_start += info->xamp_step * LEN;
    info->yamp_start = info->xamp_start;

    info->alpha_angle_step = next_alph_angle_step;
    info->alpha_angle = next_alpha_angle;
    wait_then_make(true, info);


    const float start_grow_speed = 0.006f;
    const float end_shrink_speed = 0.006f;
    const float deg_step = end_shrink_speed * PI * 0.5f;
    const float original_angle = info->rotate_angle_start;
    for (float deg = PI * 0.5f, amp_mult = start_grow_speed; amp_mult < 1; deg += deg_step, amp_mult += start_grow_speed, info->alpha_angle += info->alpha_angle_step)
    {
        const float cosine_angle = (1.0f - cosine(deg) * 0.5f * PI) * TAU;
        info->rotate_angle_start = cosine_angle + original_angle;
        info->rotate_angle_step = (((1.0f - cosine(deg + deg_step) * 0.5f * PI) * TAU) - cosine_angle) / (float)LEN;  

        const float new_amp = sine(((amp_mult + 1.5f) * 0.5f) * TAU) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        const float new_amp2 = sine(((amp_mult + 1.5f + start_grow_speed) * 0.5f) * TAU) * 0.5f;
        const float offset2 = (1.0f - new_amp2) * 2048;

        info->x_offset_start = offset * max_amp + offset_add;
        info->x_offset_step = (offset2 - offset) / (float)LEN;

        info->y_offset_start = info->x_offset_start;
        info->y_offset_step = info->x_offset_step;

        info->xamp_start = new_amp * max_amp;
        info->xamp_step = (new_amp2 - new_amp) / (float)LEN;
        info->yamp_start = info->xamp_start;
        info->yamp_step = info->xamp_step;
        wait_then_make(false, info);
    }
    info->alpha_angle = info->rotate_angle_start + info->alpha_angle_step;
    info->x_offset_start += info->x_offset_step;
    info->y_offset_start += info->y_offset_step;
    info->xamp_start += info->yamp_step;
    info->yamp_start += info->xamp_step;

    info->x_offset_step = 0;
    info->y_offset_step = 0;
    info->yamp_step = 0;
    info->xamp_step = 0;
}

void one_twist_in_out(ChordInfo *info)
{
    const float max_amp = .8;
    const float start_grow_speed = 0.006f;
    const float end_shrink_speed = 0.006f;

    const float offset_add = (1 - max_amp) * 0.5f * 4096;
    float deg_step = end_shrink_speed * PI;
    const float original_angle = info->rotate_angle_start;
    for (float deg = 0, amp_mult = 1; amp_mult > end_shrink_speed; deg += deg_step, amp_mult -= end_shrink_speed, info->alpha_angle += info->alpha_angle_step)
    {
        const float cosine_angle = (1.0f - cosine(deg) * 0.5f) * TAU;
        info->rotate_angle_start = cosine_angle + original_angle;
        info->rotate_angle_step = (((1.0f - cosine(deg + deg_step) * 0.5f) * TAU) - cosine_angle) / (float)LEN;  

        const float new_amp = sine(((amp_mult + 1.5f) * 0.5f) * TAU) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        const float new_amp2 = sine(((amp_mult + 1.5f + start_grow_speed) * 0.5f) * TAU) * 0.5f;
        const float offset2 = (1.0f - new_amp2) * 2048;

        info->x_offset_start = offset * max_amp + offset_add;
        info->x_offset_step = (offset2 - offset) / (float)LEN;

        info->y_offset_start = info->x_offset_start;
        info->y_offset_step = info->x_offset_step;

        info->xamp_start = new_amp * max_amp;
        info->xamp_step = (new_amp2 - new_amp) / (float)LEN;
        info->yamp_start = info->xamp_start;
        info->yamp_step = info->xamp_step;
        wait_then_make(false, info);
    } 

    wait_then_make(true, info);
    deg_step = end_shrink_speed * PI * 0.5f;
    for (float deg = PI * 0.5f, amp_mult = start_grow_speed; amp_mult < 1; deg += deg_step, amp_mult += start_grow_speed, info->alpha_angle += info->alpha_angle_step)
    {
        const float cosine_angle = (1.0f - cosine(deg) * 0.5f * PI) * TAU;
        info->rotate_angle_start = cosine_angle + original_angle;
        info->rotate_angle_step = (((1.0f - cosine(deg + deg_step) * 0.5f * PI) * TAU) - cosine_angle) / (float)LEN;  

        const float new_amp = sine(((amp_mult + 1.5f) * 0.5f) * TAU) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        const float new_amp2 = sine(((amp_mult + 1.5f + start_grow_speed) * 0.5f) * TAU) * 0.5f;
        const float offset2 = (1.0f - new_amp2) * 2048;

        info->x_offset_start = offset * max_amp + offset_add;
        info->x_offset_step = (offset2 - offset) / (float)LEN;

        info->y_offset_start = info->x_offset_start;
        info->y_offset_step = info->x_offset_step;

        info->xamp_start = new_amp * max_amp;
        info->xamp_step = (new_amp2 - new_amp) / (float)LEN;
        info->yamp_start = info->xamp_start;
        info->yamp_step = info->xamp_step;
        wait_then_make(false, info);
    }
    info->alpha_angle = info->rotate_angle_start + info->alpha_angle_step;
    info->x_offset_start += info->x_offset_step;
    info->y_offset_start += info->y_offset_step;
    info->xamp_start += info->yamp_step;
    info->yamp_start += info->xamp_step;

    info->x_offset_step = 0;
    info->y_offset_step = 0;
    info->yamp_step = 0;
    info->xamp_step = 0;
}

// bug in here sometimes causing non-harmonic new frequency.
void big_o(ChordInfo *const info, const int time=256)
{    
    static ChordInfo new_info;
    const float count_inv = 0.5f / (float)time * TAU;

    for (uint8_t i = 0; i < info->other_hz_count; ++i)
        new_info.other_hz[i] = info->other_hz[i];
    new_info.other_hz_count = info->other_hz_count;

    make_chord(&new_info, false, info->base_hz, info->x_count + info->y_count);

    // search for the closest hz on the x and y
    uint8_t xhz = 0, yhz = 0;
    float closest = __FLT_MAX__, closeness;
    for (uint8_t x = 0; x < info->x_count; ++x)
    {
        for (uint8_t y = 0; y < info->y_count; ++y)
        {
            closeness = fabsf(info->xhz[x] - info->yhz[y]);
            if (closeness < closest)
            {
                closest = closeness;
                xhz = x;
                yhz = y;
            }
        }
    }

    // equal hz not found, make one instead.
    float new_hz = 0;
    const float max_difference = 0.001f;
    while (closest > max_difference)
    {
        constexpr float MAX_OUT_TUNE = .1f;
        constexpr float HZ_MULT = TAU / 40000.0f;
        for (int i = 0; i < info->other_hz_count; ++i)
        {
            new_hz = info->other_hz[i] + (float)(rand() & 255) / ((255 / MAX_OUT_TUNE) + MAX_OUT_TUNE/2);
            new_hz *= HZ_MULT;
            closest = fabsf(new_hz - info->xhz[xhz]);
            if (closest < max_difference)
                break;
        }
    }

    // merge the new hz into the shape
    if (new_hz != 0)
    {
        const float original_yamp = info->yamp1[yhz];
        const int yc = info->y_count;
        info->yhz[yc] = new_hz;
        info->y_count++;
        
        for (float j = 0, k = 0; j < time; j += 1, k += count_inv)
        {
            const float amp_mult = cosine(k) * 0.5f;
            info->yamp1[yhz] = original_yamp * amp_mult;
            info->yamp1[yc] = original_yamp - info->yamp1[yhz];

            info->rotate_angle_start = info->alpha_angle;
            info->rotate_angle_step = info->alpha_angle_step / (float)LEN;
            info->alpha_angle += info->alpha_angle_step;
            wait_then_make(false, info);
        }

        info->yhz[yhz] = new_hz;
        info->yamp1[yhz] = original_yamp;
        info->y_count--;
    }


    // make a list of the hz's index that can be removed
    float x_original[5], y_original[5];
    for (uint8_t i = 0; i < info->x_count; ++i)
        x_original[i] = info->xamp1[i];
    for (uint8_t i = 0; i < info->y_count; ++i)
        y_original[i] = info->yamp1[i];
    
    // remove all hz's that are not those two close ones at the same time with cosine.
    for (float j = 0, k = 0; j < time; j += 1, k += count_inv)
    {
        const float amp_mult = cosine(k) * 0.5f;
        float xhz_add = 0, yhz_add = 0;

        for (uint8_t i = 0, j = 0; i < info->x_count; ++i, ++j)
        {
            if (i != xhz)
            {
                info->xamp1[j] = x_original[j] * amp_mult;
                xhz_add += x_original[j] - info->xamp1[j];
            }
        }

        for (uint8_t i = 0, j = 0; i < info->y_count; ++i, ++j)
        {
            if (i != yhz)
            {
                info->yamp1[j] = y_original[j] * amp_mult;
                yhz_add += y_original[j] - info->yamp1[j];
            }
        }

        info->xamp1[xhz] = x_original[xhz] + xhz_add;
        info->yamp1[yhz] = y_original[yhz] + yhz_add;
        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;
        info->alpha_angle += info->alpha_angle_step;
        wait_then_make(false, info);
    }

    // merge the new hz's into the info struct
    for (uint8_t i = 0; i < info->x_count; ++i)
    {
        if (i != xhz)
        {
            info->xhz[i] = new_info.xhz[i];
            // x_original[i] = new_info.xamp1[i];
        }
    }

    for (uint8_t i = 0; i < info->y_count; ++i)
    {
        if (i != yhz)
        {
            info->yhz[i] = new_info.yhz[i];
            // y_original[i] = new_info.yamp1[i];
        }
    }

    // add the new hz's back to the big "O"
    for (float j = 0, k = PI; j < time; j += 1, k += count_inv)
    {
        const float amp_mult = cosine(k) * 0.5f;
        float xhz_add = 0, yhz_add = 0;

        for (uint8_t i = 0, j = 0; i < info->x_count; ++i, ++j)
        {
            if (i != xhz)
            {
                info->xamp1[j] = x_original[j] * amp_mult;
                xhz_add += x_original[j] - info->xamp1[j];
            }
        }

        for (uint8_t i = 0, j = 0; i < info->y_count; ++i, ++j)
        {
            if (i != yhz)
            {
                info->yamp1[j] = y_original[j] * amp_mult;
                yhz_add += y_original[j] - info->yamp1[j];
            }
        }
        
        info->xamp1[xhz] = x_original[xhz] + xhz_add;
        info->yamp1[yhz] = y_original[yhz] + yhz_add;
        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;
        info->alpha_angle += info->alpha_angle_step;
        wait_then_make(false, info);
    }
}


bool remove_one(ChordInfo *const info, const int time=256)
{
    if (info->x_count == 2 || info->y_count == 2)
        return false;

    const float count_inv = 0.5f / (float)time * TAU;

    static float new_xamps[5], new_yamps[5];
    for (uint8_t i = 0; i < info->x_count; i++)
        new_xamps[i] = (1.0f / (info->x_count - 1) * info->xamp1[i] * info->x_count) - info->xamp1[i];
    for (uint8_t i = 0; i < info->y_count; i++)
        new_yamps[i] = (1.0f / (info->y_count - 1) * info->yamp1[i] * info->y_count) - info->yamp1[i];

    static float original_xamps[5], original_yamps[5];
    for (uint8_t i = 0; i < info->x_count; i++)
        original_xamps[i] = info->xamp1[i];
    for (uint8_t i = 0; i < info->y_count; i++)
        original_yamps[i] = info->yamp1[i];

    const uint8_t xi = info->x_count - 1;
    const uint8_t yi = info->y_count - 1;
    const float original_xamp = info->xamp1[xi];
    const float original_yamp = info->yamp1[yi];

    for (float j = 0, k = 0; j < time; j += 1, k += count_inv)
    {
        const float amp_mult = cosine(k) * 0.5f;
        info->xamp1[xi] = original_xamp * amp_mult;
        info->yamp1[yi] = original_yamp * amp_mult;
        
        for (uint8_t i = 0; i < xi; ++i)
            info->xamp1[i] = original_xamps[i] + (1.0f - amp_mult) * new_xamps[i];
        for (uint8_t i = 0; i < yi; ++i)
            info->yamp1[i] = original_yamps[i] + (1.0f - amp_mult) * new_yamps[i];

        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;
        info->alpha_angle += info->alpha_angle_step;
        wait_then_make(false, info);
    }

    info->x_count--;
    info->y_count--;

    return true;
}

// Bug in here
bool add_one(ChordInfo *const info, const int time=256)
{
    // can't add any more hz to this one.
    if (info->hz_using >= 6)
        return true;

    static ChordInfo new_info;
    for (uint8_t i = 0; i < info->other_hz_count; ++i)
        new_info.other_hz[i] = info->other_hz[i];
    new_info.other_hz_count = info->other_hz_count;

    info->hz_using += 2;
    make_chord(&new_info, false, info->base_hz, hz_using_arr[info->hz_using]);

    const float count_inv = 0.5f / (float)time * TAU;

    static float original_xamps[5], original_yamps[5];
    for (uint8_t i = 0; i < info->x_count; i++)
        original_xamps[i] = info->xamp1[i];
    for (uint8_t i = 0; i < info->y_count; i++)
        original_yamps[i] = info->yamp1[i];

    const float new_xamp = new_info.xamp1[new_info.x_count];
    const float new_yamp = new_info.yamp1[new_info.y_count];
    const float xamp_mult =  new_xamp / info->x_count;
    const float yamp_mult =  new_yamp / info->y_count;
    info->xhz[info->x_count] = new_info.xhz[new_info.x_count];
    info->xhz[info->y_count] = new_info.yhz[new_info.y_count];
    const uint8_t xc = info->x_count ++;
    const uint8_t yc = info->y_count ++;

    show_hz(info);
    
    for (float j = 0, k = PI; j < time; j += 1, k += count_inv)
    {
        // float amp_sum = 0;
        const float amp_mult = cosine(k) * 0.5f;
        for (uint8_t i = 0; i < xc; ++i)
            info->xamp1[i] = original_xamps[i] - amp_mult * xamp_mult;
        for (uint8_t i = 0; i < yc; ++i)
            info->yamp1[i] = original_yamps[i] - amp_mult * yamp_mult;

        info->xamp1[xc] = new_xamp * amp_mult;
        info->yamp1[yc] = new_yamp * amp_mult;
        // amp_sum += new_xamp * amp_mult;
        // Serial.println(amp_sum);

        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;
        info->alpha_angle += info->alpha_angle_step;
        wait_then_make(false, info);
    }

    return false;
}

void spiral(ChordInfo *const info)
{

}
// Not Done.
void dissolver(ChordInfo *const info)
{

}
// Not Done
void off_center_twist(ChordInfo *const info)
{

}
// Not Done.
void circle_orbit(ChordInfo *const info, const int time=256)
{    
    static ChordInfo new_info;
    const float count_inv = 0.5f / (float)time * TAU;

    for (uint8_t i = 0; i < info->other_hz_count; ++i)
        new_info.other_hz[i] = info->other_hz[i];
    new_info.other_hz_count = info->other_hz_count;

    make_chord(&new_info, false, info->base_hz, info->x_count + info->y_count);

    // search for the closest hz on the x and y
    uint8_t xhz = 0, yhz = 0;
    float closest = __FLT_MAX__, closeness;
    for (uint8_t x = 0; x < info->x_count; ++x)
    {
        for (uint8_t y = 0; y < info->y_count; ++y)
        {
            closeness = fabsf(info->xhz[x] - info->yhz[y]);
            if (closeness < closest)
            {
                closest = closeness;
                xhz = x;
                yhz = y;
            }
        }
    }

    // equal hz not found, make one instead.
    float new_hz = 0;
    const float max_difference = 0.001f;
    while (closest > max_difference)
    {
        constexpr float MAX_OUT_TUNE = .1f;
        constexpr float HZ_MULT = TAU / 40000.0f;
        for (int i = 0; i < info->other_hz_count; ++i)
        {
            new_hz = info->other_hz[i] + (float)(rand() & 255) / ((255 / MAX_OUT_TUNE) + MAX_OUT_TUNE/2);
            new_hz *= HZ_MULT;
            closest = fabsf(new_hz - info->xhz[xhz]);
            if (closest < max_difference)
                break;
        }
    }

    // merge the new hz into the shape
    if (new_hz != 0)
    {
        const float original_yamp = info->yamp1[yhz];
        const int yc = info->y_count;
        info->yhz[yc] = new_hz;
        info->y_count++;
        
        for (float j = 0, k = 0; j < time; j += 1, k += count_inv)
        {
            const float amp_mult = cosine(k) * 0.5f;
            info->yamp1[yhz] = original_yamp * amp_mult;
            info->yamp1[yc] = original_yamp - info->yamp1[yhz];

            info->rotate_angle_start = info->alpha_angle;
            info->rotate_angle_step = info->alpha_angle_step / (float)LEN;
            info->alpha_angle += info->alpha_angle_step;
            wait_then_make(false, info);
        }

        info->yhz[yhz] = new_hz;
        info->yamp1[yhz] = original_yamp;
        info->y_count--;
    }


    // make a list of the hz's index that can be removed
    float x_original[5], y_original[5];
    for (uint8_t i = 0; i < info->x_count; ++i)
        x_original[i] = info->xamp1[i];
    for (uint8_t i = 0; i < info->y_count; ++i)
        y_original[i] = info->yamp1[i];
    
    // remove all hz's that are not those two close ones at the same time with cosine.
    // and, move to one side a little bit.
    for (float j = 0, k = 0; j < time; j += 1, k += count_inv)
    {
        const float amp_mult = cosine(k) * 0.5f;
        float xhz_add = 0, yhz_add = 0;

        for (uint8_t i = 0, j = 0; i < info->x_count; ++i, ++j)
        {
            if (i != xhz)
            {
                info->xamp1[j] = x_original[j] * amp_mult;
                xhz_add += x_original[j] - info->xamp1[j];
            }
        }

        for (uint8_t i = 0, j = 0; i < info->y_count; ++i, ++j)
        {
            if (i != yhz)
            {
                info->yamp1[j] = y_original[j] * amp_mult;
                yhz_add += y_original[j] - info->yamp1[j];
            }
        }

        info->xamp1[xhz] = x_original[xhz] + xhz_add;
        info->yamp1[yhz] = y_original[yhz] + yhz_add;
        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;
        info->alpha_angle += info->alpha_angle_step;
        wait_then_make(false, info);
    }

    // merge the new hz's into the info struct
    for (uint8_t i = 0; i < info->x_count; ++i)
    {
        if (i != xhz)
        {
            info->xhz[i] = new_info.xhz[i];
            // x_original[i] = new_info.xamp1[i];
        }
    }

    for (uint8_t i = 0; i < info->y_count; ++i)
    {
        if (i != yhz)
        {
            info->yhz[i] = new_info.yhz[i];
            // y_original[i] = new_info.yamp1[i];
        }
    }

    // add the new hz's back to the big "O"
    for (float j = 0, k = PI; j < time; j += 1, k += count_inv)
    {
        const float amp_mult = cosine(k) * 0.5f;
        float xhz_add = 0, yhz_add = 0;

        for (uint8_t i = 0, j = 0; i < info->x_count; ++i, ++j)
        {
            if (i != xhz)
            {
                info->xamp1[j] = x_original[j] * amp_mult;
                xhz_add += x_original[j] - info->xamp1[j];
            }
        }

        for (uint8_t i = 0, j = 0; i < info->y_count; ++i, ++j)
        {
            if (i != yhz)
            {
                info->yamp1[j] = y_original[j] * amp_mult;
                yhz_add += y_original[j] - info->yamp1[j];
            }
        }
        
        info->xamp1[xhz] = x_original[xhz] + xhz_add;
        info->yamp1[yhz] = y_original[yhz] + yhz_add;
        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;
        info->alpha_angle += info->alpha_angle_step;
        wait_then_make(false, info);
    }
}



void start_flow(ChordInfo *const info)
{
    wait_then_make(true, info);

    const float max_amp = .8;
    const float start_grow_speed = 0.005f;
    const float end_shrink_speed = 0.005f;
    const int stable_time = 512;

    // random angle -.3 -> +.3
    const float total_angle = ((float)(rand() & 1023) / (1023/RANDOM_ROTATION_ANGLE/2) - RANDOM_ROTATION_ANGLE) * TAU;
    const float total_steps = (1.0f / start_grow_speed) + (1.0f / end_shrink_speed) + stable_time;
    info->alpha_angle_step = total_angle / total_steps;
    info->alpha_angle = RANDOM_ROTATION_ANGLE * 5 * TAU;

    float offset_add = (1 - max_amp) * 0.5f * 4096;
    for (float amp_mult = 0; amp_mult < 1; amp_mult += start_grow_speed, info->alpha_angle += info->alpha_angle_step)
        grow_shape(info, amp_mult, start_grow_speed, max_amp, offset_add);

    info->x_offset_start += info->x_offset_step;
    info->y_offset_start += info->y_offset_step;
    info->xamp_start += info->yamp_step;
    info->yamp_start += info->xamp_step;

    info->x_offset_step = 0;
    info->y_offset_step = 0;
    info->yamp_step = 0;
    info->xamp_step = 0;
    maintain_shape(512, info);
}

void maintain_and_change(ChordInfo *info, const int wait=400)
{
    switch (rand() & 0b11)
    {
    case 0:
        maintain_shape(600, info);
    case 1:
        reborn(info, wait);
        break;
    case 2:
        reborn(info, wait);
        reborn(info, wait);
        break;
    case 3:
        reborn(info, wait);
        reborn(info, wait);
        // reborn(info, wait);
        break;
    default:
        break;
    }
}

void transition(ChordInfo *info)
{
    static uint8_t previous_num = 255;
    const uint8_t num = random(10);

    // dont' do the same transition twice, and ensure that the color gets changed atleast every other transition
    if (num == previous_num || (previous_num > 5 && num > 5)) 
    {
        transition(info);
        return;
    }

    previous_num = num;

    // remove_one(info);
    // maintain_shape(100, info);
    // cosine_transistion(info);
    // return;


    switch (num)
    {
    case 0:
        cosine_transistion(info);
        break;
    case 1:
        tornado_twist_power4(info);
        break;
    case 2:
        cosine_twister(info);
        break;
    case 3:
        one_twist_in(info);
        break;
    case 4:
        one_twist_out(info);
        break;
    case 5:
        one_twist_in_out(info);
        break;
    case 6:
        reborn(info, 200);
        reborn(info, 300);
        break;
    case 7:
        shaker(info);
        break;
    case 8:
    case 9:
        big_o(info);
        break;
    case 10:
        if (remove_one(info))
            transition(info);
    default:
        break;
    }
    previous_num = num;

    maintain_and_change(info);
}

void flow()
{
    static ChordInfo info = {.r = 152, .g = 152, .b = 152, .center_x = 2048, .center_y = 2048, .rotate_angle = 0};
    start_flow(&info);
    while(1)
    {
        transition(&info);
    }
}

/*
    //the twister: rotate a bunch of times and shrink.
    //the tornado-twist: accellerate the rotational speed until suddenly changing the shape, then slow rotation until still.
    //the cosine: cosine interpolate shrink and grow to the new shape.
    the spiral: spiral a but off center, then spiral inward while shrinking and rotating
    the planet_vanish: go off center, rotate and orbit the center while shrinking to a dot, then while orbiting the center turn back into a new shape.
    the dissolver: try changing the HZ's all at the same time to their new HZ.
    //the shaker: shake either side to side or up and down while shrinking.
    off center twist: move a but to one side, rotate on the center axis and shrink and move back into the center and shrink.
    //UGLY. the fold: shrink only one axis and when the image is a line, then change the HZ's and unfold into the new shape.
    //re-born: change one HZ at a time that still works with the base HZ, until all of them are different.
    // LINES DONT WORK. line spinner: go into a horizontal line, make the line spin slowly, turn into a shape, and then slow down the RPM
    // LINES DONT WORK. round line:   Go into a horizontal line then the line will open into a circle then add components until shape is generated

    // one-twist: do one full rotation, then cosine shrink, then grow while twisting once until back to full size.

    // big "o": remove one HZ at a time, and arrive at a perfect circle by adding a HZ identical to the other one,
    //     then add the new HZ's back onto it. remove the seconds HZ require to make the circle at the end.
    //     other base-shapes could be used, like a "figure 8"

    circle orbit: Going to the horizontal line, then open into a circle that is slightly to the side of the shape space. 
        The circle will be rotating around the center then slowly add components until the shape is generated, 
        then slow down the RPM and go back into center.

    transistion orbit: Move sideways a little bit, then in one revolution around the shape space 
        change components one at a time until the new shape is generated 
        and also return to the original spot to the side and then move to the centre again to full size

    re-born bubble: Change one component at a time. for each component alteration, 
        the X or the Y amplitude will spike up and down and give it a bubbly wobble each time

    escape: Wobble all the way to the side like a bubble, hitting a wall such that when it hits the side, 
        the X side of the shape space the X amplitude will diminish. 
        Bounce to the other side maybe a few times, and then suddenly change the shape, 
        when it hits the side hard enough. Then return into the middle

    spinning ellipse: Remove components until into a circle. Diminish one coordinate amplitude until an ellipse. 
        Rotate until very fast. Adding one component slowly until shape is formed, then show rpm till stop

    planet ellipse: Quickly remove all components until an ellipse and shift to one side. 
        Then orbit the ellipse around Centre while also rotating it in the opposite rotation. 
        meaning that if it orbits clockwise, the rotation will be counterclockwise, 
        and while it’s rotating, add components until the shape is made, then grow and move back to centre and stop rotating and orbiting
    
    
    Think of some primitive shapes that can be used like triangles with rounded edges or something in place of the circles and ellipses That can be interchangeable 
*/
