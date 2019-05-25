#ifndef BOARD_H
#define BOARD_H

#define HAS_HPMA
// #define HAS_SGP30
// #define HAS_BME680
#define HAS_CCS811

#define I2C_SDA_PIN     D0
#define I2C_SCL_PIN     D1

#define CCS811_WAKE_PIN D6
#define CCS811_INT_PIN  D8
#define CCS811_RST_PIN  D4

#define HPMA1150_EN_PIN D5

#define PIR_INT_PIN     A3

#define GPS_EN_PIN      A5
#define GPS_FIX_PIN     A4

// Address for CCS811, it is setable in hardware so should be defined here
#define CCS811_ADDRESS  0x5a

// I2C Related constants
#define I2C_CLK_SPEED 100000


#endif //BOARD_H