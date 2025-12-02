#ifndef LCD_KEYPAD_H
#define LCD_KEYPAD_H

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// Use #define for array dimensions
#define ROWS 4
#define COLS 3

// LCD object
extern LiquidCrystal_I2C lcd;

// Keypad object
extern Keypad keypad;

// Custom characters
extern byte rightArrow[8];
extern byte leftArrow[8];

// Function declarations
void initializeLCD();
void initializeKeypad();
void initializeCustomChars();
void showMessage(String line1, String line2 = "", String line3 = "", String line4 = "", int delayTime = 1500);

#endif