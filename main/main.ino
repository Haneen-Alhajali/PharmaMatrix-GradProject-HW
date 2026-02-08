#include <AccelStepper.h>
#include <Servo.h>
#include "lcd_keypad.h"
#include "inventory.h"
#include "display_manager.h"
#include "input_handler.h"

/* ================= SYSTEM GLOBAL STATE ================= */

bool systemBusy = false;
bool operationDone = false;

int systemMode = 0;        // 1 = ADD, 2 = REQUEST
int systemQuantity = 0;
int selectedColumn = 1;
int pendingMedicineIndex = -1;


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

/* ===================== SHARED PINS ===================== */
#define DIR_LEFT     30
#define STEP_LEFT    31
#define LIMIT_LEFT   8

#define DIR_RIGHT    22
#define STEP_RIGHT   23
#define LIMIT_RIGHT  9

#define DIR_MIDDLE   32
#define STEP_MIDDLE  33
#define LIMIT_MIDDLE 53

#define DIR_BASE     26
#define STEP_BASE    27
#define LIMIT_BASE_HOME   10   // Cabinet side (HOME)
#define LIMIT_BASE_RAIL   11   // Med rail side

#define SERVO_PIN    52
#define GRIPPER_LIMIT  50  

/* ===================== ULTRASONIC PINS ===================== */
#define A1_TRIG     4
#define A1_ECHO     5
#define STOCK_TRIG  6
#define STOCK_ECHO  7
#define MED_TRIG    2
#define MED_ECHO    3
/* ===================== RAIL PINS ===================== */
// -------------------- Motor 1 (Medicine Rail) --------------------
#define M1_DIR 12
#define M1_STEP 13
#define M1_L1 36
#define M1_L2 38

// -------------------- Motor 2 (Arm Rail) --------------------
#define M2_DIR 24
#define M2_STEP 25
#define M2_L1 42
#define M2_L2 44

/* ===================== BASE MOTION SETTINGS ===================== */
const int BASE_STEP_DELAY_US = 200;   // Controls speed (lower = faster)
const int BASE_DEBOUNCE_MS   = 5;     // Limit switch debounce time
const int BASE_SETTLE_MS     = 200;   // Delay after reaching limit
long BASE_STEPS_TO_MED_RAIL = 5800;   // movement needed to reach med rail

/* ===================== BASE DIRECTIONS ===================== */
const bool BASE_DIR_TO_HOME = HIGH;
const bool BASE_DIR_TO_RAIL = LOW;

/* ===================== OBJECTS ===================== */
AccelStepper leftMotor   (AccelStepper::DRIVER, STEP_LEFT, DIR_LEFT);
AccelStepper rightMotor  (AccelStepper::DRIVER, STEP_RIGHT, DIR_RIGHT);
AccelStepper middleMotor (AccelStepper::DRIVER, STEP_MIDDLE, DIR_MIDDLE);
AccelStepper baseMotor   (AccelStepper::DRIVER, STEP_BASE, DIR_BASE);

AccelStepper rail1(AccelStepper::DRIVER, M1_STEP, M1_DIR);
AccelStepper rail2(AccelStepper::DRIVER, M2_STEP, M2_DIR);

Servo gripper;

/* ===================== SETTINGS ===================== */
const int ARM_SPEED  = 400;
const int ARM_ACCEL  = 200;
const int BASE_SPEED = 50000;
const int BASE_ACCEL = 1000;

const int GRIP_OPEN  = 100;
const int GRIP_CLOSE = 180;

int speed1 = 600;
int speed2 = 600;

/* ===================== ULTRASONIC SETTINGS ===================== */
const int A1_EMPTY_DISTANCE     = 9;
const int STOCK_EMPTY_DISTANCE  = 17;

/* ===================== BASE POSITIONS ===================== */
long BASE_TO_CABINET = 0;
long BASE_TO_RAIL    = -6200;

/* ===================== ARM RAIL POSITION ===================== */
// long ARM_RAIL_PICK_DROP = 1250;
long armMid = 670;   // mid position for arm rail
long ARM_RAIL_PICK_DROP = 1000;

/* ===================== SAFETY ===================== */
long CLEARANCE_MOVE = 5;
// int selectedColumn = 1; // global selected column

/* ===================== HELPERS ===================== */
bool atHome(int pin) {
  return digitalRead(pin) == LOW;
}

/* ===================== ULTRASONIC HELPERS ===================== */
long readUltrasonicCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return 0;
  return duration * 0.034 / 2;
}

bool isA1Empty() {
  return readUltrasonicCM(A1_TRIG, A1_ECHO) >= A1_EMPTY_DISTANCE;
}

bool isA1Full() {
  return readUltrasonicCM(A1_TRIG, A1_ECHO) <= 2;
}

bool isStockEmpty() {
  return readUltrasonicCM(STOCK_TRIG, STOCK_ECHO) >= STOCK_EMPTY_DISTANCE;
}

bool isStockFull() {
  return readUltrasonicCM(STOCK_TRIG, STOCK_ECHO) <= 6;
}

/* ===================== MED RAIL ULTRASONIC ===================== */
int addPickRetries = 0;
int reqDropRetries = 0;
int reqPickRetries = 0;
const int MAX_RETRIES = 2;
bool operationFailed = false;

const int MED_PRESENT_DISTANCE = 8;   // <=10 cm means medicine exists

bool isMedOnRail() {
  long d = readUltrasonicCM(MED_TRIG, MED_ECHO);
  return (d > 0 && d <= MED_PRESENT_DISTANCE);
}

/* ===================== RAIL HELPERS ===================== */
void waitRail2Stop() {
  while (rail2.distanceToGo() != 0) {
    rail2.run();
  }
}

// ------------------ Homing for any rail ------------------
void homeRail(AccelStepper &motor, int limitPin, int speed) {
  motor.setSpeed(-speed);
  while (digitalRead(limitPin) == HIGH) motor.runSpeed();
  motor.stop();
  motor.setCurrentPosition(0);
}

// ------------------ Move arm to mid ------------------
void goToMid() {
  homeRail(rail2, M2_L1, speed2);
  waitRail2Stop();
  rail2.moveTo(+armMid);
  while (rail2.distanceToGo() != 0) rail2.run();
}

void goToPICK_DROP() {
    homeRail(rail2, M2_L1, speed2);
    waitRail2Stop();
    rail2.moveTo(ARM_RAIL_PICK_DROP);
    while (rail2.distanceToGo() != 0) rail2.run();
}

// ------------------ Move rail to end (limit2) ------------------
void goToEnd(AccelStepper &motor, int limitPin, int speed) {
  motor.setSpeed(speed);
  while (digitalRead(limitPin) == HIGH) motor.runSpeed();
  motor.stop();
}

// ------------------ clear the Serial buffer ------------------
void clearSerialBuffer() {
  while (Serial.available()) Serial.read();
}

// ------------------ Move rail2 to column ------------------
void moveRail2ToColumn(int col) {
  if (col == 1) {
    homeRail(rail2, M2_L1, speed2);
    waitRail2Stop();
  } else if (col == 2) {
    goToMid();
  } else if (col == 3) {
    goToEnd(rail2, M2_L2, speed2);
  }
}


bool isGripperPressed() {
  return digitalRead(GRIPPER_LIMIT) == LOW;
}





/* ===================== NEW BASE CODE ===================== */

