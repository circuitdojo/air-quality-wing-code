#include "hpma115.h"

HPMA115::HPMA115(void){}

uint32_t HPMA115::setup(hpma115_init_t *p_init) {

  // Set up serial
  Serial1.begin(HPMA115_BAUD);

  // Set up callback
  this->callback = p_init->callback;

  // Enable autosend
  const char data[] = { HPMA115_HEAD, 1, HPMA115_START_AUTO_SEND_CMD, ((65536-(HPMA115_HEAD+1+HPMA115_START_AUTO_SEND_CMD)) % 256)};

  // Send the data
  Serial1.write(data);

  return HPMA115_SUCCESS;
}

uint32_t HPMA115::enable(){
  //Send command to start readings
  const char data[] = {  HPMA115_HEAD, 1, HPMA115_START_MEASUREMENT, ((65536-(HPMA115_HEAD+1+HPMA115_START_MEASUREMENT)) % 256) };

  Serial1.write(data);

  return HPMA115_SUCCESS;
}
uint32_t HPMA115::disable() {
  //Send command to shut off
  const char data[] = {  HPMA115_HEAD, 1, HPMA115_STOP_MEASUREMENT, ((65536-(HPMA115_HEAD+1+HPMA115_STOP_MEASUREMENT)) % 256) };

  Serial1.write(data);

  return HPMA115_SUCCESS;
}

uint32_t HPMA115::read(hpma115_data_t *p_data) {
  //TODO: take readings read by interrupt, and push them back through pointer
  return HPMA115_SUCCESS;
}

void HPMA115::int_handler() {
  Serial.printf("hpma115 int %d", Serial1.available());
  //TODO: check how many bytes
  //TODO: if bytes == expected, read
  //TODO: disable sensor
  //TODO: call the cb
}