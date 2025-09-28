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

    
#define TRANSITION_FOR_LOOP_ORBIT for (int j = 0, k = info->t; j < LEN; \
    ++j, ++k, angle+=info->rotate_angle_step, \
    xamp+=info->xamp_step, yamp+=info->yamp_step, \
    x_offset+=info->x_offset_step, y_offset+=info->y_offset_step, \
    x2_offset+=info->x2_offset_step, y2_offset+=info->y2_offset_step, \
    angle2+=info->rotate_angle_step2)

extern volatile Data data[2][256];
static constexpr double TAU = 6.283185307179586;
uint8_t hz_using_arr[8] = {3, 6, 9, 11, 12, 13, 14, 15};

#define ROTATE_CLAMP rotate_point_and_clamp2(&data_array[j], angle, info->center_x, info->center_y);
#define ROTATE rotate_point(&data_array[j], angle, info->center_x, info->center_y);
#define ROTATE_CLAMP2 rotate_point_and_clamp2(&data_array[j], angle2, info->center_x, info->center_y);

#define OFFSET data_array[j].laser_x += x_offset; data_array[j].laser_y += y_offset;
#define OFFSET2 data_array[j].laser_x += x2_offset; data_array[j].laser_y += y2_offset;

#define RANDOM_ROTATION_ANGLE 0.5f


extern inline void write_color(volatile const Data *const info);
const float max_amp = .75;
const int stable_time = 512;

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

    auto append_array_hz = [&](float *arr, int c) {
        for (int i = 0; i < c; i++) {
            ftoa4(tmp, arr[i] / (6.28 / 40000.0f));
            strcat(buf, tmp);
            if (i < 4) strcat(buf, " ");
        }
        strcat(buf, "\n");
    };

    append_array_hz(info->xhz, info->x_count);
    append_array_hz(info->yhz, info->y_count);
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

void rotate_point(volatile Data *const data_array, const float angle, const float cx, const float cy)
{
    const float dx = (float)(data_array->laser_x) - cx;
    const float dy = (float)(data_array->laser_y) - cy;
    const float sin_ = sine(angle) - 1.0f;
    const float cos_ = cosine(angle) - 1.0f;
    data_array->laser_x = (uint16_t)((cx + dx * cos_ - dy * sin_) + 0.5f);
    data_array->laser_y = (uint16_t)((cy + dx * sin_ + dy * cos_) + 0.5f);
}

inline void wait_for_empty_array()
{
    // wait for the interrupts to use up the data.
    volatile bool *const empty = &data[!array_reading][0].empty;
    while (!(*empty));
}

void clean_hz(ChordInfo *info)
{
    float *const amps[2] = {info->xamp1, info->yamp1};
    float *const hzs[2] = {info->xhz, info->yhz};
    const uint8_t counts[2] = {info->x_count, info->y_count};

    for (uint8_t j = 0; j < 2; ++j)
        for (uint8_t i = 0; i < counts[j]; ++i)
            if (amps[j][i] < 2)
                hzs[j][i] = hzs[j][counts[j]-1], 
                amps[j][i] = amps[j][counts[j]-1];
                
    info->x_count--;
    info->y_count--;
}




void transitioner(volatile Data *const data_array, const ChordInfo *const info)
{
    float angle = info->rotate_angle_start;
    float angle2 = info->rotate_angle_start2;
    float xamp = info->xamp_start;
    float yamp = info->yamp_start;
    float x_offset = info->x_offset_start;
    float y_offset = info->y_offset_start;
    float x2_offset = info->x2_offset_start;
    float y2_offset = info->y2_offset_start;
    data_array[0].r  = info->r; 
    data_array[0].g  = info->g; 
    data_array[0].b  = info->b;

    // println(angle, (double)info->rotate_angle_step*1e6, xamp*1e3, info->xamp_step * 1e6, "\n");

    // no offset
    if (x_offset == 0 && y_offset == 0)
    {
        switch (info->x_count << 4 | info->y_count)
        {
        case 1 << 4 | 1: TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y1; ROTATE_CLAMP; } break;
        case 1 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y2; ROTATE_CLAMP; } break;

        case 2 << 4 | 1: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y1; ROTATE_CLAMP; } break;
        case 2 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y2; ROTATE_CLAMP; } break;
        case 2 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y3; ROTATE_CLAMP; } break;

        case 3 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y2; ROTATE_CLAMP; } break;
        case 3 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y3; ROTATE_CLAMP; } break;
        case 3 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y4; ROTATE_CLAMP; } break;

        case 4 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y3; ROTATE_CLAMP; } break;
        case 4 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y4; ROTATE_CLAMP; } break;
        case 4 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y5; ROTATE_CLAMP; } break;

        case 5 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y4; ROTATE_CLAMP; } break;
        case 5 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y5; ROTATE_CLAMP; } break;
        case 5 << 4 | 6: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y6; ROTATE_CLAMP; } break;

        case 6 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X6; SET_Y5; ROTATE_CLAMP; } break;
        case 6 << 4 | 6: TRANSITION_FOR_LOOP { SET_COLOR; SET_X6; SET_Y6; ROTATE_CLAMP; } break;

        default: break;
        }
    }

    else if (info->orbit)
    {
        switch (info->x_count << 4 | info->y_count)
        {
        case 1 << 4 | 1: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X1; SET_Y1; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;
        case 1 << 4 | 2: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X1; SET_Y2; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;

        case 2 << 4 | 1: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X2; SET_Y1; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;
        case 2 << 4 | 2: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X2; SET_Y2; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;
        case 2 << 4 | 3: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X2; SET_Y3; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;

        case 3 << 4 | 2: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X3; SET_Y2; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;
        case 3 << 4 | 3: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X3; SET_Y3; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;
        case 3 << 4 | 4: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X3; SET_Y4; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;

        case 4 << 4 | 3: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X4; SET_Y3; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;
        case 4 << 4 | 4: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X4; SET_Y4; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;
        case 4 << 4 | 5: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X4; SET_Y5; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;

        case 5 << 4 | 4: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X5; SET_Y4; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;
        case 5 << 4 | 5: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X5; SET_Y5; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;
        case 5 << 4 | 6: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X5; SET_Y6; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;

        case 6 << 4 | 5: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X6; SET_Y5; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;
        case 6 << 4 | 6: TRANSITION_FOR_LOOP_ORBIT { SET_COLOR; SET_X6; SET_Y6; OFFSET; ROTATE; OFFSET2; ROTATE_CLAMP2 } break;

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

        case 2 << 4 | 1: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
        case 2 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
        case 2 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y3; OFFSET; ROTATE_CLAMP; } break;

        case 3 << 4 | 2: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
        case 3 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
        case 3 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y4; OFFSET; ROTATE_CLAMP; } break;

        case 4 << 4 | 3: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
        case 4 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
        case 4 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

        case 5 << 4 | 4: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
        case 5 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y5; OFFSET; ROTATE_CLAMP; } break;
        case 5 << 4 | 6: TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y6; OFFSET; ROTATE_CLAMP; } break;

        case 6 << 4 | 5: TRANSITION_FOR_LOOP { SET_COLOR; SET_X6; SET_Y5; OFFSET; ROTATE_CLAMP; } break;
        case 6 << 4 | 6: TRANSITION_FOR_LOOP { SET_COLOR; SET_X6; SET_Y6; OFFSET; ROTATE_CLAMP; } break;

        default: break;
        }
    }
}

