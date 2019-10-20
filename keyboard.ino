// Keyboard firmware for a sick-68 3D-printed keyboard
// https://www.thingiverse.com/thing:3478494
// I used a Teensy 3.2 instead of a 2.0
// Rows (top to bottom) are wired to pins 0-4
// Columns (left to right) are wired to 5-10 and 15-23

// Setup:
// Install Teensyduino: https://pjrc.com/teensy/td_download.html
// Board: Teensy 3.2/3.1
// USB Type: Keyboard
// Keyboard Layout: US English

#include <Keyboard.h>
#include <Bounce2.h>

#define NUM_ROWS 5
#define NUM_COLUMNS 15
#define BUTTON_INTERVAL_MS 5 // Debounce time.  Cherry claims 5ms: https://geekhack.org/index.php?topic=42385.0

const uint8_t ROW_PINS[NUM_ROWS] = {0, 1, 2, 3, 4};
const uint8_t COLUMN_PINS[NUM_COLUMNS] = {5, 6, 7, 8, 9, 10, 15, 16, 17, 18, 19, 20, 21, 22, 23};

// KEYS[row][column] is a keycode.  0 for no key.
const uint16_t KEYS[][NUM_COLUMNS] = {
  {KEY_TILDE, KEY_BACKSPACE, KEY_EQUAL, KEY_MINUS, KEY_0, KEY_9, KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8},
  {KEY_DELETE, KEY_BACKSLASH, KEY_RIGHT_BRACE, KEY_LEFT_BRACE, KEY_P, KEY_O, KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I},
  {KEY_PAGE_UP, KEY_ENTER, 0, KEY_QUOTE, KEY_SEMICOLON, KEY_L, KEY_CAPS_LOCK, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K},
  {KEY_PAGE_DOWN, KEY_UP, MODIFIERKEY_RIGHT_SHIFT, KEY_SLASH, KEY_PERIOD, KEY_COMMA, MODIFIERKEY_LEFT_SHIFT, 0, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M},
  {KEY_RIGHT, KEY_DOWN, KEY_LEFT, MODIFIERKEY_RIGHT_CTRL, MODIFIERKEY_RIGHT_ALT, 0, MODIFIERKEY_LEFT_CTRL, MODIFIERKEY_LEFT_GUI, MODIFIERKEY_LEFT_ALT, 0, 0, 0, KEY_SPACE, 0, 0} // key 5 is fn
};

const uint16_t FN_KEYS[][NUM_COLUMNS] = {
  {KEY_MEDIA_PLAY_PAUSE, KEY_INSERT, KEY_F12, KEY_F11, KEY_F10, KEY_F9, 0, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8},
  {KEY_MEDIA_MUTE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {KEY_MEDIA_VOLUME_INC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {KEY_MEDIA_VOLUME_DEC, KEY_PAGE_UP, 0, KEY_MEDIA_STOP, KEY_MEDIA_NEXT_TRACK, KEY_MEDIA_PREV_TRACK, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {KEY_END, KEY_PAGE_DOWN, KEY_HOME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

boolean fnPressed = false;

Bounce* buttons = new Bounce[NUM_COLUMNS * NUM_ROWS];

byte keys[NUM_COLUMNS][NUM_ROWS];

void setup() {
  // Rows
  for (uint8_t row = 0; row < NUM_ROWS; row ++) {
    pinMode(ROW_PINS[row], INPUT);
  }

  // Columns
  for (uint8_t column = 0; column < NUM_COLUMNS; column ++) {
    pinMode(COLUMN_PINS[column], INPUT);

    for (uint8_t row = 0; row < NUM_ROWS; row ++) {
      buttons[buttonIndex(row, column)].attach(COLUMN_PINS[column]);
      buttons[buttonIndex(row, column)].interval(BUTTON_INTERVAL_MS);
    }
  }

  // Keyboard
  Keyboard.begin();
}

void loop() {
  for (int8_t row = 0; row < NUM_ROWS; row ++) {
    // Enable the row: row -> ground without a resistor
    pinMode(ROW_PINS[row], OUTPUT);
    digitalWrite(ROW_PINS[row], LOW);

    for (int8_t column = 0; column < NUM_COLUMNS; column ++) {
      uint8_t i = buttonIndex(row, column);
      buttons[i].update();
      if (buttons[i].rose()) {
        if (isFnKey(row, column)) {
          // TODO: need to release all of the fn keys?
          fnPressed = false;
        } else {
          uint16_t key = keyCode(row, column);
          if (key != 0) {
            Keyboard.release(key);
          }
        }
      } else if (buttons[i].fell()) {
        if (isFnKey(row, column)) {
          fnPressed = true;
        } else {
          uint16_t key = keyCode(row, column);
          if (key != 0) {
            Keyboard.press(key);
          }
        }
      }
    }

    // Disable the row: row -> high impedence
    digitalWrite(ROW_PINS[row], HIGH);
    pinMode(ROW_PINS[row], INPUT);

    waitForRowComplete();
  }
}

// TODO: this could be inlined by making it a define - not sure if the compiler will.
// Returns the index of the button found at the given row and column.
// buttons is a flat array representing all of the keys on the keyboard,
// so this function identifies a single keyboard key in the buttons array.
uint8_t buttonIndex(uint8_t row, uint8_t column) {
  return row * NUM_COLUMNS + column;
}

// Returns whether the key at the given row and column is the function key.
boolean isFnKey(uint8_t row, uint8_t column) {
  return row == 4 && column == 5;
}

// Returns the key code of the given row and column - 0 if no key is mapped.
uint16_t keyCode(uint8_t row, uint8_t column) {
  if (fnPressed) {
    uint16_t key = FN_KEYS[row][column];
    // fall back to the regular key map if there's no function for the key
    if (key != 0) {
      return key;
    }
  }

  return KEYS[row][column];
}

// Poll until no more signals are found
// TODO: is there a better way to do this?  without this block (or a 20ms delay), multiple rows go LOW.
// This block takes ~16us / row, vs 20 ms for the delay
void waitForRowComplete() {
  boolean signaled = true;
  while (signaled) {
    signaled = false;
    for (int8_t column = 0; column < NUM_COLUMNS; column ++) {
      pinMode(COLUMN_PINS[column], INPUT_PULLUP);
      signaled = signaled || (digitalRead(COLUMN_PINS[column]) == LOW);
      pinMode(COLUMN_PINS[column], INPUT);
    }
  }
}
