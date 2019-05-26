#ifndef SERIAL_LOCK_H
#define SERIAL_LOCK_H

#include "stdint.h"

typedef enum {
  serial_lock_none,
  serial_lock_gps,
  serial_lock_hpma
} serial_lock_owner_t;

typedef struct {
  serial_lock_owner_t owner;
} serial_lock_t;

#endif //SERIAL_LOCK_H