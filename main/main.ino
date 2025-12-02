// #include <Wire.h>
// #include <LiquidCrystal_I2C.h>
// #include <Keypad.h>
// #include "ultrasonic.h"
// #include "shelf.h"

// // //////////////////////////////////////////////////////// UltrasonicSensor  ///////////////////////////////////////////////////////

// UltrasonicSensor shelf1Sensor(10, 9);
// Shelf shelf1(10.0, 1.0);


// void checkShelfStatus() {
//     float d = shelf1Sensor.readDistance();
//     String status = shelf1.getStatus(d);
//     int count = shelf1.estimateCount(d);

//     Serial.println("📊 === Shelf Status ===");
//     Serial.print("Distance: ");
//     Serial.print(d);
//     Serial.println(" cm");
//     Serial.print("Status: ");
//     Serial.println(status);

//     if (status == "FILLED") {
//         Serial.print("Estimated Boxes: ");
//         Serial.println(count);
//     }
//     Serial.println("=======================");
// }


// ///////////////////////////////////////////////////////////// LCD (4x20) ////////////////////////////////////////////////////////////////////////////////////////

// LiquidCrystal_I2C lcd(0x27, 20, 4);

// ///////////////////////////////////////////////////////////// Keypad (4x3) ////////////////////////////////////////////////////////////////////////////////////////

// const byte ROWS = 4;
// const byte COLS = 3;

// char keys[ROWS][COLS] = {
//   {'1', '2', '3'},
//   {'4', '5', '6'},
//   {'7', '8', '9'},
//   {'*', '0', '#'}
// };

// byte rowPins[ROWS] = {8, 7, 6, 5};
// byte colPins[COLS] = {4, 3, 2};
// Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ///////////////////////////////////////////////////////////// Data Structures ////////////////////////////////////////////////////////////////////////////////////////

// struct Medicine {
//   String barcode;
//   String name;
//   int quantity;
//   float price;
// };

// #define MAX_MEDICINES 50
// Medicine inventory[MAX_MEDICINES];
// int medicineCount = 0;

// // Simple navigation system
// int currentState = 0; // 0=Main, 1=Add, 2=Order, 3=Stock, 4=OrderQty, 5=AddBarcode, 6=AddQty
// int currentPage = 0;
// int selectedMedicineIndex = -1;
// String inputBuffer = "";
// bool needRefresh = true;

// ///////////////////////////////////////////////////////////// Helper Functions ////////////////////////////////////////////////////////////////////////////////////////

// void showMessage(String line1, String line2 = "", String line3 = "", String line4 = "", int delayTime = 1500) {
//   lcd.clear();
//   lcd.setCursor(0, 0);
//   lcd.print(line1);
//   if (line2.length() > 0) {
//     lcd.setCursor(0, 1);
//     lcd.print(line2);
//   }
//   if (line3.length() > 0) {
//     lcd.setCursor(0, 2);
//     lcd.print(line3);
//   }
//   if (line4.length() > 0) {
//     lcd.setCursor(0, 3);
//     lcd.print(line4);
//   }
//   delay(delayTime);
//   needRefresh = true;
// }

// int findMedicineByBarcode(String barcode) {
//   for (int i = 0; i < medicineCount; i++) {
//     if (inventory[i].barcode == barcode) {
//       return i;
//     }
//   }
//   return -1;
// }

// void initializeSampleData() {
//   inventory[0] = {"123456789012", "PANADOL", 25, 2.50};
//   inventory[1] = {"234567890123", "BRUFEN", 15, 3.75};
//   inventory[2] = {"345678901234", "AUGMENTIN", 8, 15.00};
//   inventory[3] = {"456789012345", "ASPIRIN", 30, 1.25};
//   inventory[4] = {"567890123456", "PARACETAMOL", 20, 2.00};
//   inventory[5] = {"678901234567", "IBUPROFEN", 12, 4.50};
//   medicineCount = 6;
// }

