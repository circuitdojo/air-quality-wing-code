#ifndef GPS_H
#define GPS_H

#include "stdint.h"
#include "application.h"
#include "serial_lock.h"
#include <Adafruit_GPS.h>

// Error codes
enum {
  GPS_SUCCESS,
  GPS_NULL_ERROR,
  GPS_NO_DAT_AVAIL,
  GPS_SERIAL_BUSY
};

typedef struct {
  uint8_t hour;          ///< GMT hours
  uint8_t minute;        ///< GMT minutes
  uint8_t seconds;       ///< GMT seconds
  uint16_t milliseconds; ///< GMT milliseconds
  uint8_t year;          ///< GMT year
  uint8_t month;         ///< GMT month
  uint8_t day;           ///< GMT day
} gps_time_t;

typedef struct {
  int32_t lon;
  char    lon_c;
  int32_t lat;
  char    lat_c;
  gps_time_t time;
} gps_data_t;

typedef void (*gps_cb)(gps_data_t * p_data);

typedef struct {
  uint8_t enable_pin;
  uint8_t fix_pin;
  gps_cb callback;
} gps_init_t;

typedef enum {
  GPS_ENABLED,
  GPS_DISABLED
} gps_state_t;

class GPS {
  public:
    GPS(void);
    uint32_t init(gps_init_t *p_init, serial_lock_t * serial_lock);
    uint32_t enable();
    uint32_t get_fix();
    bool is_enabled();
    uint32_t disable();
    uint32_t process();
  private:
    gps_init_t config;
    gps_state_t state;
    gps_data_t last_position;
    serial_lock_t * serial_lock;
    uint32_t timer;
};

#endif //GPS_H