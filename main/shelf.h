#ifndef SHELF_H
#define SHELF_H

#include <Arduino.h>

class Shelf {

private:
    float shelfHeight;
    float tolerance;

public:
    Shelf(float height = 10.0, float tol = 1.0);
    String getStatus(float distance);
    int estimateCount(float distance, float bottleHeight = 5.0);
};

#endif