// ///////////////////////////////////////////////////////////// Display Functions ////////////////////////////////////////////////////////////////////////////////////////

// // Custom right arrow
// byte rightArrow[8] = {
//   B00000,
//   B00100,
//   B00110,
//   B11111,
//   B11111,
//   B00110,
//   B00100,
//   B00000
// };

// // Custom left arrow
// byte leftArrow[8] = {
//   B00000,
//   B00100,
//   B01100,
//   B11111,
//   B11111,
//   B01100,
//   B00100,
//   B00000
// };


// void displayMainMenu() {
//   if (!needRefresh && currentState == 0) return;
  
//   lcd.clear();
//   lcd.setCursor(0, 0);
//   lcd.print("==  PharmaMatrix  ==");
//   lcd.setCursor(0, 1);
//   lcd.print("1) Add Medicine");
//   lcd.setCursor(0, 2);
//   lcd.print("2) New Order");
//   lcd.setCursor(0, 3);
//   lcd.print("3) View Stock");
  
//   currentState = 0;
//   currentPage = 0;
//   inputBuffer = "";
//   selectedMedicineIndex = -1;
//   needRefresh = false;
// }

// void displayAddMedicine() {
//   if (!needRefresh && currentState == 1) return;
  
//   lcd.clear();
//   lcd.setCursor(0, 0);
//   lcd.write(byte(1));   // custom right arrow
//   lcd.write(byte(1));   // custom right arrow
//   lcd.print("  ADD MEDICINE  ");
//     lcd.setCursor(18, 0);
//   // Left arrow (custom char)
//   lcd.write(byte(0));
//   lcd.write(byte(0));

//   lcd.setCursor(0, 1);
//   lcd.print("Scan/Enter barcode:");
//   lcd.setCursor(0, 2);
//   if (inputBuffer.length() > 0) {
//     lcd.print(inputBuffer);
//   } else {
//     lcd.print("_");
//   }
//   lcd.setCursor(0, 3);
//   lcd.print("*=OK       #=Back");  
  
//   currentState = 1;
//   needRefresh = false;
// }

// void displayAddQuantity(int medIndex) {
//   if (!needRefresh && currentState == 6) return;
  
//   lcd.clear();
//   lcd.setCursor(0, 0);
//   lcd.print("Add to:");
//   lcd.setCursor(0, 1);
//   lcd.print(inventory[medIndex].name);
//   lcd.setCursor(0, 2);
//   lcd.print("Current: ");
//   lcd.print(inventory[medIndex].quantity);
//   lcd.setCursor(0, 3);
//   lcd.print("Add: ");
//   if (inputBuffer.length() > 0) {
//     lcd.print(inputBuffer);
//   } else {
//     lcd.print("_");
//   }
//   lcd.print(" *=OK  #=Back");
  
//   currentState = 6;
//   needRefresh = false;
// }

// void displayOrderMenu() {
//   if (!needRefresh && currentState == 2) return;
  
//   int totalPages = (medicineCount + 1) / 2;
  
//   lcd.clear();
//   lcd.setCursor(0, 0);
//   lcd.write(byte(1));   // custom right arrow
//   lcd.write(byte(1));   // custom right arrow
//   lcd.print("   ORDER ");
//   lcd.print(currentPage + 1);
//   lcd.print("/");
//   lcd.print(totalPages);
//   lcd.setCursor(18, 0);
//   // Left arrow (custom char)
//   lcd.write(byte(0));
//   lcd.write(byte(0));

//   // Display 2 medicines
//   for (int i = 0; i < 2; i++) {
//     int idx = (currentPage * 2) + i;
//     if (idx < medicineCount) {
//       lcd.setCursor(0, i + 1);
//       lcd.print(i + 1);
//       lcd.print(") ");
//       String displayName = inventory[idx].name;
//       if (displayName.length() > 12) {
//         displayName = displayName.substring(0, 12);
//       }
//       lcd.print(displayName);
//       lcd.setCursor(15, i + 1);
//       lcd.print(":");
//       if (inventory[idx].quantity < 10) lcd.print(" ");
//       lcd.print(inventory[idx].quantity);
//     } else {
//       lcd.setCursor(0, i + 1);
//       lcd.print("                    ");
//     }
//   }
  
