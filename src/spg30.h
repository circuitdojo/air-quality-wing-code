#ifndef SPG30_H
#define SPG30_H

#include "stdint.h"
#include "application.h"

#define SPG30_ADDRESS 0x58

#define SPG30_GET_ID    0x3682
#define SPG30_INIT_CMD  0x2003
#define SPG30_MEAS_AIR  0x2008
#define SPG30_GET_BASE  0x2015
#define SPG30_SET_BASE  0x201e
#define SPG30_SET_HUM   0x2061
#define SPG30_MEAS_TEST 0x2032
#define SPG30_GET_VER   0x202f
#define SPG30_MEAS_RAW  0x2050

#define SPG30_READ_INTERVAL   1000

#define SPG30_BASELINE_ADDR  20

#define SPG30_MIN_TVOC_LEVEL 15

// Error codes
enum {
  SPG30_SUCCESS,
  SPG30_NULL_ERROR,
  SPG30_NO_DAT_AVAIL,
  SPG30_RUN_ERROR,
  SPG30_COMM_ERR,
  SPG30_DATA_ERR,
  SPG30_NO_BASELINE_AVAIL
};

typedef struct {
  uint16_t c02;
  uint16_t tvoc;
} spg30_data_t;

class SPG30 {
  public:
    SPG30(void);
    uint32_t setup();
    uint32_t enable(void);
    uint32_t set_env(float temp, float hum); //set humidity variable (temp not used)
    uint32_t read(spg30_data_t * p_data);
    uint32_t save_baseline();
    uint32_t restore_baseline();
    uint32_t process();
    void     set_ready();
  private:
    uint32_t read_data_check_crc(uint16_t * data);
  protected:
    spg30_data_t data;
    bool ready, data_available;
};

#endif // SPG30_H