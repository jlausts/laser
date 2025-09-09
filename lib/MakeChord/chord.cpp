#include "chord.h"

// 0.015
#define BASE_MIN 64
#define BASE_MAX (BASE_MIN * 2)
#define MAX_OUT_TUNE .1

#define SELECT_RANDOM_HZ \
for (uint8_t i = 0; i < info->x_count; ++i)\
    info->xhz[i] = info->other_hz[random(hz_count)] + (float)(rand() & 255) / ((255 / MAX_OUT_TUNE) + MAX_OUT_TUNE/2);\
for (uint8_t i = 0; i < info->y_count; ++i)\
    info->yhz[i] = info->other_hz[random(hz_count)] + (float)(rand() & 255) / ((255 / MAX_OUT_TUNE) + MAX_OUT_TUNE/2);

#define MULT_AMPLITUDES \
for (uint8_t i = 0; i < info->x_count; ++i){\
    info->xamp1[i] *= AMP_MULT; info->xhz[i] *= HZ_MULT;}\
for (uint8_t i = 0; i < info->y_count; ++i){\
    info->yamp1[i] *= AMP_MULT; info->yhz[i] *= HZ_MULT;}


void printChordInfo(const ChordInfo *c)
{
    Serial.println(F("=== ChordInfo ==="));

    Serial.print(F("x_count: ")); Serial.println(c->x_count);
    Serial.print(F("y_count: ")); Serial.println(c->y_count);
    Serial.print(F("r: "));       Serial.println(c->r);
    Serial.print(F("g: "));       Serial.println(c->g);
    Serial.print(F("b: "));       Serial.println(c->b);

    Serial.print(F("x_offset: ")); Serial.println(c->x_offset);
    Serial.print(F("y_offset: ")); Serial.println(c->y_offset);

    Serial.print(F("xhz:   "));
    for (int i = 0; i < 5; i++) {
        Serial.print(c->xhz[i], 3);
        if (i < 4) Serial.print(F(", "));
    }
    Serial.println();

    Serial.print(F("yhz:   "));
    for (int i = 0; i < 5; i++) {
        Serial.print(c->yhz[i], 3);
        if (i < 4) Serial.print(F(", "));
    }
    Serial.println();

    Serial.print(F("xamp:  "));
    for (int i = 0; i < 5; i++) {
        Serial.print(c->xamp1[i], 3);
        if (i < 4) Serial.print(F(", "));
    }
    Serial.println();

    Serial.print(F("yamp:  "));
    for (int i = 0; i < 5; i++) {
        Serial.print(c->yamp1[i], 3);
        if (i < 4) Serial.print(F(", "));
    }
    Serial.println();

    Serial.println(F("================="));
}


