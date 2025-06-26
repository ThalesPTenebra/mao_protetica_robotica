#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <Arduino.h>
#include <Servo.h>

// Pinos dos servos
#define SERVO_THUMB_PIN 3
#define SERVO_INDEX_PIN 5
#define SERVO_MIDDLE_PIN 6
#define SERVO_RING_PIN 9
#define SERVO_PINKY_PIN 10

// Posições dos servos
#define SERVO_OPEN 0
#define SERVO_CLOSED 180

// Gestos
#define GESTURE_OPEN 0
#define GESTURE_CLOSE 1
#define GESTURE_IDLE 2

// Funções públicas
void initServos();
void controlServos(int gesture);
void openHand();
void closeHand();
void moveServo(int servoIndex, int position);

#endif
