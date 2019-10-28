#include "AirQualityWing.h"

// Firmware update
#include "CCS811_FW_App_v2_0_1.h"

// Constructor
AirQualityWing::AirQualityWing() {}

// This fires after the hpma should have finished...
void AirQualityWing::hpmaTimerEvent() {
  // Set ready flag if we time out
  this->measurementComplete = true;
  this->hpmaError = true;
}

// Async publish event
void AirQualityWing::hpmaEvent() {

  this->hpmaMeasurementComplete = true;

  // // Disable HPMA
  // #ifdef HAS_HPMA
  // hpma115.disable();
  // hpma_timer.stop();
  // #endif

  // // Copy the data.
  // data.hpma115.data = *p_data;

  // // Set the data ready!
  // measurementComplete = true;

  // Log.trace("pm25 %dμg/m3 pm10 %dμg/m3\n", data.hpma115.data.pm25, data.hpma115.data.pm10);
}

// // Helper function definitions
// #ifdef HAS_BME680
// void checkIaqSensorStatus(void)
// {

//   String output;

//   if (bsec.status != BSEC_OK) {
//     if (bsec.status < BSEC_OK) {
//       output = "BSEC error code :" + String(bsec.status);
//       Log.trace(output);
//       Log.flush();
//     } else {
//       output = "BSEC warning code : " + String(bsec.status);
//       Log.trace(output);
//       Log.flush();
//     }
//   }

//   if (bsec.bme680Status != BME680_OK) {
//     if (bsec.bme680Status < BME680_OK) {
//       output = "BME680 error code : " + String(bsec.bme680Status);
//       Log.trace(output);
//       Log.flush();
//       m_error_flag = true;
//     } else {
//       output = "BME680 warning code : " + String(bsec.bme680Status);
//       Log.trace(output);
//       Log.flush();
//     }
//   }
// }
// #endif


// // Definition of timer handler
// #ifdef HAS_SGP30
// void sgp30_timer_handler() {
//   sgp30.set_ready();
// }
// #endif

// Measurement timer handler
void AirQualityWing::measureTimerEvent() {
  this->measurementStart = true;
}

AirQualityWingError_t AirQualityWing::setup(AirQualityWingHandler_t handler, AirQualityWingSettings_t settings)
{

  uint32_t err_code = success;

  // Set the handler
  this->handler_ = handler;

  // Set the settings
  this->settings_ = settings;

  // Create timers
  this->measurementTimer = new Timer(this->settings_.interval, [this](void)->void{return measureTimerEvent();});
  this->hpmaTimer = new Timer(HPMA_TIMEOUT_MS, [this](void)->void{return hpmaTimerEvent();}, true); // One shot enabled.

  // Reset variables
  this->measurementStart = false;
  this->measurementComplete = false;
  this->hpmaMeasurementComplete = false;
  this->hpmaError = false;

  // // SGP30 setup
  // #ifdef HAS_SGP30
  // err_code = sgp30.setup();
  // if( err_code != SGP30_SUCCESS ) {
  //   Log.trace("sgp30 setup err %d\n", err_code);
  //   Log.flush();
  //   m_error_flag = true;
  // }

  // // Restore the baseline
  // sgp30.restore_baseline();

  // // Start the readings every 1 sec
  // sgp30_timer.start();
  // #endif

  // // Begin BME680 work
  // #ifdef HAS_BME680
  // bsec.begin(BME680_I2C_ADDR_PRIMARY, Wire);
  // checkIaqSensorStatus();

  // // Set up BME680 sensors
  // bsec_virtual_sensor_t sensorList[7] = {
  //   BSEC_OUTPUT_RAW_TEMPERATURE,
  //   BSEC_OUTPUT_RAW_PRESSURE,
  //   BSEC_OUTPUT_RAW_HUMIDITY,
  //   BSEC_OUTPUT_RAW_GAS,
  //   BSEC_OUTPUT_IAQ,
  //   BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
  //   BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY
  // };

  // bsec.updateSubscription(sensorList, 7, BSEC_SAMPLE_RATE_LP); //BSEC_SAMPLE_RATE_LP
  // checkIaqSensorStatus();
  // #endif

  // Has the Si7021 init it
  if( this->settings_.hasSi7021 ) {
    // Init Si7021
    uint32_t err_code = si7021.setup();
    if( err_code != success ) return si7021_error;
  }


  // Has the CCS811 init it
  if( this->settings_.hasCCS811 ) {
    // Setup CC8012
    ccs811_init_t ccs811_init = {
      this->settings_.ccs811Address,
      this->settings_.ccs811IntPin,
      this->settings_.ccs811RstPin,
      this->settings_.ccs811WakePin
    };

    // Init the TVOC & C02 sensor
    err_code = ccs811.setup(&ccs811_init);
    if( err_code != success ) return ccs811_error;

    // Check for CCS811 updates
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

    Log.trace("ccs811 ver %x.%d.%d\n", version.major, version.minor, version.trivial); // TODO: logger

    // Compare and update if need be
    // TODO: relocate this?
    err_code = ccs811.update_app(&update);
    if( err_code == CCS811_NO_UPDATE_NEEDED ) {
      Log.trace("ccs811 no update needed\n");
    } else if  ( err_code != 0 ) {
      Log.trace("ccs811 update err %d\n", (int)err_code);
    }

    // Restore the baseline
    ccs811.restore_baseline();
  }

  // Has the HPMA115 init it
  if( this->settings_.hasHPMA115 ) {
    // Setup
    hpma115_init_t hpma115_init = {
      [this](void)->void{return hpmaEvent();},
      this->settings_.hpma115IntPin
    };

    // Init HPM115 sensor
    err_code = hpma115.setup(&hpma115_init);
    if (err_code != HPMA115_SUCCESS) {
      Log.trace("hpma115 enable err %d\n", (int)err_code);
      return hpma115_error;
    }
  }

  return success;
}

