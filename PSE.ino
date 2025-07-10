/*
  ====== MÃO ROBÓTICA – ESP32 + PCA9685 + BLE (COM GESTOS IMPLEMENTADOS) ======
  Adaptado do código de robô de sumô que funciona corretamente
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// ---------- Constantes ----------
#define SERVOMIN  100   // PWM mínimo
#define SERVOMAX  500   // PWM máximo
#define FREQUENCY 50    // Hz

// GPIOs
#define EMERGENCY_BTN_PIN 18
#define TOGGLE_ENABLE_PIN 19
#define OE_PIN            23
#define SDA_PIN           21
#define SCL_PIN           22
#define LED_FINGER_0      4
#define LED_FINGER_1      5
#define LED_FINGER_2      12
#define LED_FINGER_3      13
#define LED_FINGER_4      14
#define LED_EMERGENCY     2
#define LED_STATUS        15

// BLE - UUIDs únicos
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-cba987654321"
#define DEVICE_NAME         "MaoRobotica_ESP32"

// Geral
#define DEFAULT_POSITION 90
#define DEBOUNCE_DELAY   50
#define MOVE_SPEED       50
#define LED_BLINK_SPEED  300
#define GESTURE_DELAY    100  // Delay entre movimentos de gestos

// ---------- Objetos ----------
Adafruit_PWMServoDriver pwm;
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

// ---------- RTOS ----------
TaskHandle_t Task1;
TaskHandle_t Task2;

// ---------- Estado Global ----------
bool servosEnabled   = true;
bool emergencyStop   = false;
bool autoMode        = true;
bool deviceConnected = false;
bool oldDeviceConnected = false;

unsigned long lastEmergencyBtnTime = 0;
unsigned long lastToggleEnableTime = 0;
unsigned long lastStatusSent = 0;

int currentAngle   = DEFAULT_POSITION;
bool increasing    = true;
const int moveStep = 3;

int servoAngles[5] = {90, 90, 90, 90, 90};
const int fingerLedPins[5] = {
  LED_FINGER_0, LED_FINGER_1, LED_FINGER_2, LED_FINGER_3, LED_FINGER_4
};
unsigned long lastLedBlinkTime = 0;
bool ledBlinkState = false;

char mensagem_resp[200]; // Aumentado para suportar JSON maior

// ---------- Estruturas de Gestos ----------
struct Gesture {
  const char* name;
  int angles[5];
};

// Definição dos gestos disponíveis
const Gesture gestures[] = {
  {"OPEN",  {180, 180, 180, 180, 180}}, // Mão aberta
  {"CLOSE", {0, 0, 0, 0, 0}},           // Mão fechada
  {"PEACE", {0, 180, 180, 0, 0}},       // Sinal de paz
  {"OK",    {45, 45, 180, 180, 180}},   // Sinal de OK
  {"POINT", {0, 180, 0, 0, 0}},         // Apontar
  {"ROCK",  {0, 180, 0, 0, 180}},       // Rock and roll
  {"RESET", {90, 90, 90, 90, 90}}       // Posição neutra
};

const int gestureCount = sizeof(gestures) / sizeof(Gesture);

// ---------- Forward declarations ----------
void moveServo(uint8_t servoNum, uint8_t angle);
void moveAllServos(uint8_t angle);
void executeMovement();
void updateFingerLeds();
void updateFingerLedsManual();
void updateLedStatus();
void checkEmergencyButton();
void checkToggleEnableButton();
void sendBLEResponse(const String& resp);
void processBluetoothCommand(const std::string& command);
void executeGesture(const String& gestureName);
void sendStatusResponse();

// ---------- Callbacks BLE (baseado no código que funciona) ----------
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    BLEDevice::startAdvertising(); // Mantém advertising ativo
    Serial.println("📱 Cliente conectado");
    sendStatusResponse(); // Envia status inicial
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("📱 Cliente desconectado");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string value(pCharacteristic->getValue().c_str());
    
    Serial.print("📨 Comando recebido: ");
    Serial.println(value.c_str());
    
    if (value.length() > 0) {
      processBluetoothCommand(value);
    }
  }
};

// ---------- Task BLE (baseada no código que funciona) ----------
void task_ble(void* pvParameters) {
  for (;;) {
    // Gerencia conexão BLE
    if (!deviceConnected && oldDeviceConnected) {
      delay(500);
      pServer->startAdvertising();
      Serial.println("🔄 Reiniciando advertising");
      oldDeviceConnected = deviceConnected;
    }
    
    if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;
      Serial.println("✅ Nova conexão estabelecida");
      sendStatusResponse();
    }

    // Envia status periodicamente (a cada 5 segundos)
    if (deviceConnected && millis() - lastStatusSent > 5000) {
      sendStatusResponse();
      lastStatusSent = millis();
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 segundo entre verificações
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==== Mão Robótica – Setup ====");

  // I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  pwm.begin();
  pwm.setPWMFreq(FREQUENCY);

  // GPIOs
  pinMode(EMERGENCY_BTN_PIN, INPUT_PULLUP);
  pinMode(TOGGLE_ENABLE_PIN, INPUT_PULLUP);
  pinMode(OE_PIN, OUTPUT);
  digitalWrite(OE_PIN, LOW);

  for (int i = 0; i < 5; i++) {
    pinMode(fingerLedPins[i], OUTPUT);
  }
  pinMode(LED_EMERGENCY, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);

  moveAllServos(DEFAULT_POSITION);

  // --- BLE (seguindo o padrão do código que funciona) ---
  Serial.println("🔵 Iniciando BLE...");
  BLEDevice::init(DEVICE_NAME);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();

  Serial.println("🔵 BLE iniciado - aguardando conexões...");
  Serial.println("🎭 Gestos disponíveis: OPEN, CLOSE, PEACE, OK, POINT, ROCK, RESET");

  // --- RTOS (uma task como no código que funciona) ---
  xTaskCreatePinnedToCore(
    task_ble,    /* Task function */
    "BLE_Task",  /* name of task */
    10000,       /* Stack size */
    NULL,        /* parameter */
    1,           /* priority */
    &Task1,      /* Task handle */
    0            /* pin task to core 0 */
  );

  digitalWrite(LED_STATUS, HIGH);
  Serial.println("✅ Setup completo!");
}

