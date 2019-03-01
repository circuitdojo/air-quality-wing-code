#include "ccs811.h"

void CCS811::int_handler() {
  CSS811::data_ready = true;
}

uint32_t CCS811::setup( ccs811_init_t * p_init ) {

  // Return an error if init is NULL
  if( p_init == NULL ) {
    return CSS811_NULL_ERROR;
  }

  // Configures the important stuff
  CCS811::int_pin  = p_init->int_pin;
  CCS811::callback = p_init->callback;
  CCS811::address  = p_init->address;
  CSS811::rst_pin  = p_init->rst_pin;

  // Configure the pin interrupt
  attachInterrupt(CCS811::int_pin, CCS811::int_handler, FALLING);

  // Toggle reset pin
  pinMode(CSS811::rst_pin, OUTPUT);
  digitalWrite(CSS811::rst_pin, LOW);
  delay(10);
  pinMode(CSS811::rst_pin, INPUT); //TODO: confirm has pullup

  // Set the mode to off
  Wire.beginTransmission(CSS811::address);
  Wire.write(CCS811_MEAS_MODE_REG); // sends register address
  Wire.write(CSS811_IDLE_MODE);     // sends configuration byte
  Wire.endTransmission();           // stop transaction

  return CSS811_SUCCESS;

}


uint32_t CCS811::enable(void) {

  // Set mode to 10 sec mode & enable int
  Wire.beginTransmission(CSS811::address);
  Wire.write(CCS811_MEAS_MODE_REG); // sends register address
  Wire.write(CSS811_CONSTANT_MODE | CSS811_INT_EN);     // enables consant mode with interrupt
  Wire.endTransmission();           // stop transaction

  return CSS811_SUCCESS;

}

uint32_t CCS811::process(void) {

  // If the data is ready, read
  if( CSS811::data_ready ) {

      char data[4];

      // Disable flag
      CSS811::data_ready = false;

      Wire.beginTransmission(CSS811::address);
      Wire.write(CCS811_RESUKT_REG); // sends register address
      Wire.endTransmission();  // stop transaction
      Wire.readBytes(data,4);

      // TODO: Convert data to something useful
      CSS811::m_data.c02 = 0;
      CSS811::m_data.tvoc = 0;

      // Call callback
      CSS811::callback(&CSS811::m_data);
  }

  return CSS811_SUCCESS;

}
