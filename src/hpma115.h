/*
 * Project Particle Squared
 * Description: Particle powered PM2.5 and air quality sensor
 * Author: Jared Wolff
 * Date: 2/26/2019
 * License: GNU GPLv3
 */

#ifndef HPMA115_H
#define HPMA115_H

#include "application.h"
#include "serial_lock.h"

#define HPMA115_BAUD 9600

#define HPMA115_HEAD                  0x68
#define HPMA115_START_MEASUREMENT     0x01
#define HPMA115_STOP_MEASUREMENT      0x02
#define HPMA115_START_AUTO_SEND_CMD   0x40
#define HPMA115_STOP_AUTO_SEND_CMD    0x20

#define HPMA115_READING_CNT    3

// Error codes
enum {
  HPMA115_SUCCESS,
  HPMA115_NULL_ERROR,
  HPMA115_NO_DAT_AVAIL,
  HPMA115_SERIAL_BUSY
};

typedef enum {
  HPMA_READY,
  HPMA_DATA_AVAILABLE,
  HPMA_DATA_PROCESS,
  HPMA_DATA_READ,
  HPMA_DISABLED
} hpma115_state_t;

typedef struct {
  uint16_t pm25;
  uint16_t pm10;
} hpma115_data_t;

typedef void (*hpma115_cb)(hpma115_data_t * p_data);

typedef struct {
  hpma115_cb callback;
  uint8_t enable_pin;
  serial_lock_t * serial_lock;
} hpma115_init_t;

class HPMA115 {
  public:
    HPMA115(void);
    uint32_t setup(hpma115_init_t *p_init, serial_lock_t * serial_lock);
    uint32_t enable();
    uint32_t disable();
    bool is_enabled();
    void process();
    void int_handler(void);
  protected:
    hpma115_cb callback;
    hpma115_data_t data;
    bool    data_ready;
    volatile hpma115_state_t state;
    uint8_t enable_pin;
    uint8_t rx_count;
    char rx_buf[32];
    serial_lock_t * serial_lock;
};

#endif //HPMA115_H