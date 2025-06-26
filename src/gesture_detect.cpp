#include "../include/gesture_detect.h"

// Variáveis de estado
bool muscleActive = false;
unsigned long lastGestureTime = 0;
int currentGesture = GESTURE_IDLE;

int detectGesture(int processedSignal)
{
    unsigned long currentTime = millis();

    // Debounce - evitar mudanças muito rápidas
    if (currentTime - lastGestureTime < DEBOUNCE_TIME)
    {
        return currentGesture;
    }

    bool newState = isActivated(processedSignal);

    // Detectar mudança de estado
    if (newState != muscleActive)
    {
        muscleActive = newState;
        lastGestureTime = currentTime;

        if (muscleActive)
        {
            // Alternar entre abrir e fechar
            if (currentGesture == GESTURE_OPEN)
            {
                currentGesture = GESTURE_CLOSE;
            }
            else
            {
                currentGesture = GESTURE_OPEN;
            }

            Serial.print("Gesto detectado: ");
            Serial.println(currentGesture == GESTURE_OPEN ? "ABRIR" : "FECHAR");
        }
    }

    return currentGesture;
}

bool isActivated(int signal)
{
    if (muscleActive)
    {
        // Se já estava ativo, usar threshold menor para desativar (histerese)
        return signal > DEACTIVATION_THRESHOLD;
    }
    else
    {
        // Se estava inativo, usar threshold maior para ativar
        return signal > ACTIVATION_THRESHOLD;
    }
}