/* ===================== BASE GENERIC MOVE UNTIL LIMIT ===================== */
void BaseMoveUntilLimit(bool direction, int limitPin) {
  digitalWrite(DIR_BASE, direction);

  while (true) {

    digitalWrite(STEP_BASE, HIGH);
    delayMicroseconds(BASE_STEP_DELAY_US);
    digitalWrite(STEP_BASE, LOW);
    delayMicroseconds(BASE_STEP_DELAY_US);

    // Debounced limit detection
    if (digitalRead(limitPin) == LOW) {
      delay(BASE_DEBOUNCE_MS);
      if (digitalRead(limitPin) == LOW) {
        break;   // confirmed limit hit
      }
    }
  }

  delay(BASE_SETTLE_MS); // settle
}

/* ===================== MOVE TO HOME ===================== */
void baseGoToHome() {
  Serial.println("Moving to HOME limit...");
  BaseMoveUntilLimit(BASE_DIR_TO_HOME, LIMIT_BASE_HOME);
  Serial.println("Reached HOME position");
}

/* ===================== MOVE TO MED RAIL ===================== */
void baseGoToMedRail() {
  // Serial.println("Moving to MED RAIL limit...");
  // BaseMoveUntilLimit(BASE_DIR_TO_RAIL, LIMIT_BASE_RAIL);
  // Serial.println("Reached MED RAIL position");
    Serial.println("Moving to MED RAIL limit...");
    if (digitalRead(LIMIT_BASE_HOME) == HIGH){      Serial.println("home base first here");
  baseGoToHome(); }
    digitalWrite(DIR_BASE, BASE_DIR_TO_RAIL);


    for (long i = 0; i < BASE_STEPS_TO_MED_RAIL; i++) {
      digitalWrite(STEP_BASE, HIGH);
      delayMicroseconds(BASE_STEP_DELAY_US);
      digitalWrite(STEP_BASE, LOW);
      delayMicroseconds(BASE_STEP_DELAY_US);
    }

    delay(BASE_SETTLE_MS);
    Serial.println("Reached MED RAIL position");


}


/* ========================================================= */
/* ===================== REQUEST ARM ======================= */
/* ========================================================= */

/* ===================== PICK POSITIONS ===================== */
long TEMP_PICK_LEFT   = 0;
long TEMP_PICK_RIGHT  = 80;
// long TEMP_PICK_MIDDLE = 500;
long TEMP_PICK_MIDDLE = 900;

long PICK_LEFT   = TEMP_PICK_LEFT;
long PICK_RIGHT  = TEMP_PICK_RIGHT;
long PICK_MIDDLE = TEMP_PICK_MIDDLE;
// long REQ_PICK_RIGHT_EXTRA  = 0;
// long PICK_MIDDLE = 550;

/* ===================== DROP POSITIONS ===================== */
long DROP_LEFT   = 0;
long DROP_RIGHT  = 0;
long DROP_MIDDLE = 600;


/* ===================== STOCK PICK POSITIONS ===================== */
long STOCK_PICK_LEFT   = 0;
long STOCK_PICK_RIGHT  = 0;
long STOCK_PICK_MIDDLE = 950;
long STOCK_PICK_RIGHT_EXTRA  = 0;

/* ===================== STATES ===================== */
enum ReqState {
  REQ_SAFE_HOMING,
  REQ_HOME_MIDDLE,
  REQ_HOME_LEFT,
  REQ_HOME_RIGHT,
  REQ_OPEN_GRIP,
  REQ_MOVE_TO_PICK_LR,
  REQ_MOVE_TO_PICK_MIDDLE,

//   // New states for extra moves after pick stock 
  MOVE_RIGHT_EXTRA,
  //   // New states for extra moves after pick req
  REQ_MOVE_RIGHT_EXTRA,

  REQ_GRIP_MED,
  REQ_RETURN_MIDDLE,
  REQ_RETURN_LR,
  REQ_ROTATE_TO_RAIL,
  REQ_MOVE_TO_DROP,
  REQ_RELEASE,
  REQ_VERIFY_DROP, // <--- NEW STATE
  REQ_RETURN_MIDDLE_2,
  REQ_ROTATE_BACK,
  REQ_DONE
};

ReqState reqState;
bool requestArmDone = false;
unsigned long reqTimer = 0;

bool stockRequestDone = false;

void startStockRequestArm() {
  PICK_LEFT   = STOCK_PICK_LEFT;
  PICK_RIGHT  = STOCK_PICK_RIGHT;
  PICK_MIDDLE = STOCK_PICK_MIDDLE;
      Serial.println(PICK_RIGHT);

  stockRequestDone = false;
  startRequestArm();
}

void runStockRequestArm() {
  PICK_LEFT   = TEMP_PICK_LEFT;
  PICK_RIGHT  = TEMP_PICK_RIGHT;
  PICK_MIDDLE = TEMP_PICK_MIDDLE;
  runRequestArm();
  stockRequestDone = requestArmDone;
}

void startRequestArm() {
  reqState = REQ_SAFE_HOMING;
  requestArmDone = false;
  reqDropRetries = 0;
  reqPickRetries = 0;
  operationDone = false;
  operationFailed = false;
}

