#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <Arduino.h>

class UltrasonicSensor {

private:
    int trigPin;
    int echoPin;

public:
    UltrasonicSensor(int trig, int echo);
    float readDistance();   // Returns distance in cm
};

#endif
