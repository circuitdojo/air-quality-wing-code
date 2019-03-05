#include "application.h"
#line 1 "/Users/jaredwolff/Circuit_Dojo/pm25/src/pm25.ino"
/*
 * Project pm25
 * Description: Particle powered PM2.5 and air quality sensor
 * Author: Jared Wolff
 * Date: 2/26/2019
 */

#include "si7021.h"
#include "ccs811.h"
#include "hpma115.h"

void css811_pin_interrupt();
void serialEvent1();
void hpma_evt_handler(hpma115_data_t *p_data);
void setup();
void loop();
#line 12 "/Users/jaredwolff/Circuit_Dojo/pm25/src/pm25.ino"
#define I2C_SDA_PIN     D0
#define I2C_SCL_PIN     D1

#define CSS811_WAKE_PIN D6
#define CCS811_INT_PIN  D8
#define CCS811_RST_PIN  D7

// Address for CCS811, it is setable in hardware so should be defined here
#define CCS811_ADDRESS  0x5a

// Delay and timing related contsants
#define MEASUREMENT_DELAY_MS 60000

// I2C Related constants
#define I2C_CLK_SPEED 100000

// Used to do some basic timing
static uint32_t last_measurement_ms = 0;

// Static objects
static Si7021  si7021 = Si7021();
static CCS811  ccs811 = CCS811();
static HPMA115 hpma115 = HPMA115();

// css811_pin_interrupt() forwards pin interrupt on to the specific handler
void css811_pin_interrupt() {
  Serial.println("css811 int");
  ccs811.int_handler();
}

// forwards serial data interrupt to HPMA driver
void serialEvent1() {
  hpma115.int_handler();
}

// Async publish event
void hpma_evt_handler(hpma115_data_t *p_data) {
  //TODO: publish!
}

// setup() runs once, when the device is first turned on.
void setup() {

  // Set up PC based UART (for debugging)
  Serial.begin();

  // Set up I2C
  Wire.setSpeed(I2C_CLK_SPEED);
  Wire.begin();

  // Set up Si7021;
  uint32_t err_code = si7021.setup();
  if( err_code != 0 ) {
    Serial.printf("si7021 setup err %d\n", err_code);
  }

  // Setup CC8012
  ccs811_init_t css811_init;
  css811_init.int_pin = CCS811_INT_PIN;
  css811_init.address = CCS811_ADDRESS;
  css811_init.pin_interrupt = css811_pin_interrupt;
  css811_init.rst_pin = CCS811_RST_PIN;
  css811_init.wake_pin = CSS811_WAKE_PIN;

  // Init the TVOC & C02 sensor
  err_code = ccs811.setup(&css811_init);
  if( err_code != 0 ) {
    Serial.printf("ccs811 setup err %d\n", err_code);
  }

  // Start VOC measurement
  // This is an async reading.
  err_code = ccs811.enable();
  if( err_code != 0 ) {
    Serial.printf("ccs811 enable err %d\n", err_code);
  }

  // Setup HPMA115
  hpma115_init_t hpma115_init;
  hpma115_init.callback = hpma_evt_handler;

  // Init HPM115 sensor
  hpma115.setup(&hpma115_init);

}

// loop() runs over and over again, as quickly as it can execute.
void loop() {

  // If we're greater than or equal to the measurement delay
  // start taking measurements!
  if( millis()-last_measurement_ms >= MEASUREMENT_DELAY_MS) {
    // Reset the timer
    last_measurement_ms = millis();

    // Read temp and humiity
    si7021_data_t si7021_data;
    uint32_t err_code = si7021.read(&si7021_data);

    if( err_code == SI7021_SUCCESS ) {
      Serial.printf("temp: %.2f°C %.2f%%\n",si7021_data.temperature,si7021_data.humidity);

      // Publish this data
      Particle.publish("temperature", String::format("%.2f",si7021_data.temperature), PRIVATE, WITH_ACK);
      Particle.publish("humidity", String::format("%.2f",si7021_data.humidity), PRIVATE, WITH_ACK);

    } else {
       Serial.printf("temp/hum not avail\n");
    }

    // Process CCS811
    ccs811_data_t css811_data;
    err_code = ccs811.read(&css811_data);

    if ( err_code == CSS811_SUCCESS ) {
      Serial.printf("c02: %.2fppm tvoc: %.2fppb\n",css811_data.c02,css811_data.tvoc);

      // Publish this data
      Particle.publish("c02", String::format("%.2f",css811_data.c02), PRIVATE, WITH_ACK);
      Particle.publish("tvoc", String::format("%.2f",css811_data.tvoc), PRIVATE, WITH_ACK);
    } else {
      Serial.printf("tvoc/c02 not avail\n");
    }

    // Process PM2.5 and PM10 results
    // This is slightly different from the other readings
    // due to the fact that it should be shut off when not taking a reading
    // (extends the life of the device)
    hpma115.enable();

  }

  Particle.process();
}