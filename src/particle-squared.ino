/*
 * Project Particle Squared
 * Description: Particle powered PM2.5 and air quality sensor
 * Author: Jared Wolff
 * Date: 2/26/2019
 * License: GNU GPLv3
 */

#include "si7021.h"
#include "ccs811.h"
#include "hpma115.h"
#include "board.h"
#include "gps.h"

// Firmware update
#include "CCS811_FW_App_v2_0_1.h"

SerialLogHandler logHandler(LOG_LEVEL_ERROR);

#define BACKEND_PARTICLE 0
#define BACKEND_SORACOM  1

#ifndef BACKEND_ID
#define BACKEND_ID BACKEND_SORACOM
#endif

#if PLATFORM_ID != PLATFORM_XENON
SYSTEM_MODE(SEMI_AUTOMATIC);
#endif

// Watchdog timeout period
#define WATCHDOG_TIMEOUT_MS 120000

// Delay and timing related contsants
#define MEASUREMENT_DELAY_MS 300000
#define MEASUREMENT_DELAY_ALERT_MS 60000
#define MIN_MEASUREMENT_DELAY_MS 10000
#define HPMA_TIMEOUT_MS 10000
#define CELLULAR_DISCONNECT_TIMEOUT_MS 2000

// Hazard levels
#define PM25_LOW       15
#define PM25_MED       30
#define PM25_HIGH      55
#define PM25_HAZARDOUS 110

#define LED_ON_INTERVAL 10000
#define ALERT_INTERVAL 60000

// Timer handler
void timer_handler();

// Reading delay ms
static uint32_t m_reading_period = MEASUREMENT_DELAY_MS;

// Static objects
static Si7021  si7021 = Si7021();
static CCS811  ccs811 = CCS811();
static HPMA115 hpma115 = HPMA115();
static GPS     gps     = GPS();

// Set up timer
Timer timer(m_reading_period, timer_handler);
Timer disconnect_timer(CELLULAR_DISCONNECT_TIMEOUT_MS, cellular_timer_handler, true);
Timer hpma_timer(HPMA_TIMEOUT_MS, hpma_timeout_handler, true);

// Watchdog
ApplicationWatchdog wd(WATCHDOG_TIMEOUT_MS, System.reset);

// Data
static si7021_data_t si7021_data, si7021_data_last;
static hpma115_data_t hpma115_data;
static gps_data_t gps_data;

#ifdef HAS_CCS811
static ccs811_data_t ccs811_data;
#endif

#if BACKEND_ID == BACKEND_SORACOM
// Define the TCP client
TCPClient client;
#endif

// Fuel gauge output
FuelGauge fuel;

// Event flags
static bool data_check      = true;
static bool m_pir_event     = false;
static bool m_error_flag    = false;
static bool m_data_ready    = false;
static bool m_led_motion_on = false;
static bool m_has_location  = false;
static bool m_disconnect    = false;
static bool m_tcp_publish   = false;

// Motion ticks
static uint32_t m_motion_ticks = 0;
static uint32_t m_alarm_ticks = 0;

// Locking serial to one driver at a time
static serial_lock_t m_serial_lock;

// State of baseline
static uint32_t m_period_counter = 0;

// String for sending JSON data to the web
static String m_out,m_tcp_out;

// Wakeup tune
void wakeup_tune() {
  // Set pin mode
  pinMode(PIEZO_PIN, OUTPUT);

  // Actuate the piezo`
  for( int i = 2; i-- > 0; ) {
    analogWrite(PIEZO_PIN, 128, 2000);
    delay(50);
    analogWrite(PIEZO_PIN, 0, 2000);
    delay(50);
  }
}

// Wakeup tune
void alert_tune() {
  // Set pin mode
  pinMode(PIEZO_PIN, OUTPUT);

  // Actuate the piezo
  for( int i = 3; i-- > 0; ) {
    analogWrite(PIEZO_PIN, 128, 2000);
    delay(200);
    analogWrite(PIEZO_PIN, 0, 2000);
    delay(200);
  }

}


// Cellular timer - handles setting disconnect flag after
// We're done using a Particle.publish() command
// i.e. saves battery
void cellular_timer_handler() {
  m_disconnect = true;
}

// Definition of timer handler
void timer_handler() {
  data_check = true;
}

