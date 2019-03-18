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

  // Serial.printf("pm25 %dμg/m3 pm10 %dμg/m3\n", p_data->pm25, p_data->pm10);

  Particle.publish("pm25", String::format("%d",p_data->pm25), PRIVATE, WITH_ACK);
  Particle.publish("pm10", String::format("%d",p_data->pm10), PRIVATE, WITH_ACK);
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
  hpma115_init.enable_pin = HPMA1150_EN_PIN;

  // Init HPM115 sensor
  hpma115.setup(&hpma115_init);

}

// loop() runs over and over again, as quickly as it can execute.
void loop() {

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

      // Publish this data
      Particle.publish("temperature", String::format("%.2f",si7021_data.temperature), PRIVATE, WITH_ACK);
      Particle.publish("humidity", String::format("%.2f",si7021_data.humidity), PRIVATE, WITH_ACK);

    }

    // Process CCS811
    err_code = ccs811.read(&ccs811_data);

    if ( err_code == CCS811_SUCCESS ) {
      // Publish this data
      Particle.publish("c02", String::format("%d",ccs811_data.c02), PRIVATE, WITH_ACK);
      Particle.publish("tvoc", String::format("%d",ccs811_data.tvoc), PRIVATE, WITH_ACK);

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