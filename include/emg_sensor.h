#ifndef EMG_SENSOR_H
#define EMG_SENSOR_H

#include <Arduino.h>

// Configurações do sensor EMG
#define EMG_PIN A0
#define EMG_SAMPLES 10

// Funções públicas
void initEMG();
int readEMG();
int getEMGAverage();

#endif