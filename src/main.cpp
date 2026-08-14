#include <Arduino.h>

#define LED_BUILTIN 2

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
 pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
 digitalWrite(LED_BUILTIN, HIGH);

  // wait for a second
  delay(1000);

  // turn the LED off by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);

   // wait for a second
  delay(1000);
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}