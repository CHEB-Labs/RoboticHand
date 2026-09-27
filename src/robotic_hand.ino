#include <ESP32Servo.h>

Servo servos[5];

int pinosServos[] = {14, 27, 26, 25, 33};

int botoes[]      = {15, 5, 18, 19, 2};

int num_servos = 5;

void setup() {
  for (int i = 0; i < num_servos; i++) {
    servos[i].setPeriodHertz(50); 
    servos[i].attach(pinosServos[i], 500, 1500); 
    pinMode(botoes[i], INPUT_PULLUP);
  }
}

void loop() {
  for (int i = 0; i < num_servos; i++) {
    if (digitalRead(botoes[i]) == LOW) {
      servos[i].write(0);
    } else {
      servos[i].write(180);
    }
  }
}
