#include "si7021.h"

Si7021::Si7021(void) {}

uint32_t Si7021::setup() {
  // Return error if we failed
  if (Wire.requestFrom(SI7021_ADDRESS, 1) == 0) {
    return SI7021_COMMS_FAIL_ERROR;
  }
}

si7021_data_t Si7021::read(void) {

    // Output struct
    si7021_data_t ret;

    // Si7021 Temperature
    Wire.beginTransmission(SI7021_ADDRESS);
    Wire.write(SI7021_TEMP_HOLD_CMD); // sends one byte
    Wire.endTransmission();           // stop transaction
    Wire.requestFrom(SI7021_ADDRESS, 2);

    // Get the raw temperature from the device
    uint16_t temp_code = (Wire.read() & 0x00ff) << 8 | (Wire.read() & 0x00ff);

    // Then calculate the temperature
    ret.temperature = ((175.72 * temp_code) / 0xffff - 46.85) * 100;

    // Si7021 Humidity
    Wire.beginTransmission(SI7021_ADDRESS);
    Wire.write(SI7021_HUMIDITY_READ_CMD); // sends one byte
    Wire.endTransmission();               // stop transaction
    Wire.requestFrom(SI7021_ADDRESS, 2);

    // Get the raw humidity value from the evice
    uint16_t rh_code = (Wire.read() & 0x00ff) << 8 | (Wire.read() & 0x00ff);

    // Then calculate the teperature
    ret.humidity = ((125 * rh_code) / 0xffff - 6) * 100;

    return ret;
}