#include "Particle.h"
#include "motion.h"
#include "hpma115.h"

// Event flags
bool m_pir_event = false;
bool m_led_motion_on = false;

// Motion ticks
static uint32_t m_motion_ticks = 0;
static uint32_t m_last_motion_ticks = 0;

// Event for recieving motion alerts
void motionEvent(void) {
  m_pir_event = true;
}


void processMotion(AirQualityWingData_t data) {
  // If there is a PIR event handle that here
  if( m_pir_event && !m_led_motion_on ) {

    Serial.println("pir event");

    // get last pm25 measurement
    // and evaluate using the scale from here:
    // https://en.wikipedia.org/wiki/Air_quality_index#Europe
    if( data.hpma115.data.pm25 > PM25_HAZARDOUS ) {
      Serial.println("haz");
      RGB.color(255,0,0);
    } else if ( data.hpma115.data.pm25 > PM25_HIGH ) {
      Serial.println("high");
      RGB.color(0xff,0x60,0);
    } else if ( data.hpma115.data.pm25 > PM25_MED ) {
      Serial.println("med");
      RGB.color(0xff,0xff,0);
    } else {
      Serial.println("ok");
      RGB.color(0,255,0);
    }

    // turn on LED
    uint8_t bright = 0;
    for( int i = 10; i-- > 0; ) {
      RGB.brightness(bright+=25);
      delay(100);
    }

    // Full powah
    RGB.brightness(255);

    // Set ticks
    m_motion_ticks = millis();

    // Flag to check to turn things off later
    m_led_motion_on = true;

    // TODO: define action here (start taking measurements
  }

    // Handle if the LED is turned on
  if( m_led_motion_on ) {

    // If we've rolled over
    if( millis() < m_motion_ticks ) {
      m_motion_ticks = 0;
    }

    // Check if the LED has been on for more tan 10 sec
    if( millis() - m_motion_ticks > LED_ON_INTERVAL ) {

      uint8_t bright = 255;

      // shut down
      for( int i = 10; i-- > 0; ) {
        RGB.brightness(bright-=25);
        delay(100);
      }

      // Brightness to 0
      RGB.brightness(0);

      // Reset flags
      m_led_motion_on = false;
      m_pir_event = false;
    }

  }
}