void runRequestArm() {
  switch (reqState) {

    case REQ_SAFE_HOMING:
      if (leftMotor.distanceToGo() == 0 &&
          rightMotor.distanceToGo() == 0) {
        middleMotor.setSpeed(-ARM_SPEED);
        reqState = REQ_HOME_MIDDLE;
      }
      break;

    case REQ_HOME_MIDDLE:
      if (atHome(LIMIT_LEFT)) leftMotor.move(CLEARANCE_MOVE);
      if (atHome(LIMIT_RIGHT)) rightMotor.move(CLEARANCE_MOVE);
      leftMotor.run();
      rightMotor.run();
      if (!atHome(LIMIT_LEFT) && !atHome(LIMIT_RIGHT)) {
        if (!atHome(LIMIT_MIDDLE)) middleMotor.runSpeed();
        else {
          middleMotor.setCurrentPosition(0);
          leftMotor.setSpeed(-ARM_SPEED);
          reqState = REQ_HOME_LEFT;
        }
      }
      break;

    case REQ_HOME_LEFT:
      if (!atHome(LIMIT_LEFT)) leftMotor.runSpeed();
      else {
        leftMotor.setCurrentPosition(0);
        rightMotor.setSpeed(-ARM_SPEED);
        reqState = REQ_HOME_RIGHT;
      }
      break;

    case REQ_HOME_RIGHT:
      if (!atHome(LIMIT_RIGHT)) rightMotor.runSpeed();
      else {
        rightMotor.setCurrentPosition(0);
        reqTimer = millis();
        reqState = REQ_OPEN_GRIP;
        // Move rail2 to column 
        moveRail2ToColumn(selectedColumn);
      }
      break;

    /* -------- PICK MEDICINE -------- */
    case REQ_OPEN_GRIP:
      gripper.write(GRIP_OPEN);
      delay(1000);
      if (operationFailed) {
      // final failure: full shutdown sequence
      baseGoToHome();
      homeRail(rail2, M2_L1, speed2);
      homeRail(rail1, M1_L1, speed1);
      reqState = REQ_DONE;
      }
      else if (millis() - reqTimer > 500) {
        leftMotor.moveTo(PICK_LEFT);
        rightMotor.moveTo(PICK_RIGHT);
        reqState = REQ_MOVE_TO_PICK_LR;
      }
      break;

    case REQ_MOVE_TO_PICK_LR:
      leftMotor.run();
      rightMotor.run();
      if (leftMotor.distanceToGo() == 0 && rightMotor.distanceToGo() == 0) {
        middleMotor.moveTo(PICK_MIDDLE);
        reqState = REQ_MOVE_TO_PICK_MIDDLE;
      }
      break;

    case REQ_MOVE_TO_PICK_MIDDLE:
      middleMotor.run();
      if (middleMotor.distanceToGo() == 0) {
        reqTimer = millis();
        if(PICK_LEFT == STOCK_PICK_LEFT){
            Serial.println("here extra");

          rightMotor.moveTo(STOCK_PICK_RIGHT_EXTRA);
          // reqState = MOVE_RIGHT_EXTRA;          
          reqState = REQ_GRIP_MED;          

        }else{
        reqState = REQ_GRIP_MED;
        }
      }
      break;

    case MOVE_RIGHT_EXTRA:
                Serial.println("here2 insied extra");

      rightMotor.run();
      if (rightMotor.distanceToGo() == 0) {
        reqTimer = millis();
        reqState = REQ_GRIP_MED;
      }
      break;

    case REQ_GRIP_MED:
      gripper.write(GRIP_CLOSE);
      if (isGripperPressed()) {//&& millis() - reqTimer > 500
        middleMotor.moveTo(0);
        reqState = REQ_RETURN_MIDDLE;
      }
      else if (millis() - reqTimer > 5000) {

        if (reqPickRetries >= MAX_RETRIES) {
          Serial.println("ERROR: REQUEST failed - medicine not Picked");
          showMessage("ERROR:"," REQUEST failed");
          operationFailed = true;

          reqState = REQ_SAFE_HOMING; ////////// hereeeee
        }else{
        Serial.println("ERROR: Gripper failed to grab medicine now retry");
        reqPickRetries++;
        reqState = REQ_SAFE_HOMING;
        Serial.println("Retry picking medicine");
        }
      }
      // if (millis() - reqTimer > 500) {
      //   middleMotor.moveTo(0);
      //   reqState = REQ_RETURN_MIDDLE;
      // }
      break;

    /* -------- RETURN WITH MED -------- */
    case REQ_RETURN_MIDDLE:
      middleMotor.run();
      if (middleMotor.distanceToGo() == 0) {
        leftMotor.moveTo(0);
        rightMotor.moveTo(0);
        reqState = REQ_RETURN_LR;
      }
      break;

    case REQ_RETURN_LR:
      leftMotor.run();
      rightMotor.run();
      if (leftMotor.distanceToGo() == 0 && rightMotor.distanceToGo() == 0) {
        // baseMotor.moveTo(BASE_TO_RAIL);
        reqState = REQ_ROTATE_TO_RAIL;
        goToPICK_DROP();
      }
      break;

    /* -------- DROP MEDICINE -------- */
    case REQ_ROTATE_TO_RAIL:
      // baseMotor.run();
      // if (baseMotor.distanceToGo() == 0) {
        
        baseGoToMedRail();
        leftMotor.moveTo(DROP_LEFT);
        rightMotor.moveTo(DROP_RIGHT);
        middleMotor.moveTo(DROP_MIDDLE);
        reqState = REQ_MOVE_TO_DROP;
      // }
      break;

    case REQ_MOVE_TO_DROP:
      leftMotor.run();
      rightMotor.run();
      middleMotor.run();
      if (leftMotor.distanceToGo() == 0 &&
          rightMotor.distanceToGo() == 0 &&
          middleMotor.distanceToGo() == 0) {
        reqTimer = millis();
        reqState = REQ_RELEASE;
      }
      break;

    case REQ_RELEASE:
      gripper.write(GRIP_OPEN);
      if (millis() - reqTimer > 500) {
        middleMotor.moveTo(0);
        reqState = REQ_RETURN_MIDDLE_2;
      }
      break;

    /* -------- RESET -------- */
    // case REQ_RETURN_MIDDLE_2:
    //   middleMotor.run();
    //   if (middleMotor.distanceToGo() == 0) {
    //     baseMotor.moveTo(BASE_TO_CABINET);
    //     reqState = REQ_ROTATE_BACK;
    //   }
    //   break;

    case REQ_RETURN_MIDDLE_2:
      middleMotor.run();
      if (middleMotor.distanceToGo() == 0) {
        reqState = REQ_VERIFY_DROP;
      }
      break;
    
    case REQ_VERIFY_DROP:

      if (!isMedOnRail()) {
        // Drop failed
        reqDropRetries++;

        if (reqDropRetries >= MAX_RETRIES) {
          Serial.println("ERROR: REQUEST failed - medicine not dropped");
          showMessage("ERROR:"," REQUEST failed");
          operationFailed = true;
          // requestArmDone = true;
          // return;
          // baseMotor.moveTo(BASE_TO_CABINET);
          if (digitalRead(LIMIT_BASE_HOME) == HIGH){    baseGoToHome(); }
          reqState = REQ_ROTATE_BACK;
          homeRail(rail2, M2_L1, speed2);
        }else{
          
          Serial.println("Retry dropping medicine");

          gripper.write(GRIP_CLOSE);
          leftMotor.moveTo(DROP_LEFT);
          rightMotor.moveTo(DROP_RIGHT);
          middleMotor.moveTo(DROP_MIDDLE);
          reqState = REQ_MOVE_TO_DROP;
        }
      }
      else {
        // Drop success
        reqDropRetries = 0;
        // baseMotor.moveTo(BASE_TO_CABINET);
        reqState = REQ_ROTATE_BACK;
      }
      break;

    case REQ_ROTATE_BACK:
      // baseMotor.run();
      // if (baseMotor.distanceToGo() == 0) {
       if (digitalRead(LIMIT_BASE_HOME) == HIGH){    baseGoToHome(); }
        reqState = REQ_DONE;
      // }
      break;

    case REQ_DONE:
      requestArmDone = true;
      break;
  }
}

/* ========================================================= */
/* ===================== ADD ARM =========================== */
/* ========================================================= */

/* ===================== RAIL PICK ===================== */
long RAIL_PICK_LEFT   = 0;
long RAIL_PICK_RIGHT  = 0;
long RAIL_PICK_MIDDLE = 700;
long extraRightPos = -110;  // position for the extra right move
long extraMiddlePos = 930; // position for the extra middle move

/* ===================== SHELF SAFE MOVES ===================== */
long SHELF_R1_MOVE = 130;
long SHELF_L1_MOVE = 0;
long SHELF_R2_MOVE = 0;
long SHELF_R3_MOVE = 0;
long SHELF_R_BACK  = 0;
long SHELF_M_DOWN  = 800;

/* ===================== STATES ===================== */
enum HomingMode {
  HOME_STARTUP,
  HOME_AFTER_PICK,
  HOME_AFTER_PICK_AND_ROTATE,
  HOME_FINAL
};

/* ===== Shelf mini-sequence ===== */
enum ShelfStep {
  STEP_R1,
  STEP_L1,
  STEP_R2,
  STEP_R3,
  STEP_L_HOME,
  STEP_R_BACK,
  STEP_M_DOWN,
  STEP_RELEASE
};

enum AddState {
  ADD_SAFE_HOMING,
  ADD_HOME_MIDDLE,
  ADD_HOME_LEFT,
  ADD_HOME_RIGHT,
  ADD_ROTATE_TO_RAIL,
  ADD_MOVE_TO_RAIL_PICK,
  ADD_GRIP,
  // New states for extra moves after pick
  ADD_MOVE_RIGHT_EXTRA,
  ADD_MOVE_MIDDLE_EXTRA,
  ADD_VERIFY_PICK, // <--- NEW STATE
  ADD_FAIL_ROTATE_BASE,   // <--- NEW
  ADD_MOVE_RAIL2_TO_COLUMN, // <--- NEW STATE
  ADD_ROTATE_TO_SHELF,
  ADD_MOVE_TO_SHELF_DROP,
  ADD_RELEASE,
  ADD_DONE
};