//   // FIXED NAVIGATION: Show Prev only when there's a previous page
//   lcd.setCursor(0, 3);
//   if (currentPage < totalPages - 1) {
//     lcd.print("*=Next");
//   } else {
//     lcd.print("      ");  // Clear when no next page
//   }
  
//   // FIXED: Only show "0=Prev" when currentPage > 0
//   if (currentPage > 0) {
//     lcd.setCursor(7, 3);
//     lcd.print("0=Prev");
//   } else {
//     lcd.setCursor(7, 3);
//     lcd.print("      ");  // Clear when no previous page
//   }
  
//   lcd.setCursor(14, 3);
//   lcd.print("#=Back");
  
//   currentState = 2;
//   needRefresh = false;
// }

// void displayOrderQuantity() {
//   if (!needRefresh && currentState == 4) return;
  
//   if (selectedMedicineIndex < 0 || selectedMedicineIndex >= medicineCount) {
//     displayMainMenu();
//     return;
//   }
  
//   lcd.clear();
//   lcd.setCursor(0, 0);
//   lcd.write(byte(1));   // custom right arrow
//   lcd.print(" ORDER: ");
//   lcd.print(inventory[selectedMedicineIndex].name);
//   lcd.setCursor(0, 1);
//   lcd.print("Available: ");
//   lcd.print(inventory[selectedMedicineIndex].quantity);
//   lcd.setCursor(0, 2);
//   lcd.print("Enter quantity:");
//   lcd.setCursor(0, 3);
//   if (inputBuffer.length() > 0) {
//     lcd.print(inputBuffer);
//   } else {
//     lcd.print("_");
//   }
//   lcd.print(" *=OK #=Back");
  
//   currentState = 4;
//   needRefresh = false;
// }

// void displayStockMenu() {
//   if (!needRefresh && currentState == 3) return;
  
//   int totalPages = (medicineCount + 1) / 2;
  
//   lcd.clear();
//   lcd.setCursor(0, 0);
//   lcd.write(byte(1));   // custom right arrow
//   lcd.write(byte(1));   // custom right arrow
//   lcd.print("   STOCK ");
//   lcd.print(currentPage + 1);
//   lcd.print("/");
//   lcd.print(totalPages);
//   lcd.setCursor(18, 0);
//   // Left arrow (custom char)
//   lcd.write(byte(0));
//   lcd.write(byte(0));
  
//   // Display 2 medicines
//   for (int i = 0; i < 2; i++) {
//     int idx = (currentPage * 2) + i;
//     if (idx < medicineCount) {
//       lcd.setCursor(0, i + 1);
//       String displayName = inventory[idx].name;
//       if (displayName.length() > 15) {
//         displayName = displayName.substring(0, 15);
//       }
//       lcd.print(displayName);
//       lcd.setCursor(16, i + 1);
//       lcd.print(":");
//       if (inventory[idx].quantity < 10) lcd.print(" ");
//       lcd.print(inventory[idx].quantity);
//     } else {
//       lcd.setCursor(0, i + 1);
//       lcd.print("                    ");
//     }
//   }
  
//   // FIXED NAVIGATION: Show Prev only when there's a previous page
//   lcd.setCursor(0, 3);
//   if (currentPage < totalPages - 1) {
//     lcd.print("*=Next");
//   } else {
//     lcd.print("      ");  // Clear when no next page
//   }
  
//   // FIXED: Only show "0=Prev" when currentPage > 0
//   if (currentPage > 0) {
//     lcd.setCursor(7, 3);
//     lcd.print("0=Prev");
//   } else {
//     lcd.setCursor(7, 3);
//     lcd.print("      ");  // Clear when no previous page
//   }
  
