#ifndef BOARD_H
#define BOARD_H

// Hardware version types
#define HW_VER1 1
#define HW_VER2 2
#define HW_VER3 3
#define HW_VER4 HW_VER3
#define HW_VER5 5

// Default version to 5
#ifndef HW_VERSION
#define HW_VERSION HW_VER5
#endif

#define I2C_SDA_PIN     D0
#define I2C_SCL_PIN     D1

#if HW_VERSION < HW_VER5
#define CCS811_WAKE_PIN D6
#define CCS811_INT_PIN  D8
#endif

// Reset is moved to D4 for Rev3
#if HW_VERSION == HW_VER2 || HW_VERSION == HW_VER1
#define CCS811_RST_PIN  D7
#elif HW_VERSION == HW_VER3
#define CCS811_RST_PIN  D4
#elif HW_VERSION == HW_VER5
#else
#error HW_VERSION must be set!
#endif

#define HPMA1150_EN_PIN D5

// Address for CCS811, it is setable in hardware so should be defined here
#define CCS811_ADDRESS  0x5a

// Note: all other addresses are set in the device's .h file

// I2C Related constants
#define I2C_CLK_SPEED 100000


#endif //BOARD_H