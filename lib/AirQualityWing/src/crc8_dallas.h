/*
 * Project Particle Squared
 * Description: CRC8 Dallas Little Endian Function
 * Author: Jared Wolff
 * Date: 2/26/2019
 * License: GNU GPLv3
 * Attribution: Inspiration came from here: https://stackoverflow.com/questions/29214301/ios-how-to-calculate-crc-8-dallas-maxim-of-nsdata
 */

#ifndef CRC8_DALLAS_H
#define CRC8_DALLAS_H

#include <stdint.h>
#include "application.h"

uint8_t crc8_dallas_little(uint8_t *data, uint16_t len);

#endif //CRC8_DALLAS_H