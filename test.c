#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

typedef struct {
    uint8_t r, g, b;
    uint16_t laser_x, laser_y, audio_l, audio_r;
    uint64_t t;
} Data;

void pack(const Data *const data_array, uint8_t *const arr)
{
    for (int i = 0, j = 0; j < 256; ++i, j += 16)
    {
        uint8_t *const packed = &arr[j];
        const Data *const data = &data_array[i];

        // rgb: pack to 11 bits (5+6+7 bits, rounded)
        packed[0]  =  (data->r >> 3) & 0b00011111;       // r 
        packed[0] |= ((data->g >> 3) & 0b00000111) << 5; // g 

        packed[1]  = ((data->g >> 6) & 0b00000011);      // g 
        packed[1] |= ((data->b >> 1) & 0b01111100);      // b 

        // laser_x (12 bits)
        packed[2] =   data->laser_x;
        packed[3] = ((data->laser_x >> 8) & 0x0F);

        // laser_y (12 bits)
        packed[3] |= (data->laser_y << 4) & 0xF0;
        packed[4]  =  data->laser_y >> 4;

        // audio_l (12 bits)
        packed[5] =   data->audio_l;
        packed[6] = ((data->audio_l >> 8) & 0x0F);

        // audio_r (12 bits)
        packed[6] |= (data->audio_r << 4) & 0xF0;
        packed[7]  =  data->audio_r >> 4;

        // timestamp: 64-bit little endian
        packed[8]  = (uint8_t) data->t;
        packed[9]  = (uint8_t)(data->t >> 8);
        packed[10] = (uint8_t)(data->t >> 16);
        packed[11] = (uint8_t)(data->t >> 24);
        packed[12] = (uint8_t)(data->t >> 32);
        packed[13] = (uint8_t)(data->t >> 40);
        packed[14] = (uint8_t)(data->t >> 48);
        packed[15] = (uint8_t)(data->t >> 56);

    }
}

HANDLE setup_serial()
{
    HANDLE hSerial = CreateFileA("\\\\.\\COM3", GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (hSerial == INVALID_HANDLE_VALUE) 
    {
        fprintf(stderr, "Error opening COM3\n");
        exit(0);
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    GetCommState(hSerial, &dcbSerialParams);
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity   = NOPARITY;
    SetCommState(hSerial, &dcbSerialParams);

    return hSerial;
}

void square(HANDLE hSerial)
{
    static Data data_array[256] = {0};
    static uint8_t packed[256] = {0};
    DWORD bytesWritten;

    for (int i = 0, j = 0; i < 16; ++i, j += 16)
    {
        pack(&data_array[j], packed);
        WriteFile(hSerial, packed, 256, &bytesWritten, NULL);
    }

}

int main() 
{
    HANDLE hSerial = setup_serial();
    square(hSerial);
    CloseHandle(hSerial);
    puts("DONE");
    return 0;
}
