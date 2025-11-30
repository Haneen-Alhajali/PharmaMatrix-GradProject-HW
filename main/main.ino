#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for I2C LCD displays
#include <Keypad.h>  // Include the keypad library
#include "ultrasonic.h"
#include "shelf.h"

UltrasonicSensor shelf1Sensor(10, 9);
Shelf shelf1(10.0, 1.0);


void checkShelfStatus() {
    float d = shelf1Sensor.readDistance();
    String status = shelf1.getStatus(d);
    int count = shelf1.estimateCount(d);

    Serial.println("📊 === Shelf Status ===");
    Serial.print("Distance: ");
    Serial.print(d);
    Serial.println(" cm");
    Serial.print("Status: ");
    Serial.println(status);

    if (status == "FILLED") {
        Serial.print("Estimated Boxes: ");
        Serial.println(count);
    }
    Serial.println("=======================");
}

///////////////////////////////////////////////////////////// LCD  ////////////////////////////////////////////////////////////////////////////////////////

// Create an LCD object at I2C address 0x27, with 16 columns and 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2);

void LCDdisplay(){

    cd.init(); // Initialize the LCD
    lcd.backlight(); // Turn on the LCD backlight
    // Set the cursor to column 0, row 0 (top-left corner)
    lcd.setCursor(0, 0);
    lcd.print("Hello, world!"); // Print text on the first row
    // Set the cursor to column 0, row 1 (start of the second row)
    lcd.setCursor(0, 1);
    lcd.print("test"); // Print text on the second row
}


///////////////////////////////////////////////////////////// Keypad ////////////////////////////////////////////////////////////////////////////////////////

// Define the number of rows and columns in the keypad
const byte ROWS = 4;
const byte COLS = 4;

// Create a 2D array representing the keys on the keypad
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Define the Arduino pins connected to the row pins of the keypad
byte rowPins[ROWS] = {9, 8, 7, 6};

// Define the Arduino pins connected to the column pins of the keypad
byte colPins[COLS] = {5, 4, 3, 2};

// Create a Keypad object using the keymap and pin connections
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void Keypadtyping(){
  // Read the key pressed on the keypad
  char key = keypad.getKey();
  
  // If a key is pressed, print it to the Serial Monitor
  if (key) {
    Serial.println(key);
  }
}



///////////////////////////////////////////////////////////// Main ////////////////////////////////////////////////////////////////////////////////////////

void setup() {
    Serial.begin(9600);
    Serial.println("🚀 PharmaMatrix System Started!");



}

void loop() {
    // ultrasonic code
    checkShelfStatus();
    LCDdisplay();
    Keypadtyping();

    delay(500);
}
