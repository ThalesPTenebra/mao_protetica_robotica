#ifndef WIFI_COMM_H
#define WIFI_COMM_H

// Usar a biblioteca correta para o hardware (ESP8266 ou ESP32)
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

// Configurações WiFi
#define WIFI_SSID "SuaRedeWiFi"
#define WIFI_PASSWORD "SuaSenha"
#define LED_PIN 2 // LED_BUILTIN para a maioria das placas ESP8266/ESP32

// Funções públicas
void initWiFi();
void handleWiFiClient(); // Nova função para lidar com clientes
void sendWiFiData(int emgValue, int gesture);
void updateStatusLED(bool connected);

#endif