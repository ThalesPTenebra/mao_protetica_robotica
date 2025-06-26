#ifndef GESTURE_DETECT_H
#define GESTURE_DETECT_H

#include <Arduino.h>
#include "servo_control.h" // Para constantes de gesto

// Configurações de detecção
#define ACTIVATION_THRESHOLD 600 // Limiar para ativação (0-1023)
#define DEACTIVATION_THRESHOLD 550
#define DEBOUNCE_TIME 200 // ms

// Funções públicas
int detectGesture(int processedSignal);
bool isActivated(int signal);

#endif