// ---------- Funções de controle ----------
void moveServo(uint8_t servoNum, uint8_t angle) {
  if (servoNum > 4) return;
  
  // Verifica se pode mover
  if (emergencyStop || !servosEnabled) {
    Serial.println("⚠️  Movimento bloqueado - Emergency/Servos disabled");
    return;
  }
  
  int pw = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(servoNum, 0, pw);
  servoAngles[servoNum] = angle;
  
  Serial.printf("🤖 Servo %d -> %d°\n", servoNum, angle);
}

void moveAllServos(uint8_t angle) {
  if (emergencyStop || !servosEnabled) {
    Serial.println("⚠️  Movimento bloqueado - Emergency/Servos disabled");
    return;
  }
  
  Serial.printf("🤖 Todos servos -> %d°\n", angle);
  for (uint8_t i = 0; i < 5; i++) {
    moveServo(i, angle);
  }
}

void executeMovement() {
  if (increasing) {
    currentAngle += moveStep;
    if (currentAngle >= 180) {
      currentAngle = 180;
      increasing = false;
    }
  } else {
    currentAngle -= moveStep;
    if (currentAngle <= 0) {
      currentAngle = 0;
      increasing = true;
    }
  }
  moveAllServos(currentAngle);
}

void executeGesture(const String& gestureName) {
  if (emergencyStop) {
    sendBLEResponse("ERROR:EMERGENCY_ACTIVE");
    Serial.println("⚠️  Gesto bloqueado - Emergência ativa");
    return;
  }
  
  if (!servosEnabled) {
    sendBLEResponse("ERROR:SERVOS_DISABLED");
    Serial.println("⚠️  Gesto bloqueado - Servos desabilitados");
    return;
  }
  
  // Procura o gesto na lista
  for (int i = 0; i < gestureCount; i++) {
    if (gestureName.equals(gestures[i].name)) {
      Serial.printf("🎭 Executando gesto: %s\n", gestures[i].name);
      
      // Move cada servo para a posição do gesto
      for (int j = 0; j < 5; j++) {
        moveServo(j, gestures[i].angles[j]);
        delay(GESTURE_DELAY); // Pequeno delay entre movimentos
      }
      
      sendBLEResponse("GESTURE:" + gestureName + ":OK");
      return;
    }
  }
  
  // Gesto não encontrado
  sendBLEResponse("ERROR:UNKNOWN_GESTURE");
  Serial.println("❌ Gesto não reconhecido: " + gestureName);
}

