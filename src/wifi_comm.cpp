#include "../include/wifi_comm.h"

WiFiServer server(80);

bool wifiConnected = false;

void initWiFi()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.print("Conectando a ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20)
    {
        delay(500);
        Serial.print(".");
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi conectado!");
        Serial.print("Endereço IP: ");
        Serial.println(WiFi.localIP());
        updateStatusLED(true);
        server.begin(); // Iniciar o servidor
    }
    else
    {
        Serial.println("\nFalha ao conectar ao WiFi.");
        updateStatusLED(false);
    }
}

void handleWiFiClient()
{
    WiFiClient client = server.available();
    if (!client)
    {
        return;
    }

    // Aguardar dados do cliente
    while (!client.available())
    {
        delay(1);
    }

    // Ler a primeira linha da requisição
    String req = client.readStringUntil('\r');
    client.flush();

    // Responder ao cliente
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("");
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<h1>Servidor da Mão Robótica</h1>");
    client.println("</html>");
    client.stop();
}

void sendWiFiData(int emgValue, int gesture)
{
    if (wifiConnected)
    {
        // Lógica para enviar dados (ex: para um servidor web, MQTT, etc.)
        // Por enquanto, apenas imprimimos no Serial
        Serial.print("[WiFi] EMG: ");
        Serial.print(emgValue);
        Serial.print(", Gesto: ");
        Serial.println(gesture);
    }

    // Piscar LED para indicar atividade de loop
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
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