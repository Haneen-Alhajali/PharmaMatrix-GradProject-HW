#include "ultrasonic.h"

UltrasonicSensor::UltrasonicSensor(int trig, int echo) {
    trigPin = trig;
    echoPin = echo;

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
}

float UltrasonicSensor::readDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    float duration = pulseIn(echoPin, HIGH);
    float distance = duration * 0.034 / 2;

    return distance;
}
