
#ifndef SIGNAL_PROCESS_H
#define SIGNAL_PROCESS_H

#include <Arduino.h>

// Configurações do filtro
#define FILTER_ALPHA 0.1 // Constante do filtro passa-baixa
#define NOISE_THRESHOLD 50

// Funções públicas
int processSignal(int rawSignal);
int lowPassFilter(int input);
int removeNoise(int signal);

#endif