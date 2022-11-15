/*
 * Project Project
 * Description:
 * Author:
 * Date:
 */
#include "Keypad_Particle.h"

const byte ROWS = 3;
const byte COLS = 3;

char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
};

byte rowPins[ROWS] = { D7, D6, D5 };
byte colPins[COLS] = { D3, D2, D1 };

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.
  Serial.begin(9600);
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {

  char key = keypad.getKey();
  
  if (key){
    Serial.println(key);
  }

}