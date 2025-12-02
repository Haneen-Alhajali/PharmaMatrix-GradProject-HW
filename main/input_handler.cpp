#include "input_handler.h"
#include "inventory.h"

void handleKeypadInput(char key) {
  Serial.print("Key: ");
  Serial.println(key);
  
  switch(currentState) {
    case STATE_MAIN:
      handleMainMenu(key);
      break;
    case STATE_ADD_MEDICINE:
      handleAddMedicine(key);
      break;
    case STATE_ORDER:
      handleOrderMenu(key);
      break;
    case STATE_STOCK:
      handleStockMenu(key);
      break;
    case STATE_ORDER_QTY:
      handleOrderQuantity(key);
      break;
    case STATE_ADD_QTY:
      handleAddQuantity(key);
      break;
  }
}

void handleMainMenu(char key) {
  if (key == '1') {
    inputBuffer = "";
    displayAddMedicine();
  } else if (key == '2') {
    currentPage = 0;
    displayOrderMenu();
  } else if (key == '3') {
    currentPage = 0;
    displayStockMenu();
  }
}

void handleAddMedicine(char key) {
  if (key >= '0' && key <= '9') {
    if (inputBuffer.length() < 12) {
      inputBuffer += key;
      needRefresh = true;
    }
  } else if (key == '#') {
    displayMainMenu();
  } else if (key == '*') {
    if (inputBuffer.length() > 0) {
      int medIndex = findMedicineByBarcode(inputBuffer);
      if (medIndex != -1) {
        selectedMedicineIndex = medIndex;
        inputBuffer = "";
        displayAddQuantity(medIndex);
      } else {
        showMessage("Not found!", "Cannot add new", "medicine", "Press any key", 3000);
        displayMainMenu();
      }
    }
  }
}

void handleOrderMenu(char key) {
  if (key == '*' && currentPage < ((medicineCount + 1) / 2) - 1) {
    currentPage++;
    needRefresh = true;
  } else if (key == '0' && currentPage > 0) {
    currentPage--;
    needRefresh = true;
  } else if (key == '#') {
    displayMainMenu();
  } else if (key == '1' || key == '2') {
    int selection = key - '1';
    selectedMedicineIndex = (currentPage * 2) + selection;
    if (selectedMedicineIndex < medicineCount) {
      inputBuffer = "";
      displayOrderQuantity();
    }
  }
}

void handleStockMenu(char key) {
  if (key == '*' && currentPage < ((medicineCount + 1) / 2) - 1) {
    currentPage++;
    needRefresh = true;
  } else if (key == '0' && currentPage > 0) {
    currentPage--;
    needRefresh = true;
  } else if (key == '#') {
    displayMainMenu();
  }
}

void handleOrderQuantity(char key) {
  if (key >= '0' && key <= '9') {
    if (inputBuffer.length() < 3) {
      inputBuffer += key;
      needRefresh = true;
    }
  } else if (key == '#') {
    currentPage = 0;
    displayOrderMenu();
  } else if (key == '*') {
    if (inputBuffer.length() > 0) {
      int orderQty = inputBuffer.toInt();
      
      if (orderQty <= 0) {
        showMessage("Error!", "Qty must be >0", "", "Press any key", 3000);
        inputBuffer = "";
        needRefresh = true;
      } else if (orderQty > inventory[selectedMedicineIndex].quantity) {
        showMessage("Error!", "Not enough stock", "Press any key", "", 3000);
        inputBuffer = "";
        needRefresh = true;
      } else {
        inventory[selectedMedicineIndex].quantity -= orderQty;
        showMessage("Order placed!", 
                   inventory[selectedMedicineIndex].name,
                   "Qty: " + String(orderQty),
                   "Remaining: " + String(inventory[selectedMedicineIndex].quantity), 3000);
        currentPage = 0;
        displayOrderMenu();
      }
    }
  }
}

void handleAddQuantity(char key) {
  if (key >= '0' && key <= '9') {
    if (inputBuffer.length() < 3) {
      inputBuffer += key;
      needRefresh = true;
    }
  } else if (key == '#') {
    inputBuffer = "";
    displayAddMedicine();
  } else if (key == '*') {
    if (inputBuffer.length() > 0) {
      int addQty = inputBuffer.toInt();
      if (addQty > 0) {
        inventory[selectedMedicineIndex].quantity += addQty;
        showMessage("Added!", 
                   inventory[selectedMedicineIndex].name,
                   "New total: " + String(inventory[selectedMedicineIndex].quantity),
                   "", 3000);
        displayMainMenu();
      }
    }
  }
}

void refreshCurrentDisplay() {
  switch(currentState) {
    case STATE_MAIN: displayMainMenu(); break;
    case STATE_ADD_MEDICINE: displayAddMedicine(); break;
    case STATE_ORDER: displayOrderMenu(); break;
    case STATE_STOCK: displayStockMenu(); break;
    case STATE_ORDER_QTY: displayOrderQuantity(); break;
    case STATE_ADD_QTY: displayAddQuantity(selectedMedicineIndex); break;
  }
}