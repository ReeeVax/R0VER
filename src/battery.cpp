#include "battery.h"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA260.h>
#include <math.h>

Adafruit_INA260 ina260;
static bool ina260Ready = false;

void batterySetup()
{
    Wire.begin(21, 22); // SDA = GPIO21, SCL = GPIO22

    ina260Ready = ina260.begin();
    if (!ina260Ready)
    {
        Serial.println("INA260 not found!");
        return;
    }

    Serial.println("INA260 connected.");
}

bool batteryIsReady()
{
    return ina260Ready;
}

float getBatteryVoltage()
{
    if (!ina260Ready)
    {
        return NAN;
    }
    return ina260.readBusVoltage() / 1000.0;
}

float getBatteryCurrent()
{
    if (!ina260Ready)
    {
        return NAN;
    }
    return ina260.readCurrent() / 1000.0;
}

float getBatteryPower()
{
    if (!ina260Ready)
    {
        return NAN;
    }
    return ina260.readPower() / 1000.0;
}