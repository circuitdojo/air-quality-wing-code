#ifndef SI7021_H
#define SI7021_H

#include "application.h"

#define SI7021_ADDRESS            0x40

#define SI7021_HUMIDITY_HOLD_CMD  0xE5
#define SI7021_TEMP_HOLD_CMD      0xE3

// Error code
#define SI7021_SUCCESS            0
#define SI7021_COMMS_FAIL_ERROR   1

typedef struct {
  float temperature;
  float humidity;
} si7021_data_t;

class Si7021 {
  public:
    Si7021(void);
    uint32_t setup();
    uint32_t read(si7021_data_t * p_data);
};

#endif