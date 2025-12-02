#include "inventory.h"

Medicine inventory[MAX_MEDICINES];
int medicineCount = 0;

void initializeSampleData() {
  inventory[0] = {"123456789012", "PANADOL", 25, 2.50};
  inventory[1] = {"234567890123", "BRUFEN", 15, 3.75};
  inventory[2] = {"345678901234", "AUGMENTIN", 8, 15.00};
  inventory[3] = {"456789012345", "ASPIRIN", 30, 1.25};
  inventory[4] = {"567890123456", "PARACETAMOL", 20, 2.00};
  inventory[5] = {"678901234567", "IBUPROFEN", 12, 4.50};
  medicineCount = 6;
}

int findMedicineByBarcode(String barcode) {
  for (int i = 0; i < medicineCount; i++) {
    if (inventory[i].barcode == barcode) {
      return i;
    }
  }
  return -1;
}