// 34.5us -> 85.6us
void make_chord(ChordInfo *info, const bool one_hz, const float base_hz, const uint8_t hz_using)
{    

    static constexpr float TAU = 6.28318530717958647f;
    static constexpr float HZ_MULT = TAU / 40000.0f;
    static constexpr float AMP_MULT = 4095.0 / 2.0;

    if (one_hz)
    {
        info->xhz[0] = info->other_hz[random(info->other_hz_count)] + (float)(rand() & 255) / ((255 / MAX_OUT_TUNE) + MAX_OUT_TUNE/2);
        info->yhz[0] = info->other_hz[random(info->other_hz_count)] + (float)(rand() & 255) / ((255 / MAX_OUT_TUNE) + MAX_OUT_TUNE/2);
        info->xhz[0] *= HZ_MULT;
        info->yhz[0] *= HZ_MULT;
        return;
    }

    // random hz between 64 and 128 to 1/32 precision
    const float hz = (base_hz == 0 ? (float)(((uint16_t)rand() & (2048 - 1)) | 1024) / 16.0f : base_hz);
    info->base_hz = hz;


    int hz_count = 0;
    for (float tmp_hz = hz; tmp_hz <= 1024; tmp_hz += hz, ++hz_count)
        info->other_hz[hz_count] = tmp_hz;
    
    

    info->other_hz_count = hz_count;
    switch (hz_using == 0 ? rand() & 0b1111 : hz_using)
    {

    case 0:
    case 1:
    case 2:
    case 3:
    {
        info->hz_using = 0;
        info->x_count = 2;
        info->y_count = 2;

        // between .25 -> .75
        info->xamp1[0] = (float)(rand() & 255) / 512 + 0.25f;
        info->xamp1[1] = 1.0f - info->xamp1[0];

        info->yamp1[0] = (float)(rand() & 255) / 512 + 0.25f;
        info->yamp1[1] = 1.0f - info->yamp1[0];

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }

    case 4:
    case 5:
    case 6:
    {
        info->hz_using = 1;
        info->x_count = 3;
        info->y_count = 2;

        // between .25 -> .4 
        info->xamp1[0] = (float)(rand() & 255) / 1700 + 0.25f;
        info->xamp1[1] = (float)(rand() & 255) / 1700 + 0.25f;
        info->xamp1[2] = 1 - (info->xamp1[0] + info->xamp1[1]);

        info->yamp1[0] = (float)(rand() & 255) / 512 + 0.25f;
        info->yamp1[1] = 1.0f - info->yamp1[0];

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }

    case 7:
    case 8:
    case 9:
    {
        info->hz_using = 2;
        info->x_count = 2;
        info->y_count = 3;

        // between .25 -> .4 
        info->yamp1[0] = (float)(rand() & 255) / 1700 + 0.25f;
        info->yamp1[1] = (float)(rand() & 255) / 1700 + 0.25f;
        info->yamp1[2] = 1.0f - (info->yamp1[0] + info->yamp1[1]);

        info->xamp1[0] = (float)(rand() & 255) / 512 + 0.25f;
        info->xamp1[1] = 1.0f - info->xamp1[0];

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }

    case 10:
    case 11:
    {
        info->hz_using = 3;
        info->x_count = 3;
        info->y_count = 3;

        info->xamp1[0] = (float)(rand() & 255) / 1700 + 0.25f;
        info->xamp1[1] = (float)(rand() & 255) / 1700 + 0.25f;
        info->xamp1[2] = 1 - (info->xamp1[0] + info->xamp1[1]);

        // between .25 -> .4 
        info->yamp1[0] = (float)(rand() & 255) / 1700 + 0.25f;
        info->yamp1[1] = (float)(rand() & 255) / 1700 + 0.25f;
        info->yamp1[2] = 1 - (info->yamp1[0] + info->yamp1[1]);

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }

    case 12:
    {
        info->hz_using = 4;
        info->x_count = 4;
        info->y_count = 3;

        info->xamp1[0] = (float)(rand() & 255) / 2550 + 0.2f;
        info->xamp1[1] = (float)(rand() & 255) / 2550 + 0.2f;
        info->xamp1[2] = (float)(rand() & 255) / 2550 + 0.2f;
        info->xamp1[3] = 1 - (info->xamp1[0] + info->xamp1[1] + info->xamp1[2]);

        // between .25 -> .4 
        info->yamp1[0] = (float)(rand() & 255) / 1700 + 0.25f;
        info->yamp1[1] = (float)(rand() & 255) / 1700 + 0.25f;
        info->yamp1[2] = 1 - (info->yamp1[0] + info->yamp1[1]);

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }

    case 13:
    {
        info->hz_using = 5;
        info->x_count = 3;
        info->y_count = 4;

        info->xamp1[0] = (float)(rand() & 255) / 1700 + 0.25f;
        info->xamp1[1] = (float)(rand() & 255) / 1700 + 0.25f;
        info->xamp1[2] = 1 - (info->xamp1[0] + info->xamp1[1]);

        // between .2 -> .3 
        info->yamp1[0] = (float)(rand() & 255) / 2550 + 0.2f;
        info->yamp1[1] = (float)(rand() & 255) / 2550 + 0.2f;
        info->yamp1[2] = (float)(rand() & 255) / 2550 + 0.2f;
        info->yamp1[3] = 1 - (info->yamp1[0] + info->yamp1[1] + info->yamp1[2]);

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }

    case 14:
    {
        info->hz_using = 6;
        info->x_count = 4;
        info->y_count = 4;

        info->xamp1[0] = (float)(rand() & 255) / 2550 + 0.2f;
        info->xamp1[1] = (float)(rand() & 255) / 2550 + 0.2f;
        info->xamp1[2] = (float)(rand() & 255) / 2550 + 0.2f;
        info->xamp1[3] = 1 - (info->xamp1[0] + info->xamp1[1] + info->xamp1[2]);

        // between .2 -> .3 
        info->yamp1[0] = (float)(rand() & 255) / 2550 + 0.2f;
        info->yamp1[1] = (float)(rand() & 255) / 2550 + 0.2f;
        info->yamp1[2] = (float)(rand() & 255) / 2550 + 0.2f;
        info->yamp1[3] = 1 - (info->yamp1[0] + info->yamp1[1] + info->yamp1[2]);

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }

    case 15:
    {
        info->hz_using = 7;
        info->x_count = 5;
        info->y_count = 5;
        //np.array([255, 0]) / 4080 + .2 - .03125

        info->xamp1[0] = (float)(rand() & 255) / 4080 + 0.16875f;
        info->xamp1[1] = (float)(rand() & 255) / 4080 + 0.16875f;
        info->xamp1[2] = (float)(rand() & 255) / 4080 + 0.16875f;
        info->xamp1[3] = (float)(rand() & 255) / 4080 + 0.16875f;
        info->xamp1[4] = 1 - (info->xamp1[0] + info->xamp1[1] + info->xamp1[2] + info->xamp1[3]);

        // between .2 -> .3 
        info->yamp1[0] = (float)(rand() & 255) / 4080 + 0.16875f;
        info->yamp1[1] = (float)(rand() & 255) / 4080 + 0.16875f;
        info->yamp1[2] = (float)(rand() & 255) / 4080 + 0.16875f;
        info->yamp1[3] = (float)(rand() & 255) / 4080 + 0.16875f;
        info->yamp1[4] = 1 - (info->yamp1[0] + info->yamp1[1] + info->yamp1[2] + info->yamp1[3]);

        SELECT_RANDOM_HZ;
        MULT_AMPLITUDES;
        break;
    }
    
    default:
        break;
    }

}

uint8_t hz_count_to_num_hz(const int x_count, const int y_count)
{
    switch (x_count | (y_count << 5))
    {
    case 2 | (2 << 5):
        return 3;
        break;

    case 3 | (2 << 5):
        return 6;
        break;

    case 2 | (3 << 5):
        return 9;
        break;

    case 3 | (3 << 5):
        return 11;
        break;

    case 4 | (3 << 5):
        return 12;
        break;

    case 3 | (4 << 5):
        return 13;
        break;

    case 4 | (4 << 5):
        return 14;
        break;

    case 5 | (5 << 5):
        return 15;
        break;
    
    default:
        return 2;
        break;
    }

    return 2;
}