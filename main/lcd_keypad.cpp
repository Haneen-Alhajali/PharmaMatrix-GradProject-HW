#include "lcd_keypad.h"

// LCD object
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Keypad configuration
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[ROWS] = {8, 7, 6, 5};
byte colPins[COLS] = {4, 3, 2};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Custom characters
byte rightArrow[8] = {
  B00000,
  B00100,
  B00110,
  B11111,
  B11111,
  B00110,
  B00100,
  B00000
};

byte leftArrow[8] = {
  B00000,
  B00100,
  B01100,
  B11111,
  B11111,
  B01100,
  B00100,
  B00000
};

void initializeLCD() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void initializeKeypad() {
  // Keypad is already initialized with its constructor
}

void initializeCustomChars() {
  lcd.createChar(0, leftArrow);   // store at index 0
  lcd.createChar(1, rightArrow);  // store at index 1
}

void showMessage(String line1, String line2, String line3, String line4, int delayTime) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  if (line2.length() > 0) {
    lcd.setCursor(0, 1);
    lcd.print(line2);
  }
  if (line3.length() > 0) {
    lcd.setCursor(0, 2);
    lcd.print(line3);
  }
  if (line4.length() > 0) {
    lcd.setCursor(0, 3);
    lcd.print(line4);
  }
  delay(delayTime);
}