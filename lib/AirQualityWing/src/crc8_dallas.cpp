/*
 * Project Particle Squared
 * Description: CRC8 Dallas Little Endian Function
 * Author: Jared Wolff
 * Date: 2/26/2019
 * License: GNU GPLv3
 * Attribution: Inspiration came from here: https://stackoverflow.com/questions/29214301/ios-how-to-calculate-crc-8-dallas-maxim-of-nsdata
 */

#include "crc8_dallas.h"

#define CRC8_DALLAS_INIT 0xff
#define CRC8_DALLAS_POLY 0x31

// Little Endian CRC8 Calculation For The SGP30
uint8_t crc8_dallas_little(uint8_t *data, uint16_t size)
{
    // Initializing (Typical start values 0x00 and 0xff)
    uint8_t crc = CRC8_DALLAS_INIT;

    while (size--)
    {
        // Increment to the next byte
        crc ^= *(data+size);

        // Iterate through all bits
        for (int i = 0; i < 8; i++)
        {
            // First bit always needs to be 1 for CRC checks
            // Note: left shift so we can retain the remainder
            // The operation "Carries over" into the next byte
            // by an XOR operation
            if (crc & 0x80) {
              crc = (crc << 1) ^ CRC8_DALLAS_POLY;
            } else {
              // Otherwise shift to the next position
              crc <<= 1;
            }
        }
    }
    return crc;
}