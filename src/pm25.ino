/*
 * Project pm25
 * Description: Particle powered PM2.5 and air quality sensor
 * Author: Jared Wolff
 * Date: 2/26/2019
 */

#include "si7021.h"
#include "ccs811.h"
#include "hpma115.h"

// SYSTEM_MODE(MANUAL);

#define I2C_SDA_PIN     D0
#define I2C_SCL_PIN     D1

#define CCS811_WAKE_PIN D6
#define CCS811_INT_PIN  D8
#define CCS811_RST_PIN  D7

#define HPMA1150_EN_PIN D5

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
static si7021_data_t si7021_data;
static ccs811_data_t ccs811_data;
static hpma115_data_t hpma115_data;

// Data state ready
static bool hpma115_data_ready = false;
static bool si7021_data_ready = false;
static bool ccs811_data_ready = false;

// ccs811_pin_interrupt() forwards pin interrupt on to the specific handler
void ccs811_pin_interrupt() {
  ccs811.int_handler();
}

// forwards serial data interrupt to HPMA driver
void serialEvent1() {
  hpma115.int_handler();
}

// Async publish event
void hpma_evt_handler(hpma115_data_t *p_data) {
  hpma115_data = *p_data;

  // Serial.printf("pm25 %dμg/m3 pm10 %dμg/m3\n", hpma115_data.pm25, hpma115_data.pm10);

  hpma115_data_ready = true;
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
    System.reset();
  }

  // Setup CC8012
  ccs811_init_t ccs811_init;
  ccs811_init.int_pin = CCS811_INT_PIN;
  ccs811_init.address = CCS811_ADDRESS;
  ccs811_init.pin_interrupt = ccs811_pin_interrupt;
  ccs811_init.rst_pin = CCS811_RST_PIN;
  ccs811_init.wake_pin = CCS811_WAKE_PIN;

  // Init the TVOC & C02 sensor
  err_code = ccs811.setup(&ccs811_init);
  if( err_code != 0 ) {
    Serial.printf("ccs811 setup err %d\n", err_code);
    System.reset();
  }

  // Start VOC measurement
  // This is an async reading.
  err_code = ccs811.enable();
  if( err_code != 0 ) {
    Serial.printf("ccs811 enable err %d\n", err_code);
    System.reset();
  }

  // Setup HPMA115
  hpma115_init_t hpma115_init;
  hpma115_init.callback = hpma_evt_handler;
  hpma115_init.enable_pin = HPMA1150_EN_PIN;

  // Init HPM115 sensor
  err_code = hpma115.setup(&hpma115_init);
  if (err_code != HPMA115_SUCCESS) {
    Serial.printf("hpma115 enable err %d\n", err_code);
    System.reset();
  }

}

// loop() runs over and over again, as quickly as it can execute.
void loop() {

  // If all the data is ready, send it as one data blob
  if (ccs811_data_ready && si7021_data_ready && hpma115_data_ready ) {
    String out = String::format("{\"temperature\":%.2f,\"humidity\":%.2f,\"pm25\":%d,\"pm10\":%d,\"tvoc\":%d,\"c02\":%d}",si7021_data.temperature,si7021_data.humidity,hpma115_data.pm25,hpma115_data.pm10,ccs811_data.tvoc,ccs811_data.c02);
    Particle.publish("blob", out , PRIVATE, WITH_ACK);

    // Reset to false
    ccs811_data_ready = false;
    si7021_data_ready = false;
    hpma115_data_ready = false;
  }

  // If we're greater than or equal to the measurement delay
  // start taking measurements!
  if( millis()-last_measurement_ms >= MEASUREMENT_DELAY_MS ) {
    // Reset the timer
    last_measurement_ms = millis();

    // Read temp and humiity
    uint32_t err_code = si7021.read(&si7021_data);

    if( err_code == SI7021_SUCCESS ) {
      // Set env data in the CCS811
      ccs811.set_env(si7021_data.temperature,si7021_data.humidity);

      si7021_data_ready = true;
    }

    // Process CCS811
    err_code = ccs811.read(&ccs811_data);

    if ( err_code == CCS811_SUCCESS ) {
      ccs811_data_ready = true;
    }

    // Process PM2.5 and PM10 results
    // This is slightly different from the other readings
    // due to the fact that it should be shut off when not taking a reading
    // (extends the life of the device)
    hpma115.enable();

  }

  // Processes any avilable serial data
  hpma115.process();

  // Send updates/communicate with service when connected
  if( Particle.connected ) {
    Particle.process();
  }

}