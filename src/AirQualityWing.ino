/*
 * Project Air Quality Wing Library Example
 * Description: Basic example using the Air Quality Wing for Particle Mesh
 * Author: Jared Wolff (Circuit Dojo LLC)
 * Date: 10/27/2019
 * License: GNU GPLv3
 */

#include "AirQualityWing.h"
#include "board.h"

// SYSTEM_MODE(SEMI_AUTOMATIC);

// Logger settings
SerialLogHandler logHandler(115200,
                            LOG_LEVEL_WARN,
                            {
                                {"app", LOG_LEVEL_INFO},
                                {"sgp40", LOG_LEVEL_INFO},
                                {"shtc3", LOG_LEVEL_WARN},
                            });

// AirQualityWing object
AirQualityWing AirQual = AirQualityWing();

// Handler is called in main loop.
// Ok to run Particle.Publish
static void AirQualityWingEvent()
{
  Log.info(AirQual.toString());

  Particle.publish(AirQual.toString());
}

// setup() runs once, when the device is first turned on.
void setup()
{

  // Set up PC based UART (for debugging)
  Serial.blockOnOverrun(false);
  Serial.begin();

  while (!Serial.isConnected())
  {
    delay(100);
  }

  // Set up I2C
  Wire.setSpeed(I2C_CLK_SPEED);
  Wire.begin();

  // Default settings
  AirQualityWingSettings_t defaultSettings =
      {
          10000,          //Measurement Interval
          true,           //Has HPMA115 (PM2.5)
          true,           //Has SGP40 (VOC)
          true,           //Has SHTC3 (Temp/humidity)
          HPMA1150_EN_PIN //HPMA int pin
      };

  // Setup & Begin Air Quality
  AirQual.setup(AirQualityWingEvent, defaultSettings);
  AirQual.begin();

  // Set up keep alive
  Particle.keepAlive(60);

  // Startup message
  Serial.println("Air Quality Wing");
}

// loop() runs over and over again, as quickly as it can execute.
void loop()
{

  uint32_t err_code = AirQual.process();
  if (err_code != success)
  {

    switch (err_code)
    {
    case shtc3_error:
      Particle.publish("err", "shtc3", PRIVATE, NO_ACK);
      Log.error("Error shtc3");
    case hpma115_error:
      Particle.publish("err", "hpma115", PRIVATE, NO_ACK);
      Log.error("Error hpma115");
    case sgp40_error:
      Particle.publish("err", "sgp40", PRIVATE, NO_ACK);
      Log.error("Error sgp40");
    default:
      break;
    }
  }

  System.sleep(SLEEP_MODE_DEEP, 1);
}