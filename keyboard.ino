// Keyboard firmware for a sick-68 3D-printed keyboard
// https://www.thingiverse.com/thing:3478494
// I used a Teensy 3.2 instead of a 2.0
// Rows (top to bottom) are wired to pins 0-4
// Columns (left to right) are wired to 5-10 and 15-23

#include <Bounce2.h>

#define LED_PIN 13
#define NUM_ROWS 5
#define NUM_COLUMNS 15
#define BUTTON_INTERVAL_MS 25

const uint8_t ROW_PINS[NUM_ROWS] = {0, 1, 2, 3, 4};
const uint8_t COLUMN_PINS[NUM_COLUMNS] = {5, 6, 7, 8, 9, 10, 15, 16, 17, 18, 19, 20, 21, 22, 23};

// Bounce* buttons = new Bounce[NUM_COLUMNS * NUM_ROWS]; // TODO: button per row x column (key) instead of 

byte keys[NUM_COLUMNS][NUM_ROWS];

void setup() {
  // Rows
  for (uint8_t row = 0; row < NUM_ROWS; row ++) {
    pinMode(ROW_PINS[row], INPUT);
//    pinMode(ROW_PINS[row], OUTPUT);
//    digitalWrite(ROW_PINS[row], HIGH);
  }
  
  // Columns
  for (uint8_t column = 0; column < NUM_COLUMNS; column ++) {
    pinMode(COLUMN_PINS[column], INPUT);
  }

  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Debugging
  Serial.begin(9600);  
  delay(1000); // Wait because computer may not recognize keystrokes until USB enumeration is complete.
}

void loop() {
  // Sketch of firmware: pins 5-10, 15-23 are columns, pins 0-4 are rows.
  // each row: HIGH
  // each column:
  //  if HIGH: send Keyboard.press() on rising edge, remember
  //  if LOW: send Keyboard.release() on falling edge, forget
  // Keyboard.set_modifier();
  // Libraries: Button2, Keyboard
  // Improvements:
  // chording: fn+[1-0-=] -> function keys
  // macros: fn+escape -> key -> keys to record the macro -> fn+escape (record); fn+key to play - modifier to include delays or not
  
  digitalWrite(LED_PIN, HIGH);

  for (int8_t row = 0; row < NUM_ROWS; row ++) {
    // Enable the row: row -> ground without a resistor
    pinMode(ROW_PINS[row], OUTPUT);
    digitalWrite(ROW_PINS[row], LOW);

    for (int8_t column = 0; column < NUM_COLUMNS; column ++) {
      // Signal the column: column -> pullup resistor.  Current will flow from the column to the row.
      pinMode(COLUMN_PINS[column], INPUT_PULLUP);
      keys[column][row] = digitalRead(COLUMN_PINS[column]);
      // Disable the column: column -> high impedence
      pinMode(COLUMN_PINS[column], INPUT);
    }

    // Disable the row: row -> high impedence
    digitalWrite(ROW_PINS[row], HIGH);
    pinMode(ROW_PINS[row], INPUT);

    // Poll until no more signals are found
    // TODO: is there a better way to do this?  without this block (or a 20ms delay), multiple rows go LOW
    unsigned long startUS = micros();
    unsigned long loops = 0;
    boolean signaled = true;
    while (signaled) {
      loops ++;
      signaled = false;
      for (int8_t column = 0; column < NUM_COLUMNS; column ++) {
        pinMode(COLUMN_PINS[column], INPUT_PULLUP);
        if (digitalRead(COLUMN_PINS[column]) == LOW) {
          signaled = true;
        }
        pinMode(COLUMN_PINS[column], INPUT);
      }
    }
    unsigned long endUS = micros();

    Serial.print("Done with row ");
    Serial.print(row);
    Serial.print(" in ");
    Serial.print(endUS - startUS);
    Serial.print("us - ");
    Serial.print(loops);
    Serial.println(" loops");
  }

  Serial.println("    5  6  7  8  9 10 15 16 17 18 19 20 21 22 23");
  for (uint8_t row = 0; row < NUM_ROWS; row ++) {
    Serial.print(ROW_PINS[row]);
    Serial.print(":");
    
    for (uint8_t column = 0; column < NUM_COLUMNS; column ++) {
      Serial.print("  ");
      Serial.print(keys[column][row]);
    }
    Serial.println("");
  }
  Serial.println("");

/*
  // Light up each row - columns will go LOW if the key on that row x column is pressed
  for (uint8_t row = 0; row < NUM_ROWS; row ++) {
    digitalWrite(ROW_PINS[row], HIGH);

    // Check each of the column buttons on the row
    for (int8_t column = 0; column < NUM_COLUMNS; column ++) {
      int index = buttonIndex(row, column);
      buttons[index].update();
      if (buttons[index].rose()) {  // HIGH -> LOW - means the button was pressed
        printRowColumn("Rose", row, column);
      } else if (buttons[index].fell()) { // LOW -> HIGH - means the button was released
        printRowColumn("Fell", row, column);
      }
    }

    digitalWrite(ROW_PINS[row], LOW);
  }
  */
  
//  delay(200);
  digitalWrite(LED_PIN, LOW);
//  delay(800);
  delay(500);
}

// TODO: this could be inlined by making it a define - not sure if the compiler will.
// Returns the index of the button found at the given row and column.
// buttons is a flat array representing all of the keys on the keyboard,
// so this function identifies a single keyboard key in the buttons array.
int buttonIndex(uint8_t row, uint8_t column) {
  return row * NUM_COLUMNS + column;
}

void printRowColumn(String action, uint8_t row, uint8_t column) {
  Serial.print(action + ": ");
  Serial.print(row);
  Serial.print(" x ");
  Serial.println(column);
}
