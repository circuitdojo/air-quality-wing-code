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

#ifdef HAS_SGP30
#include "sgp30.h"
#endif

#ifdef HAS_BME680
#include "bsec.h"
#endif

// Firmware update
#include "CCS811_FW_App_v2_0_1.h"

#if PLATFORM_ID != PLATFORM_XENON
SYSTEM_MODE(SEMI_AUTOMATIC);
#endif

// Watchdog timeout period
#define WATCHDOG_TIMEOUT_MS 120000

// Delay and timing related contsants
#define MEASUREMENT_DELAY_MS 120000
#define MIN_MEASUREMENT_DELAY_MS 10000
#define HPMA_TIMEOUT_MS 10000

// Timer handler
void timer_handler();

// Reading delay ms
static uint32_t m_reading_period = MEASUREMENT_DELAY_MS;

// Static objects
static Si7021  si7021 = Si7021();
static CCS811  ccs811 = CCS811();
static HPMA115 hpma115 = HPMA115();

#ifdef HAS_SGP30
static SGP30   sgp30 = SGP30();
#endif

#ifdef HAS_BME680
static Bsec    bsec = Bsec();
#endif

// Set up timer
Timer timer(m_reading_period, timer_handler);
Timer hpma_timer(HPMA_TIMEOUT_MS, hpma_timeout_handler, true);

#ifdef HAS_SGP30
Timer sgp30_timer(SGP30_READ_INTERVAL, sgp30_timer_handler);
#endif

// Watchdog
ApplicationWatchdog wd(WATCHDOG_TIMEOUT_MS, System.reset);

// Data
static si7021_data_t si7021_data, si7021_data_last;
static hpma115_data_t hpma115_data;

#ifdef HAS_CCS811
static ccs811_data_t ccs811_data;
#endif

#ifdef HAS_SGP30
static sgp30_data_t sgp30_data;
#endif

// Data check bool
static bool data_check = false;

// Error reset flag
static bool m_error_flag = false;

// Data state ready
static bool m_data_ready = false;
#ifdef HAS_BME680
static bool m_bme680_ready = false;
#endif

// State of baseline
static uint32_t m_period_counter = 0;

static String m_out;

// Definition of timer handler
#ifdef HAS_SGP30
void sgp30_timer_handler() {
  sgp30.set_ready();
}
#endif

// Definition of timer handler
void timer_handler() {
  data_check = true;
}

