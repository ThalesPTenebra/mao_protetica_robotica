#include "../include/signal_process.h"

// Variáveis do filtro
float filteredValue = 0;

int processSignal(int rawSignal)
{
    // 1. Remover ruído
    int cleanSignal = removeNoise(rawSignal);

    // 2. Aplicar filtro passa-baixa
    int filteredSignal = lowPassFilter(cleanSignal);

    return filteredSignal;
}

int lowPassFilter(int input)
{
    // Filtro passa-baixa simples: y[n] = α*x[n] + (1-α)*y[n-1]
    filteredValue = FILTER_ALPHA * input + (1 - FILTER_ALPHA) * filteredValue;
    return (int)filteredValue;
}

int removeNoise(int signal)
{
    // Remover ruído abaixo do threshold
    if (abs(signal - 512) < NOISE_THRESHOLD)
    {               // 512 = centro ADC 10-bit
        return 512; // Retornar valor neutro
    }
    return signal;
}