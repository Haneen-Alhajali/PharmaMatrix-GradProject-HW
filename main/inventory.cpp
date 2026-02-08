#include "inventory.h"

Medicine inventory[MAX_MEDICINES];
int medicineCount = 0;

void initializeSampleData() {
  inventory[0] = {"1", "Siafil", 2, 2.50, 2, "A1", 2};
  inventory[1] = {"2", "Lamirase", 2, 3.75, 3, "B1", 0};
  inventory[2] = {"3", "Vermmazol", 2, 15.00, 2, "A2", 0};
  inventory[3] = {"4", "Lorias", 2, 1.25, 3, "B2", 0};
  inventory[4] = {"5", "Ultrafen Lc", 2, 2.00, 2, "A3", 0};
  inventory[5] = {"6", "Clovix 75", 3, 4.50, 3, "B3", 0};
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