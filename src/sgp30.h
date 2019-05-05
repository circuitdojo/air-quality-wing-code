#ifndef SGP30_H
#define SGP30_H

#include "stdint.h"
#include "application.h"

#define SGP30_ADDRESS 0x58

#define SGP30_GET_ID    0x3682
#define SGP30_INIT_CMD  0x2003
#define SGP30_MEAS_AIR  0x2008
#define SGP30_GET_BASE  0x2015
#define SGP30_SET_BASE  0x201e
#define SGP30_SET_HUM   0x2061
#define SGP30_MEAS_TEST 0x2032
#define SGP30_GET_VER   0x202f
#define SGP30_MEAS_RAW  0x2050

#define SGP30_READ_INTERVAL   1000

#define SGP30_BASELINE_ADDR  20

#define SGP30_MIN_TVOC_LEVEL 15

// Error codes
enum {
  SGP30_SUCCESS,
  SGP30_NULL_ERROR,
  SGP30_NO_DAT_AVAIL,
  SGP30_RUN_ERROR,
  SGP30_COMM_ERR,
  SGP30_DATA_ERR,
  SGP30_NO_BASELINE_AVAIL
};

typedef struct {
  uint16_t c02;
  uint16_t tvoc;
} sgp30_data_t;

class SGP30 {
  public:
    SGP30(void);
    uint32_t setup();
    uint32_t enable(void);
    uint32_t set_env(float temp, float hum); //set humidity variable (temp not used)
    uint32_t read(sgp30_data_t * p_data);
    uint32_t save_baseline();
    uint32_t restore_baseline();
    uint32_t process();
    void     set_ready();
  private:
    uint32_t read_data_check_crc(uint16_t * data);
  protected:
    sgp30_data_t data;
    bool ready, data_available;
};

#endif // SGP30_H