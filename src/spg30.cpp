/*
 * Project Particle Squared
 * Description: Particle powered PM2.5 and air quality sensor
 * Author: Jared Wolff
 * Date: 4/27/2019
 * License: GNU GPLv3
 */

#include <math.h>
#include "SPG30.h"
#include "crc8_dallas.h"

SPG30::SPG30() {}

uint32_t SPG30::setup() {

  // Init variables
  this->ready = false;
  this->data_available = false;

  // Run the app
  Wire.beginTransmission(SPG30_ADDRESS);
  Wire.write((SPG30_INIT_CMD >> 8) & 0xff); // sends register address
  Wire.write(SPG30_INIT_CMD & 0xff); // sends register address
  uint8_t ret = Wire.endTransmission();  // stop transaction

  // Return an error if we have an error
  if (ret != 0) {
    Serial.printf("spg30 err: %d\n", ret);
    return SPG30_COMM_ERR;
  }

  // Startup delay
  delay(30);

  return SPG30_SUCCESS;

}

void SPG30::set_ready() {
  this->ready = true;
}

uint32_t SPG30::save_baseline() {

  Serial.println("save baseline");

  Wire.beginTransmission(SPG30_ADDRESS);
  Wire.write((SPG30_GET_BASE >> 8) & 0xff); // sends register address
  Wire.write(SPG30_GET_BASE & 0xff); // sends register address
  uint8_t ret = Wire.endTransmission();  // stop transaction

  // Return on error
  if( ret != 0 ) {
    return SPG30_COMM_ERR;
  }

  Wire.requestFrom(SPG30_ADDRESS, 6); // request the bytes

  uint16_t c02_baseline, tvoc_baseline;

  // Get C02 data
  uint32_t err_code = this->read_data_check_crc(&c02_baseline);
  if( err_code != SPG30_SUCCESS ) {
    return SPG30_DATA_ERR;
  }

  // Get the TVOC data
  err_code = this->read_data_check_crc(&tvoc_baseline);
  if( err_code != SPG30_SUCCESS ) {
    return SPG30_DATA_ERR;
  }

  // Write to the address
  EEPROM.put(SPG30_C02_BASELINE_ADDR, c02_baseline);
  EEPROM.put(SPG30_TVOC_BASELINE_ADDR, tvoc_baseline);

  return NRF_SUCCESS;
}

uint32_t SPG30::restore_baseline() {

  Serial.println("restore baseline");

  uint8_t c02_baseline[2],tvoc_baseline[2];

  // Get the baseline to the address
  EEPROM.get(SPG30_C02_BASELINE_ADDR, c02_baseline);

  // If it's uninitialized, return invalid data
  if ( c02_baseline[0] == 0xff && c02_baseline[1] == 0xff) {
    Serial.println("restore error");
    return NRF_ERROR_INVALID_DATA;
  }

  // Get the baseline to the address
  EEPROM.get(SPG30_C02_BASELINE_ADDR, tvoc_baseline);

    // If it's uninitialized, return invalid data
  if ( tvoc_baseline[0] == 0xff && tvoc_baseline[1] == 0xff) {
    Serial.println("restore error");
    return NRF_ERROR_INVALID_DATA;
  }

  // TODO: flush this out

  // // Write to the chip
  // Wire.beginTransmission(SPG30_ADDRESS);
  // Wire.write(SPG30_BASELINE_REG); // sends register address
  // Wire.write(baseline[0]);
  // Wire.write(baseline[1]);
  // err_code = Wire.endTransmission();           // stop transaction
  // if( err_code != 0 ){
  //   return err_code;
  // }

  return SPG30_SUCCESS;
}

uint32_t SPG30::read(spg30_data_t * p_data) {

  if( this->data_available ) {
    //Copy the data
    *p_data = this->data;

    return SPG30_SUCCESS;
  } else {
    return SPG30_NO_DAT_AVAIL;
  }

}