void sendStatusResponse() {
  if (!deviceConnected || !pCharacteristic) return;
  
  // Cria JSON com status completo
  DynamicJsonDocument doc(300);
  doc["servosEnabled"] = servosEnabled;
  doc["emergencyStop"] = emergencyStop;
  doc["autoMode"] = autoMode;
  
  JsonObject angles = doc.createNestedObject("angles");
  for (int i = 0; i < 5; i++) {
    angles[String(i)] = servoAngles[i];
  }
  
  String statusJson = "STATUS:";
  serializeJson(doc, statusJson);
  
  sendBLEResponse(statusJson);
}

void updateFingerLeds() {
  bool on = (currentAngle > 45 && currentAngle < 135);
  for (int i = 0; i < 5; i++) {
    digitalWrite(fingerLedPins[i], on);
  }
}

void updateFingerLedsManual() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(fingerLedPins[i], servoAngles[i] > 90);
  }
}

void updateLedStatus() {
  // LED emergência pisca
  if (emergencyStop && millis() - lastLedBlinkTime > LED_BLINK_SPEED) {
    lastLedBlinkTime = millis();
    ledBlinkState = !ledBlinkState;
    digitalWrite(LED_EMERGENCY, ledBlinkState);
  }
  if (!emergencyStop) {
    digitalWrite(LED_EMERGENCY, LOW);
  }

  // LED status
  digitalWrite(LED_STATUS, (servosEnabled && !emergencyStop));
}

void checkEmergencyButton() {
  static bool lastState = HIGH;
  bool state = digitalRead(EMERGENCY_BTN_PIN);
  
  if (state != lastState && millis() - lastEmergencyBtnTime > DEBOUNCE_DELAY) {
    lastEmergencyBtnTime = millis();
    if (state == LOW && lastState == HIGH) {
      emergencyStop = !emergencyStop;
      if (emergencyStop) {
        moveAllServos(DEFAULT_POSITION);
        sendBLEResponse("EMERGENCY:ON");
        Serial.println("🛑 Emergência ativada");
      } else {
        currentAngle = DEFAULT_POSITION;
        increasing = true;
        sendBLEResponse("EMERGENCY:OFF");
        Serial.println("✅ Emergência desativada");
      }
    }
    lastState = state;
  }
}

void checkToggleEnableButton() {
  static bool lastState = HIGH;
  bool state = digitalRead(TOGGLE_ENABLE_PIN);
  
  if (state != lastState && millis() - lastToggleEnableTime > DEBOUNCE_DELAY) {
    lastToggleEnableTime = millis();
    if (state == LOW && lastState == HIGH) {
      servosEnabled = !servosEnabled;
      digitalWrite(OE_PIN, servosEnabled ? LOW : HIGH);
      sendBLEResponse(String("SERVOS:") + (servosEnabled ? "ON" : "OFF"));
      Serial.println(String("⚙️  Servos: ") + (servosEnabled ? "ON" : "OFF"));
    }
    lastState = state;
  }
}

void sendBLEResponse(const String& resp) {
  if (deviceConnected && pCharacteristic) {
    pCharacteristic->setValue(resp.c_str());
    pCharacteristic->notify();
    Serial.print("📤 Resposta enviada: ");
    Serial.println(resp);
    delay(10); // Delay aumentado para estabilidade BLE
  }
}

