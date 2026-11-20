#include "telemetry.h"

#include <Arduino.h>

#include "battery.h"

static const unsigned long TELEMETRY_INTERVAL_MS = 1000;
static unsigned long lastTelemetryMs = 0;

void telemetrySetup()
{
    lastTelemetryMs = millis();
}

void telemetryUpdate()
{
    const unsigned long now = millis();
    if (static_cast<unsigned long>(now - lastTelemetryMs) < TELEMETRY_INTERVAL_MS)
    {
        return;
    }
    lastTelemetryMs = now;

    if (!batteryIsReady())
    {
        Serial.println("TELEMETRY battery=unavailable");
        return;
    }

    Serial.printf("TELEMETRY voltage=%.3f V current=%.3f A power=%.3f W\n",
                  getBatteryVoltage(),
                  getBatteryCurrent(),
                  getBatteryPower());
}