AddState addState;
bool addArmDone = false;
unsigned long addTimer = 0;
HomingMode homingMode = HOME_STARTUP;
ShelfStep shelfStep = STEP_R1;

void startAddArm() {
  addState = ADD_SAFE_HOMING;
  addArmDone = false;
  homingMode = HOME_STARTUP;
  shelfStep = STEP_R1;
  addPickRetries = 0;
  operationDone = false;
  operationFailed = false;
}

void runAddArm() {
  switch (addState) {

    case ADD_SAFE_HOMING:
      if (homingMode == HOME_STARTUP || homingMode == HOME_FINAL)
        gripper.write(GRIP_OPEN);

      middleMotor.setSpeed(-ARM_SPEED);
      addState = ADD_HOME_MIDDLE;
      break;

    case ADD_HOME_MIDDLE:
      if (atHome(LIMIT_LEFT)) leftMotor.move(CLEARANCE_MOVE);
      if (atHome(LIMIT_RIGHT)) rightMotor.move(CLEARANCE_MOVE);
      leftMotor.run();
      rightMotor.run();
      if (!atHome(LIMIT_LEFT) && !atHome(LIMIT_RIGHT)) {
        if (!atHome(LIMIT_MIDDLE)) middleMotor.runSpeed();
        else {
          middleMotor.setCurrentPosition(0);
          leftMotor.setSpeed(-ARM_SPEED);
          addState = ADD_HOME_LEFT;
        }
      }
      break;

    case ADD_HOME_LEFT:
      if (!atHome(LIMIT_LEFT)) leftMotor.runSpeed();
      else {
        leftMotor.setCurrentPosition(0);
        rightMotor.setSpeed(-ARM_SPEED);
        addState = ADD_HOME_RIGHT;
      }
      break;

    case ADD_HOME_RIGHT:
      if (!atHome(LIMIT_RIGHT)) rightMotor.runSpeed();
      else {
        rightMotor.setCurrentPosition(0);

        if (homingMode == HOME_STARTUP) {
          // baseMotor.moveTo(BASE_TO_RAIL);
          addState = ADD_ROTATE_TO_RAIL;
          // Serial.println(operationFailed);
          if (operationFailed) {
            // final failure: full shutdown sequence
    if (digitalRead(LIMIT_BASE_HOME) == HIGH){    baseGoToHome(); }
            Serial.println("here entered faild");
            homeRail(rail2, M2_L1, speed2);
            homeRail(rail1, M1_L1, speed1);
            addState = ADD_DONE;
            }
        }
        // else if (homingMode == HOME_AFTER_PICK) {
        //   baseMotor.moveTo(BASE_TO_CABINET);
        //   shelfStep = STEP_R1;
        //   addState = ADD_ROTATE_TO_SHELF;
        // }
        else if (homingMode == HOME_AFTER_PICK) {
          // addState = ADD_VERIFY_PICK;
          // baseMotor.moveTo(BASE_TO_CABINET);
          shelfStep = STEP_R1;
          addState = ADD_ROTATE_TO_SHELF;
        }
        else if (homingMode ==   HOME_AFTER_PICK_AND_ROTATE) {
          addState = ADD_MOVE_TO_SHELF_DROP;
        }
        else addState = ADD_DONE;
      }
      break;

    /* -------- PICK FROM RAIL -------- */
    case ADD_ROTATE_TO_RAIL:
      // baseMotor.run();
      // if (baseMotor.distanceToGo() == 0) {
        baseGoToMedRail();
        leftMotor.moveTo(RAIL_PICK_LEFT);
        rightMotor.moveTo(RAIL_PICK_RIGHT);
        middleMotor.moveTo(RAIL_PICK_MIDDLE);
        addState = ADD_MOVE_TO_RAIL_PICK;
      // }
      break;

    case ADD_MOVE_TO_RAIL_PICK:
      leftMotor.run();
      rightMotor.run();
      middleMotor.run();
      if (leftMotor.distanceToGo() == 0 &&
          rightMotor.distanceToGo() == 0 &&
          middleMotor.distanceToGo() == 0) {
        addTimer = millis();
        // addState = ADD_GRIP;
        rightMotor.moveTo(extraRightPos);    // move right to new extra position
        middleMotor.moveTo(extraMiddlePos);  // move middle to new extra position (will execute after right)
        addState = ADD_MOVE_RIGHT_EXTRA;
      }
      break;

    // case ADD_GRIP:
    //   gripper.write(GRIP_CLOSE);
    //   if (millis() - addTimer > 500) {
    //     homingMode = HOME_AFTER_PICK;
    //     addState = ADD_SAFE_HOMING;
    //   }
    //   break;

    case ADD_MOVE_RIGHT_EXTRA:
      rightMotor.run();
      if (rightMotor.distanceToGo() == 0) {
        middleMotor.moveTo(extraMiddlePos);
        addState = ADD_MOVE_MIDDLE_EXTRA;
      }
      break;

    case ADD_MOVE_MIDDLE_EXTRA:
      middleMotor.run();
      if (middleMotor.distanceToGo() == 0) {
        addState = ADD_GRIP;
      }
      break;

    case ADD_GRIP:
      gripper.write(GRIP_CLOSE);
        delay(10000); 
      if (isGripperPressed()) {
        homingMode = HOME_AFTER_PICK;
        addState = ADD_SAFE_HOMING;
      }
      else if (millis() - addTimer > 2000) {
        Serial.println("ERROR: Gripper failed to grab medicine");
        // operationFailed = true;
        // addState = ADD_DONE;
        addState = ADD_VERIFY_PICK;
      }
      // if (millis() - addTimer > 500) {
        // homingMode = HOME_AFTER_PICK;
        // addState = ADD_SAFE_HOMING;
      // }
      break;

    case ADD_VERIFY_PICK:

      if (isMedOnRail()) {
        // Pick failed
        addPickRetries++;

        if (addPickRetries >= MAX_RETRIES) {
          Serial.println("ERROR: ADD failed - medicine not picked");
          showMessage("ERROR:"," ADD failed");
          operationFailed = true;
          // addArmDone = true;
          // return;
          homingMode = HOME_STARTUP;
          addState = ADD_SAFE_HOMING;
          // baseMotor.moveTo(BASE_TO_CABINET);
          // addState = ADD_FAIL_ROTATE_BASE;
        }
        else{
          Serial.println("Retry picking medicine from rail");

          gripper.write(GRIP_OPEN);
          homingMode = HOME_STARTUP;
          addState = ADD_SAFE_HOMING;
        }
      }
      else {
        // Pick success
        addPickRetries = 0;
        // baseMotor.moveTo(BASE_TO_CABINET);
        // shelfStep = STEP_R1;
        // addState = ADD_ROTATE_TO_SHELF;
        homingMode = HOME_AFTER_PICK;
        addState = ADD_SAFE_HOMING;
      }
      break;

      case ADD_FAIL_ROTATE_BASE:
        // baseMotor.run();

        // if (baseMotor.distanceToGo() == 0) {
    if (digitalRead(LIMIT_BASE_HOME) == HIGH){    baseGoToHome(); }
          homeRail(rail2, M2_L1, speed2);

          addTimer = millis();
          addState = ADD_RELEASE;
        // }
        break;

    case ADD_MOVE_RAIL2_TO_COLUMN:
      moveRail2ToColumn(selectedColumn);
      addState = ADD_MOVE_TO_SHELF_DROP;
      break;

    /* -------- DROP TO SHELF -------- */
    case ADD_ROTATE_TO_SHELF:
      // baseMotor.run();
      // if (baseMotor.distanceToGo() == 0) {
    if (digitalRead(LIMIT_BASE_HOME) == HIGH){    baseGoToHome(); }
        addState = ADD_MOVE_RAIL2_TO_COLUMN; // move rail2 before shelf
      // }
      break;

    case ADD_MOVE_TO_SHELF_DROP:
      switch (shelfStep) {

        case STEP_R1:
          rightMotor.move(SHELF_R1_MOVE);
          shelfStep = STEP_L1;
          break;

        case STEP_L1:
          if (rightMotor.distanceToGo() != 0) rightMotor.run();
          else {
            leftMotor.move(SHELF_L1_MOVE);
            shelfStep = STEP_R2;
          }
          break;

        case STEP_R2:
          if (leftMotor.distanceToGo() != 0) leftMotor.run();
          else {
            rightMotor.move(SHELF_R2_MOVE);
            shelfStep = STEP_R3;
          }
          break;

        case STEP_R3:
          if (rightMotor.distanceToGo() != 0) rightMotor.run();
          else {
            rightMotor.move(SHELF_R3_MOVE);
            shelfStep = STEP_L_HOME;
          }
          break;

        case STEP_L_HOME:
          if (!atHome(LIMIT_LEFT)) {
            leftMotor.setSpeed(-ARM_SPEED);
            leftMotor.runSpeed();
          } else {
            leftMotor.setCurrentPosition(0);
            shelfStep = STEP_R_BACK;
          }
          break;

        case STEP_R_BACK:
          rightMotor.move(SHELF_R_BACK);
          shelfStep = STEP_M_DOWN;
          break;

        case STEP_M_DOWN:
          if (rightMotor.distanceToGo() != 0) rightMotor.run();
          else {
            middleMotor.move(SHELF_M_DOWN);
            shelfStep = STEP_RELEASE;
          }
          break;

        case STEP_RELEASE:
          if (middleMotor.distanceToGo() != 0) middleMotor.run();
          else {
            addTimer = millis();
            addState = ADD_RELEASE;
          }
          break;
      }
      break;


    case ADD_RELEASE:
      gripper.write(GRIP_OPEN);
              delay(20000); 

      if (!isGripperPressed() && millis() - addTimer > 500) {
        homingMode = HOME_FINAL;
        addState = ADD_SAFE_HOMING;
      }
      else if (millis() - addTimer > 2000) {
        Serial.println("ERROR: Gripper failed to grab medicine");
        // operationFailed = true;
        // addState = ADD_DONE;
        addState = ADD_VERIFY_PICK;
      }

      // gripper.write(GRIP_OPEN);
      // if (millis() - addTimer > 500) {
      //   homingMode = HOME_FINAL;
      //   addState = ADD_SAFE_HOMING;
      // }
      break;

    case ADD_DONE:
      addArmDone = true;
      break;
  }
}

