#ifndef WIFI_COMM_H
#define WIFI_COMM_H

#include <Arduino.h>

// Configurações WiFi
#define WIFI_SSID "SuaRedeWiFi"
#define WIFI_PASSWORD "SuaSenha"
#define LED_PIN 13

// Funções públicas
void initWiFi();
void sendWiFiData(int emgValue, int gesture);
void updateStatusLED(bool connected);

#endif