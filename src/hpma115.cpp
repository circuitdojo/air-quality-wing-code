/*
 * Project Particle Squared
 * Description: Particle powered PM2.5 and air quality sensor
 * Author: Jared Wolff
 * Date: 2/26/2019
 * License: GNU GPLv3
 */

#include "hpma115.h"

HPMA115::HPMA115(void){}

uint32_t HPMA115::setup(hpma115_init_t *p_init, serial_lock_t * p_serial_lock) {

  // Returns null error
  if( p_init == NULL || p_serial_lock == NULL ) {
    return HPMA115_NULL_ERROR;
  }

  // Sets serial lock
  this->serial_lock = p_serial_lock;

  // Set up callback
  this->callback = p_init->callback;

  // Set enable pin
  this->enable_pin = p_init->enable_pin;

  // Set rx count to 0
  this->rx_count = 0;

  // Stop device
  this->disable();

  return HPMA115_SUCCESS;
}

uint32_t HPMA115::enable(){

  // Return busy error
  if( this->serial_lock->owner != serial_lock_none ){
    return HPMA115_SERIAL_BUSY;
  }

  // Set the owner
  this->serial_lock->owner = serial_lock_hpma;

  // Set up serial
  Serial1.begin(HPMA115_BAUD);

  // set gpio high
  pinMode(this->enable_pin, OUTPUT);
  digitalWrite(this->enable_pin, HIGH); // Has a pulldown

  if( this->state == HPMA_DISABLED ) {
    this->state = HPMA_READY;
  }

  return HPMA115_SUCCESS;
}

uint32_t HPMA115::disable() {

  // Return busy error
  if( this->serial_lock->owner != serial_lock_hpma ){
    return HPMA115_SERIAL_BUSY;
  }

  // Release serial for use elsewhere
  Serial1.end();

  // Disable device
  pinMode(this->enable_pin, INPUT);
  this->state = HPMA_DISABLED;

  // Reset rx count
  this->rx_count = 0;

  // Release the owner
  this->serial_lock->owner = serial_lock_none;

  return HPMA115_SUCCESS;
}

void HPMA115::int_handler() {

  // If we are ready, change state
  if ( this->state == HPMA_READY ) {
    this->state = HPMA_DATA_AVAILABLE;
  }

}

bool HPMA115::is_enabled() {

  // If we are ready, change state
  if ( getPinMode(this->enable_pin) == OUTPUT ) {
    return true;
  } else {
    return false;
  }

}

void HPMA115::process() {

  // Checking if the hpma115 owns the serial bus
  if( this->serial_lock->owner == serial_lock_hpma ) {

    // First read
    if( this->state == HPMA_DATA_AVAILABLE && Serial1.available() >= 2 ) {

      // Erase the rx_buf
      memset(this->rx_buf,0,32);

      // Read first byte in
      this->rx_buf[0] = Serial1.read();

      // Make sure first byte is equal otherwise return
      if( this->rx_buf[0] != 0x42 ) {
        return;
      }

      // Reaad the second byte in
      this->rx_buf[1] = Serial1.read();

      // Confirm its value
      if( this->rx_buf[1] == 0x4d ) {
        this->state = HPMA_DATA_READ;
      }

    }

    // Read remaining bytes
    if( this->state == HPMA_DATA_READ && Serial1.available() >= 30) {
      // Then read
      Serial1.readBytes(this->rx_buf+2,30);

      uint16_t calc_checksum = 0;

      // Calculate the checksum
      for( int i = 0; i < 30; i++ ) {
        calc_checksum += this->rx_buf[i];
      }

      // Un-serialize checksum from data
      uint16_t data_checksum = (this->rx_buf[30] << 8) + this->rx_buf[31];

      // Serial.printf("%x %x\n",calc_checksum,data_checksum);

      // Make sure the calculated and the provided are the same
      // or, make sure we've collected a few data points before
      // sending the data
      if ( calc_checksum != data_checksum ) {

        Serial.println("hpma: checksum fail");
        Particle.publish("err", "hpma: checksum" , PRIVATE, NO_ACK);

        this->state = HPMA_READY;
        return;
      }

      // Increment the valid rx count
      this->rx_count++;

      // Take another reading. Minimum of HPMA115_READING_CNT readings
      if ( this->rx_count < HPMA115_READING_CNT ) {

        // Go right to read or go to ready state if no data
        if( Serial1.available() > 0 ) {
          this->state = HPMA_DATA_AVAILABLE;
        } else {
          this->state = HPMA_READY;
        }

        return;
      }

      // Reset this
      this->rx_count = 0;

      // Combine the serialized data
      this->data.pm25 = (this->rx_buf[6] << 8) + this->rx_buf[7];
      this->data.pm10 = (this->rx_buf[8] << 8) + this->rx_buf[9];

      // Callback
      this->callback(&this->data);

      // State is back to ready
      this->state = HPMA_READY;

    }
  }
}