/* ========================================================= */
/* ============== REFILL A1 FROM STOCK ====================== */
/* ========================================================= */

/* ===================== PICK FROM C ===================== */
long C_PICK_LEFT   = 0;    
long C_PICK_RIGHT  = 0;   
long C_PICK_MIDDLE = 950;  
long C_PICK_RIGHT_EXTRA  = 0;

/* ===================== STATES ===================== */
enum StockState {
  STOCK_SAFE_HOMING,
  STOCK_HOME_MIDDLE,
  STOCK_HOME_LEFT,
  STOCK_HOME_RIGHT,

  STOCK_OPEN_GRIP,
  STOCK_MOVE_TO_C_LR,
  STOCK_MOVE_TO_C_MIDDLE,

  //   // New states for extra moves after pick
  STOCK_MOVE_RIGHT_EXTRA,

  STOCK_GRIP_MED,

  STOCK_RETURN_MIDDLE_HOME,
  STOCK_RETURN_LR_HOME,

  STOCK_MOVE_TO_SHELF_DROP,
  STOCK_RELEASE_MED,

  STOCK_FINAL_HOMING,
  STOCK_DONE
};



StockState stockState;
bool stockArmDone = false;
unsigned long stockTimer = 0;

void startStockArm() {
  stockState = STOCK_SAFE_HOMING;    // Use stock arm states
  shelfStep = STEP_R1;
  homingMode = HOME_STARTUP;
  stockArmDone = false;
}

