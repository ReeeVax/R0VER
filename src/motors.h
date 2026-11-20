#ifndef MOTORS_H
#define MOTORS_H

void motorsSetup();

void setDrive(int leftSpeed, int rightSpeed);
void setMotor(int speed);
void stopMotor();
void stopMotors();

void setRightMotors(int speed);
void setLeftMotors(int speed);


#endif