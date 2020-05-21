/*
 * Project Air Quality Wing
 * Description: Particle powered PM2.5 and air quality sensor
 * Author: Jared Wolff
 * Date: 4/27/2019
 * License: GNU GPLv3
 */

#include <math.h>
#include "sgp30.h"
#include "crc8_dallas.h"

SGP30::SGP30() {}

uint32_t SGP30::setup()
{

  // Init variables
  this->ready = false;
  this->data_available = false;

  // Run the app
  Wire.beginTransmission(SGP30_ADDRESS);
  Wire.write((SGP30_INIT_CMD >> 8) & 0xff); // sends register address
  Wire.write(SGP30_INIT_CMD & 0xff);        // sends register address
  uint8_t ret = Wire.endTransmission();     // stop transaction

  // Return an error if we have an error
  if (ret != 0)
  {
    Serial.printf("sgp30 err: %d\n", ret);
    return SGP30_COMM_ERR;
  }

  // Startup delay
  delay(30);

  return SGP30_SUCCESS;
}

void SGP30::set_ready()
{
  this->ready = true;
}

uint32_t SGP30::save_baseline()
{

  Serial.println("save sgp30 baseline");

  Wire.beginTransmission(SGP30_ADDRESS);
  Wire.write((SGP30_GET_BASE >> 8) & 0xff); // sends register address
  Wire.write(SGP30_GET_BASE & 0xff);        // sends register address
  uint8_t ret = Wire.endTransmission();     // stop transaction

  // Return on error
  if (ret != 0)
  {
    return SGP30_COMM_ERR;
  }

  size_t bytes_read = Wire.requestFrom(SGP30_ADDRESS, 6); // request the bytes

  uint8_t baseline[6];
  uint8_t index = 0;

  if (bytes_read == 6)
  {

    while (Wire.available())
    {
      // Sock away the data
      baseline[index++] = Wire.read();
    }
  }

  Serial.printf("baseline: %x%x%x%x%x%x\n", baseline[0], baseline[1], baseline[2], baseline[3], baseline[4], baseline[5]);

  // Write to the address
  EEPROM.put(SGP30_BASELINE_ADDR, baseline);

  return NRF_SUCCESS;
}

uint32_t SGP30::restore_baseline()
{

  Serial.println("restore sgp30 baseline");

  uint8_t baseline[6];

  // Get the baseline to the address
  EEPROM.get(SGP30_BASELINE_ADDR, baseline);

  // Check to make sure there is valid data
  if ((baseline[0] == 0xff) &&
      (baseline[1] == 0xff) &&
      (baseline[2] == 0xff) &&
      (baseline[3] == 0xff) &&
      (baseline[4] == 0xff) &&
      (baseline[5] == 0xff))
  {
    Serial.println("no sgp30 baseline available");
    return SGP30_NO_BASELINE_AVAIL;
  }

  // Write to the chip
  Wire.beginTransmission(SGP30_ADDRESS);
  Wire.write((SGP30_SET_BASE >> 8) & 0xff); // sends register address
  Wire.write(SGP30_SET_BASE & 0xff);        // sends register address
  Wire.write(baseline, 6);

  uint32_t err_code = Wire.endTransmission(); // stop transaction
  if (err_code != 0)
  {
    return err_code;
  }

  return SGP30_SUCCESS;
}

uint32_t SGP30::read(sgp30_data_t *p_data)
{

  if (this->data_available)
  {
    //Copy the data
    *p_data = this->data;

    return SGP30_SUCCESS;
  }
  else
  {
    return SGP30_NO_DAT_AVAIL;
  }
}

uint32_t SGP30::read_data_check_crc(uint16_t *data)
{

  // Read temp c02 data and get CRC
  uint16_t temp = (Wire.read() << 8) + Wire.read();
  uint8_t crc_calc = crc8_dallas_little((uint8_t *)&temp, 2);
  uint8_t crc = Wire.read();

  // Return if CRC is incorrect
  if (crc != crc_calc)
  {
    Serial.printf("crc fail %x %x\n", crc, crc_calc);
    return SGP30_DATA_ERR;
  }

  // Copy data over
  *data = temp;

  // Return on success!
  return SGP30_SUCCESS;
}

