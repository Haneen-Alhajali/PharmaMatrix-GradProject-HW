#ifndef INVENTORY_H
#define INVENTORY_H

#include <Arduino.h>

#define MAX_MEDICINES 50

struct Medicine {
  String barcode;
  String name;
  int quantity;
  float price;
};

// Global variables
extern Medicine inventory[MAX_MEDICINES];
extern int medicineCount;

// Function declarations
void initializeSampleData();
int findMedicineByBarcode(String barcode);

#endif