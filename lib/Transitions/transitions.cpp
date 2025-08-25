#include "transitions.h"
#include "transition_variables.h"

#define TRANSITION_FOR_LOOP for (int j = 0, k = info->t; j < LEN; ++j, ++k)



void rotate_point_and_clamp(volatile Data *const data_array, const float angle, const float cx, const float cy)
{
    const float dx = (float)(data_array->laser_x) - cx;
    const float dy = (float)(data_array->laser_y) - cy;
    const float sin_ = sine(angle) - 1.0f;
    const float cos_ = cosine(angle) - 1.0f;
    // Serial.println(String((uint16_t)((cx + dx * cos_ - dy * sin_) + 0.5f)) + " " + String(data_array->laser_y));
    data_array->laser_x = (uint16_t)((cx + dx * cos_ - dy * sin_) + 0.5f);
    data_array->laser_y = (uint16_t)((cy + dx * sin_ + dy * cos_) + 0.5f);

    if (data_array->laser_x > MAX_POSITION) data_array->laser_x = MAX_POSITION;
    if (data_array->laser_y > MAX_POSITION) data_array->laser_y = MAX_POSITION;
}


void transitioner(volatile Data *const data_array, const ChordInfo *const info)
{
    if (info->rotate_angle > 0.000001f)
    {
        // no offset
        if (info->x_offset == 0 && info->y_offset == 0)
        {
            switch (info->x_count << 4 | info->y_count)
            {
            case 1 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y1; ROTATE_CLAMP; } break;
            case 1 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y2; ROTATE_CLAMP; } break;
            case 1 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y3; ROTATE_CLAMP; } break;
            case 1 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y4; ROTATE_CLAMP; } break;
            case 1 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y5; ROTATE_CLAMP; } break;

            case 2 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y1; ROTATE_CLAMP; } break;
            case 2 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y2; ROTATE_CLAMP; } break;
            case 2 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y3; ROTATE_CLAMP; } break;
            case 2 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y4; ROTATE_CLAMP; } break;
            case 2 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y5; ROTATE_CLAMP; } break;

            case 3 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y1; ROTATE_CLAMP; } break;
            case 3 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y2; ROTATE_CLAMP; } break;
            case 3 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y3; ROTATE_CLAMP; } break;
            case 3 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y4; ROTATE_CLAMP; } break;
            case 3 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y5; ROTATE_CLAMP; } break;

            case 4 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y1; ROTATE_CLAMP; } break;
            case 4 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y2; ROTATE_CLAMP; } break;
            case 4 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y3; ROTATE_CLAMP; } break;
            case 4 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y4; ROTATE_CLAMP; } break;
            case 4 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y5; ROTATE_CLAMP; } break;

            case 5 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y1; ROTATE_CLAMP; } break;
            case 5 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y2; ROTATE_CLAMP; } break;
            case 5 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y3; ROTATE_CLAMP; } break;
            case 5 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y4; ROTATE_CLAMP; } break;
            case 5 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y5; ROTATE_CLAMP; } break;

            default: break;
            }
        }
        
        // contains offset. (stored in laser_x&y)
        else
        {
            switch (info->x_count << 4 | info->y_count)
            {
            case 1 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 1 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 1 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 1 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 1 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            case 2 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 2 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 2 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 2 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 2 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            case 3 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 3 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 3 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 3 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 3 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            case 4 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 4 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 4 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 4 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 4 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            case 5 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y1; OFFSET; ROTATE_CLAMP; } break;
            case 5 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y2; OFFSET; ROTATE_CLAMP; } break;
            case 5 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y3; OFFSET; ROTATE_CLAMP; } break;
            case 5 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y4; OFFSET; ROTATE_CLAMP; } break;
            case 5 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y5; OFFSET; ROTATE_CLAMP; } break;

            default: break;
            }
        }
    }
    else
    {
        // no offset.
        if (info->x_offset == 0 && info->y_offset == 0)
        {
            switch (info->x_count << 4 | info->y_count)
            {
            case 1 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y1; CLAMP; } break;
            case 1 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y2; CLAMP; } break;
            case 1 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y3; CLAMP; } break;
            case 1 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y4; CLAMP; } break;
            case 1 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y5; CLAMP; } break;

            case 2 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y1; CLAMP; } break;
            case 2 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y2; CLAMP; } break;
            case 2 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y3; CLAMP; } break;
            case 2 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y4; CLAMP; } break;
            case 2 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y5; CLAMP; } break;

            case 3 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y1; CLAMP; } break;
            case 3 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y2; CLAMP; } break;
            case 3 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y3; CLAMP; } break;
            case 3 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y4; CLAMP; } break;
            case 3 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y5; CLAMP; } break;

            case 4 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y1; CLAMP; } break;
            case 4 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y2; CLAMP; } break;
            case 4 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y3; CLAMP; } break;
            case 4 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y4; CLAMP; } break;
            case 4 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y5; CLAMP; } break;

            case 5 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y1; CLAMP; } break;
            case 5 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y2; CLAMP; } break;
            case 5 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y3; CLAMP; } break;
            case 5 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y4; CLAMP; } break;
            case 5 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y5; CLAMP; } break;

            default: break;
            }
        }
        
        // contains offset. (stored in laser_x&y)
        else
        {
            switch (info->x_count << 4 | info->y_count)
            {
            case 1 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y1; OFFSET; CLAMP; } break;
            case 1 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y2; OFFSET; CLAMP; } break;
            case 1 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y3; OFFSET; CLAMP; } break;
            case 1 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y4; OFFSET; CLAMP; } break;
            case 1 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X1; SET_Y5; OFFSET; CLAMP; } break;

            case 2 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y1; OFFSET; CLAMP; } break;
            case 2 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y2; OFFSET; CLAMP; } break;
            case 2 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y3; OFFSET; CLAMP; } break;
            case 2 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y4; OFFSET; CLAMP; } break;
            case 2 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X2; SET_Y5; OFFSET; CLAMP; } break;

            case 3 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y1; OFFSET; CLAMP; } break;
            case 3 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y2; OFFSET; CLAMP; } break;
            case 3 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y3; OFFSET; CLAMP; } break;
            case 3 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y4; OFFSET; CLAMP; } break;
            case 3 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X3; SET_Y5; OFFSET; CLAMP; } break;

            case 4 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y1; OFFSET; CLAMP; } break;
            case 4 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y2; OFFSET; CLAMP; } break;
            case 4 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y3; OFFSET; CLAMP; } break;
            case 4 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y4; OFFSET; CLAMP; } break;
            case 4 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X4; SET_Y5; OFFSET; CLAMP; } break;

            case 5 << 4 | 1:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y1; OFFSET; CLAMP; } break;
            case 5 << 4 | 2:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y2; OFFSET; CLAMP; } break;
            case 5 << 4 | 3:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y3; OFFSET; CLAMP; } break;
            case 5 << 4 | 4:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y4; OFFSET; CLAMP; } break;
            case 5 << 4 | 5:
                TRANSITION_FOR_LOOP { SET_COLOR; SET_X5; SET_Y5; OFFSET; CLAMP; } break;

            default: break;
            }
        }
    }
}

