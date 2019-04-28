#include <EEPROM.h>
#include "bsec.h"
#include "bsec_serialized_configurations_iaq.h"
#include "logos.h"
#include "Adafruit_NeoPixel.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <gfxfont.h>

#define STATE_SAVE_PERIOD	UINT32_C(360 * 60 * 1000) // 360 minutes - 4 times a day
#define SCROLL_DUR  6000 // Milliseconds
#define SCROLL_LOCK_BUTTON  2
#define PIXEL_PIN   13
#define NUMPIXELS   2
#define HEATING_RATE  (0.3f)   // °C/min
#define HEATING_MAX   (3.0f)   // °C

// Helper functions declarations
void checkIaqSensorStatus(void);
void errLeds(void);
void loadState(void);
void updateState(void);
void displayTask(uint32_t period);
void compensateBoardHeat(void);

// Create a Bsec object
Bsec iaqSensor;
uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};
uint16_t stateUpdateCounter = 0;

// Create a Neopixel object
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIXEL_PIN, NEO_GRBW + NEO_KHZ400);

// Create an SSD1306 display object
Adafruit_SSD1306 display;

// Variables for display management
uint8_t boschByteArray[128];
uint8_t tempByteArray[128];
uint8_t presByteArray[128];
uint8_t humByteArray[128];
uint8_t gasByteArray[128];
uint8_t infoIndex = 0;
int brightness = 50;
int fadeAmount = 2;
uint32_t lastColorTime = 0;
uint32_t lastOledTime = 0;
volatile bool scroll = true;
volatile bool scrollPrint = false;

// Variable for heat compensation
bool heatCompensated = false;

// Create a String object for printing debug info to the terminal
String output;
String fwVersion;

// Entry point for the example
void setup(void)
{
  EEPROM.begin(BSEC_MAX_STATE_BLOB_SIZE + 1); // 1st address for the length
  Serial.begin(115200);

  // Initialize the RGB LEDs aka NeoPixels
  pixels.begin();
  pixels.clear();

  // Decode logos from uint32_t x 32 (32x32) to uint8_t x 128
  decodeLogo(boschByteArray, logo_bosch);
  decodeLogo(tempByteArray, logo_temp);
  decodeLogo(presByteArray, logo_pres);
  decodeLogo(humByteArray, logo_hum);
  decodeLogo(gasByteArray, logo_gas);

  // Initialize the OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();
  display.display();
  showInfo(infoIndex);
  attachInterrupt(SCROLL_LOCK_BUTTON, scrollToggle, FALLING);

  // Initialize the sensor interface, BSEC library, retrieve version and print it. In case of error, print error
  initI2CInterface(&Wire);
  iaqSensor.begin(BME680_I2C_ADDR_PRIMARY, BME680_I2C_INTF, i2cRead, i2cWrite, displayTask);
  fwVersion = "BSEC v" + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
  Serial.println(fwVersion);
  checkIaqSensorStatus();

  // Set config
  iaqSensor.setConfig((uint8_t *)bsec_config_iaq);
  checkIaqSensorStatus();

  loadState();

  bsec_virtual_sensor_t sensorList[7] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  iaqSensor.updateSubscription(sensorList, 7, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();

  // Print the header
  output = "Timestamp [ms], raw temperature [°C], pressure [hPa], raw relative humidity [%], gas [Ohm], IAQ, IAQ accuracy, temperature [°C], relative humidity [%]";
  Serial.println(output);
}

// Function that is looped forever
void loop(void)
{
  unsigned long time_trigger = millis();
  if (iaqSensor.run()) {
    output = String(time_trigger);
    output += ", " + String(iaqSensor.rawTemperature);
    output += ", " + String(iaqSensor.pressure);
    output += ", " + String(iaqSensor.rawHumidity);
    output += ", " + String(iaqSensor.gasResistance);
    output += ", " + String(iaqSensor.iaqEstimate);
    output += ", " + String(iaqSensor.iaqAccuracy);
    output += ", " + String(iaqSensor.temperature);
    output += ", " + String(iaqSensor.humidity);
    Serial.println(output);
    updateState();
  } else {
      checkIaqSensorStatus();
  }
  displayTask(0);
  if (!heatCompensated)
    compensateBoardHeat();
  delay(1); // For stability
}

// Check the return values of the BSEC & BME680 functions
void checkIaqSensorStatus(void)
{
  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BSEC warning code : " + String(iaqSensor.status);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      output = "BME680 error code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BME680 warning code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
    }
  }
}

