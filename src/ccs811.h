/*
 * Project Particle Squared
 * Description: Particle powered PM2.5 and air quality sensor
 * Author: Jared Wolff
 * Date: 2/26/2019
 * License: GNU GPLv3
 */

#ifndef CCS811_H
#define CCS811_H

#include "stdint.h"
#include "application.h"

#define CCS811_STATUS_REG     0x00
#define CCS811_MEAS_MODE_REG  0x01
#define CCS811_RESULT_REG     0x02
#define CCS811_ENV_REG        0x05

#define CCS811_BASELINE_REG   0x11

#define CCS811_HW_ID_REG      0x20
#define CCS811_APP_VER_REG    0x24
#define CCS811_START_APP      0xF4

#define CCS811_IDLE_MODE      0x00
#define CCS811_CONSTANT_MODE  0x20
#define CCS811_INT_EN         0x08

#define CCS811_FW_MODE_RUN    0x90

#define CCS811_MIN_C02_LEVEL  400

#define CCS811_BASELINE_ADDR  10

// Error codes
enum {
  CCS811_SUCCESS,
  CCS811_NULL_ERROR,
  CCS811_NO_DAT_AVAIL,
  CCS811_RUN_ERROR,
  CCS811_COMM_ERR
};

typedef struct {
  uint16_t c02;
  uint16_t tvoc;
} ccs811_data_t;

typedef struct {
  uint8_t address;
  raw_interrupt_handler_t pin_interrupt;
  uint8_t int_pin;
  uint8_t rst_pin;
  uint8_t wake_pin;
} ccs811_init_t;

typedef struct {
  union {
    struct {
    uint8_t major: 4;
    uint8_t minor: 4;
    };
    uint8_t majorminor;
  };
  uint8_t trivial;
} ccs811_app_ver_t;

class CCS811 {
  public:
    CCS811(void);
    uint32_t setup(ccs811_init_t * init);
    uint32_t enable(void);
    uint32_t set_env(float temp, float hum); //set the temp and humidity variables
    uint32_t read(ccs811_data_t * p_data);
    uint32_t save_baseline();
    uint32_t restore_baseline();
    uint32_t get_app_version(ccs811_app_ver_t * p_app_ver);
    void int_handler(void);
  protected:
    uint8_t address;
    raw_interrupt_handler_t pin_interrupt;
    uint8_t int_pin;
    uint8_t rst_pin;
    uint8_t wake_pin;
    bool    data_ready;
};

#endif