// This fires after the hpma should have finished...
void hpma_timeout_handler() {
  if( hpma115.is_enabled() ) {
    Serial.println("hpma timeout");
    #if BACKEND_ID == BACKEND_PARTICLE
    Particle.publish("err", "hpma timeout" , PRIVATE, NO_ACK);
    #endif
    hpma115.disable();
  }

  m_data_ready = true;
}

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

  // Disable HPMA
  #ifdef HAS_HPMA
  hpma115.disable();
  hpma_timer.stop();
  #endif

  // Copy the data.
  hpma115_data = *p_data;

  // Serial.printf("pm25 %dμg/m3 pm10 %dμg/m3\n", hpma115_data.pm25, hpma115_data.pm10);

  // Concat the data into the json blob
  m_out = String( m_out + String::format(",\"pm25\":%d,\"pm10\":%d", hpma115_data.pm25,hpma115_data.pm10) );

  // Set the data ready!
  m_data_ready = true;

  Serial.println("hpma rdy");
}

int set_reading_period( String period ) {

  uint32_t temp_period = (uint32_t)period.toInt();

  if( temp_period != m_reading_period && temp_period >= MIN_MEASUREMENT_DELAY_MS ) {
    Serial.printf("update reading period %d\n",temp_period);
    m_reading_period = temp_period;

    // Change period if variable is updated
    timer.changePeriod(m_reading_period);

    return 1;

  }

  return -1;

}

// Event for recieving motion alerts
void pir_event_handler(void) {
  m_pir_event = true;
}

// Event handler for GPS
void gps_event_handler(gps_data_t * p_data) {

  // Copy data over
  gps_data = *p_data;

  // Set flag to add to output data;
  m_has_location = true;

}

// setup() runs once, when the device is first turned on.
void setup() {

  // Turn off the LED. This app controls the LED.
  RGB.control(true);
  RGB.brightness(0);

  // Set up PC based UART (for debugging)
  Serial.blockOnOverrun(false);
  Serial.begin();

  // Set up I2C
  Wire.setSpeed(I2C_CLK_SPEED);
  Wire.begin();

  // Set up Si7021;
  si7021_data_last.humidity = 0;
  si7021_data_last.temperature = 0;

  uint32_t err_code = si7021.setup();
  if( err_code != 0 ) {
    Serial.printf("si7021 setup err %d\n", err_code);
    Serial.flush();
    m_error_flag = true;
  }

  // Setup CC8012
  #ifdef HAS_CCS811
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
    Serial.flush();
    m_error_flag = true;
  }

  // Update!
  const ccs811_app_update_t update {
    .ver = {
      .major = 2,
      .minor = 0,
      .trivial = 1
    },
    .data = CCS811_FW_App_v2_0_1_bin,
    .size = CCS811_FW_App_v2_0_1_bin_len
  };

    // Get the version and print it
  ccs811_app_ver_t version;
  ccs811.get_app_version(&version);

  Serial.printf("ccs811 ver %x.%d.%d\n", version.major, version.minor, version.trivial);

  // Checkfor updates
  err_code = ccs811.update_app(&update);
  if( err_code == CCS811_NO_UPDATE_NEEDED ) {
    Serial.printf("ccs811 no update needed\n");
    Serial.flush();
  } else if  ( err_code != 0 ) {
    Serial.printf("ccs811 update err %d\n", err_code);
    Serial.flush();
  }

  // Restore the baseline
  ccs811.restore_baseline();

  // Start VOC measurement
  // This is an async reading.
  err_code = ccs811.enable();
  if( err_code != 0 ) {
    Serial.printf("ccs811 enable err %d\n", err_code);
    Serial.flush();
    m_error_flag = true;
  }

  #endif

  #ifdef HAS_HPMA
  // Setup HPMA115
  hpma115_init_t hpma115_init;
  hpma115_init.callback = hpma_evt_handler;
  hpma115_init.enable_pin = HPMA1150_EN_PIN;
  hpma115_init.serial_lock = &m_serial_lock;

  // Init HPM115 sensor
  err_code = hpma115.setup(&hpma115_init,&m_serial_lock);
  if (err_code != HPMA115_SUCCESS) {
    Serial.printf("hpma115 enable err %d\n", err_code);
    Serial.flush();
    m_error_flag = true;
  }
  #endif

  // Set up GPS
  gps_init_t gps_init = {
    .enable_pin = GPS_EN_PIN,
    .fix_pin = GPS_FIX_PIN,
    .callback = gps_event_handler
  };

  err_code = gps.init(&gps_init,&m_serial_lock);
  if( err_code != GPS_SUCCESS ) {
    Serial.printf("gps init err %d\n", err_code);
    Serial.flush();
    m_error_flag = true;
  }

  // Enable the gps
  err_code = gps.enable();
  if( err_code != GPS_SUCCESS ) {
    Serial.printf("gps enable err %d\n", err_code);
    Serial.flush();
    m_error_flag = true;
  }

  // Set up PIR interrupt
  pinMode(PIR_INT_PIN, INPUT_PULLDOWN);
  attachInterrupt(PIR_INT_PIN, pir_event_handler, RISING);
  // TODO: wake the device up from sleep

  // Start the timer
  timer.start();

  // Set up cloud variable
  #if BACKEND_ID == BACKEND_PARTICLE
  Particle.function("set_period", set_reading_period);

  // Set up keep alive
  Particle.keepAlive(60);
  #endif

  // Publish vitals once on startup
  #if PLATFORM_ID == PLATFORM_BORON
  Cellular.setActiveSim(EXTERNAL_SIM);
  Cellular.setCredentials("soracom.io","sora","sora");
  #endif

  // Beep beep
  wakeup_tune();

  delay(2000);

}

