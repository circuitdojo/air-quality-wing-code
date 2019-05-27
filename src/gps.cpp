#include "gps.h"
#include "application.h"

#define GPSECHO false

// Serial init
Adafruit_GPS gps(&Serial1);

// Constructor
GPS::GPS(void){
  this->timer = millis();
}

uint32_t GPS::init(gps_init_t *p_init, serial_lock_t * p_serial_lock) {

  // Returns an error if the argument is null
  if( p_init == NULL ||  p_serial_lock == NULL) {
    return GPS_NULL_ERROR;
  }

  // Set up
  this->serial_lock = p_serial_lock;

  // Copy config over
  this->config = *p_init;

  // Disable by default
  this->disable();

  // Delay
  delay(50);

  return GPS_SUCCESS;
}

uint32_t GPS::enable() {

  // Return if the serial is busy
  if( this->serial_lock->owner != serial_lock_none  ) {
    return GPS_SERIAL_BUSY;
  }

  // Set up lock
  this->serial_lock->owner = serial_lock_gps;

  gps.begin(GPS_BAUD);

  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time
  // Set the update rate
  gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request no updates on antenna status
  gps.sendCommand(PGCMD_NOANTENNA);

  // Set to an output
  pinMode(this->config.enable_pin, OUTPUT);

  // Drive it high (this pin is active low)
  digitalWrite(this->config.enable_pin, LOW);

  // Set state to enabled
  this->state = GPS_ENABLED;

  return GPS_SUCCESS;
}

uint32_t GPS::get_fix() {

  return GPS_SUCCESS;
}

uint32_t GPS::disable() {

  // Return if the serial is busy
  if( this->serial_lock->owner != serial_lock_gps  ) {
    return GPS_SERIAL_BUSY;
  }

  // Set to disabled
  this->state = GPS_DISABLED;

  // disable serial etc
  Serial1.end();

  // Set to an output
  pinMode(this->config.enable_pin, OUTPUT);

  // Drive it high (this pin is active low)
  digitalWrite(this->config.enable_pin, HIGH);

  // Release lock
  this->serial_lock->owner = serial_lock_none;

  return GPS_SUCCESS;
}

bool GPS::is_enabled() {

  if( this->state == GPS_ENABLED ) {
    return true;
  } else {
    return false;
  }

}

uint32_t GPS::process() {

  // If enabled and owned by gps start doing stuff
  if( this->state == GPS_ENABLED && this->serial_lock->owner == serial_lock_gps) {
    // read data from the GPS in the 'main loop'
    char c = gps.read();
    // if you want to debug, this is a good time to do it!
    if (GPSECHO)
      if (c) Serial.print(c);
    // if a sentence is received, we can check the checksum, parse it...
    if (gps.newNMEAreceived()) {
      // a tricky thing here is if we print the NMEA sentence, or data
      // we end up not listening and catching other sentences!
      // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
      // Serial.println(gps.lastNMEA()); // this also sets the newNMEAreceived() flag to false
      if (!gps.parse(gps.lastNMEA())) // this also sets the newNMEAreceived() flag to false
        return GPS_SUCCESS; // we can fail to parse a sentence in which case we should just wait for another
    }

    // Once we get a fix, store the info and shutdown
    if( gps.fix ) {

      // store the lat long
      this->last_position.lat = gps.latitude_fixed;
      this->last_position.lat_c = gps.lat;

      this->last_position.lon = gps.longitude_fixed;
      this->last_position.lon_c = gps.lon;

      this->last_position.time.year = gps.year;
      this->last_position.time.month = gps.month;
      this->last_position.time.day = gps.day;
      this->last_position.time.hour = gps.hour;
      this->last_position.time.minute = gps.minute;
      this->last_position.time.seconds = gps.seconds;
      this->last_position.time.milliseconds = gps.milliseconds;

      // Callback
      this->config.callback(&this->last_position);

      // Print out location
      Serial.printf("fixed: %i%c, %i%c\n",this->last_position.lat,this->last_position.lat_c,this->last_position.lon,this->last_position.lon_c);

      // disable device
      this->disable();
    }

  }

  return GPS_SUCCESS;
}