// ---------- Processa comando BLE ----------
void processBluetoothCommand(const std::string& command) {
  String cmd = String(command.c_str());
  String response = "OK";
  
  Serial.print("⚙️  Processando: ");
  Serial.println(cmd);
  
  // Comandos de emergência
  if (cmd == "EMERGENCY:ON") {
    emergencyStop = true;
    moveAllServos(DEFAULT_POSITION);
    response = "EMERGENCY:ON";
  }
  else if (cmd == "EMERGENCY:OFF") {
    emergencyStop = false;
    currentAngle = DEFAULT_POSITION;
    increasing = true;
    response = "EMERGENCY:OFF";
  }
  // Comandos de servos
  else if (cmd == "SERVOS:ON") {
    servosEnabled = true;
    digitalWrite(OE_PIN, LOW);
    response = "SERVOS:ON";
  }
  else if (cmd == "SERVOS:OFF") {
    servosEnabled = false;
    digitalWrite(OE_PIN, HIGH);
    response = "SERVOS:OFF";
  }
  // Comandos de modo
  else if (cmd == "AUTO:ON") {
    autoMode = true;
    currentAngle = DEFAULT_POSITION;
    increasing = true;
    response = "AUTO:ON";
  }
  else if (cmd == "AUTO:OFF") {
    autoMode = false;
    response = "AUTO:OFF";
  }
  // Comando de status
  else if (cmd == "STATUS") {
    sendStatusResponse();
    return; // Não envia resposta adicional
  }
  // Controle todos os servos
  else if (cmd.startsWith("ALL:")) {
    int angle = cmd.substring(4).toInt();
    angle = constrain(angle, 0, 180);
    moveAllServos(angle);
    response = "ALL:" + String(angle);
  }
  // Controle servo individual
  else if (cmd.startsWith("SERVO:")) {
    int colonPos = cmd.indexOf(':', 6);
    if (colonPos > 0) {
      int num = cmd.substring(6, colonPos).toInt();
      int angle = cmd.substring(colonPos + 1).toInt();
      if (num >= 0 && num <= 4) {
        angle = constrain(angle, 0, 180);
        moveServo(num, angle);
        response = "SERVO:" + String(num) + ":" + String(angle);
      } else {
        response = "ERROR:INVALID_SERVO";
      }
    } else {
      response = "ERROR:INVALID_FORMAT";
    }
  }
  // NOVO: Comando de gestos
  else if (cmd.startsWith("GESTURE:")) {
    String gestureName = cmd.substring(8);
    gestureName.toUpperCase(); // Garante maiúsculas
    executeGesture(gestureName);
    return; // executeGesture já envia a resposta
  }
  // Comandos legados de compatibilidade
  else if (cmd.indexOf("ON") != -1) {
    autoMode = true;
    response = "AUTO:ON";
  }
  else if (cmd.indexOf("OFF") != -1) {
    autoMode = false;
    response = "AUTO:OFF";
  }
  // Comando não reconhecido
  else {
    response = "ERROR:UNKNOWN_COMMAND:" + cmd;
    Serial.println("❌ Comando não reconhecido: " + cmd);
  }
  
  sendBLEResponse(response);
}

// ---------- Loop principal ----------
void loop() {
  static unsigned long lastMove = 0;
  
  // Botões físicos
  checkEmergencyButton();
  checkToggleEnableButton();

  // Controle de movimento e LEDs
  if (!emergencyStop && servosEnabled) {
    if (autoMode && millis() - lastMove > MOVE_SPEED) {
      lastMove = millis();
      executeMovement();
      updateFingerLeds();
    } else if (!autoMode) {
      updateFingerLedsManual();
    }
  } else {
    // Parado - apaga LEDs de dedo
    for (int i = 0; i < 5; i++) {
      digitalWrite(fingerLedPins[i], LOW);
    }
  }

  // Status LEDs sempre
  updateLedStatus();

  delay(10); // Pequeno delay no loop principal
}