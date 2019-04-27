#ifndef BOARD_H
#define BOARD_H

#define HAS_HPMA
#define HAS_SPG30

#define I2C_SDA_PIN     D0
#define I2C_SCL_PIN     D1

#define CCS811_WAKE_PIN D6
#define CCS811_INT_PIN  D8
#define CCS811_RST_PIN  D7

#define HPMA1150_EN_PIN D5

// Address for CCS811, it is setable in hardware so should be defined here
#define CCS811_ADDRESS  0x5a

// I2C Related constants
#define I2C_CLK_SPEED 100000


#endif //BOARD_H