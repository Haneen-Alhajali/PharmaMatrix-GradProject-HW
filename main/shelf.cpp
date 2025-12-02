#include "shelf.h"

Shelf::Shelf(float height, float tol) {
    shelfHeight = height;
    tolerance = tol;
}

String Shelf::getStatus(float distance) {
    if (distance >= shelfHeight - tolerance) {
        return "EMPTY";
    } 
    else if (distance > 0 && distance < shelfHeight) {
        return "FILLED";
    }
    else {
        return "ERROR";
    }
}

int Shelf::estimateCount(float distance, float bottleHeight) {
    if (distance >= shelfHeight - tolerance) return 0;

    float filledHeight = shelfHeight - distance;
    return filledHeight / bottleHeight;
}