uint32_t SGP30::process()
{

  uint32_t err_code;

  if (this->ready)
  {

    // Reset this var
    this->ready = false;

    Wire.beginTransmission(SGP30_ADDRESS);
    Wire.write((SGP30_MEAS_AIR >> 8) & 0xff); // sends register address
    Wire.write(SGP30_MEAS_AIR & 0xff);        // sends register address
    uint8_t ret = Wire.endTransmission();     // stop transaction

    // Return on error
    if (ret != 0)
    {
      return SGP30_COMM_ERR;
    }

    // Set up measurement delay
    delay(13);

    // Start Rx 2 tvoc, 2 c02, 2 CRC
    uint8_t bytes_recieved = Wire.requestFrom(SGP30_ADDRESS, 6); // request the bytes

    // If no bytes recieved return
    if (bytes_recieved == 0 || bytes_recieved != 6)
    {
      return SGP30_COMM_ERR;
    }

    // Get C02 data
    err_code = this->read_data_check_crc(&this->data.c02);
    if (err_code != SGP30_SUCCESS)
    {
      return SGP30_DATA_ERR;
    }

    // Get the TVOC data
    err_code = this->read_data_check_crc(&this->data.tvoc);
    if (err_code != SGP30_SUCCESS)
    {
      return SGP30_DATA_ERR;
    }

    // Print results
    // Serial.printf("sgp30 c02: %dppm tvoc: %dppb\n", this->data.c02, this->data.tvoc);

    // data is ready!
    this->data_available = true;
  }

  return SGP30_SUCCESS;
}

uint32_t SGP30::set_env(float temp, float hum)
{

  const float a = 216.7;
  const float b = 6.122;
  const float c = 17.62;
  const float d = 243.12;
  const float e = 273.15;

  // Exponent in formula from Sensirion
  // https://www.sensirion.com/fileadmin/user_upload/customers/sensirion/Dokumente/9_Gas_Sensors/Sensirion_Gas_Sensors_SGP30_Driver-Integration-Guide_SW_I2C.pdf
  const float exp = (c * temp) / (d + temp);

  // Calculate the actual
  // Value is in mg/m3
  const float actual_humidity = a * ((hum / 100.0) * pow(b, exp)) / (e + temp);

  // Temphum
  // Serial.printf("temp: %f hum: %f \n", temp, hum);

  // Write the actual to the sgp30
  // Serial.printf("actual humidity %f\n", actual_humidity);

  // Organize the bytes
  uint8_t data[3];
  data[0] = (uint8_t)actual_humidity;                     // whole portion
  data[1] = (uint8_t)((actual_humidity - data[0]) * 256); // 1/256 decimal portion

  // Reverse byte order
  uint8_t crc_check[2];
  crc_check[0] = data[1];
  crc_check[1] = data[0];

  // Cacluate the crc for the last byte
  data[2] = crc8_dallas_little(crc_check, 2);

  // Serial.printf("data to sgp30: 0x%x%x%x\n", data[0], data[1], data[2]);

  // Write the humidity data
  Wire.beginTransmission(SGP30_ADDRESS);
  Wire.write((SGP30_SET_HUM >> 8) & 0xff); // sends register address
  Wire.write(SGP30_SET_HUM & 0xff);        // sends register address
  Wire.write(data, 3);
  uint8_t ret = Wire.endTransmission(); // stop transaction

  // Return on error
  if (ret != 0)
  {
    Serial.printf("set env err %d\n", ret);
    return SGP30_COMM_ERR;
  }

  return SGP30_SUCCESS;
}

uint32_t SGP30::get_ver()
{

  // SGP30_GET_VER

  Wire.beginTransmission(SGP30_ADDRESS);
  Wire.write((SGP30_GET_VER >> 8) & 0xff); // sends register address
  Wire.write(SGP30_GET_VER & 0xff);        // sends register address
  uint8_t ret = Wire.endTransmission();    // stop transaction

  // Return on error
  if (ret != 0)
  {
    return SGP30_COMM_ERR;
  }

  size_t bytes_read = Wire.requestFrom(SGP30_ADDRESS, 3); // request the bytes

  uint8_t ver[3];
  uint8_t index = 0;

  if (bytes_read == 3)
  {

    while (Wire.available())
    {
      // Sock away the data
      ver[index++] = Wire.read();
    }
  }

  // Reverse byte order
  uint8_t crc_check[2];
  crc_check[0] = ver[1];
  crc_check[1] = ver[0];

  uint8_t crc_calc = crc8_dallas_little(crc_check, 2);

  if (crc_calc != ver[2])
  {
    Serial.printf("crc error on sgp30 ver\n");
    return SGP30_DATA_ERR;
  }

  Serial.printf("sgp version: 0x%x\r\n", ver[1]);
  Serial.flush();

  return SGP30_SUCCESS;
}
