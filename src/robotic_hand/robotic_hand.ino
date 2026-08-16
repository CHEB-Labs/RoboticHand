#include <Servo.h>

Servo servos[5];
int botoes[] = {2, 3, 4, 5, 6}; 
int num_servos = 5;

void setup() {
  for (int i = 0; i < num_servos; i++) {
    servos[i].attach(9 + i); 
    pinMode(botoes[i], INPUT_PULLUP); 
  }
}

void loop() {
  for (int i = 0; i < num_servos; i++) {
    if (digitalRead(botoes[i]) == LOW) { 
      servos[i].write(180); 
    } else {
      servos[i].write(0); 
    }
  }
}