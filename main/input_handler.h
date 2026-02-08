#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "display_manager.h"

// Function declarations
void handleKeypadInput(char key);
void handleMainMenu(char key);
void handleAddMedicine(char key);
void handleOrderMenu(char key);
void handleStockMenu(char key);
void handleOrderQuantity(char key);
void handleAddQuantity(char key);
void refreshCurrentDisplay();

#endif