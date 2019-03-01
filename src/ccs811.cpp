#include "ccs811.h"

CCS811::CCS811() {}

void CCS811::int_handler() {
  this->data_ready = true;
}

uint32_t CCS811::setup( ccs811_init_t * p_init ) {

  // Return an error if init is NULL
  if( p_init == NULL ) {
    return CSS811_NULL_ERROR;
  }

  // Configures the important stuff
  this->int_pin       = p_init->int_pin;
  this->callback      = p_init->callback;
  this->address       = p_init->address;
  this->rst_pin       = p_init->rst_pin;
  this->pin_interrupt = p_init->pin_interrupt;

  // Configure the pin interrupt
  attachInterrupt(this->int_pin, this->pin_interrupt, FALLING);

  // Toggle reset pin
  pinMode(this->rst_pin, OUTPUT);
  digitalWrite(this->rst_pin, LOW);
  delay(10);
  pinMode(this->rst_pin, INPUT); //TODO: confirm has pullup

  // Set the mode to off
  Wire.beginTransmission(this->address);
  Wire.write(CCS811_MEAS_MODE_REG); // sends register address
  Wire.write(CSS811_IDLE_MODE);     // sends configuration byte
  Wire.endTransmission();           // stop transaction

  return CSS811_SUCCESS;

}


uint32_t CCS811::enable(void) {

  // Set mode to 10 sec mode & enable int
  Wire.beginTransmission(this->address);
  Wire.write(CCS811_MEAS_MODE_REG); // sends register address
  Wire.write(CSS811_CONSTANT_MODE | CSS811_INT_EN);     // enables consant mode with interrupt
  Wire.endTransmission();           // stop transaction

  return CSS811_SUCCESS;

}

uint32_t CCS811::process(void) {

  // If the data is ready, read
  if( this->data_ready ) {

      char data[4];

      // Disable flag
      this->data_ready = false;

      Wire.beginTransmission(this->address);
      Wire.write(CCS811_RESUKT_REG); // sends register address
      Wire.endTransmission();  // stop transaction
      Wire.readBytes(data,4);

      // TODO: Convert data to something useful
      this->m_data.c02 = 0;
      this->m_data.tvoc = 0;

      // Call callback
      this->callback(&this->m_data);
  }

  return CSS811_SUCCESS;

}
