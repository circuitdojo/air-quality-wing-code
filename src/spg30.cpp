/*
 * Project Particle Squared
 * Description: Particle powered PM2.5 and air quality sensor
 * Author: Jared Wolff
 * Date: 4/27/2019
 * License: GNU GPLv3
 */

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

uint32_t SPG30::set_env(float temp, float hum) {

  // // Shift left b/c this register is shifted left by 1
  // uint8_t hum_conv[2];
  // hum_conv[0] = (uint8_t)hum << 1;
  // hum_conv[1] = 0; // Not bothering with sending fraction

  // // Serial.printf("temp %.2f", temp);
  // // Serial.printf("hum %.2f", hum);
  // // Serial.printf("frac %.2f", frac_part);

  // // Generate the calculated values
  // uint8_t temp_conv[2];
  // temp_conv[0] = (((uint8_t)temp + 25) << 1);
	// temp_conv[1] = 0; // Not bothering with sending fraction

  // // Data to send
  // uint8_t data[4];

  // // Copy bytes to output
  // memcpy(data,&hum_conv,sizeof(hum_conv));
  // memcpy(data+2,&temp_conv,sizeof(temp_conv));

  // // Write this
  // Wire.beginTransmission(SPG30_ADDRESS);
  // Wire.write(SPG30_ENV_REG);
  // Wire.write(data,sizeof(data));
  // Wire.endTransmission();  // stop transaction

  return SPG30_SUCCESS;

}

uint32_t SPG30::enable(void) {

  // uint32_t err_code;

  // // Set mode to 10 sec mode & enable int
  // Wire.beginTransmission(SPG30_ADDRESS);
  // Wire.write(SPG30_MEAS_MODE_REG); // sends register address
  // Wire.write(SPG30_CONSTANT_MODE | SPG30_INT_EN);     // enables consant mode with interrupt
  // err_code = Wire.endTransmission();           // stop transaction
  // if( err_code != 0 ){
  //   return err_code;
  // }

  // // Clear any interrupts
  // Wire.beginTransmission(SPG30_ADDRESS);
  // Wire.write(SPG30_RESULT_REG); // sends register address
  // err_code = Wire.endTransmission(false);  // stop transaction
  // if( err_code != 0 ){
  //   return err_code;
  // }

  // // Flush bytes
  // Wire.requestFrom(SPG30_ADDRESS, (uint8_t)4); // Read the bytes
  // while(Wire.available()) {
  //   Wire.read();
  // }

  return SPG30_SUCCESS;

}

uint32_t SPG30::save_baseline() {

//   Serial.println("save baseline");

//   Wire.beginTransmission(SPG30_ADDRESS);
//   Wire.write(SPG30_BASELINE_REG); // sends register address
//   Wire.endTransmission();  // stop transaction
//   Wire.requestFrom(SPG30_ADDRESS, (uint8_t)2); // request the bytes

//   uint8_t baseline[2];
//   baseline[0] = Wire.read();
//   baseline[1] = Wire.read();

//   // Write to the address
//   EEPROM.put(SPG30_BASELINE_ADDR, baseline);

//   return NRF_SUCCESS;
// }

// uint32_t SPG30::restore_baseline() {

//   uint32_t err_code;

//   Serial.println("restore baseline");

//   uint8_t baseline[2];

//   // Get the baseline to the address
//   EEPROM.get(SPG30_BASELINE_ADDR, baseline);

//   // If it's uninitialized, return invalid data
//   if ( baseline[0] == 0xff && baseline[1] == 0xff) {
//     Serial.println("restore error");
//     return NRF_ERROR_INVALID_DATA;
//   }

//   // Write to the chip
//   Wire.beginTransmission(SPG30_ADDRESS);
//   Wire.write(SPG30_BASELINE_REG); // sends register address
//   Wire.write(baseline[0]);
//   Wire.write(baseline[1]);
//   err_code = Wire.endTransmission();           // stop transaction
//   if( err_code != 0 ){
//     return err_code;
//   }

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

uint32_t SPG30::process() {

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
    if ( bytes_recieved == 0 ) {
      return SPG30_COMM_ERR;
    }

    // Read temp c02 data and get CRC
    uint16_t temp_c02 = (Wire.read()<<8) + Wire.read();
    uint8_t  c02_crc_calc = crc8_dallas_little((uint8_t*)&temp_c02,2);
    uint8_t  c02_crc = Wire.read();

    // Read temp tvoc data and get CRC
    uint16_t temp_tvoc = (Wire.read()<<8) + Wire.read();
    uint8_t  tvoc_crc_calc = crc8_dallas_little((uint8_t*)&temp_tvoc,2);
    uint8_t  tvoc_crc = Wire.read();

    // Return if CRC is incorrect
    if( c02_crc != c02_crc_calc ) {
      Serial.printf("c02 crc fail %x %x\n", c02_crc, c02_crc_calc);
      return SPG30_DATA_ERR;
    }

    // Return if tvoc does not match
    if( tvoc_crc != tvoc_crc_calc ) {
      Serial.printf("tvoc crc fail %x %x\n", tvoc_crc, tvoc_crc_calc);
      return SPG30_DATA_ERR;
    }

    // Convert data to something useful
    this->data.c02 = temp_c02;

    // Read TVOC
    this->data.tvoc = temp_tvoc;

    // Print results
    // Serial.printf("spg30 c02: %dppm tvoc: %dppb\n",this->data.c02,this->data.tvoc);

    // data is ready!
    this->data_available = true;

  }

  return SPG30_SUCCESS;
}
