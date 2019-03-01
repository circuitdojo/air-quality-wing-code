#ifndef CCS811_H
#define CCS811_H

#include "stdint.h"
#include "application.h"

#define CCS811_STATUS_REG     0x00
#define CCS811_MEAS_MODE_REG  0x01
#define CCS811_RESUKT_REG     0x02
#define CCS811_HW_ID_REG      0x20

#define CSS811_IDLE_MODE      0x00
#define CSS811_CONSTANT_MODE  0x10
#define CSS811_INT_EN         0x08

#define CSS811_SUCCESS        0
#define CSS811_NULL_ERROR     1

typedef struct {
  float c02;
  float tvoc;
} ccs811_data_t;

typedef void (*ccs811_cb)(ccs811_data_t * p_data);

typedef struct {
  uint8_t address;
  ccs811_cb callback;
  raw_interrupt_handler_t pin_interrupt;
  uint8_t int_pin;
  uint8_t rst_pin;
} ccs811_init_t;

class CCS811 {
  public:
    CCS811(void);
    uint32_t setup(ccs811_init_t * init);
    uint32_t enable(void);
    uint32_t process(void);
    void int_handler(void);
  protected:
    uint8_t address;
    ccs811_cb callback;
    raw_interrupt_handler_t pin_interrupt;
    uint8_t int_pin;
    uint8_t rst_pin;
    bool    data_ready;
    ccs811_data_t m_data;
};

#endif