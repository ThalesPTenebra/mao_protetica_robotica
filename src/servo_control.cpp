#include "../include/servo_control.h"

// Objetos servo
Servo servoThumb;
Servo servoIndex;
Servo servoMiddle;
Servo servoRing;
Servo servoPinky;

Servo servos[5];
int servoPins[5] = {SERVO_THUMB_PIN, SERVO_INDEX_PIN, SERVO_MIDDLE_PIN, SERVO_RING_PIN, SERVO_PINKY_PIN};

void initServos()
{
    // Anexar servos aos pinos
    for (int i = 0; i < 5; i++)
    {
        servos[i].attach(servoPins[i]);
        servos[i].write(SERVO_OPEN); // Posição inicial aberta
    }

    delay(1000); // Aguardar posicionamento
    Serial.println("Servos inicializados");
}

void controlServos(int gesture)
{
    switch (gesture)
    {
    case GESTURE_OPEN:
        openHand();
        break;
    case GESTURE_CLOSE:
        closeHand();
        break;
    case GESTURE_IDLE:
        // Manter posição atual
        break;
    }
}

void openHand()
{
    for (int i = 0; i < 5; i++)
    {
        servos[i].write(SERVO_OPEN);
    }
    Serial.println("Mão aberta");
}

void closeHand()
{
    for (int i = 0; i < 5; i++)
    {
        servos[i].write(SERVO_CLOSED);
    }
    Serial.println("Mão fechada");
}

void moveServo(int servoIndex, int position)
{
    if (servoIndex >= 0 && servoIndex < 5)
    {
        servos[servoIndex].write(position);
    }
}