void runStockArm() {
  switch (stockState) {

    /* -------- SAFE HOMING -------- */
    case STOCK_SAFE_HOMING:
      middleMotor.setSpeed(-ARM_SPEED);
      stockState = STOCK_HOME_MIDDLE;
      break;

    case STOCK_HOME_MIDDLE:
      if (atHome(LIMIT_LEFT))  leftMotor.move(CLEARANCE_MOVE);
      if (atHome(LIMIT_RIGHT)) rightMotor.move(CLEARANCE_MOVE);

      leftMotor.run();
      rightMotor.run();

      if (!atHome(LIMIT_LEFT) && !atHome(LIMIT_RIGHT)) {
        if (!atHome(LIMIT_MIDDLE)) middleMotor.runSpeed();
        else {
          middleMotor.setCurrentPosition(0);
          leftMotor.setSpeed(-ARM_SPEED);
          stockState = STOCK_HOME_LEFT;
        }
      }
      break;

    case STOCK_HOME_LEFT:
      if (!atHome(LIMIT_LEFT)) leftMotor.runSpeed();
      else {
        leftMotor.setCurrentPosition(0);
        rightMotor.setSpeed(-ARM_SPEED);
        stockState = STOCK_HOME_RIGHT;
      }
      break;

    case STOCK_HOME_RIGHT:
      if (!atHome(LIMIT_RIGHT)) rightMotor.runSpeed();
      else {
        rightMotor.setCurrentPosition(0);
        stockTimer = millis();
        if (homingMode == HOME_STARTUP){
            stockState = STOCK_OPEN_GRIP;
        } else {
            stockState = STOCK_DONE;
          } 
      }
      break;

    /* -------- PICK FROM STOCK (COL 1) -------- */
    case STOCK_OPEN_GRIP:
                Serial.println("here in open");
      gripper.write(GRIP_OPEN);
      if (millis() - stockTimer > 500) {
        leftMotor.moveTo(C_PICK_LEFT);
        rightMotor.moveTo(C_PICK_RIGHT);
        stockState = STOCK_MOVE_TO_C_LR;
      }
      break;

    case STOCK_MOVE_TO_C_LR:
                            Serial.println("inside the mills here");

      leftMotor.run();
      rightMotor.run();
      if (leftMotor.distanceToGo() == 0 &&
          rightMotor.distanceToGo() == 0) {
                                        Serial.println("inside the ==0 ");

        middleMotor.moveTo(C_PICK_MIDDLE);
        stockState = STOCK_MOVE_TO_C_MIDDLE;
      }
      break;

    case STOCK_MOVE_TO_C_MIDDLE:
                                Serial.println("inside the STOCK_MOVE_TO_C_MIDDLEe");

      middleMotor.run();
      if (middleMotor.distanceToGo() == 0) {
        // stockTimer = millis();
        // stockState = STOCK_GRIP_MED;
        rightMotor.moveTo(C_PICK_RIGHT_EXTRA);
        stockState = STOCK_MOVE_RIGHT_EXTRA;
      }
      break;


    case STOCK_MOVE_RIGHT_EXTRA:
      rightMotor.run();
      if (rightMotor.distanceToGo() == 0) {
        stockTimer = millis();
        stockState = STOCK_GRIP_MED;
      }
      break;




    case STOCK_GRIP_MED:
      gripper.write(GRIP_CLOSE);
      if (isGripperPressed() && millis() - stockTimer > 500) {
        middleMotor.moveTo(0);
        stockState = STOCK_RETURN_MIDDLE_HOME;
      }
      else if (millis() - stockTimer > 2000) {
        Serial.println("ERROR: Gripper failed to grab medicine");
        // operationFailed = true;
        // addState = ADD_DONE;
        homingMode = HOME_STARTUP;
        stockState = STOCK_SAFE_HOMING;
      }


      // if (millis() - stockTimer > 500) {
      //   middleMotor.moveTo(0);
      //   stockState = STOCK_RETURN_MIDDLE_HOME;
      // }
      break;

    /* -------- RETURN WITH MED -------- */
    case STOCK_RETURN_MIDDLE_HOME:
      middleMotor.run();
      if (middleMotor.distanceToGo() == 0) {
        leftMotor.moveTo(0);
        rightMotor.moveTo(0);
        stockState = STOCK_RETURN_LR_HOME;
      }
      break;

    case STOCK_RETURN_LR_HOME:
      leftMotor.run();
      rightMotor.run();
      if (leftMotor.distanceToGo() == 0 &&
          rightMotor.distanceToGo() == 0) {
        // stockArmDone = true; // ready to move rail2
        moveRail2ToColumn(2);
        stockState = STOCK_MOVE_TO_SHELF_DROP;
      }
      break;

    /* -------- DROP TO A1 (SAME SHELF LOGIC) -------- */
    case STOCK_MOVE_TO_SHELF_DROP:
      switch (shelfStep) {
        case STEP_R1:
          rightMotor.move(SHELF_R1_MOVE);
          shelfStep = STEP_L1;
          break;

        case STEP_L1:
          if (rightMotor.distanceToGo() != 0) rightMotor.run();
          else {
            leftMotor.move(SHELF_L1_MOVE);
            shelfStep = STEP_R2;
          }
          break;

        case STEP_R2:
          if (leftMotor.distanceToGo() != 0) leftMotor.run();
          else {
            rightMotor.move(SHELF_R2_MOVE);
            shelfStep = STEP_R3;
          }
          break;

        case STEP_R3:
          if (rightMotor.distanceToGo() != 0) rightMotor.run();
          else {
            rightMotor.move(SHELF_R3_MOVE);
            shelfStep = STEP_L_HOME;
          }
          break;

        case STEP_L_HOME:
          if (!atHome(LIMIT_LEFT)) {
            leftMotor.setSpeed(-ARM_SPEED);
            leftMotor.runSpeed();
          } else {
            leftMotor.setCurrentPosition(0);
            shelfStep = STEP_R_BACK;
          }
          break;

        case STEP_R_BACK:
          rightMotor.move(SHELF_R_BACK);
          shelfStep = STEP_M_DOWN;
          break;

        case STEP_M_DOWN:
          if (rightMotor.distanceToGo() != 0) rightMotor.run();
          else {
            middleMotor.move(SHELF_M_DOWN);
            shelfStep = STEP_RELEASE;
          }
          break;

        case STEP_RELEASE:
          if (middleMotor.distanceToGo() != 0) middleMotor.run();
          else {
            stockTimer = millis();
            stockState = STOCK_RELEASE_MED;
          }
          break;
      }
      break;

    case STOCK_RELEASE_MED:
      gripper.write(GRIP_OPEN);
      if (millis() - stockTimer > 500) {
        stockState = STOCK_SAFE_HOMING; // ready for next pickup
      }
      break;

    case STOCK_DONE:
      stockArmDone = true;
      break;
  }
}

//////////////////////////////// system
// void runAddSequence() {

//   static int step = 0;

//   switch (step) {

//     case 0:
//       homeRail(rail1, M1_L1, speed1);
//       homeRail(rail2, M2_L1, speed2);
//       step++;
//       break;

//     case 1:
//       goToEnd(rail1, M1_L2, speed1);
//       step++;
//       break;

//     case 2:
//       startAddArm();
//       step++;
//       break;

//     case 3:
//       if (!addArmDone) {
//         runAddArm();
//       } else {
//         step = 0;
//         operationDone = true;
//       }
//       break;
//   }
// }

// void runRequestSequence() {

//   static int count = 0;

//   if (count == 0) {
//     homeRail(rail1, M1_L1, speed1);
//     homeRail(rail2, M2_L1, speed2);
//     goToEnd(rail1, M1_L2, speed1);
//   }

//   if (count < systemQuantity) {
//     startRequestArm();
//     if (!requestArmDone) {
//       runRequestArm();
//     } else {
//       count++;
//       homeRail(rail2, M2_L1, speed2);
//     }
//   } else {
//     count = 0;
//     operationDone = true;
//   }
// }




////////// my merge
void runAddSequence() {
     Serial.println("ADD Mode Selected");

      if (selectedColumn == 2 && isA1Full()) {
        Serial.println("A1 Full.");
        if (!isStockFull()) selectedColumn = 1;
        else {
          Serial.println("No free space in A1 or stock.");
          showMessage("No free space");
          operationDone = true;
          return;
        }
      }

    if (digitalRead(LIMIT_BASE_HOME) == HIGH){    baseGoToHome(); }
      // 1) Homing both rails
      homeRail(rail1, M1_L1, speed1);
      homeRail(rail2, M2_L1, speed2);

      // 2) Rail1 waits at home until user places medicine
      showMessage("Please place","the medicine","on the carriage...");
      Serial.println("Waiting for medicine on rail...");

      while (!isMedOnRail()) {
        delay(200); // small debounce delay
      }

      Serial.println("Medicine detected on rail.");
      showMessage("Start adding"," the medicine...");

      // 3) Rail1 moves to END
      goToEnd(rail1, M1_L2, speed1);
      Serial.println("Rail1 at end");

      // 4) Arm moves to PICK_DROP position to take the medicine
      goToPICK_DROP();

      // 5) Arm start adding the medicine 
      startAddArm();
      while (!addArmDone) runAddArm();

      // 6) Rail1 returns to HOME
      homeRail(rail1, M1_L1, speed1);

      // 7) Arm back to home
      homeRail(rail2, M2_L1, speed2);
      if(!operationFailed){
        Serial.println("add Completed.");
        showMessage("ADD Completed");
      }
      operationDone = true;

}

