#ifndef BOARD_H
#define BOARD_H

// Note: this is the board defnition
// which matches the Air Quality Wing for Particle Mesh
// You may need to modify depending on your needs.
// Have fun!

// Hardware version types
#define HW_VER1 1
#define HW_VER2 2
#define HW_VER3 3

// Default version to 2
#ifndef AQW_HW_VERSION
#define AQW_HW_VERSION HW_VER3
#endif

#define I2C_SDA_PIN D0
#define I2C_SCL_PIN D1

#define CCS811_WAKE_PIN D6
#define CCS811_INT_PIN D8

// Reset is moved to D4 for Rev3
#if AQW_HW_VERSION == HW_VER2 || AQW_HW_VERSION == HW_VER1
#define CCS811_RST_PIN D7
#elif AQW_HW_VERSION == HW_VER3
#define CCS811_RST_PIN D4
#else
#error AQW_HW_VERSION must be set!
#endif

#define HPMA1150_EN_PIN D5

#define PIR_INT_PIN A3
#define PIEZO_PIN A2

// Address for CCS811, it is setable in hardware so should be defined here
#define CCS811_ADDRESS 0x5a

// I2C Related constants
#define I2C_CLK_SPEED 100000

#endif //BOARD_H