#include "transitions.h"
#include "transition_variables.h"
#include <math.h>

#define TRANSITION_FOR_LOOP for (int j = 0, k = info->t; j < LEN; \
    ++j, ++k, angle+=info->rotate_angle_step, \
    xamp+=info->xamp_step, yamp+=info->yamp_step, \
    x_offset+=info->x_offset_step, y_offset+=info->y_offset_step)

extern volatile Data data[2][256];
static const double TAU = 6.283185307179586;

#define ROTATE_CLAMP rotate_point_and_clamp2(&data_array[j], angle, info->center_x, info->center_y);

#define OFFSET data_array[j].laser_x += x_offset;\
               data_array[j].laser_y += y_offset;

#define RANDOM_ROTATION_ANGLE 0.5f 

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
    println(angle);
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

void make_shape(const bool new_shape, ChordInfo *info)
{
    if (new_shape)
    {    
        info->t = 0;
        make_chord(info);
        random_color(info);
    }

    for (uint8_t i = 0; i < info->x_count; ++i)
        info->xamp[i] = info->xamp1[i];// * amp_mult;
    for (uint8_t i = 0; i < info->y_count; ++i)
        info->yamp[i] = info->yamp1[i];// * amp_mult;

    transitioner(data[!array_reading], info);
    info->t += 255;
}

void wait_then_make(const bool new_shape, ChordInfo *info)
{
    wait_for_empty_array();
    make_shape(new_shape, info);
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

void twister(ChordInfo *const info)
{
    
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


void tornado_twist(ChordInfo *const info)
{
    const float start_grow_speed = 0.005f;
    const float end_shrink_speed = 0.005f;
    const float angle_step = 0.01f;
    const int stable_time = 512;
    const float twist_count = 4.5;
    const float angle_mult = 2.5f;

    float solved_x = info->alpha_angle_step * 100 / 2.0f / angle_mult;
    float y_adder = solved_x * solved_x;

    for (float deg = 0, deg2 = 0, i = 0, j = angle_step; 
        i < twist_count + y_adder * angle_mult * 2; 
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
    info->alpha_angle = RANDOM_ROTATION_ANGLE * 10 * TAU;
    float end_x = twist_count - info->alpha_angle_step * 100 / 2.0f / angle_mult;

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

void tornado_twist2(ChordInfo *const info)
{
    const float start_grow_speed = 0.005f;
    const float end_shrink_speed = 0.005f;
    const float angle_step = 0.01f;
    const int stable_time = 512;
    const float twist_count = 2.5;
    const float angle_mult = 1.0f;

    float solved_x = cbrtf(info->alpha_angle_step * (float)LEN / 4.0f);
    float y_adder = solved_x * solved_x * solved_x * solved_x;

    for (float deg = info->alpha_angle, deg2 = info->alpha_angle, i = 0, j = angle_step; 
        i < twist_count; 
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

    // random angle -.3 -> +.3
    const float total_angle = ((float)(rand() & 1023) / (1023/RANDOM_ROTATION_ANGLE)) * TAU;
    const float total_steps = (1.0f / start_grow_speed) + (1.0f / end_shrink_speed) + stable_time - 2.0f;
    info->alpha_angle_step = total_angle / total_steps;
    info->alpha_angle = RANDOM_ROTATION_ANGLE * 5 * TAU;
    float end_x = twist_count - cbrtf(info->alpha_angle_step * (float)LEN / 4.0f);
    wait_then_make(true, info);
    for (float deg = info->alpha_angle, deg2 = info->alpha_angle, i = 0, j = angle_step; 
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

void spiral(ChordInfo *const info)
{
    
}

void dissolver(ChordInfo *const info)
{
    
}

void shaker(ChordInfo *const info)
{
    
}

void off_center_twist(ChordInfo *const info)
{
    
}

void fold(ChordInfo *const info)
{
    
}

void big_o(ChordInfo *const info)
{
    
}

void reborn(ChordInfo *const info)
{
    
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

void flow()
{
    static ChordInfo info = {.r = 152, .g = 152, .b = 152, .center_x = 2048, .center_y = 2048, .rotate_angle = 0};
    start_flow(&info);
    while(1)
    {
        tornado_twist(&info);
        maintain_shape(512, &info);
        cosine_transistion(&info);
        maintain_shape(512, &info);
    }
}

/*
    the twister: rotate a bunch of times and shrink.
    the tornado-twist: accellerate the rotational speed until suddenly changing the shape, then slow rotation until still.
    the spiral: spiral a but off center, then spiral inward while shrinking and rotating
    the planet_vanish: go off center, rotate and orbit the center while shrinking to a dot, then while orbiting the center turn back into a new shape.
    the dissolver: try changing the HZ's all at the same time to their new HZ.
    //the cosine: cosine interpolate shrink and grow to the new shape.
    the shaker: shake either side to side or up and down while shrinking.
    off center twist: move a but to one side, rotate on the center axis and shrink and move back into the center and shrink.
    the fold: shrink only one axis and when the image is a line, then change the HZ's and unfold into the new shape.
    big "o": remove one HZ at a time, and arrive at a perfect circle by adding a HZ identical to the other one, 
        then add the new HZ's back onto it. remove the seconds HZ require to make the circle at the end.
        other base-shapes could be used, like a "figure 8"
    re-born: change one HZ at a time that still works with the base HZ, until all of them are different.
*/