// Function to blink the LEDs in case of a detected crash
void errLeds(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}

// Function to load the state from EEPROM
void loadState(void)
{
  if (EEPROM.read(0) == BSEC_MAX_STATE_BLOB_SIZE) {
    // Existing state in EEPROM
    Serial.println("Reading state from EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
      bsecState[i] = EEPROM.read(i + 1);
      Serial.println(bsecState[i], HEX);
    }

    iaqSensor.setState(bsecState);
    checkIaqSensorStatus();
  } else {
    // Erase the EEPROM with zeroes
    Serial.println("Erasing EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE + 1; i++)
      EEPROM.write(i, 0);

    EEPROM.commit();
  }
}

// Function to update the BSEC state to EEPROM
void updateState(void)
{
  bool update = false;
  if (stateUpdateCounter == 0) {
    /* First state update when IAQ accuracy is >= 3 */
    if (iaqSensor.iaqAccuracy >= 3) {
      update = true;
      stateUpdateCounter++;
    }
  } else {
    /* Update every STATE_SAVE_PERIOD minutes */
    if ((stateUpdateCounter * STATE_SAVE_PERIOD) < millis()) {
      update = true;
      stateUpdateCounter++;
    }
  }

  if (update) {
    iaqSensor.getState(bsecState);
    checkIaqSensorStatus();

    Serial.println("Writing state to EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE ; i++) {
      EEPROM.write(i + 1, bsecState[i]);
      Serial.println(bsecState[i], HEX);
    }

    EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
    EEPROM.commit();
  }
}

// ISR callback for the scroll lock button
void scrollToggle(void)
{
  scroll = !scroll;
  scrollPrint = true;
}

// Function to show information on the screen
void showInfo(uint8_t order)
{
  switch (order) {
    case 0: // Bosch
      display.clearDisplay();
      display.drawBitmap(0, 0, boschByteArray, 32, 32, WHITE);
      if (!scroll)
        display.drawBitmap(112, 16, bmp_lock, 16, 16, WHITE);
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(40, 10);
      display.println("BOSCH");
      display.setTextSize(1);
      display.setCursor(40, 0);
      display.println(fwVersion);
      display.display();
      break;

    case 1: // Temp
      display.clearDisplay();
      display.drawBitmap(0, 0, tempByteArray, 32, 32, WHITE);
      if (!scroll)
        display.drawBitmap(112, 16, bmp_lock, 16, 16, WHITE);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(40, 0);
      display.println("Temp., degC");
      display.setTextSize(2);
      display.setCursor(40, 10);
      display.println(iaqSensor.temperature);
      display.display();
      break;

    case 2: // Pres
      display.clearDisplay();
      display.drawBitmap(0, 0, presByteArray, 32, 32, WHITE);
      if (!scroll)
        display.drawBitmap(112, 16, bmp_lock, 16, 16, WHITE);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(40, 0);
      display.println("Pressure, hPa");
      display.setTextSize(2);
      display.setCursor(40, 10);
      display.println(iaqSensor.pressure / 100.0);
      display.display();
      break;

    case 3: // Hum
      display.clearDisplay();
      display.drawBitmap(0, 0, humByteArray, 32, 32, WHITE);
      if (!scroll)
        display.drawBitmap(112, 16, bmp_lock, 16, 16, WHITE);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(40, 0);
      display.println("Humidity, %rH");
      display.setTextSize(2);
      display.setCursor(40, 10);
      display.println(iaqSensor.humidity);
      display.display();
      break;

    case 4: // IAQ
      display.clearDisplay();
      display.drawBitmap(0, 0, gasByteArray, 32, 32, WHITE);
      if (!scroll)
        display.drawBitmap(112, 16, bmp_lock, 16, 16, WHITE);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(40, 0);
      display.print("IAQ, Acc: ");
      display.print(iaqSensor.iaqAccuracy);
      //if (iaqAcc != 0) {
      display.setTextSize(2);
      display.setCursor(40, 10);
      display.println(iaqSensor.iaqEstimate);
      //} else {
      //  display.setTextSize(1);
      //  display.setCursor(40, 10);
      //  display.println("Calibrating");
      //}
      display.display();
      break;
  }
}

// Polled task to display information
void displayTask(uint32_t period)
{
  uint32_t entryTime = millis();
  if (period != 0) {
    Serial.print("displayTask ");
    Serial.println(period);
  }

  if ((millis() - lastColorTime) > 30) { //33 Hz refresh rate in idle
    lastColorTime = millis();
    brightness = brightness + fadeAmount;
    if (brightness <= 20 || brightness >= 100) {
      fadeAmount = -fadeAmount;
    }
    flash_led_iaq(brightness); //Enable when IAQ starts to work
  }

  if ((millis() - lastOledTime) > SCROLL_DUR) {
    lastOledTime = millis();
    if (scroll) {
      infoIndex++;
      infoIndex = infoIndex % 5;
    }
    showInfo(infoIndex);
  }

  if (scrollPrint) {
    showInfo(infoIndex);
    scrollPrint = false;
  }

  uint32_t exitTime = millis();

  if (period != 0)
    delay(period - (exitTime - entryTime));
}

// Flash Neopixel LEDs for IAQ indication
void flash_led_iaq(float lum)
{
  float iaq = iaqSensor.iaqEstimate, red = 0, green = 0, blue = 0;
  int scale = 1;
  if (iaq < 25) { // RGB=(0,255,0)
    red = 0.0;
    green = 255.0;
    blue = 0.0;
    if (fadeAmount < 0)
      fadeAmount = -2 * scale;
    else
      fadeAmount = 2 * scale;
  } else if ((iaq >= 25) && (iaq < 75)) { // RGB=(127-255,255-127,0)
    red = 127.0 + ((iaq - 25.0) * 127.0 / 50.0);
    green = 255.0 - ((iaq - 25.0) * 127.0 / 50.0);
    blue = 0.0;
    if (fadeAmount < 0)
      fadeAmount = -3 * scale;
    else
      fadeAmount = 3 * scale;
  } else if ((iaq >= 75) && (iaq < 175)) { // RGB=(255,127-0,0)
    red = 255.0;
    green = 127.0 - ((iaq - 75.0) * 127.0 / 100.0);
    blue = 0.0;
    if (fadeAmount < 0)
      fadeAmount = -4 * scale;
    else
      fadeAmount = 4 * scale;
  } else if ((iaq >= 175) && (iaq < 300)) { // RGB=(255-127,0,0-127)
    red = 255.0 - ((iaq - 175.0) * 127.0 / 125.0);
    green = 0.0;
    blue = ((iaq - 175.0) * 127.0 / 125.0);
    if (fadeAmount < 0)
      fadeAmount = -5 * scale;
    else
      fadeAmount = 5 * scale;
  } else if (iaq > 300) { // RGB=(127,0,127)
    red = 127.0;
    green = 0.0;
    blue = 127.0;
    if (fadeAmount < 0)
      fadeAmount = -6 * scale;
    else
      fadeAmount = 6 * scale;
  }
  red = red * lum / 100;
  green = green * lum / 100;
  blue = blue * lum / 100;
  for (int i = 0; i < NUMPIXELS; i++) {
    //if (iaqAcc == 0)
    //pixels.setPixelColor(i, pixels.Color(2.55 * lum, 2.55 * lum, 2.55 * lum, 2.55 * lum)); // White
    //else
    pixels.setPixelColor(i, pixels.Color(red, green, blue, 0));
  }
  pixels.show();
}

// Function to compensate the heating of the PCB linearly over time
void compensateBoardHeat(void)
{
  float tempOffset = (float(millis() / 1000) * HEATING_RATE) / 60.0f;

  if (tempOffset > HEATING_MAX)
    heatCompensated = true;
  else
    iaqSensor.setTemperatureOffset(tempOffset);
}

