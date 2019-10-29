/*
 * Project Canary Example using Air Quality Wing Library
 * Description: Basic example using the Air Quality Wing Library plus The Canary
 * Author: Jared Wolff (Circuit Dojo LLC)
 * Date: 10/27/2019
 * License: GNU GPLv3
 */

#include "AirQualityWing.h"
#include "board.h"
#include "tune.h"
#include "motion.h"

// Alert interval
#define ALERT_INTERVAL  120000

// Logger
SerialLogHandler logHandler(115200, LOG_LEVEL_ERROR, {
    { "app", LOG_LEVEL_WARN }, // enable all app messages
});

// Forward declaration of event handler
void AirQualityWingEvent();

// AirQualityWing object
AirQualityWing AirQual = AirQualityWing();

// Alarm tracking
uint32_t m_alarm_ticks = 0;
bool     m_silenced = false;

// Local data
AirQualityWingData_t m_data;

// Handler is called in main loop.
// Ok to run Particle.Publish
void AirQualityWingEvent() {

  Log.trace("pub");

  // Publish event
  Particle.publish("blob", AirQual.toString(), PRIVATE, WITH_ACK);

  // Copy data
  m_data = AirQual.getData();

  // Process alarm
  processAlarm(m_data);

}

// Cloud function for setting interval
int set_interval( String period ) {

  // Set the interval with the air quality code
  AirQual.setInterval((uint32_t)period.toInt());

  return -1;

}

// Cloud funciton for silencing
int silence( String data ) {

  uint32_t is_silenced = data.toInt();

  if( is_silenced ) {
    m_silenced = true;
  } else {
    m_silenced = false;
  }

  return 1;

}


// setup() runs once, when the device is first turned on.
void setup() {

  // Turn off the LED
  RGB.control(true);
  RGB.brightness(0);

  // Set up PIR interrupt
  pinMode(PIR_INT_PIN, INPUT_PULLDOWN);
  attachInterrupt(PIR_INT_PIN, motionEvent, RISING);
  // TODO: wake the device up from sleep

  // Set up PC based UART (for debugging)
  Serial.blockOnOverrun(false);
  Serial.begin();

  // Set up I2C
  Wire.setSpeed(I2C_CLK_SPEED);
  Wire.begin();

  // Default settings
  AirQualityWingSettings_t defaultSettings =
  { MEASUREMENT_DELAY_MS, //Measurement Interval
    true,                 //Has HPMA115
    true,                 //Has CCS811
    true,                 //Has Si7021
    CCS811_ADDRESS,       //CCS811 address
    CCS811_INT_PIN,       //CCS811 intpin
    CCS811_RST_PIN,       //CCS811 rst pin
    CCS811_WAKE_PIN,      //CCS811 wake pin
    HPMA1150_EN_PIN       //HPMA int pin
  };

  // Setup & Begin Air Quality
  AirQual.setup(AirQualityWingEvent, defaultSettings);
  AirQual.begin();

  // Set up cloud variable
  Particle.function("set_interval", set_interval);
  Particle.function("silence", silence);

  // Startup message
  Serial.println("Air Quality Wing for Particle Mesh");

  // Play happy wakeup tune
  wakeup_tune();

}

void processAlarm(AirQualityWingData_t data) {
    // Set audible alarm
  if ( data.hpma115.data.pm25 > PM25_HIGH && !m_silenced ) {

    // If we've overflowed reset
    if( millis() < m_alarm_ticks ) {
      m_alarm_ticks = 0;
    }

    //Every two minutes play alert tune
    if( millis() - m_alarm_ticks > ALERT_INTERVAL ) {
      m_alarm_ticks = millis();
      alert_tune();
    }

  }
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {

  // Process motion events
  processMotion(m_data);

  // Process air quality
  uint32_t err_code = AirQual.process();
  if( err_code != success ) {

    switch(err_code) {
      case si7021_error:
         Particle.publish("err", "si7021" , PRIVATE, NO_ACK);
          Log.error("Error si7021");
      case ccs811_error:
         Particle.publish("err", "ccs811" , PRIVATE, NO_ACK);
          Log.error("Error ccs811");
      case hpma115_error:
         Particle.publish("err", "hpma115" , PRIVATE, NO_ACK);
          Log.error("Error hpma115");
      default:
        break;
    }

  }

}