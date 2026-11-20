#ifndef BATTERY_H
#define BATTERY_H

void batterySetup();
bool batteryIsReady();

float getBatteryVoltage();
float getBatteryCurrent();
float getBatteryPower();

#endif