AirQualityWingError_t AirQualityWing::begin() {

  uint32_t err_code = success;

  // Start the timer
  this->measurementTimer->start();

  // Start VOC measurement
  // This is an async reading.
  if( this->settings_.hasCCS811 ) {
    err_code = ccs811.enable();
    if( err_code != 0 ) {
      Log.trace("ccs811 enable err %d\n", (int)err_code);
      return ccs811_error;
    }
  }

  // Start measurement
  this->measurementStart = true;

  return success;
}

void AirQualityWing::end() {

}

String AirQualityWing::toString() {

  String out = "{";

  // If we have CCS811 data, concat
  if( this->data.hpma115.hasData ) {
    out = String( out + String::format("\"pm25\":%d,\"pm10\":%d", this->data.hpma115.data.pm25,this->data.hpma115.data.pm10) );
  }

  // If we have Si7021 data, concat
  // TODO: fix this
  if( this->data.si7021.hasData ) {

    // Add comma
    if( this->data.hpma115.hasData ) {
      out = String( out + ",");
    }

    out = String( out + String::format("\"temperature\":%.2f,\"humidity\":%.2f",this->data.si7021.data.temperature, this->data.si7021.data.humidity) );
  }

  // If we have HPMA data, concat
  if( this->data.ccs811.hasData ) {

    // Add comma
    // TODO: fix this
    if( this->data.hpma115.hasData || this->data.si7021.hasData) {
      out = String( out + ",");
    }

    out = String( out + String::format("\"tvoc\":%d,\"c02\":%d",this->data.ccs811.data.tvoc, this->data.ccs811.data.c02) );
  }

  return String( out + "}" );

}

AirQualityWingData_t AirQualityWing::getData() {
  return this->data;
}

void AirQualityWing::attachHandler( AirQualityWingHandler_t handler ) {
  this->handler_ = handler;
}

void AirQualityWing::deattachHandler() {
  this->handler_ = nullptr;
}

