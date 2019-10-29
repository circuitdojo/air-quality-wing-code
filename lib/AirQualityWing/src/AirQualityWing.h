#ifndef AIR_QUALITY_H
#define AIR_QUALITY_H

#include "Particle.h"

#if (PLATFORM_ID != PLATFORM_XENON) && (PLATFORM_ID != PLATFORM_ARGON) && (PLATFORM_ID != PLATFORM_BORON)
#error The Air Quality Wing Library only supports Xenon, Argon and Boron.
#endif

#include "si7021.h"
#include "ccs811.h"
#include "hpma115.h"
#include "stdbool.h"

// #ifdef HAS_SGP30
// #include "sgp30.h"
// #endif

// #ifdef HAS_BME680
// #include "bsec.h"
// #endif

// Delay and timing related contsants
#define MEASUREMENT_DELAY_S 120
#define MEASUREMENT_DELAY_MS (MEASUREMENT_DELAY_S * 1000)
#define MIN_MEASUREMENT_DELAY_MS 10000
#define HPMA_TIMEOUT_MS 10000

typedef enum {
  success = 0,
  hpma115_error,
  ccs811_error,
  si7021_error
} AirQualityWingError_t;

// Structure for holding data.
typedef struct {
  struct {
    bool hasData;
    ccs811_data_t data;
  } ccs811;
  struct {
    bool hasData;
    si7021_data_t data;
  } si7021;
  struct {
    bool hasData;
    hpma115_data_t data;
  } hpma115;
} AirQualityWingData_t;

typedef struct {
  uint32_t interval;
  bool hasHPMA115;
  bool hasCCS811;
  bool hasSi7021;
  uint8_t ccs811Address;
  uint8_t ccs811IntPin;
  uint8_t ccs811RstPin;
  uint8_t ccs811WakePin;
  uint8_t hpma115IntPin;
} AirQualityWingSettings_t;

// Handler defintion
typedef std::function<void()> AirQualityWingHandler_t;

// Air quality class. Only create one of these!
class AirQualityWing
{
private:
  // Private data used to store latest values
  AirQualityWingHandler_t handler_;
  AirQualityWingSettings_t settings_;

  // Sensor objects
  Si7021  si7021;
  CCS811  ccs811;
  HPMA115 hpma115;

  void ccs811Event();

  // #ifdef HAS_SGP30
  // static SGP30   sgp30 = SGP30();
  // #endif

  // #ifdef HAS_BME680
  // static Bsec    bsec = Bsec();
  // #endif

  // Variables
  // TODO: init these guys
  Timer *measurementTimer;
  Timer *hpmaTimer;

  // Static measurement timer event function
  void hpmaEvent();
  void measureTimerEvent();
  void hpmaTimerEvent();

  // Static var
  bool measurementStart;
  bool measurementComplete;
  bool hpmaMeasurementComplete;
  bool hpmaError;

  // Data
  AirQualityWingData_t data;

  // #ifdef HAS_SGP30
  // Timer sgp30_timer(SGP30_READ_INTERVAL, sgp30_timer_handler);
  // #endif
public:

  // Using defaults
  AirQualityWing();

  // Inits the devices
  AirQualityWingError_t setup(AirQualityWingHandler_t handler, AirQualityWingSettings_t settings);

  // Begins data collection
  AirQualityWingError_t begin();

  // Stops data collection, de-inits
  void end();

  // Prints out a string representation in JSON of the data
  String toString();

  // Returns a copy of the full data structure
  AirQualityWingData_t getData();

  // Attaches event handler. Event handler fires when a round of data has completed successfully
  void attachHandler(AirQualityWingHandler_t handler);

  // Deattaches event handler. The only way to fetch new data is using the `data()` method
  void deattachHandler();

  // Process method is required to process data correctly. Place in `loop()` function
  AirQualityWingError_t process();

  // Set measurement interval.
  // Accepts intervals from 20 seconds
  void setInterval(uint32_t interval);
};

#endif