void make_shape(const bool new_shape, ChordInfo *info, bool new_color = true, const bool same_count=false)
{
    if (new_shape)
    {
        info->t = 0;
        if (same_count)
            make_chord(info, false, 0.0f, hz_count_to_num_hz(info->x_count, info->y_count));
        else
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

void wait_then_make(const bool new_shape, ChordInfo *info, bool new_color=true, const bool same_count=false)
{
    pin10of;
    wait_for_empty_array();
    pin10on;
    make_shape(new_shape, info, new_color, same_count);
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

void grow_shape_calc(ChordInfo *const info, const float amp_mult,
                const float start_grow_speed, const float max_amp,
                const float offset_add)
{
    // base angle
    const float u = ((amp_mult + 1.5f) * 0.5f) * TAU;

    // amplitude at current amp_mult
    const float new_amp = sine(u) * 0.5f;        // same as original

    // offset at current amp_mult
    const float offset  = (1.0f - new_amp) * 2048;

    // derivative of amplitude wrt amp_mult
    const float d_amp = (TAU * 0.25f) * cosine(u);

    // derivative of offset wrt amp_mult
    const float d_offset = -d_amp * 2048;

    // steps via calculus (derivative * delta / LEN)
    const float amp_step    = (d_amp    * start_grow_speed) / (float)LEN;
    const float offset_step = (d_offset * start_grow_speed) / (float)LEN;

    // fill info struct
    info->rotate_angle_start = info->alpha_angle;
    info->rotate_angle_step  = info->alpha_angle_step / (float)LEN;

    info->x_offset_start = offset * max_amp + offset_add;
    info->x_offset_step  = offset_step * max_amp;

    info->y_offset_start = info->x_offset_start;
    info->y_offset_step  = info->x_offset_step;

    info->xamp_start = new_amp * max_amp;
    info->xamp_step  = amp_step * max_amp;

    info->yamp_start = info->xamp_start;
    info->yamp_step  = info->xamp_step;

    wait_then_make(false, info);
}

void cosine_transistion(ChordInfo *const info, float time_shrink=0, float time_grow=0)
{
    // between 1 -> 2
    if (time_shrink==0)
        time_shrink = (float)(rand() & 255) / 255.0f + 1.0f;

    // within 1/8 of a second of the shrink speed
    if (time_grow==0)
        time_grow = (float)(rand() & 255) / 255.0f / 4.0f - 0.125f + time_shrink;

    const float start_grow_speed = 0.000025 * LEN / time_grow ;
    const float end_shrink_speed = 0.000025 * LEN / time_shrink ;
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

void maintain_shape(int stable_time_, ChordInfo *info)
{
    for (int i = 0; i < stable_time_; ++i, info->alpha_angle += info->alpha_angle_step)
    {
        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;
        wait_then_make(false, info);
    }
}

void cosine_twister_iterations(const ChordInfo *const info, int *first, float *next_alpha_angle, 
    float *next_alpha_angle_step, float *solved_x, float *y_adder, const float direction=1)
{
    const float start_grow_speed = 0.005f;
    const float end_shrink_speed = 0.005f;
    const float angle_step = 0.01f;
    const float twist_count = 2.5;
    const float angle_mult = 1.0f;
    *solved_x = cbrtf(info->alpha_angle_step / angle_step / 4.0f / angle_mult * direction);
    *y_adder = *solved_x * *solved_x * *solved_x * *solved_x;
    const float stop_slope = 4.0f * twist_count * twist_count * twist_count * angle_mult*angle_step;

    for (float deg = 0, deg2 = 0, i = 0, j = angle_step;
        deg2-deg < stop_slope;
        i += angle_step, j += angle_step, *first = *first+1)
    {
        float ii = (i + *solved_x);
        float jj = (j + *solved_x);
        deg = ii * ii * ii * ii * angle_mult + info->alpha_angle - * y_adder;
        deg2 = jj * jj * jj * jj * angle_mult + info->alpha_angle - * y_adder;
    }

    // random angle -.3 -> +.3
    const float total_angle = ((float)(rand() & 1023) / (1023/RANDOM_ROTATION_ANGLE)) * TAU;
    const float total_steps = (1.0f / start_grow_speed) + (1.0f / end_shrink_speed) + stable_time - 2.0f;
    *next_alpha_angle_step = total_angle / total_steps;
    *next_alpha_angle = RANDOM_ROTATION_ANGLE * 7 * TAU;
}

void cosine_twister(ChordInfo *const info)
{
    const float direction = 1; // -1 or +1
    int first_iterations = 0;
    float next_alpha_angle = 0, next_alph_angle_step = 0, solved_x, y_adder;
    cosine_twister_iterations(info, &first_iterations, &next_alpha_angle, &next_alph_angle_step, &solved_x, &y_adder, direction);
    const float angle_step = 0.01f;
    const float twist_count = 2.5;
    const float angle_mult = 1.0f;
    const float amp_step = TAU * 0.45f / first_iterations;
    float deg2 = 0;
    const float offset_add = (1 - max_amp) * 0.5f * 4096;

    if (direction == 1)
    {
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
    }

    else
    {
        const float original_start_angle = info->rotate_angle_start + PI;
        for (float deg = 0, i = 0, j = angle_step, count = 0, amp = 0;
            count < first_iterations;
            i += angle_step, j += angle_step, ++count, amp += amp_step)
        {
            float ii = (i + solved_x);
            float jj = (j + solved_x);
            deg = ii * ii * ii * ii * angle_mult + info->alpha_angle - y_adder;
            deg2 = jj * jj * jj * jj * angle_mult + info->alpha_angle - y_adder;
            info->rotate_angle_start = original_start_angle - deg;
            // println(deg, info->rotate_angle_start, original_start_angle, "");
            info->rotate_angle_step = -(deg2 - deg) / (float)LEN;
            // info->rotate_angle_step = -(ii*ii*ii * 4 * angle_mult) / (float)LEN;

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

void shake_calc(ChordInfo *const info, const int i, const float center, 
    const float mult = 25*PI, const float shake_mult = 60/*was 40*/)
{
    info->rotate_angle_start = info->alpha_angle;
    info->rotate_angle_step = info->alpha_angle_step / (float)LEN;

    float ex = e_to_x((float)i * 4.096f);
    float ex_plus1 = 1 + ex;
    float der1 = (cosine(mult / ex_plus1) - 1) * mult * ex / (ex_plus1 * ex_plus1) * shake_mult;
    info->x_offset_start = der1 + center; 

    ex = e_to_x((float)(i+1) * 4.096f);
    ex_plus1 = 1 + ex;
    float der2 = (cosine(mult / ex_plus1) - 1) * mult * ex / ex_plus1 / ex_plus1 * shake_mult;
    info->x_offset_step = (der2 - der1) / LEN;  
    wait_then_make(false, info);
}

void shaker(ChordInfo *const info)
{
    if (info->x_count == 6 || info->y_count == 6)
        return;

    int mult = (rand()&7) + 19; // 19 -> 26

    int i = 100;
    const float center = info->x_offset_start;
    const int first_part = 390;
    const int total_transition_count = 610;
    const int ex_end = 900;

    ChordInfo new_info = {.x_count = info->x_count, .y_count = info->y_count};
    make_chord(&new_info, false, 0.0f, hz_count_to_num_hz(new_info.x_count, new_info.y_count));

    for (; i < first_part; ++i, info->alpha_angle += info->alpha_angle_step)
        shake_calc(info, i, center, mult*PI);
    
    int count_per_xhz = (int)((total_transition_count - first_part) / (float)info->x_count + 0.5f);
    int count_per_yhz = (int)((total_transition_count - first_part) / (float)info->y_count + 0.5f);
    const float mult_add_x = PI / (float)count_per_xhz;
    const float mult_add_y = PI / (float)count_per_yhz;
    float amp_mult_shrink_x = TAU;
    float amp_mult_shrink_y = TAU;

    info->xhz[info->x_count] = new_info.xhz[0];
    info->yhz[info->y_count] = new_info.yhz[0];
    float original_xamp = info->xamp1[0];
    float original_yamp = info->yamp1[0];
    const uint8_t x_count = info->x_count++;
    const uint8_t y_count = info->y_count++;
    info->xamp1[x_count] = 0;
    info->yamp1[y_count] = 0;
    int xc = 0, yc = 0, xhz = 0, yhz = 0;


    for (; i < total_transition_count; 
        ++i, info->alpha_angle += info->alpha_angle_step, ++xc, ++yc, 
        amp_mult_shrink_x -= mult_add_x, amp_mult_shrink_y -= mult_add_y)
    {
        float mul = cosine(amp_mult_shrink_x) * 0.5f;
        info->xamp1[xhz] = original_xamp * mul;
        info->xamp1[x_count] = original_xamp * (1-mul);
        
        mul = cosine(amp_mult_shrink_y) * 0.5f;
        info->yamp1[yhz] = original_yamp * mul;
        info->yamp1[y_count] = original_yamp * (1-mul);

        if (xc == count_per_xhz)
        {
            info->xamp1[x_count] = 0;
            info->xamp1[xhz] = original_xamp;
            xc = 0;
            amp_mult_shrink_x = TAU;
            info->xhz[xhz] = new_info.xhz[xhz];
            original_xamp = info->xamp1[++xhz];
            info->xhz[x_count] = new_info.xhz[xhz];
        }  

        if (yc == count_per_yhz)
        {
            info->yamp1[y_count] = 0;
            info->yamp1[yhz] = original_yamp;
            yc = 0;
            amp_mult_shrink_y = TAU;
            info->yhz[yhz] = new_info.yhz[yhz];
            original_yamp = info->yamp1[++yhz];
            info->yhz[y_count] = new_info.yhz[yhz];
        }
                
        shake_calc(info, i, center, mult*PI);
    }

    shake_calc(info, i, center, mult*PI);

    for (; i < ex_end; ++i, info->alpha_angle += info->alpha_angle_step)
        shake_calc(info, i, center, mult*PI);

    clean_hz(info);
}

void tornado_twist_power2(ChordInfo *const info)
{
    const float start_grow_speed = 0.005f;
    const float end_shrink_speed = 0.005f;
    const float angle_step = 0.01f;
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
    if (info->x_count == 6 || info->y_count == 6)
        return;

    const float start_grow_speed = 0.005f;
    const float end_shrink_speed = 0.005f;
    const float angle_step = (float)(rand() & 255) / 63750.0f + 0.003f; // .003->.007
    const float twist_count = (float)(rand() & 255) / 510.0f + 2.2f; // 2.2->2.7    2.7 is the max value until for some reason the shape turns into a point. during the transition.

    float solved_x = cbrtf(info->alpha_angle_step / angle_step / 4.0f);
    float y_adder = solved_x * solved_x * solved_x * solved_x;
    const float stop_slope = 4.0f * twist_count * twist_count * twist_count *angle_step;
    float deg2 = 0;

    // approximate number of frames:
    float twist_time = -146.8796 -141950.2655*angle_step +964.1840*twist_count +25247540.5481*angle_step*angle_step -110667.1081*angle_step* twist_count +7.8001*twist_count*twist_count;

    int start_changing = twist_time * 0.25f;
    int stop_changing = twist_time * 0.75f;

    ChordInfo new_info = {.x_count = info->x_count, .y_count = info->y_count};
    make_chord(&new_info, false, 0.0f, hz_count_to_num_hz(new_info.x_count, new_info.y_count));
    
    int count_per_xhz = (int)(twist_time / (float)info->x_count * 0.5f + 0.5f);
    int count_per_yhz = (int)(twist_time / (float)info->y_count * 0.5f + 0.5f);
    const float mult_add_x = PI / (float)count_per_xhz;
    const float mult_add_y = PI / (float)count_per_yhz;
    float amp_mult_shrink_x = TAU;
    float amp_mult_shrink_y = TAU;

    info->xhz[info->x_count] = new_info.xhz[0];
    info->yhz[info->y_count] = new_info.yhz[0];
    float original_xamp = info->xamp1[0];
    float original_yamp = info->yamp1[0];
    const uint8_t x_count = info->x_count++;
    const uint8_t y_count = info->y_count++;
    info->xamp1[x_count] = 0;
    info->yamp1[y_count] = 0;
    int xc = 0, yc = 0, xhz = 0, yhz = 0, counter = 0;

    int count = 0;
    for (float deg = 0, i = 0, j = angle_step;
        deg2-deg < stop_slope;
        i += angle_step, j += angle_step, ++counter)
    {
        float ii = (i + solved_x);
        float jj = (j + solved_x);
        deg = ii * ii * ii * ii + info->alpha_angle - y_adder;
        deg2 = jj * jj * jj * jj + info->alpha_angle - y_adder;
        info->rotate_angle_start = deg;
        info->rotate_angle_step = (deg2 - deg) / (float)LEN;
        count++;

        if (counter > start_changing && counter < stop_changing)
        {
            float mul = cosine(amp_mult_shrink_x) * 0.5f;
            info->xamp1[xhz] = original_xamp * mul;
            info->xamp1[x_count] = original_xamp * (1-mul);
            
            mul = cosine(amp_mult_shrink_y) * 0.5f;
            info->yamp1[yhz] = original_yamp * mul;
            info->yamp1[y_count] = original_yamp * (1-mul);

            if (xc == count_per_xhz)
            {
                info->xamp1[x_count] = 0;
                info->xamp1[xhz] = original_xamp;
                xc = 0;
                amp_mult_shrink_x = TAU;
                info->xhz[xhz] = new_info.xhz[xhz];
                original_xamp = info->xamp1[++xhz];
                info->xhz[x_count] = new_info.xhz[xhz];
            }  

            if (yc == count_per_yhz)
            {
                info->yamp1[y_count] = 0;
                info->yamp1[yhz] = original_yamp;
                yc = 0;
                amp_mult_shrink_y = TAU;
                info->yhz[yhz] = new_info.yhz[yhz];
                original_yamp = info->yamp1[++yhz];
                info->yhz[y_count] = new_info.yhz[yhz];
            }
            
            ++xc;
            ++yc;
            amp_mult_shrink_x -= mult_add_x;
            amp_mult_shrink_y -= mult_add_y;
        }  

        wait_then_make(false, info);
    }
    info->rotate_angle_start = deg2; 

    // random angle -.3 -> +.3
    const float total_angle = ((float)(rand() & 1023) / (1023/RANDOM_ROTATION_ANGLE)) * TAU;
    const float total_steps = (1.0f / start_grow_speed) + (1.0f / end_shrink_speed) + stable_time - 2.0f;
    info->alpha_angle_step = total_angle / total_steps;
    info->alpha_angle = RANDOM_ROTATION_ANGLE * 7 * TAU;
    float end_x = twist_count - cbrtf(info->alpha_angle_step / angle_step / 4.0f);
    const float adder = fabsf(-twist_count * twist_count * twist_count * twist_count + info->alpha_angle) + info->rotate_angle_start;

    wait_then_make(false, info);
    for (float deg = 0, deg2 = 0, i = 0, j = angle_step;
        i < end_x;
        i += angle_step, j += angle_step, ++counter)
    {
        float ii = (twist_count - i);
        float jj = (twist_count - j);
        deg = -ii * ii * ii * ii + info->alpha_angle + adder;
        deg2 = -jj * jj * jj * jj + info->alpha_angle + adder;
        info->rotate_angle_start = deg;
        info->rotate_angle_step = (deg2 - deg) / (float)LEN;

        if (counter > start_changing && counter < stop_changing)
        {
            float mul = cosine(amp_mult_shrink_x) * 0.5f;
            info->xamp1[xhz] = original_xamp * mul;
            info->xamp1[x_count] = original_xamp * (1-mul);
            
            mul = cosine(amp_mult_shrink_y) * 0.5f;
            info->yamp1[yhz] = original_yamp * mul;
            info->yamp1[y_count] = original_yamp * (1-mul);

            if (xc == count_per_xhz)
            {
                info->xamp1[x_count] = 0;
                info->xamp1[xhz] = original_xamp;
                xc = 0;
                amp_mult_shrink_x = TAU;
                info->xhz[xhz] = new_info.xhz[xhz];
                original_xamp = info->xamp1[++xhz];
                info->xhz[x_count] = new_info.xhz[xhz];
            }  

            if (yc == count_per_yhz)
            {
                info->yamp1[y_count] = 0;
                info->yamp1[yhz] = original_yamp;
                yc = 0;
                amp_mult_shrink_y = TAU;
                info->yhz[yhz] = new_info.yhz[yhz];
                original_yamp = info->yamp1[++yhz];
                info->yhz[y_count] = new_info.yhz[yhz];
            }

            ++xc;
            ++yc;
            amp_mult_shrink_x -= mult_add_x;
            amp_mult_shrink_y -= mult_add_y;
        }  

        wait_then_make(false, info);
        count++;
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

    clean_hz(info);
}

bool reborn(ChordInfo *const info, int time=0)
{
    if (info->x_count == 5 || info->y_count == 5)
        return false;

    static ChordInfo new_info;
    for (uint8_t i = 0; i < info->other_hz_count; ++i)
        new_info.other_hz[i] = info->other_hz[i];
    new_info.other_hz_count = info->other_hz_count;

    make_chord(&new_info, true);

    // between 5 -> 1.5 seconds
    if (time==0)
        time = (float)(rand() & 511) + 256;
    else if (time > 5)
        time = (float)(time & 511) + 256;
    else
        time = (float)time / (LEN * 0.000025);

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
    for (float j = 0, k = 0; j < time; j++, k += count_inv)
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

int num_steps_twist_in_remove(ChordInfo *info, const float grow_speed, const bool shrink, const float start_mult)
{
    const float deg_step = grow_speed * PI * 0.5f;
    const int add_or_remove = (info->rotate_angle_step > 0) * 2 - 1;

    if (shrink)
    {
        // compute ratio
        float ratio = (2.0f * info->rotate_angle_step * add_or_remove * LEN) / (TAU * PI * deg_step);

        // clamp to [-1,1] to stay safe
        if (ratio > 1.0f) ratio = 1.0f;
        if (ratio < -1.0f) ratio = -1.0f;

        // convert to step count
        return (int)(asinf(ratio) / deg_step + 0.5f) * add_or_remove;
    }

    const float matching_slope = info->alpha_angle_step / (float)LEN;
    const float t = 0.5f * PI * TAU;

    // compute threshold value for sin
    float target = (matching_slope * LEN) / (t * deg_step);

    // clamp, since asin only works for [-1,1]
    if (target > 1.0f) target = 1.0f;
    if (target < -1.0f) target = -1.0f;

    // solve for degree analytically
    float deg_solution = PI - asinf(target);   // adjust into correct quadrant

    return (int)((deg_solution - (PI * 0.5f)) / deg_step);
}

void one_twist_in(ChordInfo *const info)
{
    const float start_mult = 0.1f;
    float grow_speed = (float)(rand()&255) / (256.0f * 300.0f) + 0.001f; // .001 -> .005
    int num_steps_add = num_steps_twist_in_remove(info, grow_speed, true, start_mult);
    const float offset_add = (1 - max_amp) * 0.5f * 4096;
    float deg_step = grow_speed * PI * 0.5f;
    const float original_angle = info->rotate_angle_start - (1.0f - cosine(num_steps_add*deg_step+TAU - deg_step) * 0.5f) * TAU * PI;
    

    for (float deg = num_steps_add*deg_step+TAU, amp_mult = 1; amp_mult > start_mult; 
        deg += deg_step, amp_mult -= grow_speed, info->alpha_angle += info->alpha_angle_step)
    {
        const float cosine_angle = (1.0f - cosine(deg) * 0.5f) * TAU * PI;
        info->rotate_angle_step = (0.5f * TAU * PI * (sine(deg) - 1.0f) * deg_step) / (float)LEN;
        info->rotate_angle_start = cosine_angle + original_angle;
        const float new_amp = sine(((amp_mult + 1.5f) * 0.5f) * TAU) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        const float new_amp2 = sine(((amp_mult + 1.5f + grow_speed) * 0.5f) * TAU) * 0.5f;
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




    const float start_mult = 0.1;

    float grow_speed = (float)(rand()&255) / (256.0f * 300.0f) + 0.001f; // .001 -> .005

    // random angle -.3 -> +.3
    const float total_angle = ((float)(rand() & 1023) / (1023/RANDOM_ROTATION_ANGLE/2) - RANDOM_ROTATION_ANGLE) * TAU;
    const float total_steps = (1.0f / grow_speed) + (1.0f / grow_speed) + stable_time - 2.0f;
    info->alpha_angle_step = total_angle / total_steps;
    info->alpha_angle = RANDOM_ROTATION_ANGLE * 5 * TAU;

    float num_steps_add = num_steps_twist_in_remove(info, grow_speed, false, start_mult);
    wait_then_make(true, info);

    float deg_step = grow_speed * PI * 0.5f;
    grow_speed = (1.0f - start_mult) / num_steps_add;

    for (float deg = PI * 0.5f + deg_step, amp_mult = start_mult; amp_mult < 1; 
        deg += deg_step, amp_mult += grow_speed, info->alpha_angle += info->alpha_angle_step)
    {
        const float cosine_angle = 3*TAU - cosine(deg) * PI * PI;
        // info->rotate_angle_step = ((TAU - cosine(deg + deg_step) * PI * PI) - cosine_angle) / (float)LEN;  
        info->rotate_angle_step = (PI * PI * (sine(deg) - 1.0f) * deg_step) / (float)LEN;
        info->rotate_angle_start = cosine_angle;// + original_angle;
        const float new_amp = sine(((amp_mult + 1.5f) * 0.5f) * TAU) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        const float new_amp2 = sine(((amp_mult + 1.5f + grow_speed) * 0.5f) * TAU) * 0.5f;
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
    const float start_mult = 0.1f;
    float grow_speed = (float)(rand()&255) / (256.0f * 300.0f) + 0.001f; // .001 -> .005
    int num_steps_add = num_steps_twist_in_remove(info, grow_speed, true, start_mult);
    const float offset_add = (1 - max_amp) * 0.5f * 4096;
    float deg_step = grow_speed * PI * 0.5f;
    const float original_angle = info->rotate_angle_start - (1.0f - cosine(num_steps_add*deg_step+TAU - deg_step) * 0.5f) * TAU * PI;


    for (float deg = num_steps_add*deg_step+TAU, amp_mult = 1; amp_mult > start_mult; 
        deg += deg_step, amp_mult -= grow_speed, info->alpha_angle += info->alpha_angle_step)
    {
        const float cosine_angle = (1.0f - cosine(deg) * 0.5f) * TAU * PI;
        info->rotate_angle_step = (0.5f * TAU * PI * (sine(deg) - 1.0f) * deg_step) / (float)LEN;
        info->rotate_angle_start = cosine_angle + original_angle;
        const float new_amp = sine(((amp_mult + 1.5f) * 0.5f) * TAU) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        const float new_amp2 = sine(((amp_mult + 1.5f + grow_speed) * 0.5f) * TAU) * 0.5f;
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

    grow_speed = (float)(rand()&255) / (256.0f * 300.0f) + 0.001f; // .001 -> .005

    // random angle -.3 -> +.3
    const float total_angle = ((float)(rand() & 1023) / (1023/RANDOM_ROTATION_ANGLE/2) - RANDOM_ROTATION_ANGLE) * TAU;
    const float total_steps = (1.0f / grow_speed) + (1.0f / grow_speed) + stable_time - 2.0f;
    info->alpha_angle_step = total_angle / total_steps;
    info->alpha_angle = RANDOM_ROTATION_ANGLE * 5 * TAU;

    num_steps_add = num_steps_twist_in_remove(info, grow_speed, false, start_mult);
    wait_then_make(true, info);

    deg_step = grow_speed * PI * 0.5f;
    grow_speed = (1.0f - start_mult) / num_steps_add;

    for (float deg = PI * 0.5f + deg_step, amp_mult = start_mult; amp_mult < 1; 
        deg += deg_step, amp_mult += grow_speed, info->alpha_angle += info->alpha_angle_step)
    {
        const float cosine_angle = TAU - cosine(deg) * PI * PI;
        info->rotate_angle_step = (PI * PI * (sine(deg) - 1.0f) * deg_step) / (float)LEN;
        info->rotate_angle_start = cosine_angle + original_angle;
        const float new_amp = sine(((amp_mult + 1.5f) * 0.5f) * TAU) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        const float new_amp2 = sine(((amp_mult + 1.5f + grow_speed) * 0.5f) * TAU) * 0.5f;
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

void big_o(ChordInfo *const info)
{    

    // verify at least one xhz is low enough
    // Serial.println("START");

    const int time = (rand() & 511) + 300;
    static ChordInfo new_info;
    const float count_inv = 0.5f / (float)time * TAU;
    constexpr float max_hz = 500.0 / 40000.0 * 6.28;
    {
        bool good = false;
        for (uint8_t i = 0; i < info->x_count && !good; ++i)
            if (info->xhz[i] < max_hz)
                good = true;
        if (good) goto HAS_SMALL_X;

        // Serial.println("NEED CHANGE");
        float new_hz = 0;
        uint8_t xhz = 0;
        // chose an xhz that is low enough
        constexpr float MAX_OUT_TUNE = .1f;
        constexpr float HZ_MULT = TAU / 40000.0f;
        for (int i = 0; i < info->other_hz_count; ++i)
        {
            new_hz = info->other_hz[i] + (float)(rand() & 255) / ((255 / MAX_OUT_TUNE) + MAX_OUT_TUNE/2);
            new_hz *= HZ_MULT;
            if (new_hz < max_hz)
                break;
        }

        // merge the new hz into the shape
        if (new_hz != 0)
        {
            const float original_xamp = info->xamp1[xhz];
            const int xc = info->x_count;
            info->xhz[xc] = new_hz;
            info->x_count++;
            
            for (float j = 0, k = 0; j < time; j += 1, k += count_inv)
            {
                const float amp_mult = cosine(k) * 0.5f;
                info->xamp1[xhz] = original_xamp * amp_mult;
                info->xamp1[xc] = original_xamp - info->xamp1[xhz];

                info->rotate_angle_start = info->alpha_angle;
                info->rotate_angle_step = info->alpha_angle_step / (float)LEN;
                info->alpha_angle += info->alpha_angle_step;
                wait_then_make(false, info);
            }

            info->xhz[xhz] = new_hz;
            info->xamp1[xhz] = original_xamp;
            info->x_count--;
        }
    }

    HAS_SMALL_X:

    for (uint8_t i = 0; i < info->other_hz_count; ++i)
        new_info.other_hz[i] = info->other_hz[i];
    new_info.other_hz_count = info->other_hz_count;

    make_chord(&new_info, false, info->base_hz, hz_count_to_num_hz(info->x_count, info->y_count));

    // search for the closest hz on the x and y
    uint8_t xhz = 0, yhz = 0;
    float closest = __FLT_MAX__, closeness;
    for (uint8_t x = 0; x < info->x_count; ++x)
    {
        if (info->xhz[x] > max_hz) continue;
        for (uint8_t y = 0; y < info->y_count; ++y)
        {
            if (info->yhz[y] > max_hz) continue;
            closeness = fabsf(info->xhz[x] - info->yhz[y]);
            if (closeness < closest)
            {
                closest = closeness;
                xhz = x;
                yhz = y;
            }
        }
    }
    // if (closest > 0.001f) Serial.println("NO CLOSE");
    // Serial.println(closest, 7);

    // equal hz not found, make one instead.
    float new_hz = 0;
    const float max_difference = 0.001f;
    // chose an xhz that is low enough
    for (uint8_t x = 0; x < info->x_count; ++x)
    {
        if (info->xhz[x] > max_hz) continue;
        {
            xhz = x;
            break;
        }
    }
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

    
    // Serial.println(info->xhz[xhz] * 40000 / 6.28);
    // Serial.println(info->yhz[yhz] * 40000 / 6.28);
    // Serial.println(new_hz * 40000 / 6.28);
    // Serial.println();

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

    // show_hz(info);
    
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
        // Serial.print(info->xamp1[xhz]);
        // Serial.print(", ");
        // Serial.println(info->yamp1[yhz]);
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
        }
    }

    for (uint8_t i = 0; i < info->y_count; ++i)
    {
        if (i != yhz)
        {
            info->yhz[i] = new_info.yhz[i];
        }
    }    
    
    info->x_count = new_info.x_count;
    info->y_count = new_info.y_count;


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

void remove_one(ChordInfo *const info)
{
    static int time = random(210, 410);
    const float count_inv = 0.5f / (float)time * TAU;
    const float xadd = info->xamp1[info->x_count - 1] / (float)(info->x_count - 1);
    const float yadd = info->yamp1[info->y_count - 1] / (float)(info->y_count - 1);

    static float original_xamps[sizeof(info->xamp) / sizeof(float)], original_yamps[sizeof(info->xamp) / sizeof(float)];
    for (uint8_t i = 0; i < info->x_count; i++)
        original_xamps[i] = info->xamp1[i];
    for (uint8_t i = 0; i < info->y_count; i++)
        original_yamps[i] = info->yamp1[i];

    const uint8_t xi = info->x_count - 1;
    const uint8_t yi = info->y_count - 1;

    for (float j = 0, k = 0; j < time; j += 1, k += count_inv)
    {
        const float amp_mult = cosine(k) * 0.5f;
        info->xamp1[xi] = original_xamps[xi] * amp_mult;
        info->yamp1[yi] = original_yamps[yi] * amp_mult;
        
        for (uint8_t i = 0; i < xi; ++i)
            info->xamp1[i] = original_xamps[i] + (1.0f - amp_mult) * xadd;
        for (uint8_t i = 0; i < yi; ++i)
            info->yamp1[i] = original_yamps[i] + (1.0f - amp_mult) * yadd;

        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;
        info->alpha_angle += info->alpha_angle_step;
        wait_then_make(false, info);
    }

    info->x_count--;
    info->y_count--;
}

bool add_one(ChordInfo *const info)
{
    if (info->x_count == (sizeof(info->xamp) / sizeof(float)) || info->y_count == (sizeof(info->yamp) / sizeof(float)))
        return true;

    static int time = random(210, 410);

    ChordInfo new_info = {.x_count = (uint8_t)(info->x_count + 1), .y_count = (uint8_t)(info->y_count + 1)};
    make_chord(&new_info, false, 0.0f, hz_count_to_num_hz(new_info.x_count, new_info.y_count));

    const float count_inv = 0.5f / (float)time * TAU;
    const float xadd = new_info.xamp1[0] / (float)info->x_count;
    const float yadd = new_info.yamp1[0] / (float)info->y_count;

    static float original_xamps[sizeof(info->xamp) / sizeof(float)], original_yamps[sizeof(info->xamp) / sizeof(float)];
    for (uint8_t i = 0; i < info->x_count; i++)
        original_xamps[i] = info->xamp1[i];
    for (uint8_t i = 0; i < info->y_count; i++)
        original_yamps[i] = info->yamp1[i];

    const uint8_t xi = info->x_count;
    const uint8_t yi = info->y_count;
    info->xhz[xi] = new_info.xhz[0];
    info->yhz[yi] = new_info.yhz[0];
    info->x_count++;
    info->y_count++;

    for (float j = 0, k = PI; j < time; j += 1, k += count_inv)
    {
        const float amp_mult = cosine(k) * 0.5f;
        info->xamp1[xi] = new_info.xamp1[0] * amp_mult;
        info->yamp1[yi] = new_info.yamp1[0] * amp_mult;
        
        for (uint8_t i = 0; i < xi; ++i)
            info->xamp1[i] = original_xamps[i] - amp_mult * xadd;
        for (uint8_t i = 0; i < yi; ++i)
            info->yamp1[i] = original_yamps[i] - amp_mult * yadd;

        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;
        info->alpha_angle += info->alpha_angle_step;
        wait_then_make(false, info); 
    }

    return false;
}

void around_the_world(ChordInfo *const info)
{    
    const float start_mult = 0.1f;
    float grow_speed = (float)(rand()&255) / (256.0f * 300.0f) + 0.001f; // .001 -> .005
    const float offset_add = (1 - max_amp) * 0.5f * 4096;
    float cosine_adder = grow_speed * PI / (1.0f - start_mult);
    float rotate_adder = cosine_adder * 0.5f;
    info->orbit = true;
    int rotations = (rand() & 3) + 1; // 1 -> 32
    const float offset_mult = 2048;
    const float rotate2_mult = rotations * TAU;
    float cos = PI;
    float rotate_cos = PI;
    float ex = 0;
    float ex_step = grow_speed * (ex_len>>1) / (1.0f - start_mult);

    // rotations += 2;
    // if (rotations > 40)
    //     rotations = 1;

    for (float amp_mult = 1; amp_mult > start_mult; 
        amp_mult -= grow_speed, info->alpha_angle += info->alpha_angle_step, cos += cosine_adder, rotate_cos += rotate_adder, ex += ex_step)
    {
        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;

        const float new_amp = sine(((amp_mult + 1.5f) * 0.5f) * TAU) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        const float new_amp2 = sine(((amp_mult + 1.5f + grow_speed) * 0.5f) * TAU) * 0.5f;
        const float offset2 = (1.0f - new_amp2) * 2048;

        info->x_offset_start = offset * max_amp + offset_add;
        info->x_offset_step = (offset2 - offset) / (float)LEN;
        info->y_offset_start = info->x_offset_start;
        info->y_offset_step = info->x_offset_step;
        info->xamp_start = new_amp * max_amp;
        info->xamp_step = (new_amp2 - new_amp) / (float)LEN;
        info->yamp_start = info->xamp_start;
        info->yamp_step = info->xamp_step;

        info->rotate_angle_start2 = e_to_x(ex)/(1+e_to_x(ex)) * rotate2_mult;
        info->x2_offset_start = cosine(cos) * 0.5f * offset_mult;

        info->x2_offset_step = (cosine(cos + cosine_adder) * 0.5f * offset_mult - info->x2_offset_start) / (float)LEN;
        info->rotate_angle_step2 = (e_to_x(ex + ex_step)/(1+e_to_x(ex + ex_step)) * rotate2_mult - info->rotate_angle_start2) / (float)LEN;
        wait_then_make(false, info);
    } 

    // random angle -.3 -> +.3
    const float total_angle = ((float)(rand() & 1023) / (1023/RANDOM_ROTATION_ANGLE/2) - RANDOM_ROTATION_ANGLE) * TAU;
    const float total_steps = (1.0f / grow_speed) + (1.0f / grow_speed) + stable_time - 2.0f;
    info->alpha_angle_step = total_angle / total_steps;
    info->alpha_angle = RANDOM_ROTATION_ANGLE * 5 * TAU;
    
    // wait_then_make(true, info);

    bool first = true;
    for (float amp_mult = start_mult; amp_mult < 1; 
        amp_mult += grow_speed, info->alpha_angle += info->alpha_angle_step, cos += cosine_adder, rotate_cos += rotate_adder, ex += ex_step)
    {
        info->rotate_angle_start = info->alpha_angle;
        info->rotate_angle_step = info->alpha_angle_step / (float)LEN;

        const float new_amp = sine(((amp_mult + 1.5f) * 0.5f) * TAU) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        const float new_amp2 = sine(((amp_mult + 1.5f + grow_speed) * 0.5f) * TAU) * 0.5f;
        const float offset2 = (1.0f - new_amp2) * 2048;

        info->x_offset_start = offset * max_amp + offset_add;
        info->x_offset_step = (offset2 - offset) / (float)LEN;
        info->y_offset_start = info->x_offset_start;
        info->y_offset_step = info->x_offset_step;
        info->xamp_start = new_amp * max_amp;
        info->xamp_step = (new_amp2 - new_amp) / (float)LEN;
        info->yamp_start = info->xamp_start;
        info->yamp_step = info->xamp_step;

        info->rotate_angle_start2 = e_to_x(ex)/(1+e_to_x(ex)) * rotate2_mult;
        info->x2_offset_start = cosine(cos) * 0.5f * offset_mult;

        info->x2_offset_step = (cosine(cos + cosine_adder) * 0.5f * offset_mult - info->x2_offset_start) / (float)LEN;
        info->rotate_angle_step2 = (e_to_x(ex + ex_step)/(1+e_to_x(ex + ex_step)) * rotate2_mult - info->rotate_angle_start2) / (float)LEN;
        wait_then_make(first, info);
        first = false;
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
    info->orbit = false;
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



void start_flow(ChordInfo *const info)
{
    wait_then_make(true, info);

    const float start_grow_speed = 0.005f;
    const float end_shrink_speed = 0.005f;

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

void maintain_and_change(ChordInfo *info)
{
    switch (rand() & 0b11)
    {
    case 0:
        maintain_shape(rand()&511, info);
    case 1:
        maintain_shape(rand()&511, info);
        reborn(info);
        maintain_shape(rand()&511, info);
        break;
    case 2:
    case 3:
        maintain_shape(rand()&511, info);
        reborn(info);
        maintain_shape(rand()&511, info);
        reborn(info);
        maintain_shape(rand()&511, info);
        break;
    default:
        break;
    }
}

void transition(ChordInfo *info)
{
    static uint8_t previous_num = 255;
    const uint8_t num = random(12);

    // dont do the same transition twice, and ensure that the color gets changed atleast every other transition
    if (num == previous_num || (previous_num > 5 && num > 5)) 
    {
        transition(info);
        return;
    }

    previous_num = num;

    around_the_world(info);
    // shaker(info);
    cosine_twister(info);
    maintain_shape(200, info);
    return;


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
        reborn(info);
        reborn(info);
        break;
    case 7:
        shaker(info);
        break;
    case 8:
        big_o(info);
        break;
    case 9:
        remove_one(info);
    case 10:
        if (add_one(info))
            return;
    case 11:
        around_the_world(info);
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

    bubble: suddenly change one hz, and cause the shape to bounce and wobble like if a bubble bounced against something.

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
    
    multi-linear-reborn: change the hz's in a linear way instead of cosine. 
        multiple of them should be done at the same time for cool effect.
    multi-cosine-reborn: change multiple hz's simultaneously at different start and end and speeds.

    Think of some primitive shapes that can be used like triangles with rounded edges or something in place of the circles and ellipses That can be interchangeable 
*/