//   lcd.setCursor(14, 3);
//   lcd.print("#=Back");
  
//   currentState = 3;
//   needRefresh = false;
// }

// ///////////////////////////////////////////////////////////// Main Loop ////////////////////////////////////////////////////////////////////////////////////////


// void setup() {
//   Serial.begin(9600);
//   lcd.createChar(0, leftArrow);   // store at index 0
//   lcd.createChar(1, rightArrow);  // store at index 1

//   // Initialize LCD
//   lcd.init();
//   lcd.backlight();
//   lcd.clear();
  
//   // Initialize sample data
//   initializeSampleData();
  
//   // --- Welcome Message ---
//   lcd.clear();

//   // Line 1: Title framed with arrows
//   lcd.setCursor(0, 0);
//   lcd.print("    PharmaMatrix    ");

//   // Line 2: Subtitle centered
//   lcd.setCursor(0, 1);  // adjust for centering on 20 chars
//   lcd.print("  Inventory System");

//   // Line 3: Loading animation
//   lcd.setCursor(0, 2);
//   lcd.print("  Loading data");
//   for (int i = 0; i < 3; i++) {
//     lcd.print(".");
//     delay(500);
//   }

//   // Line 4: Ready message
//   lcd.setCursor(0, 3);
//   lcd.print("  System Ready!");

//   delay(2000); // pause before menu
  
//   needRefresh = true;

//   Serial.println("🚀 PharmaMatrix System Started!");
  
// }



// void loop() {

//   // ultrasonic code
//   checkShelfStatus();

//   char key = keypad.getKey();
  
//   // Handle key input
//   if (key) {
//     Serial.print("Key: ");
//     Serial.println(key);
    
//     switch(currentState) {
//       case 0: // Main Menu
//         if (key == '1') {
//           inputBuffer = "";
//           displayAddMedicine();
//         } else if (key == '2') {
//           currentPage = 0;
//           displayOrderMenu();
//         } else if (key == '3') {
//           currentPage = 0;
//           displayStockMenu();
//         }
//         break;
        
//       case 1: // Add Medicine (barcode entry)
//         if (key >= '0' && key <= '9') {
//           if (inputBuffer.length() < 12) {
//             inputBuffer += key;
//             needRefresh = true;
//           }
//         } else if (key == '#') {
//           // Back to main menu
//           displayMainMenu();
//         } else if (key == '*') {
//           // Check barcode - USING '*' AS CONFIRM
//           if (inputBuffer.length() > 0) {
//             int medIndex = findMedicineByBarcode(inputBuffer);
//             if (medIndex != -1) {
//               selectedMedicineIndex = medIndex;
//               inputBuffer = "";
//               displayAddQuantity(medIndex);
//             } else {
//               showMessage("Not found!", "Cannot add new", "medicine", "Press any key", 3000);
//               displayMainMenu();
//             }
//           }
//         }
//         // REMOVED: '0' for clear function
//         break;
        
//       case 2: // Order Menu
//         if (key == '*' && currentPage < ((medicineCount + 1) / 2) - 1) {
//           currentPage++;
//           needRefresh = true;
//         } else if (key == '0' && currentPage > 0) {
//           currentPage--;
//           needRefresh = true;
//         } else if (key == '#') {
//           // Back to main menu
//           displayMainMenu();
//         } else if (key == '1' || key == '2') {
//           int selection = key - '1';
//           selectedMedicineIndex = (currentPage * 2) + selection;
//           if (selectedMedicineIndex < medicineCount) {
//             inputBuffer = "";
//             displayOrderQuantity();
//           }
//         }
//         break;
        
//       case 3: // Stock Menu
//         if (key == '*' && currentPage < ((medicineCount + 1) / 2) - 1) {
//           currentPage++;
//           needRefresh = true;
//         } else if (key == '0' && currentPage > 0) {
//           currentPage--;
//           needRefresh = true;
//         } else if (key == '#') {
//           // Back to main menu
//           displayMainMenu();
//         }
//         break;
        
