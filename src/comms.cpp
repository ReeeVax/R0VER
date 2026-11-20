#include "comms.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#include "motors.h"

static const unsigned long COMMAND_TIMEOUT_MS = 1000;
static char commandBuffer[48];
static size_t commandLength = 0;
static bool commandOverflow = false;
static bool driveCommandActive = false;
static unsigned long lastDriveCommandMs = 0;

static bool parseSpeed(const char *text, int &speed)
{
    if (text == nullptr || *text == '\0')
    {
        return false;
    }

    char *end = nullptr;
    const long parsed = strtol(text, &end, 10);
    if (*end != '\0' || parsed < -255 || parsed > 255)
    {
        return false;
    }

    speed = static_cast<int>(parsed);
    return true;
}

static void processCommand(bool motorsPermitted)
{
    commandBuffer[commandLength] = '\0';
    char *command = strtok(commandBuffer, " \t\r");
    if (command == nullptr)
    {
        return;
    }

    if (strcmp(command, "S") == 0)
    {
        char *extra = strtok(nullptr, " \t\r");
        if (extra != nullptr)
        {
            stopMotors();
            driveCommandActive = false;
            Serial.println("ERR usage: S");
            return;
        }
        stopMotors();
        driveCommandActive = false;
        Serial.println("OK stopped");
        return;
    }

    char *leftText = strtok(nullptr, " \t\r");
    char *rightText = strtok(nullptr, " \t\r");
    char *extra = strtok(nullptr, " \t\r");
    int leftSpeed = 0;
    int rightSpeed = 0;

    if (strcmp(command, "D") != 0 ||
        !parseSpeed(leftText, leftSpeed) ||
        !parseSpeed(rightText, rightSpeed) ||
        extra != nullptr)
    {
        stopMotors();
        driveCommandActive = false;
        Serial.println("ERR use D <left -255..255> <right -255..255> or S");
        return;
    }

    if (!motorsPermitted)
    {
        stopMotors();
        driveCommandActive = false;
        Serial.println("ERR drive disabled: INA260 unavailable");
        return;
    }

    setDrive(leftSpeed, rightSpeed);
    driveCommandActive = leftSpeed != 0 || rightSpeed != 0;
    lastDriveCommandMs = millis();
    Serial.println("OK drive");
}

void commsSetup()
{
    commandLength = 0;
    commandOverflow = false;
    driveCommandActive = false;
}

void commsUpdate(bool motorsPermitted)
{
    if (!motorsPermitted)
    {
        stopMotors();
        driveCommandActive = false;
    }

    while (Serial.available() > 0)
    {
        const char incoming = static_cast<char>(Serial.read());
        if (incoming == '\n')
        {
            if (commandOverflow)
            {
                Serial.println("ERR line too long");
                stopMotors();
                driveCommandActive = false;
            }
            else
            {
                processCommand(motorsPermitted);
            }

            commandLength = 0;
            commandOverflow = false;
        }
        else if (!commandOverflow && incoming != '\r')
        {
            if (commandLength + 1 < sizeof(commandBuffer))
            {
                commandBuffer[commandLength++] = incoming;
            }
            else
            {
                commandOverflow = true;
            }
        }
    }

    if (driveCommandActive &&
        static_cast<unsigned long>(millis() - lastDriveCommandMs) >= COMMAND_TIMEOUT_MS)
    {
        stopMotors();
        driveCommandActive = false;
        Serial.println("OK command timeout; stopped");
    }
}
