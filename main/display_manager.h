#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include "lcd_keypad.h"
#include "inventory.h"

// State definitions
enum AppState {
  STATE_MAIN = 0,
  STATE_ADD_MEDICINE = 1,
  STATE_ORDER = 2,
  STATE_STOCK = 3,
  STATE_ORDER_QTY = 4,
  STATE_ADD_BARCODE = 5,
  STATE_ADD_QTY = 6
};

// Global navigation variables
extern int currentState;
extern int currentPage;
extern int selectedMedicineIndex;
extern String inputBuffer;
extern bool needRefresh;

// Display functions
void displayMainMenu();
void displayAddMedicine();
void displayAddQuantity(int medIndex);
void displayOrderMenu();
void displayOrderQuantity();
void displayStockMenu();

#endif