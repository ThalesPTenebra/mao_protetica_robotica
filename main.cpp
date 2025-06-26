#include "include/emg_sensor.h"
#include "include/servo_control.h"
#include "include/wifi_comm.h"
#include "include/signal_process.h"
#include "include/gesture_detect.h"

// Configurações principais
#define LOOP_DELAY 50 // 50ms = 20Hz

void setup()
{
    Serial.begin(9600);
    Serial.println("Iniciando Prótese Mão Robótica...");

    // Inicializar módulos
    initEMG();
    initServos();
    initWiFi();

    Serial.println("Sistema pronto!");
}

void loop()
{
    // 1. Ler sinal EMG
    int emgValue = readEMG();

    // 2. Processar sinal
    int processedSignal = processSignal(emgValue);

    // 3. Detectar gesto
    int gesture = detectGesture(processedSignal);

    // 4. Controlar servos
    controlServos(gesture);

    // 5. Enviar dados via WiFi (opcional)
    sendWiFiData(emgValue, gesture);

    delay(LOOP_DELAY);
}