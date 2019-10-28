#ifndef MOTION_H
#define MOTION_H

#include "AirQualityWing.h"

// Hazard levels
#define PM25_LOW       15
#define PM25_MED       30
#define PM25_HIGH      55
#define PM25_HAZARDOUS 110

// Timeframe for LED on
#define LED_ON_INTERVAL 10000

void processMotion(AirQualityWingData_t data);
void motionEvent();

#endif