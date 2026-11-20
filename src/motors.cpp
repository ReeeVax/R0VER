#include "motors.h"

#include <Arduino.h>

// Two dual-channel drivers: one for each side of the rover.
// GPIO12/34/35 from the original declarations are not used: GPIO12 is a
// boot-strapping pin and GPIO34/35 are input-only on the ESP32.
static const int LEFT_PWMA = 13;
static const int LEFT_AIN1 = 14;
static const int LEFT_AIN2 = 4;
static const int LEFT_BIN1 = 26;
static const int LEFT_BIN2 = 27;
static const int LEFT_PWMB = 18;

static const int RIGHT_PWMA = 33;
static const int RIGHT_AIN1 = 16;
static const int RIGHT_AIN2 = 32;
static const int RIGHT_BIN1 = 25;
static const int RIGHT_BIN2 = 23;
static const int RIGHT_PWMB = 19;

// Both driver's standby inputs share this enable signal.
static const int DRIVER_STBY = 17;

static int boundedSpeed(int speed)
{
    return constrain(speed, -255, 255);
}

void motorsSetup()
{
    // Assert standby before configuring any bridge control signals.
    pinMode(DRIVER_STBY, OUTPUT);
    digitalWrite(DRIVER_STBY, LOW);

    const int outputPins[] = {
        LEFT_AIN1, LEFT_AIN2, LEFT_BIN1, LEFT_BIN2,
        LEFT_PWMA, LEFT_PWMB,
        RIGHT_AIN1, RIGHT_AIN2, RIGHT_BIN1, RIGHT_BIN2,
        RIGHT_PWMA, RIGHT_PWMB
    };

    for (unsigned int i = 0; i < sizeof(outputPins) / sizeof(outputPins[0]); ++i)
    {
        pinMode(outputPins[i], OUTPUT);
        digitalWrite(outputPins[i], LOW);
    }

    analogWrite(LEFT_PWMA, 0);
    analogWrite(LEFT_PWMB, 0);
    analogWrite(RIGHT_PWMA, 0);
    analogWrite(RIGHT_PWMB, 0);
}

void setLeftMotors(int speed)
{
    const int value = boundedSpeed(speed);
    digitalWrite(LEFT_AIN1, value > 0 ? HIGH : LOW);
    digitalWrite(LEFT_AIN2, value < 0 ? HIGH : LOW);
    digitalWrite(LEFT_BIN1, value > 0 ? HIGH : LOW);
    digitalWrite(LEFT_BIN2, value < 0 ? HIGH : LOW);

    const int pwm = value < 0 ? -value : value;
    analogWrite(LEFT_PWMA, pwm);
    analogWrite(LEFT_PWMB, pwm);
    if (value != 0)
    {
        digitalWrite(DRIVER_STBY, HIGH);
    }
}

void setRightMotors(int speed)
{
    const int value = boundedSpeed(speed);
    digitalWrite(RIGHT_AIN1, value > 0 ? HIGH : LOW);
    digitalWrite(RIGHT_AIN2, value < 0 ? HIGH : LOW);
    digitalWrite(RIGHT_BIN1, value > 0 ? HIGH : LOW);
    digitalWrite(RIGHT_BIN2, value < 0 ? HIGH : LOW);

    const int pwm = value < 0 ? -value : value;
    analogWrite(RIGHT_PWMA, pwm);
    analogWrite(RIGHT_PWMB, pwm);
    if (value != 0)
    {
        digitalWrite(DRIVER_STBY, HIGH);
    }
}

void setDrive(int leftSpeed, int rightSpeed)
{
    const int left = boundedSpeed(leftSpeed);
    const int right = boundedSpeed(rightSpeed);
    setLeftMotors(left);
    setRightMotors(right);
    digitalWrite(DRIVER_STBY, (left == 0 && right == 0) ? LOW : HIGH);
}

void setMotor(int speed)
{
    setDrive(speed, speed);
}

void stopMotors()
{
    setDrive(0, 0);
}

void stopMotor()
{
    stopMotors();
}
