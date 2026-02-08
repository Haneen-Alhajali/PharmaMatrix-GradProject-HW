#include "input_handler.h"
#include "lcd_keypad.h"
#include "inventory.h"
#include "display_manager.h"

/* ========================================================= */
/* ============== SYSTEM CONTROL VARIABLES ================= */
/* ========================================================= */

// These variables are executed by the main loop controller
extern bool systemBusy;
extern int systemMode;       // 1 = ADD, 2 = REQUEST
extern int systemQuantity;
extern int selectedColumn;
extern int pendingMedicineIndex;

/* ========================================================= */
/* =================== INPUT HANDLER ======================= */
/* ========================================================= */

void handleKeypadInput(char key) {
  Serial.print("Key: ");
  Serial.println(key);

  switch (currentState) {
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

/* ========================================================= */
/* ===================== MAIN MENU ========================= */
/* ========================================================= */

void handleMainMenu(char key) {
  if (key == '1') {
    inputBuffer = "";
    displayAddMedicine();
  }
  else if (key == '2') {
    currentPage = 0;
    displayOrderMenu();
  }
  else if (key == '3') {
    currentPage = 0;
    displayStockMenu();
  }
}

/* ========================================================= */
/* ================== ADD MEDICINE FLOW ==================== */
/* ========================================================= */

// void handleAddMedicine(char key) {
//   if (key >= '0' && key <= '9') {
//     if (inputBuffer.length() < 12) {
//       inputBuffer += key;
//       needRefresh = true;
//     }
//   }
//   else if (key == '#') {
//     displayMainMenu();
//   }
//   else if (key == '*') {
//     if (inputBuffer.length() > 0) {
//       int medIndex = findMedicineByBarcode(inputBuffer);

//       if (medIndex != -1) {
//         selectedMedicineIndex = medIndex;
//         inputBuffer = "";
//         displayAddQuantity(medIndex);
//       }
//       else {
//         showMessage("Not found!",
//                     "Cannot add new",
//                     "medicine",
//                     "Press any key", 3000);
//         displayMainMenu();
//       }
//     }
//   }
// }

void handleAddMedicine(char key) {
  if (key == '*' && currentPage < ((medicineCount + 1) / 2) - 1) {
    currentPage++;
    needRefresh = true;
  }
  else if (key == '0' && currentPage > 0) {
    currentPage--;
    needRefresh = true;
  }
  else if (key == '#') {
    displayMainMenu();
  }
  else if (key == '1' || key == '2') {
    int selection = key - '1';
    selectedMedicineIndex = (currentPage * 2) + selection;

    if (selectedMedicineIndex < medicineCount) {
      inputBuffer = "";
      displayAddQuantity(selectedMedicineIndex);
    }
  }
}

void handleAddQuantity(char key) {
  if (key >= '0' && key <= '9') {
    if (inputBuffer.length() < 3) {
      inputBuffer += key;
      needRefresh = true;
    }
  }
  else if (key == '#') {
    inputBuffer = "";
    displayAddMedicine();
  }
  else if (key == '*') {
    if (inputBuffer.length() > 0 && !systemBusy) {
      int addQty = inputBuffer.toInt();

      if (addQty > 0) {
        systemMode = 1;
        systemQuantity = addQty;
        selectedColumn = inventory[selectedMedicineIndex].column;
        pendingMedicineIndex = selectedMedicineIndex;
        systemBusy = true;

        showMessage("Adding...",
                    inventory[selectedMedicineIndex].name,
                    "Qty: " + String(addQty),
                    "Please wait", 3000);

        displayMainMenu();
      }
    }
  }
}

/* ========================================================= */
/* ================= ORDER MEDICINE FLOW =================== */
/* ========================================================= */

void handleOrderMenu(char key) {
  if (key == '*' && currentPage < ((medicineCount + 1) / 2) - 1) {
    currentPage++;
    needRefresh = true;
  }
  else if (key == '0' && currentPage > 0) {
    currentPage--;
    needRefresh = true;
  }
  else if (key == '#') {
    displayMainMenu();
  }
  else if (key == '1' || key == '2') {
    int selection = key - '1';
    selectedMedicineIndex = (currentPage * 2) + selection;

    if (selectedMedicineIndex < medicineCount) {
      inputBuffer = "";
      displayOrderQuantity();
    }
  }
}

void handleOrderQuantity(char key) {
  if (key >= '0' && key <= '9') {
    if (inputBuffer.length() < 3) {
      inputBuffer += key;
      needRefresh = true;
    }
  }
  else if (key == '#') {
    currentPage = 0;
    displayOrderMenu();
  }
  else if (key == '*') {
    if (inputBuffer.length() > 0 && !systemBusy) {
      int orderQty = inputBuffer.toInt();

      if (orderQty > 0 && orderQty <= inventory[selectedMedicineIndex].quantity) {
        systemMode = 2;
        systemQuantity = orderQty;
        selectedColumn = inventory[selectedMedicineIndex].column;
        pendingMedicineIndex = selectedMedicineIndex;
        systemBusy = true;

        showMessage("Processing...",
                    inventory[selectedMedicineIndex].name,
                    "Qty: " + String(orderQty),
                    "Please wait", 3000);

        displayOrderMenu();
      }
    }
  }
}

/* ========================================================= */
/* ===================== STOCK VIEW ======================== */
/* ========================================================= */

void handleStockMenu(char key) {
  if (key == '*' && currentPage < ((medicineCount + 1) / 2) - 1) {
    currentPage++;
    needRefresh = true;
  }
  else if (key == '0' && currentPage > 0) {
    currentPage--;
    needRefresh = true;
  }
  else if (key == '#') {
    displayMainMenu();
  }
}

/* ========================================================= */
/* ================== DISPLAY REFRESH ====================== */
/* ========================================================= */

void refreshCurrentDisplay() {
  switch (currentState) {
    case STATE_MAIN:
      displayMainMenu();
      break;

    case STATE_ADD_MEDICINE:
      displayAddMedicine();
      break;

    case STATE_ORDER:
      displayOrderMenu();
      break;

    case STATE_STOCK:
      displayStockMenu();
      break;

    case STATE_ORDER_QTY:
      displayOrderQuantity();
      break;

    case STATE_ADD_QTY:
      displayAddQuantity(selectedMedicineIndex);
      break;
  }
}