// loop() runs over and over again, as quickly as it can execute.
void loop() {

  uint32_t err_code;

  // Connect if not connected..
  #if PLATFORM_ID == PLATFORM_XENON
  if (Mesh.ready() == false) {
    Serial.println("Not connected..");
    Mesh.connect();
  }
  #elif PLATFORM_ID == PLATFORM_ARGON
  // TODO: connect only when data is available
  if (Particle.connected() == false) {
    Particle.connect();
  }
  #elif PLATFORM_ID == PLATFORM_BORON
  // connect only when data is available
  if (m_data_ready && !Cellular.ready() && !Cellular.connecting()) {
    Cellular.on();
    Cellular.connect();
  }
  #endif

  // This gets run after cellular disconnect timer expires
  #if PLATFORM_ID == PLATFORM_BORON
  if( m_disconnect ) {
    m_disconnect = false;

    Serial.println("disconnect");

    #if BACKEND_ID == BACKEND_SORACOM
    client.stop();
    #endif

    // disconnect on success
    Cellular.disconnect();
    Cellular.off();
  }
    #endif

  // If all the data is ready, send it as one data blob
  // only publish when connected...
  if ( m_data_ready && Cellular.ready() ) {
    Serial.println("data send");

    // Cap off the JSON
    m_out = String( m_out + "}");

    // Publish data to Particle Cloud
    #if BACKEND_ID == BACKEND_PARTICLE
    Particle.publish("blob", m_out , PRIVATE, WITH_ACK);

    // Start disconnect timer
    #if PLATFORM_ID == PLATFORM_BORON
    disconnect_timer.start();
    #endif

    #elif BACKEND_ID == BACKEND_SORACOM

    // Copy string
    m_tcp_out = m_out;
    m_tcp_publish = true;

    #else
    #error BACKEND_ID needs to be defined.
    #endif

    // Reset to false
    m_data_ready = false;

  }

  // While we have a response read it
  #if BACKEND_ID == BACKEND_SORACOM

  if( m_tcp_publish ) {
    // Publish data to SORACOM
    // Note: this is not ideal as it's not encrypted.
    // But if using in conjunction with
    // an end to end VPN tunnel it's less of an issue..
    if( !client.status() ) {
      client.connect("harvest.soracom.io", 8514);
    } else {
      int bytes = client.print(m_out);
      int err = client.getWriteError();
      if (err != 0) {
        Log.trace("TCPClient::write() failed (error = %d), number of bytes written: %d", err, bytes);
      } else {
        m_tcp_publish = false;
      }
    }
  }

  // Once we have data
  // Flush in rx buf
  // and disconnect
  if (client.available())
  {
    while(client.available()) client.read();
    m_data_ready = false;
    m_disconnect = true;
  }
  #endif

  // If there is a PIR event handle that here
  if( m_pir_event && !m_led_motion_on ) {

    Serial.println("pir event");

    // get last pm25 measurement
    // and evaluate using the scale from here:
    // https://en.wikipedia.org/wiki/Air_quality_index#Europe
    if( hpma115_data.pm25 > PM25_HAZARDOUS ) {
      RGB.color(255,0,0);
    } else if ( hpma115_data.pm25 > PM25_HIGH ) {
      RGB.color(241,132,50);
    } else if ( hpma115_data.pm25 > PM25_MED ) {
      RGB.color(233,194,67);
    } else if ( hpma115_data.pm25 > PM25_LOW ) {
      RGB.color(183,197,93);
    } else {
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

  // Set audible alarm
  if ( hpma115_data.pm25 > PM25_HIGH ) {

    // Check reading every minute
    if( m_reading_period != MEASUREMENT_DELAY_ALERT_MS ) {
      m_reading_period = MEASUREMENT_DELAY_ALERT_MS;
      timer.changePeriod(m_reading_period);
    }

    // If we've overflowed reset
    if( millis() < m_alarm_ticks ) {
      m_alarm_ticks = 0;
    }

    //Every minute play alart tune
    if( millis() - m_alarm_ticks > ALERT_INTERVAL ) {
      m_alarm_ticks = millis();
      alert_tune();
    }

  } else {
    // Check reading every 5 min
    if( m_reading_period != MEASUREMENT_DELAY_MS ) {
      m_reading_period = MEASUREMENT_DELAY_MS;
      timer.changePeriod(m_reading_period);
    }
  }

  // If we're greater than or equal to the measurement delay
  // start taking measurements!
  if( data_check ) {

    // Set state variable to false
    data_check = false;

    // Set start of string.
    m_out = String("{");

    // Disable HPMA
    #ifdef HAS_HPMA
    hpma115.disable();
    #endif

    // Read temp and humiity
    err_code = si7021.read(&si7021_data);

    if( err_code == SI7021_SUCCESS ) {
      // Set env data in the CCS811
      #ifdef HAS_CCS811
      ccs811.set_env(si7021_data.temperature,si7021_data.humidity);
      #endif

      // Concatinate temp and humidity data
      m_out = String( m_out + String::format("\"temperature\":%.2f,\"humidity\":%.2f",si7021_data.temperature, si7021_data.humidity) );
      Serial.println("temp rdy");
    } else {
      #if BACKEND_ID == BACKEND_PARTICLE
      Particle.publish("err", "temp" , PRIVATE, NO_ACK);
      #endif
      Serial.println("temp err");
    }

    // Process CCS811
    #ifdef HAS_CCS811
    err_code = ccs811.read(&ccs811_data);

    if ( err_code == CCS811_SUCCESS ) {

      // Concatinate ccs811 tvoc
      m_out = String( m_out + String::format(",\"tvoc\":%d,\"c02\":%d", ccs811_data.tvoc, ccs811_data.c02) );
      Serial.println("tvoc rdy");
    } else if( err_code == CCS811_NO_DAT_AVAIL ) {
      Serial.println("fatal tvoc error");
      Serial.flush();
      m_error_flag = true;
    } else {
      #if BACKEND_ID == BACKEND_PARTICLE
      Particle.publish("err", "tvoc" , PRIVATE, NO_ACK);
      #endif
      Serial.println("tvoc err");
    }
    #endif

    // Add battery info
    m_out = String( m_out + String::format(",\"batt\":%f", fuel.getSoC()) );

    // Publish location
    if ( m_has_location ) {
      Serial.println("has location");
      int32_t lat_deg = gps_data.lat/10000000;
      int32_t long_deg = gps_data.lon/10000000;
      int32_t lat_min = gps_data.lat-(lat_deg*10000000);
      int32_t long_min = gps_data.lon-(long_deg*10000000);
      const char * long_char = (gps_data.lon_c == 'W') ? "-" : "";
      const char * lat_char = (gps_data.lat_c == 'N') ? "" : "-";
      m_out = String( m_out + String::format(",\"lat\":\"%s%i.%i\",\"lng\":\"%s%i.%i\"",lat_char,lat_deg,lat_min,long_char,long_deg,long_min) );
      // Serial.printf("%i %i\n", lat_deg,long_deg);
      // Serial.printf("%i %i\n", lat_min,long_min);
      m_has_location = false;
    }

    // Process PM2.5 and PM10 results
    // This is slightly different from the other readings
    // due to the fact that it should be shut off when not taking a reading
    // (extends the life of the device)
    #ifdef HAS_HPMA
    hpma115.enable();
    hpma_timer.start();
    #else
    m_data_ready = true;
    #endif

  }

  // Save the baseline if we're > 4hr
  uint32_t periods = System.uptime()/60/60/4;
  if( periods > m_period_counter) {

    //Update the counter
    m_period_counter = periods;

    #ifdef HAS_CCS811
    ccs811.save_baseline();
    #endif

  }

  // Process gps stuff
  gps.process();

  hpma115.process();

  // Send updates/communicate with service when connected
  #if BACKEND_ID == BACKEND_PARTICLE
  if( Particle.connected() ) {
    Particle.process();
  }
  #endif

  // Checking with WD -- if there's an error flag, no check in. That allows for a sufficent update window.
  if( !m_error_flag ) {
    wd.checkin();
  }

}