#include "chord.h"

// 0.015
#define BASE_MIN 64
#define BASE_MAX (BASE_MIN * 2)
#define MAX_OUT_TUNE .1

#define SELECT_RANDOM_HZ \
for (uint8_t i = 0; i < *x_count; ++i)\
    xhz[i] = hzs[random(hz_count)] + (float)(rand() & 255) / ((255 / MAX_OUT_TUNE) + MAX_OUT_TUNE/2);\
for (uint8_t i = 0; i < *y_count; ++i)\
    yhz[i] = hzs[random(hz_count)] + (float)(rand() & 255) / ((255 / MAX_OUT_TUNE) + MAX_OUT_TUNE/2);

#define MULT_AMPLITUDES \
for (uint8_t i = 0; i < *x_count; ++i){\
    xamp[i] *= AMP_MULT; xhz[i] *= HZ_MULT;}\
for (uint8_t i = 0; i < *y_count; ++i){\
    yamp[i] *= AMP_MULT; yhz[i] *= HZ_MULT;}


// 34.5us -> 85.6us
void make_chord(float *const xhz, float *const yhz, 
    float *const xamp, float *const yamp,
    uint8_t *const x_count, uint8_t *const y_count)
{    
    // random hz between 64 and 128 to 1/32 precision
    const float hz = (float)(((uint16_t)rand() & (2048 - 1)) | 1024) / 16.0f;

    static constexpr float TAU = 6.28318530717958647f;
    static constexpr float HZ_MULT = TAU / 40000.0f;
    static constexpr float AMP_MULT = 4095.0 / 2.0;

    int hz_count = 0;
    static float hzs[16];
    for (float tmp_hz = hz; tmp_hz <= 1024; tmp_hz += hz, ++hz_count)
        hzs[hz_count] = tmp_hz;

    switch (rand() & 0b1111)
    {

    case 0:
    case 1:
    case 2:
    case 3:
    {
        *x_count = 2;
        *y_count = 2;

        // between .25 -> .75
        xamp[0] = (float)(rand() & 255) / 512 + 0.25f;
        xamp[1] = 1.0f - xamp[0];

        yamp[0] = (float)(rand() & 255) / 512 + 0.25f;
        yamp[1] = 1.0f - yamp[0];

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }

    case 4:
    case 5:
    case 6:
    {
        *x_count = 3;
        *y_count = 2;

        // between .25 -> .4 
        xamp[0] = (float)(rand() & 255) / 1700 + 0.25f;
        xamp[1] = (float)(rand() & 255) / 1700 + 0.25f;
        xamp[2] = 1 - (xamp[0] + xamp[1]);

        yamp[0] = (float)(rand() & 255) / 512 + 0.25f;
        yamp[1] = 1.0f - yamp[0];

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }

    case 7:
    case 8:
    case 9:
    {
        *x_count = 2;
        *y_count = 3;

        // between .25 -> .4 
        yamp[0] = (float)(rand() & 255) / 1700 + 0.25f;
        yamp[1] = (float)(rand() & 255) / 1700 + 0.25f;
        yamp[2] = 1.0f - (yamp[0] + yamp[1]);

        xamp[0] = (float)(rand() & 255) / 512 + 0.25f;
        xamp[1] = 1.0f - xamp[0];

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }

    case 10:
    case 11:
    {
        *x_count = 3;
        *y_count = 3;

        xamp[0] = (float)(rand() & 255) / 1700 + 0.25f;
        xamp[1] = (float)(rand() & 255) / 1700 + 0.25f;
        xamp[2] = 1 - (xamp[0] + xamp[1]);

        // between .25 -> .4 
        yamp[0] = (float)(rand() & 255) / 1700 + 0.25f;
        yamp[1] = (float)(rand() & 255) / 1700 + 0.25f;
        yamp[2] = 1 - (yamp[0] + yamp[1]);

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }

    case 12:
    {
        *x_count = 4;
        *y_count = 3;

        xamp[0] = (float)(rand() & 255) / 2550 + 0.2f;
        xamp[1] = (float)(rand() & 255) / 2550 + 0.2f;
        xamp[2] = (float)(rand() & 255) / 2550 + 0.2f;
        xamp[3] = 1 - (xamp[0] + xamp[1] + xamp[2]);

        // between .25 -> .4 
        yamp[0] = (float)(rand() & 255) / 1700 + 0.25f;
        yamp[1] = (float)(rand() & 255) / 1700 + 0.25f;
        yamp[2] = 1 - (yamp[0] + yamp[1]);

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }

    case 13:
    {
        *x_count = 3;
        *y_count = 4;

        xamp[0] = (float)(rand() & 255) / 1700 + 0.25f;
        xamp[1] = (float)(rand() & 255) / 1700 + 0.25f;
        xamp[2] = 1 - (xamp[0] + xamp[1]);

        // between .2 -> .3 
        yamp[0] = (float)(rand() & 255) / 2550 + 0.2f;
        yamp[1] = (float)(rand() & 255) / 2550 + 0.2f;
        yamp[2] = (float)(rand() & 255) / 2550 + 0.2f;
        yamp[3] = 1 - (yamp[0] + yamp[1] + yamp[2]);

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }

    case 14:
    {
        *x_count = 4;
        *y_count = 4;

        xamp[0] = (float)(rand() & 255) / 2550 + 0.2f;
        xamp[1] = (float)(rand() & 255) / 2550 + 0.2f;
        xamp[2] = (float)(rand() & 255) / 2550 + 0.2f;
        xamp[3] = 1 - (xamp[0] + xamp[1] + xamp[2]);

        // between .2 -> .3 
        yamp[0] = (float)(rand() & 255) / 2550 + 0.2f;
        yamp[1] = (float)(rand() & 255) / 2550 + 0.2f;
        yamp[2] = (float)(rand() & 255) / 2550 + 0.2f;
        yamp[3] = 1 - (yamp[0] + yamp[1] + yamp[2]);

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }

    case 15:
    {
        *x_count = 5;
        *y_count = 5;
        //np.array([255, 0]) / 4080 + .2 - .03125

        xamp[0] = (float)(rand() & 255) / 4080 + 0.16875f;
        xamp[1] = (float)(rand() & 255) / 4080 + 0.16875f;
        xamp[2] = (float)(rand() & 255) / 4080 + 0.16875f;
        xamp[3] = (float)(rand() & 255) / 4080 + 0.16875f;
        xamp[4] = 1 - (xamp[0] + xamp[1] + xamp[2] + xamp[3]);

        // between .2 -> .3 
        yamp[0] = (float)(rand() & 255) / 4080 + 0.16875f;
        yamp[1] = (float)(rand() & 255) / 4080 + 0.16875f;
        yamp[2] = (float)(rand() & 255) / 4080 + 0.16875f;
        yamp[3] = (float)(rand() & 255) / 4080 + 0.16875f;
        yamp[4] = 1 - (yamp[0] + yamp[1] + yamp[2] + yamp[3]);

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }
    
    default:
        break;
    }

}