void runRequestSequence() {
    Serial.println("Request Mode Selected");
    showMessage("Start dispensing"," the medicine...");
    if (digitalRead(LIMIT_BASE_HOME) == HIGH){     Serial.println("here to home ");
   baseGoToHome(); }

    // 1) Homing both rails
    // homeRail(rail1, M1_L1, speed1);
    homeRail(rail2, M2_L1, speed2);

    // 2) Rail1 moves to END to receive medicine
    goToEnd(rail1, M1_L2, speed1);

    // ================= A1 EMPTY CASE =================
    if (selectedColumn == 2 && isA1Empty()) {
      Serial.println("A1 empty");

        if (isStockEmpty()) {
          Serial.println("Stock empty.");
          showMessage("Stock empty...");
          operationFailed = true;
          operationDone = true;
          systemBusy = false;
          return;
        }

        // -------- STEP 1: Serve user from STOCK --------
        for (int i = 0; i < systemQuantity; i++) {

          if (isStockEmpty() || isA1Full()) break;
          
          Serial.println("surve user from STOCK");
          selectedColumn = 1;
          // 3) Arm start requesting the medicine from stock
          startStockRequestArm();
          while (!stockRequestDone) runStockRequestArm();

          // 4) Rail2 returns to HOME
          homeRail(rail2, M2_L1, speed2);
        }

        // 5) Rail1 returns to HOME
        homeRail(rail1, M1_L1, speed1);
        if(!operationFailed){
        Serial.println("Retrieve Completed.");
        showMessage("Retrieve Completed.");
        }
      
        // -------- STEP 2: Refill A1 from STOCK --------
        if(!isStockEmpty() && !isA1Full()){
          Serial.println("Refilling A1 from STOCK...");
          showMessage("Refilling the ","shelf from STOCK...");

            // Loop until either stock empty or A1 is full
            while (!isStockEmpty() && !isA1Full()) {

                // Pick medicine from STOCK (Rail2 col1) -----
                selectedColumn = 1; // col1 for pickup
                homeRail(rail2, M2_L1, speed2);
                startStockArm();
                while (!stockArmDone) runStockArm();

                // Rail2 returns to HOME
                homeRail(rail2, M2_L1, speed2);

            }
          }
            if(!operationFailed){
            Serial.println("A1 Refill Completed.");
            showMessage("Refill Completed");
            }
            return;
      }
      // else{
      //     Serial.println("Stock empty.");
      //     showMessage("Stock empty.");
      // }

    // ================= NORMAL REQUEST =================
    for (int i = 0; i < systemQuantity; i++) {

    // 3) Arm start requesting the medicine 
      startRequestArm();
      while (!requestArmDone) runRequestArm();

    // 4) Rail2 returns to HOME
      homeRail(rail2, M2_L1, speed2);
    }

    // 5) Rail1 returns to HOME
    homeRail(rail1, M1_L1, speed1);
    Serial.println("Retrieve Completed.");
    showMessage("Retrieve Completed.");

    operationDone = true;

}

void runSystemController() {

  if (!systemBusy) return;

  switch (systemMode) {

    case 1:
      runAddSequence();
      break;

    case 2:
      runRequestSequence();
      break;
  }

  if (operationDone) {

    if (!operationFailed) {
      if (systemMode == 1) {
        inventory[pendingMedicineIndex].quantity += systemQuantity;
      }
      else if (systemMode == 2) {
        inventory[pendingMedicineIndex].quantity -= systemQuantity;
      }
    } else {
      Serial.println("OPERATION FAILED - DATABASE NOT UPDATED");
      showMessage("OPERATION FAILED");
    }

    operationFailed = false;
    systemBusy = false;
    operationDone = false;
    needRefresh = true;
  }

}

/* ========================================================= */
/* ===================== MAIN LOOP ========================= */
/* ========================================================= */

void setup() {

  Serial.begin(9600);

  pinMode(LIMIT_LEFT, INPUT_PULLUP);
  pinMode(LIMIT_RIGHT, INPUT_PULLUP);
  pinMode(LIMIT_MIDDLE, INPUT_PULLUP);
  pinMode(LIMIT_BASE_HOME, INPUT_PULLUP);
  pinMode(LIMIT_BASE_RAIL, INPUT_PULLUP);

  pinMode(M1_L1, INPUT_PULLUP);
  pinMode(M1_L2, INPUT_PULLUP);
  pinMode(M2_L1, INPUT_PULLUP);
  pinMode(M2_L2, INPUT_PULLUP);
  pinMode(GRIPPER_LIMIT, INPUT_PULLUP);

  pinMode(A1_TRIG, OUTPUT);
  pinMode(A1_ECHO, INPUT);
  pinMode(STOCK_TRIG, OUTPUT);
  pinMode(STOCK_ECHO, INPUT);
  pinMode(MED_TRIG, OUTPUT);
  pinMode(MED_ECHO, INPUT);

  leftMotor.setMaxSpeed(ARM_SPEED);
  rightMotor.setMaxSpeed(ARM_SPEED);
  middleMotor.setMaxSpeed(ARM_SPEED);

  leftMotor.setAcceleration(ARM_ACCEL);
  rightMotor.setAcceleration(ARM_ACCEL);
  middleMotor.setAcceleration(ARM_ACCEL);

  baseMotor.setMaxSpeed(BASE_SPEED);
  baseMotor.setAcceleration(BASE_ACCEL);
  baseMotor.setCurrentPosition(0);

  rail1.setMaxSpeed(speed1);
  rail2.setMaxSpeed(speed2);
  rail1.setAcceleration(400);
  rail2.setAcceleration(400);

  gripper.attach(SERVO_PIN);
  gripper.write(GRIP_OPEN);

  // Initialize hardware
  initializeLCD();
  initializeCustomChars();
  initializeKeypad();
  
  // Initialize data
  initializeSampleData();

  // Display welcome
  // displayWelcomeMessage();
  
  needRefresh = true;
  Serial.println("🚀 PharmaMatrix System Started!");

  // Serial.println("System Ready");
  // Serial.println("Type: ");
  // Serial.println("1 = Add Medicine");
  // Serial.println("2 = Request Medicine");
}

