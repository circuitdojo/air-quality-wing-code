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

void ccs811_handler( ccs811_data_t * p_data );
void hpma_handler( void );
void css811_pin_interrupt();
void setup();
void loop();
#line 11 "/Users/jaredwolff/Circuit_Dojo/pm25/src/pm25.ino"
#define I2C_SDA_PIN     D0
#define I2C_SCL_PIN     D1
#define UART_TX_PIN     D2
#define UART_RX_PIN     D3
#define CCS811_INT_PIN  D4

// Address for CCS811, it is setable in hardware so should be defined here
#define CCS811_ADDRESS  0x5a

// Delay and timing related contsants
#define MEASUREMENT_DELAY_MS 60000

// I2C Related constants
#define I2C_CLK_SPEED 100000

// Used to do some basic timing
static uint32_t last_measurement_ms = 0;

// Static objects
static Si7021 si7021 = Si7021();
static CCS811 ccs811 = CCS811();

// Callback for CCS811
void ccs811_handler( ccs811_data_t * p_data ) {

}

void hpma_handler( void ) {

}

// css811_pin_interrupt() forwards pin interrupt on to the specific handler
void css811_pin_interrupt() {
  ccs811.int_handler();
}

// setup() runs once, when the device is first turned on.
void setup() {

  // Set up PC based UART (for debugging)
  Serial.begin();

  // Set up I2C
  Wire.setSpeed(I2C_CLK_SPEED);
  Wire.begin();

  // Set up Si7021;
  si7021.setup();
  // TODO: fault if 0

  // Setup CC8012
  ccs811_init_t init;
  init.int_pin = CCS811_INT_PIN;
  init.callback = ccs811_handler;
  init.address = CCS811_ADDRESS;
  init.pin_interrupt = css811_pin_interrupt;

  // Init the TVOC & C02 sensor
  ccs811.setup(&init);

  // Start VOC measurement
  // This is an async reading.
  ccs811.enable();

  // Set up PM2.5 sensor

}

// loop() runs over and over again, as quickly as it can execute.
void loop() {

  // If we're greater than or equal to the measurement delay
  // start taking measurements!
  // TODO: if possible replace with app timer
  if( millis()-last_measurement_ms >= MEASUREMENT_DELAY_MS) {
    // Reset the timer
    last_measurement_ms = millis();

    // Read temp and humiity
    si7021_data_t si7021_data = si7021.read();

    // Publish this data
    Particle.publish("temperature", String::format("%.2f",si7021_data.temperature), PRIVATE, WITH_ACK);
    Particle.publish("humidity", String::format("%.2f",si7021_data.humidity), PRIVATE, WITH_ACK);

    // Process CCS811
    ccs811.process();

    // Start PM2.5 measurement
    // This is an "async" reading.

  }



  // Process PM2.5

  Particle.process();
}