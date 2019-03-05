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
  this->address       = p_init->address;
  this->rst_pin       = p_init->rst_pin;
  this->pin_interrupt = p_init->pin_interrupt;
  this->wake_pin      = p_init->wake_pin;

  // Configure the pin interrupt
  pinMode(this->int_pin, INPUT);
  attachInterrupt(this->int_pin, this->pin_interrupt, FALLING);

  // Wake it up
  pinMode(this->wake_pin, OUTPUT);
  digitalWrite(this->wake_pin, LOW); // Has a pullup

  // Toggle reset pin
  pinMode(this->rst_pin, OUTPUT);
  digitalWrite(this->rst_pin, LOW);
  delay(1);
  digitalWrite(this->rst_pin, HIGH);
  delay(1);

  // Set the mode to off
  Wire.beginTransmission(this->address);
  Wire.write(CCS811_MEAS_MODE_REG); // sends register address
  Wire.write(CSS811_IDLE_MODE);     // sends configuration byte
  return Wire.endTransmission();    // stop transaction

}


uint32_t CCS811::enable(void) {

  uint32_t err_code;
  char data[4];

  // Set mode to 10 sec mode & enable int
  Wire.beginTransmission(this->address);
  Wire.write(CCS811_MEAS_MODE_REG); // sends register address
  Wire.write(CSS811_CONSTANT_MODE | CSS811_INT_EN);     // enables consant mode with interrupt
  err_code = Wire.endTransmission();           // stop transaction
  if( err_code != 0 ){
    return err_code;
  }

  // Clear any interrupts
  Wire.beginTransmission(this->address);
  Wire.write(CCS811_RESULT_REG); // sends register address
  err_code = Wire.endTransmission();  // stop transaction
  if( err_code != 0 ){
    return err_code;
  }

  size_t bytes_read = Wire.readBytes(data,4);
  // if( bytes_read != 4 ) {
  //   Serial.println("no dat");
  //   return CSS811_NO_DAT_AVAIL;
  // }

  uint8_t intpin = digitalRead(this->int_pin);
  Serial.printf("Int pin %d\n",intpin);

  return CSS811_SUCCESS;

}

uint32_t CCS811::read(ccs811_data_t * p_data) {

  // If the data is ready, read
  if( this->data_ready ) {
      char data[4];

      // Disable flag
      this->data_ready = false;

      Wire.beginTransmission(this->address);
      Wire.write(CCS811_RESULT_REG); // sends register address
      Wire.endTransmission();  // stop transaction
      Wire.readBytes(data,4);

      // Convert data to something useful
      p_data->c02 = data[0];
      p_data->c02 = (p_data->c02<<8) + data[1];

      p_data->tvoc = data[2];
      p_data->tvoc = (p_data->tvoc<<8) + data[3];

      return CSS811_SUCCESS;
  } else {
      return CSS811_NO_DAT_AVAIL;
  }

}
