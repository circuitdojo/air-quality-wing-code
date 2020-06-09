#ifndef BOARD_H
#define BOARD_H

// Hardware version types
#define AQW_HW_VER1 1
#define AQW_HW_VER2 2
#define AQW_HW_VER3 3
#define AQW_HW_VER4 AQW_HW_VER3
#define AQW_HW_VER5 AQW_HW_VER4
#define AQW_HW_VER6 6

#define I2C_SDA_PIN D0
#define I2C_SCL_PIN D1

// IMPORTANT: you need to set the hardware version of your board here.
#define AQW_HW_VERSION AQW_HW_VER4

// If not defined, show an error.
#ifndef AQW_HW_VERSION
#error Please set AQW_HW_VERSION in board.h
#endif

#if AQW_HW_VERSION < AQW_HW_VER6
#define CCS811_WAKE_PIN D6
#define CCS811_INT_PIN D8
#endif

// Reset is moved to D4 for Rev3
#if AQW_HW_VERSION == AQW_HW_VER2 || AQW_HW_VERSION == AQW_HW_VER1
#define CCS811_RST_PIN D7
#elif AQW_HW_VERSION == AQW_HW_VER3
#define CCS811_RST_PIN D4
#elif AQW_HW_VERSION == AQW_HW_VER6
#else
#error AQW_HW_VERSION must be set!
#endif

#define HPMA1150_EN_PIN D5

// Address for CCS811, it is setable in hardware so should be defined here
#define CCS811_ADDRESS 0x5a

// Note: all other addresses are set in the device's .h file

// I2C Related constants
#define I2C_CLK_SPEED 100000

#endif //BOARD_H