AirQualityWingError_t AirQualityWing::process() {

  uint32_t err_code = success;

  // Set flag if measurement is done!
  if( this->hpmaMeasurementComplete == true ) {
    // Reset
    this->hpmaMeasurementComplete = false;

    Log.trace("hpma complete");

    // Disable hpma
    this->hpma115.disable();

    // Copy data
    this->data.hpma115.data = this->hpma115.getData();
    this->data.hpma115.hasData = true;

    // Set available flag
    this->measurementComplete = true;
  }

  // If we're greater than or equal to the measurement delay
  // start taking measurements!
  if( this->measurementStart ) {

    Log.trace("measurement start");

    // Set state variable to false
    this->measurementStart = false;

    // Reset has data variables
    this->data.si7021.hasData = false;
    this->data.hpma115.hasData = false;
    this->data.ccs811.hasData = false;

    // Disable HPMA
    if( this->settings_.hasHPMA115 ) hpma115.disable();

    if( this->settings_.hasSi7021 ) {
      // Read temp and humiity
      err_code = si7021.read(&this->data.si7021.data);

      if( err_code == SI7021_SUCCESS ) {
        // Set has data flag
        this->data.si7021.hasData = true;

        // Set env data in the CCS811
        if( this->settings_.hasCCS811 ) {
          ccs811.set_env(this->data.si7021.data.temperature,this->data.si7021.data.humidity);
        }

        // // Set the env data for the SGP30
        // #ifdef HAS_SGP30
        // sgp30.set_env(si7021_data.temperature,si7021_data.humidity);
        // #endif
      } else {
        Log.error("Error temp - fatal err"); //TODO: logger
        return si7021_error;
      }
    }

    // Process CCS811
    if( this->settings_.hasCCS811 ) {
      err_code = ccs811.read(&this->data.ccs811.data);

      if( err_code == CCS811_SUCCESS) {
        // Set has data flag
        this->data.ccs811.hasData = true;
      } else if( err_code == CCS811_NO_DAT_AVAIL ) {
        Log.warn("Error tvoc - no data");
      } else {
        Log.error("Error tvoc - fatal");
        // return ccs811_error;
      }
    }

    // #ifdef HAS_SGP30
    // err_code = sgp30.read(&sgp30_data);

    // if ( err_code == SGP30_SUCCESS ) {

    //   // concatinate sgp30 data
    //   m_out = String( m_out + String::format(",\"sgp30_tvoc\":%d,\"sgp30_c02\":%d", sgp30_data.tvoc, sgp30_data.c02) );
    //   Log.trace("sgp30 rdy");
    // } else {
    //   Particle.publish("err", "sgp30" , PRIVATE, NO_ACK);
    //   Log.trace("sgp30 err");
    // }
    // #endif

    // // bme680 data publish!
    // #ifdef HAS_BME680
    // if (m_bme680_ready) {
    //   m_bme680_ready = false;
    //   Log.trace("bme680 rdy");
    //   m_out = String( m_out + String::format(",\"bme680_temp\":%.2f,\"bme680_pres\":%.2f", bsec.rawTemperature, bsec.pressure/100.0f) );
    //   m_out = String( m_out + String::format(",\"bme680_hum\":%.2f,\"bme680_iaq\":%.2f", bsec.rawHumidity, bsec.iaqEstimate) );
    //   m_out = String( m_out + String::format(",\"bme680_temp_calc\":%.2f,\"bme680_hum_calc\":%.2f", bsec.temperature, bsec.humidity) );
    // } else {
    //   Log.trace("bme680 err");
    // }
    // #endif

    // Process PM2.5 and PM10 results
    // This is slightly different from the other readings
    // due to the fact that it should be shut off when not taking a reading
    // (extends the life of the device)
    if( this->settings_.hasHPMA115 ) {
      this->hpma115.enable();
      this->hpmaTimer->start();
    } else {
      this->measurementComplete = true;
    }

    return success;

  }

  // // Save the baseline if we're > 4hr
  // uint32_t periods = System.uptime()/60/60/4;
  // if( periods > m_period_counter) {

  //   //Update the counter
  //   m_period_counter = periods;

  //   #ifdef HAS_CCS811
  //   ccs811.save_baseline();
  //   #endif

  //   #ifdef HAS_SGP30
  //   sgp30.save_baseline();
  //   #endif
  // }

  // #ifdef HAS_SGP30
  // // Processes any avilable serial data
  // err_code = sgp30.process();

  // if( err_code != SGP30_SUCCESS ) {
  //   Log.trace("sp30 process error");
  // }
  // #endif

  // Only run process command if this setup has HPMA115
  if( this->settings_.hasHPMA115 ) hpma115.process();

  // // Proces BME680
  // #ifdef HAS_BME680
  // if (bsec.run()) { // If new data is available
  //   m_bme680_ready = true;
  // } else {
  //   checkIaqSensorStatus();
  // }
  // #endif

  // Send event if complete
  if ( this->measurementComplete ) {

    Log.trace("measurement complete");

    // Set flag to false
    this->measurementComplete = false;

    // Only handle if we have an hpma
    if( this->settings_.hasHPMA115 ) {
      // Stop timer
      this->hpmaTimer->stop();

      //If error show it here
      if( this->hpmaError ) {
        Log.error("hpma timeout");

        // Disable on error
        this->hpma115.disable();

        // Reset error flag
        this->hpmaError = false;

        // Return error
        return hpma115_error;
      }
    }


    // Call handler
    if( this->handler_ != nullptr ) this->handler_();

  }

  return success;

}

void AirQualityWing::setInterval(uint32_t interval) {

  if( interval >= MIN_MEASUREMENT_DELAY_MS ) {

    // Set the interval
    this->settings_.interval = interval;

    Log.trace("update reading period %d\n", (int)interval);

    // Change period if variable is updated
    this->measurementTimer->changePeriod(interval);

  }

  // TODO: Update the timer

}