uint32_t SPG30::read_data_check_crc( uint16_t * data ){

    // Read temp c02 data and get CRC
    uint16_t temp = (Wire.read()<<8) + Wire.read();
    uint8_t  crc_calc = crc8_dallas_little((uint8_t*)&temp,2);
    uint8_t  crc = Wire.read();

    // Return if CRC is incorrect
    if( crc != crc_calc ) {
      Serial.printf("crc fail %x %x\n", crc, crc_calc);
      return SPG30_DATA_ERR;
    }

    // Copy data over
    *data = temp;

    // Return on success!
    return SPG30_SUCCESS;

}

uint32_t SPG30::process() {

  uint32_t err_code;

  if( this->ready ) {

    // Reset this var
    this->ready = false;

    Wire.beginTransmission(SPG30_ADDRESS);
    Wire.write((SPG30_MEAS_AIR >> 8) & 0xff); // sends register address
    Wire.write(SPG30_MEAS_AIR & 0xff); // sends register address
    uint8_t ret = Wire.endTransmission();  // stop transaction

    // Return on error
    if( ret != 0 ) {
      return SPG30_COMM_ERR;
    }

    // Set up measurement delay
    delay(13);

    // Start Rx 2 tvoc, 2 c02, 2 CRC
    uint8_t bytes_recieved = Wire.requestFrom(SPG30_ADDRESS, 6); // request the bytes

    // If no bytes recieved return
    if ( bytes_recieved == 0 || bytes_recieved != 6 ) {
      return SPG30_COMM_ERR;
    }

    // Get C02 data
    err_code = this->read_data_check_crc(&this->data.c02);
    if( err_code != SPG30_SUCCESS ) {
      return SPG30_DATA_ERR;
    }

    // Get the TVOC data
    err_code = this->read_data_check_crc(&this->data.tvoc);
    if( err_code != SPG30_SUCCESS ) {
      return SPG30_DATA_ERR;
    }

    // Print results
    // Serial.printf("spg30 c02: %dppm tvoc: %dppb\n",this->data.c02,this->data.tvoc);

    // data is ready!
    this->data_available = true;

  }

  return SPG30_SUCCESS;
}

uint32_t SPG30::set_env(float temp, float hum) {

  const float a = 5.018;
  const float b = .32321;
  const float c = 0.0081847;
  const float d = 0.00031243;

  Serial.printf("temp %.2f%% %.2f°c\n", hum, temp);

  // Calculate the saturation vapor desnity
  // Based on the curves here: http://hyperphysics.phy-astr.gsu.edu/hbase/Kinetic/relhum.html#c3
  // Value is in gm/m3
  float sat_vapor_density = a + b * temp + c * pow(temp,2) + d * pow(temp,3);

  Serial.printf("saturation vapor density %.4f\n", sat_vapor_density);

  // Calculate the actual
  // Value is in gm/m3
  float actual_vapor_density = hum * sat_vapor_density / 100.0;

  // Write the actual to the spg30
  Serial.printf("actual vapor density %.4f\n", actual_vapor_density);

  // Organize the bytes
  uint8_t data[3];
  data[0] = (uint8_t)actual_vapor_density;
  data[1] = (uint8_t)((actual_vapor_density-data[0]) * 256);

  Serial.printf("data to spg30: 0x%x%x\n",data[0],data[1]);

  // Cacluate the crc for the last byte
  data[2] = crc8_dallas_little(data,2);

  // Write the humidity data
  Wire.beginTransmission(SPG30_ADDRESS);
  Wire.write((SPG30_SET_HUM >> 8) & 0xff); // sends register address
  Wire.write(SPG30_SET_HUM & 0xff); // sends register address
  Wire.write(data,3);
  uint8_t ret = Wire.endTransmission();  // stop transaction

  // // Return on error
  // if( ret != 0 ) {
  //   Serial.printf("set env err %d\n",ret);
  //   return SPG30_COMM_ERR;
  // }


  return SPG30_SUCCESS;
}