void make_shape(const bool new_shape, const float amp_mult, 
    const uint16_t x_offset, const uint16_t y_offset, const float rotate_angle)
{
    static ChordInfo info = {.r = 152, .g = 152, .b = 152, .center_x = 2048, .center_y = 2048, .rotate_angle = 0};
    info.x_offset = x_offset;
    info.y_offset = y_offset;
    info.rotate_angle = rotate_angle;

    if (new_shape)
    {    
        info.t = 0;
        make_chord(&info);
        random_color(&info);
    }

    for (uint8_t i = 0; i < info.x_count; ++i)
        info.xamp[i] = info.xamp1[i] * amp_mult;
    for (uint8_t i = 0; i < info.y_count; ++i)
        info.yamp[i] = info.yamp1[i] * amp_mult;

    transitioner(data[!array_reading], &info);
    info.t += 255;
}

void wait_then_make(const bool new_shape, const float amp_mult, const uint16_t x_offset, const uint16_t y_offset, const float rotate_angle)
{
    wait_for_empty_array();
    make_shape(new_shape, amp_mult, x_offset, y_offset, rotate_angle);
}


void twister(ChordInfo *const info)
{
    
}

void spiral(ChordInfo *const info)
{
    
}

void dissolver(ChordInfo *const info)
{
    
}

void cosine_transistion(ChordInfo *const info)
{
    wait_then_make(true, 0, 2048, 2048, 0);

    const float max_amp = .8;
    const float start_grow_speed = 0.005f;
    const float end_shrink_speed = 0.005f;
    const int stable_time = 512;

    // random angle -.25 -> +.25
    const float total_angle = ((float)(rand() & 1023) / 2048.0f - 0.25f) * TAU;
    const float total_steps = (1.0f / start_grow_speed) + (1.0f / end_shrink_speed) + stable_time;
    const float angle_step = total_angle / total_steps;
    float angle = 3 * TAU;

    float offset_add = (1 - max_amp) * 0.5f * 4096;
    for (float amp_mult = 0; amp_mult < 1; amp_mult += start_grow_speed, angle += angle_step)
    {
        const float new_amp = sine(((amp_mult + 1.5f) * 0.5f) * TAU) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        wait_then_make(false, new_amp * max_amp, offset * max_amp + offset_add, offset * max_amp + offset_add, angle);
    }
    
    for (int i = 0; i < stable_time; ++i, angle += angle_step)
        wait_then_make(false, max_amp, offset_add, offset_add, angle);
    
    for (float amp_mult = 0; amp_mult < 1; amp_mult += end_shrink_speed, angle += angle_step)
    {
        const float new_amp = sine(((amp_mult + 0.5f) * 0.5f) * TAU) * 0.5f;
        const float offset = (1.0f - new_amp) * 2048;
        wait_then_make(false, new_amp * max_amp, offset * max_amp + offset_add, offset * max_amp + offset_add, angle);
    }

    wait_then_make(false, 0, 2048, 2048, angle);
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


/*
    the twister: rotate a bunch of times and shrink.
    the super-twister: accellerate the rotational speed until suddenly changing the shape, then slow rotation until still.
    the spiral: spiral a but off center, then spiral inward while shrinking and rotating
    the dissolver: try changing the HZ's all at the same time to their new HZ.
    the cosine: cosine interpolate shrink and grow to the new shape.
    the shaker: shake either side to side or up and down while shrinking.
    off center twist: move a but to one side, rotate on the center axis and shrink and move back into the center and shrink.
    the fold: shrink only one axis and when the image is a line, then change the HZ's and unfold into the new shape.
    big "o": remove one HZ at a time, and arrive at a perfect circle by adding a HZ identical to the other one, 
        then add the new HZ's back onto it. remove the seconds HZ require to make the circle at the end.
        other base-shapes could be used, like a "figure 8"
    re-born: change one HZ at a time that still works with the base HZ, until all of them are different.
*/
