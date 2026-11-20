#include <Arduino.h>

#include "battery.h"
#include "comms.h"
#include "motors.h"
#include "telemetry.h"

void setup()
{
    Serial.begin(115200);

    // Keep driver outputs in standby before initializing anything else.
    motorsSetup();
    batterySetup();
    commsSetup();
    telemetrySetup();

    if (!batteryIsReady())
    {
        Serial.println("Drive disabled: INA260 battery monitor is unavailable.");
    }
    Serial.println("Rover ready. Send D <left -255..255> <right -255..255> or S.");
}

void loop()
{
    commsUpdate(batteryIsReady());
    telemetryUpdate();
    delay(5);
}
