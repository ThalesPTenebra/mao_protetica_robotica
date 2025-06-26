
#include "../include/wifi_comm.h"

bool wifiConnected = false;

void initWiFi()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Inicialização básica do WiFi
    // Nota: Para ESP8266, usar biblioteca WiFi.h
    Serial.println("WiFi inicializado");
    updateStatusLED(false);
}

void sendWiFiData(int emgValue, int gesture)
{
    if (wifiConnected)
    {
        // Enviar dados via WiFi
        Serial.print("EMG: ");
        Serial.print(emgValue);
        Serial.print(", Gesto: ");
        Serial.println(gesture);
    }

    // Piscar LED para indicar atividade
    digitalWrite(LED_PIN, HIGH);
    delay(10);
    digitalWrite(LED_PIN, LOW);
}

void updateStatusLED(bool connected)
{
    wifiConnected = connected;
    if (connected)
    {
        digitalWrite(LED_PIN, HIGH); // LED ligado = conectado
    }
    else
    {
        digitalWrite(LED_PIN, LOW); // LED desligado = desconectado
    }
}