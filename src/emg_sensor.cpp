#include "../include/emg_sensor.h"

// Variáveis privadas
int emgBuffer[EMG_SAMPLES];
int bufferIndex = 0;

void initEMG()
{
    pinMode(EMG_PIN, INPUT);

    // Inicializar buffer
    for (int i = 0; i < EMG_SAMPLES; i++)
    {
        emgBuffer[i] = 0;
    }

    Serial.println("Sensor EMG inicializado");
}

int readEMG()
{
    int rawValue = analogRead(EMG_PIN);

    // Adicionar ao buffer circular
    emgBuffer[bufferIndex] = rawValue;
    bufferIndex = (bufferIndex + 1) % EMG_SAMPLES;

    return rawValue;
}

int getEMGAverage()
{
    long sum = 0;
    for (int i = 0; i < EMG_SAMPLES; i++)
    {
        sum += emgBuffer[i];
    }
    return sum / EMG_SAMPLES;
}