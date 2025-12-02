#include "display_manager.h"

// Navigation variables
int currentState = 0;
int currentPage = 0;
int selectedMedicineIndex = -1;
String inputBuffer = "";
bool needRefresh = true;

void displayMainMenu() {
  if (!needRefresh && currentState == STATE_MAIN) return;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("==  PharmaMatrix  ==");
  lcd.setCursor(0, 1);
  lcd.print("1) Add Medicine");
  lcd.setCursor(0, 2);
  lcd.print("2) New Order");
  lcd.setCursor(0, 3);
  lcd.print("3) View Stock");
  
  currentState = STATE_MAIN;
  currentPage = 0;
  inputBuffer = "";
  selectedMedicineIndex = -1;
  needRefresh = false;
}

void displayAddMedicine() {
  if (!needRefresh && currentState == STATE_ADD_MEDICINE) return;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(1));
  lcd.write(byte(1));
  lcd.print("  ADD MEDICINE  ");
  lcd.setCursor(18, 0);
  lcd.write(byte(0));
  lcd.write(byte(0));

  lcd.setCursor(0, 1);
  lcd.print("Scan/Enter barcode:");
  lcd.setCursor(0, 2);
  if (inputBuffer.length() > 0) {
    lcd.print(inputBuffer);
  } else {
    lcd.print("_");
  }
  lcd.setCursor(0, 3);
  lcd.print("*=OK       #=Back");  
  
  currentState = STATE_ADD_MEDICINE;
  needRefresh = false;
}

void displayAddQuantity(int medIndex) {
  if (!needRefresh && currentState == STATE_ADD_QTY) return;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Add to:");
  lcd.setCursor(0, 1);
  lcd.print(inventory[medIndex].name);
  lcd.setCursor(0, 2);
  lcd.print("Current: ");
  lcd.print(inventory[medIndex].quantity);
  lcd.setCursor(0, 3);
  lcd.print("Add: ");
  if (inputBuffer.length() > 0) {
    lcd.print(inputBuffer);
  } else {
    lcd.print("_");
  }
  lcd.print(" *=OK  #=Back");
  
  currentState = STATE_ADD_QTY;
  needRefresh = false;
}

void displayOrderMenu() {
  if (!needRefresh && currentState == STATE_ORDER) return;
  
  int totalPages = (medicineCount + 1) / 2;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(1));
  lcd.write(byte(1));
  lcd.print("   ORDER ");
  lcd.print(currentPage + 1);
  lcd.print("/");
  lcd.print(totalPages);
  lcd.setCursor(18, 0);
  lcd.write(byte(0));
  lcd.write(byte(0));

  for (int i = 0; i < 2; i++) {
    int idx = (currentPage * 2) + i;
    if (idx < medicineCount) {
      lcd.setCursor(0, i + 1);
      lcd.print(i + 1);
      lcd.print(") ");
      String displayName = inventory[idx].name;
      if (displayName.length() > 12) {
        displayName = displayName.substring(0, 12);
      }
      lcd.print(displayName);
      lcd.setCursor(15, i + 1);
      lcd.print(":");
      if (inventory[idx].quantity < 10) lcd.print(" ");
      lcd.print(inventory[idx].quantity);
    } else {
      lcd.setCursor(0, i + 1);
      lcd.print("                    ");
    }
  }
  
  lcd.setCursor(0, 3);
  if (currentPage < totalPages - 1) {
    lcd.print("*=Next");
  } else {
    lcd.print("      ");
  }
  
  if (currentPage > 0) {
    lcd.setCursor(7, 3);
    lcd.print("0=Prev");
  } else {
    lcd.setCursor(7, 3);
    lcd.print("      ");
  }
  
  lcd.setCursor(14, 3);
  lcd.print("#=Back");
  
  currentState = STATE_ORDER;
  needRefresh = false;
}

void displayOrderQuantity() {
  if (!needRefresh && currentState == STATE_ORDER_QTY) return;
  
  if (selectedMedicineIndex < 0 || selectedMedicineIndex >= medicineCount) {
    displayMainMenu();
    return;
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(1));
  lcd.print(" ORDER: ");
  lcd.print(inventory[selectedMedicineIndex].name);
  lcd.setCursor(0, 1);
  lcd.print("Available: ");
  lcd.print(inventory[selectedMedicineIndex].quantity);
  lcd.setCursor(0, 2);
  lcd.print("Enter quantity:");
  lcd.setCursor(0, 3);
  if (inputBuffer.length() > 0) {
    lcd.print(inputBuffer);
  } else {
    lcd.print("_");
  }
  lcd.print(" *=OK #=Back");
  
  currentState = STATE_ORDER_QTY;
  needRefresh = false;
}

void displayStockMenu() {
  if (!needRefresh && currentState == STATE_STOCK) return;
  
  int totalPages = (medicineCount + 1) / 2;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(1));
  lcd.write(byte(1));
  lcd.print("   STOCK ");
  lcd.print(currentPage + 1);
  lcd.print("/");
  lcd.print(totalPages);
  lcd.setCursor(18, 0);
  lcd.write(byte(0));
  lcd.write(byte(0));
  
  for (int i = 0; i < 2; i++) {
    int idx = (currentPage * 2) + i;
    if (idx < medicineCount) {
      lcd.setCursor(0, i + 1);
      String displayName = inventory[idx].name;
      if (displayName.length() > 15) {
        displayName = displayName.substring(0, 15);
      }
      lcd.print(displayName);
      lcd.setCursor(16, i + 1);
      lcd.print(":");
      if (inventory[idx].quantity < 10) lcd.print(" ");
      lcd.print(inventory[idx].quantity);
    } else {
      lcd.setCursor(0, i + 1);
      lcd.print("                    ");
    }
  }
  
  lcd.setCursor(0, 3);
  if (currentPage < totalPages - 1) {
    lcd.print("*=Next");
  } else {
    lcd.print("      ");
  }
  
  if (currentPage > 0) {
    lcd.setCursor(7, 3);
    lcd.print("0=Prev");
  } else {
    lcd.setCursor(7, 3);
    lcd.print("      ");
  }
  
  lcd.setCursor(14, 3);
  lcd.print("#=Back");
  
  currentState = STATE_STOCK;
  needRefresh = false;
}