void loop() {

  
  // Handle keypad input
  char key = keypad.getKey();
  if (key) {
    handleKeypadInput(key);
  }
  
  // Refresh display if needed
  if (!systemBusy) {
    refreshCurrentDisplay();
  }
    
  runSystemController();










  // if (!systemBusy) return;

  // // ---------------------------------------------------
  // // -------------------- ADD MEDICINE -----------------
  // // ---------------------------------------------------
  // if (systemMode == 1) {
    //   Serial.println("ADD Mode Selected");

    //   if (selectedColumn == 2 && isA1Full()) {
    //     Serial.println("A1 Full.");
    //     if (!isStockFull()) selectedColumn = 1;
    //     else {
    //       Serial.println("No free space in A1 or stock.");
    //       return;
    //     }
    //   }

    //   // 1) Homing both rails
    //   homeRail(rail1, M1_L1, speed1);
    //   homeRail(rail2, M2_L1, speed2);

    //   // 2) Rail1 waits at home until user places medicine
    //   Serial.println("Place the medicine...");
    //   delay(5000);

    //   // 3) Rail1 moves to END
    //   goToEnd(rail1, M1_L2, speed1);
    //   Serial.println("Rail1 at end");

    //   // 4) Arm moves to PICK_DROP position to take the medicine
    //   goToPICK_DROP();

    //   // 5) Arm start adding the medicine 
    //   startAddArm();
    //   while (!addArmDone) runAddArm();

    //   // 6) Rail1 returns to HOME
    //   homeRail(rail1, M1_L1, speed1);

    //   // 7) Arm back to home
    //   homeRail(rail2, M2_L1, speed2);
    //   Serial.println("add Completed.");
    // }

  // // ---------------------------------------------------
  // // ------------------ REQUEST MEDICINE --------------
  // // ---------------------------------------------------
  // else if (systemMode == 2) {
  //     Serial.println("Request Mode Selected");

  //     // 1) Homing both rails
  //     homeRail(rail1, M1_L1, speed1);
  //     homeRail(rail2, M2_L1, speed2);

  //     // 2) Rail1 moves to END to receive medicine
  //     goToEnd(rail1, M1_L2, speed1);

  //   // ================= A1 EMPTY CASE =================
  //   if (selectedColumn == 2 && isA1Empty()) {
  //     Serial.println("A1 empty");

  //       if (isStockEmpty()) {
  //         Serial.println("Stock empty.");
  //         return;
  //       }

  //       // -------- STEP 1: Serve user from STOCK --------
  //       for (int i = 0; i < n; i++) {

  //         if (isStockEmpty() || isA1Full()) break;
          
  //         Serial.println("surve user from STOCK");
  //         selectedColumn = 1;
  //         // 3) Arm start requesting the medicine from stock
  //         startStockRequestArm();
  //         while (!stockRequestDone) runStockRequestArm();

  //         // 4) Rail2 returns to HOME
  //         homeRail(rail2, M2_L1, speed2);
  //       }

  //       // 5) Rail1 returns to HOME
  //       homeRail(rail1, M1_L1, speed1);
  //       Serial.println("Retrieve Completed.");
      
  //       // -------- STEP 2: Refill A1 from STOCK --------
  //       if(!isStockEmpty() && !isA1Full()){
  //         Serial.println("Refilling A1 from STOCK...");

  //           // Loop until either stock empty or A1 is full
  //           while (!isStockEmpty() && !isA1Full()) {

  //               // Pick medicine from STOCK (Rail2 col1) -----
  //               selectedColumn = 1; // col1 for pickup
  //               homeRail(rail2, M2_L1, speed2);
  //               startStockArm();
  //               while (!stockArmDone) runStockArm();

  //               // Rail2 returns to HOME
  //               homeRail(rail2, M2_L1, speed2);

  //           }
  //         }
  //           Serial.println("A1 Refill Completed.");
  //           return;
  //     }
  //     else{
  //         Serial.println("Stock empty.");
  //     }

  //   // ================= NORMAL REQUEST =================
  //   for (int i = 0; i < n; i++) {

  //   // 3) Arm start requesting the medicine 
  //     startRequestArm();
  //     while (!requestArmDone) runRequestArm();

  //   // 4) Rail2 returns to HOME
  //     homeRail(rail2, M2_L1, speed2);
  //   }

  //   // 5) Rail1 returns to HOME
  //   homeRail(rail1, M1_L1, speed1);
  //   Serial.println("Retrieve Completed.");
  // }
  
  // else {
  //   Serial.println("Invalid option. Use 1 or 2.");
  // }

  // systemBusy = false;
  // needRefresh = true;















  // if (!Serial.available()) return;
  // int mode = Serial.parseInt();

  // // ---------------------- CHOOSE COLUMN ----------------------
  // clearSerialBuffer();
  // Serial.println("Enter target column (1,2,3):");
  // while (!Serial.available());
  // selectedColumn = Serial.parseInt();
  // clearSerialBuffer();


  // // ---------------------------------------------------
  // // -------------------- ADD MEDICINE -----------------
  // // ---------------------------------------------------
  // if (mode == 1) {
  //   Serial.println("ADD Mode Selected");

  //   if (selectedColumn == 2 && isA1Full()) {
  //     Serial.println("A1 Full.");
  //     if (!isStockFull()) selectedColumn = 1;
  //     else {
  //       Serial.println("No free space in A1 or stock.");
  //       return;
  //     }
  //   }

  //   // 1) Homing both rails
  //   homeRail(rail1, M1_L1, speed1);
  //   homeRail(rail2, M2_L1, speed2);

  //   // 2) Rail1 waits at home until user places medicine
  //   Serial.println("Place the medicine...");
  //   delay(5000);

  //   // 3) Rail1 moves to END
  //   goToEnd(rail1, M1_L2, speed1);
  //   Serial.println("Rail1 at end");

  //   // 4) Arm moves to PICK_DROP position to take the medicine
  //   goToPICK_DROP();

  //   // 5) Arm start adding the medicine 
  //   startAddArm();
  //   while (!addArmDone) runAddArm();

  //   // 6) Rail1 returns to HOME
  //   homeRail(rail1, M1_L1, speed1);

  //   // 7) Arm back to home
  //   homeRail(rail2, M2_L1, speed2);
  //   Serial.println("add Completed.");
  //   clearSerialBuffer();
  // }

  // // ---------------------------------------------------
  // // ------------------ REQUEST MEDICINE --------------
  // // ---------------------------------------------------
  // else if (mode == 2) {
  //   Serial.println("Request Mode Selected");

  //   Serial.println("Enter quantity:");
  //   while (!Serial.available());
  //   int n = Serial.parseInt();
  //   clearSerialBuffer();

  //   // 1) Homing both rails
  //   homeRail(rail1, M1_L1, speed1);
  //   homeRail(rail2, M2_L1, speed2);

  //   // 2) Rail1 moves to END to receive medicine
  //   goToEnd(rail1, M1_L2, speed1);

  // // ================= A1 EMPTY CASE =================
  //  if (selectedColumn == 2 && isA1Empty()) {
  //    Serial.println("A1 empty");

  //     if (isStockEmpty()) {
  //       Serial.println("Stock empty.");
  //       return;
  //     }

  //     // -------- STEP 1: Serve user from STOCK --------
  //     for (int i = 0; i < n; i++) {

  //       if (isStockEmpty() || isA1Full()) break;
        
  //       Serial.println("surve user from STOCK");
  //       selectedColumn = 1;
  //       // 3) Arm start requesting the medicine from stock
  //       startStockRequestArm();
  //       while (!stockRequestDone) runStockRequestArm();

  //       // 4) Rail2 returns to HOME
  //       homeRail(rail2, M2_L1, speed2);
  //     }

  //     // 5) Rail1 returns to HOME
  //     homeRail(rail1, M1_L1, speed1);
  //     Serial.println("Retrieve Completed.");
    
  //     // -------- STEP 2: Refill A1 from STOCK --------
  //     if(!isStockEmpty() && !isA1Full()){
  //       Serial.println("Refilling A1 from STOCK...");

  //         // Loop until either stock empty or A1 is full
  //         while (!isStockEmpty() && !isA1Full()) {

  //             // Pick medicine from STOCK (Rail2 col1) -----
  //             selectedColumn = 1; // col1 for pickup
  //             homeRail(rail2, M2_L1, speed2);
  //             startStockArm();
  //             while (!stockArmDone) runStockArm();

  //             // Rail2 returns to HOME
  //             homeRail(rail2, M2_L1, speed2);

  //         }
  //       }
  //         Serial.println("A1 Refill Completed.");
  //         clearSerialBuffer();
  //         return;
  //   }
  //   else{
  //       Serial.println("Stock empty.");
  //   }

  //   // ================= NORMAL REQUEST =================
  //   for (int i = 0; i < n; i++) {

  //   // 3) Arm start requesting the medicine 
  //     startRequestArm();
  //     while (!requestArmDone) runRequestArm();

  //   // 4) Rail2 returns to HOME
  //     homeRail(rail2, M2_L1, speed2);
  //   }

  //   // 5) Rail1 returns to HOME
  //   homeRail(rail1, M1_L1, speed1);
  //   Serial.println("Retrieve Completed.");
  //   clearSerialBuffer();
  // }
  
  // else {
  //   Serial.println("Invalid option. Use 1 or 2.");
  //   clearSerialBuffer();
  // }
}
