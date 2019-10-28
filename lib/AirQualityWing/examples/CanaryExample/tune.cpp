#include "Particle.h"
#include "tune.h"
#include "board.h"

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