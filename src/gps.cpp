#include "gps.h"
#include "application.h"

GPS::GPS(void){}

uint32_t GPS::init(gps_init_t *p_init) {

  // Returns an error if the argument is null
  if( p_init == NULL ) {
    return GPS_NULL_ERROR;
  }

  // Copy config over
  this->config = *p_init;

  // Disable by default
  this->disable();

  return GPS_SUCCESS;
}

uint32_t GPS::enable() {

  return GPS_SUCCESS;
}

uint32_t GPS::get_fix() {

  return GPS_SUCCESS;
}

uint32_t GPS::disable() {

  // TODO: disable serial etc

  // Set to an output
  pinMode(this->config.enable_pin, OUTPUT);

  // Drive it high (this pin is active low)
  digitalWrite(this->config.enable_pin, HIGH);

  return GPS_SUCCESS;
}