// This fires after the hpma should have finished...
void hpma_timeout_handler() {
  if( hpma115.is_enabled() ) {
    Serial.println("hpma timeout");
    Particle.publish("err", "hpma timeout" , PRIVATE, NO_ACK);
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

// Helper function definitions
#ifdef HAS_BME680
void checkIaqSensorStatus(void)
{

  String output;

  if (bsec.status != BSEC_OK) {
    if (bsec.status < BSEC_OK) {
      output = "BSEC error code :" + String(bsec.status);
      Serial.println(output);
      Serial.flush();
    } else {
      output = "BSEC warning code : " + String(bsec.status);
      Serial.println(output);
      Serial.flush();
    }
  }

  if (bsec.bme680Status != BME680_OK) {
    if (bsec.bme680Status < BME680_OK) {
      output = "BME680 error code : " + String(bsec.bme680Status);
      Serial.println(output);
      Serial.flush();
      m_error_flag = true;
    } else {
      output = "BME680 warning code : " + String(bsec.bme680Status);
      Serial.println(output);
      Serial.flush();
    }
  }
}
#endif

// setup() runs once, when the device is first turned on.
void setup() {

  // Turn off the LED
  // RGB.control(true);
  // RGB.brightness(0);

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

  // SGP30 setup
  #ifdef HAS_SGP30
  err_code = sgp30.setup();
  if( err_code != SGP30_SUCCESS ) {
    Serial.printf("sgp30 setup err %d\n", err_code);
    Serial.flush();
    m_error_flag = true;
  }

  // Restore the baseline
  sgp30.restore_baseline();

  // Start the readings every 1 sec
  sgp30_timer.start();
  #endif

  #ifdef HAS_HPMA
  // Setup HPMA115
  hpma115_init_t hpma115_init;
  hpma115_init.callback = hpma_evt_handler;
  hpma115_init.enable_pin = HPMA1150_EN_PIN;

  // Init HPM115 sensor
  err_code = hpma115.setup(&hpma115_init);
  if (err_code != HPMA115_SUCCESS) {
    Serial.printf("hpma115 enable err %d\n", err_code);
    Serial.flush();
    m_error_flag = true;
  }
  #endif

  // Begin BME680 work
  #ifdef HAS_BME680
  bsec.begin(BME680_I2C_ADDR_PRIMARY, Wire);
  checkIaqSensorStatus();

  // Set up BME680 sensors
  bsec_virtual_sensor_t sensorList[7] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY
  };

  bsec.updateSubscription(sensorList, 7, BSEC_SAMPLE_RATE_LP); //BSEC_SAMPLE_RATE_LP
  checkIaqSensorStatus();
  #endif

  // Start the timer
  timer.start();

  // Set up cloud variable
  Particle.function("set_period", set_reading_period);

  // Set up keep alive
  Particle.keepAlive(60);

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
  #else
  if (Particle.connected() == false) {
    Particle.connect();
  }
  #endif

  // If all the data is ready, send it as one data blob
  if ( m_data_ready ) {
    Serial.println("data send");

    // Cap off the JSON
    m_out = String( m_out + "}");

    // Publish data
    Particle.publish("blob", m_out , PRIVATE, WITH_ACK);

    // Reset to false
    m_data_ready = false;
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

      // Set the env data for the SGP30
      #ifdef HAS_SGP30
      sgp30.set_env(si7021_data.temperature,si7021_data.humidity);
      #endif

      // Concatinate temp and humidity data
      m_out = String( m_out + String::format("\"temperature\":%.2f,\"humidity\":%.2f",si7021_data.temperature, si7021_data.humidity) );
      Serial.println("temp rdy");
    } else {
      Particle.publish("err", "temp" , PRIVATE, NO_ACK);
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
      Particle.publish("err", "tvoc" , PRIVATE, NO_ACK);
      Serial.println("tvoc err");
    }
    #endif

    #ifdef HAS_SGP30
    err_code = sgp30.read(&sgp30_data);

    if ( err_code == SGP30_SUCCESS ) {

      // concatinate sgp30 data
      m_out = String( m_out + String::format(",\"sgp30_tvoc\":%d,\"sgp30_c02\":%d", sgp30_data.tvoc, sgp30_data.c02) );
      Serial.println("sgp30 rdy");
    } else {
      Particle.publish("err", "sgp30" , PRIVATE, NO_ACK);
      Serial.println("sgp30 err");
    }
    #endif

    // bme680 data publish!
    #ifdef HAS_BME680
    if (m_bme680_ready) {
      m_bme680_ready = false;
      Serial.println("bme680 rdy");
      m_out = String( m_out + String::format(",\"bme680_temp\":%.2f,\"bme680_pres\":%.2f", bsec.rawTemperature, bsec.pressure/100.0f) );
      m_out = String( m_out + String::format(",\"bme680_hum\":%.2f,\"bme680_iaq\":%.2f", bsec.rawHumidity, bsec.iaqEstimate) );
      m_out = String( m_out + String::format(",\"bme680_temp_calc\":%.2f,\"bme680_hum_calc\":%.2f", bsec.temperature, bsec.humidity) );
    } else {
      Serial.println("bme680 err");
    }
    #endif

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

    #ifdef HAS_SGP30
    sgp30.save_baseline();
    #endif
  }

  #ifdef HAS_SGP30
  // Processes any avilable serial data
  err_code = sgp30.process();

  if( err_code != SGP30_SUCCESS ) {
    Serial.println("sp30 process error");
  }
  #endif

  hpma115.process();

  // Proces BME680
  #ifdef HAS_BME680
  if (bsec.run()) { // If new data is available
    m_bme680_ready = true;
  } else {
    checkIaqSensorStatus();
  }
  #endif

  // Send updates/communicate with service when connected
  if( Particle.connected() ) {
    Particle.process();
  }

  // Checking with WD -- if there's an error flag, no check in. That allows for a sufficent update window.
  if( !m_error_flag ) {
    wd.checkin();
  }

}