//       case 4: // Order Quantity
//         if (key >= '0' && key <= '9') {
//           if (inputBuffer.length() < 3) {
//             inputBuffer += key;
//             needRefresh = true;
//           }
//         } else if (key == '#') {
//           // Back to order menu
//           currentPage = 0;
//           displayOrderMenu();
//         } else if (key == '*') {
//           // Confirm order
//           if (inputBuffer.length() > 0) {
//             int orderQty = inputBuffer.toInt();
            
//             if (orderQty <= 0) {
//               showMessage("Error!", "Qty must be >0", "", "Press any key", 3000);
//               inputBuffer = "";
//               needRefresh = true;
//             } else if (orderQty > inventory[selectedMedicineIndex].quantity) {
//               showMessage("Error!", "Not enough stock", "Press any key", "", 3000);
//               inputBuffer = "";
//               needRefresh = true;
//             } else {
//               // Process order
//               inventory[selectedMedicineIndex].quantity -= orderQty;
//               showMessage("Order placed!", 
//                          inventory[selectedMedicineIndex].name,
//                          "Qty: " + String(orderQty),
//                          "Remaining: " + String(inventory[selectedMedicineIndex].quantity), 3000);
//               currentPage = 0;
//               displayOrderMenu();
//             }
//           }
//         }
//         break;
        
//       case 6: // Add Quantity
//         if (key >= '0' && key <= '9') {
//           if (inputBuffer.length() < 3) {
//             inputBuffer += key;
//             needRefresh = true;
//           }
//         } else if (key == '#') {
//           // Back to add medicine screen
//           inputBuffer = "";
//           displayAddMedicine();
//         } else if (key == '*') {
//           // Confirm add
//           if (inputBuffer.length() > 0) {
//             int addQty = inputBuffer.toInt();
//             if (addQty > 0) {
//               inventory[selectedMedicineIndex].quantity += addQty;
//               showMessage("Added!", 
//                          inventory[selectedMedicineIndex].name,
//                          "New total: " + String(inventory[selectedMedicineIndex].quantity),
//                          "", 3000);
//               displayMainMenu();
//             }
//           }
//         }
//         break;
//     }
//   }
  
//   // Refresh display if needed
//   switch(currentState) {
//     case 0: displayMainMenu(); break;
//     case 1: displayAddMedicine(); break;
//     case 2: displayOrderMenu(); break;
//     case 3: displayStockMenu(); break;
//     case 4: displayOrderQuantity(); break;
//     case 6: displayAddQuantity(selectedMedicineIndex); break;
//   }
  
//   delay(50);
// }

#include "ultrasonic.h"
#include "shelf.h"
#include "lcd_keypad.h"
#include "inventory.h"
#include "display_manager.h"
#include "input_handler.h"

// Ultrasonic sensors
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

void displayWelcomeMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("    PharmaMatrix    ");
  lcd.setCursor(0, 1);
  lcd.print("  Inventory System");
  lcd.setCursor(0, 2);
  lcd.print("  Loading data");
  for (int i = 0; i < 3; i++) {
    lcd.print(".");
    delay(500);
  }
  lcd.setCursor(0, 3);
  lcd.print("  System Ready!");
  delay(2000);
}

void setup() {
  Serial.begin(9600);
  
  // Initialize hardware
  initializeLCD();
  initializeCustomChars();
  initializeKeypad();
  
  // Initialize data
  initializeSampleData();
  
  // Display welcome
  displayWelcomeMessage();
  
  needRefresh = true;
  Serial.println("🚀 PharmaMatrix System Started!");
}

void loop() {
  // Check shelf status
  checkShelfStatus();
  
  // Handle keypad input
  char key = keypad.getKey();
  if (key) {
    handleKeypadInput(key);
  }
  
  // Refresh display if needed
  refreshCurrentDisplay();
  
  delay(50);
}