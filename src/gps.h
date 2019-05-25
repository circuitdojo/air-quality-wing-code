#ifndef GPS_H
#define GPS_H

#include "stdint.h"

// Error codes
enum {
  GPS_SUCCESS,
  GPS_NULL_ERROR,
  GPS_NO_DAT_AVAIL
};

typedef struct {
  uint32_t lat;
  uint32_t lon;
} gps_data_t;

typedef void (*gps_cb)(gps_data_t * p_data);

typedef struct {
  uint8_t enable_pin;
  uint8_t fix_pin;
  gps_cb callback;
} gps_init_t;

class GPS {
  public:
    GPS(void);
    uint32_t init(gps_init_t *p_init);
    uint32_t enable();
    uint32_t get_fix();
    uint32_t disable();
  private:
    gps_init_t config;
};

#endif //GPS_H