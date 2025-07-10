Claro\! Criei um `README.md` completo e bem estruturado para o seu projeto. Ele explica não apenas _o que_ o código faz, mas também _o porquê_ de cada decisão de arquitetura, como o uso de RTOS e JSON.

---

# 🤖 Mão Robótica com ESP32, PCA9685 e BLE

Este é o firmware para uma mão robótica de 5 servos controlada por um microcontrolador ESP32. A comunicação é feita via Bluetooth Low Energy (BLE), permitindo o controle total dos movimentos, a execução de gestos pré-programados e o monitoramento de status em tempo real a partir de um smartphone ou outro dispositivo central BLE.

## ✨ Features Principais

- **Controle Remoto via BLE:** Comunicação sem fio para operar a mão à distância.
- **Controle Preciso de 5 Servos:** Utiliza o driver **PCA9685** para controlar 5 servos de forma independente via I2C, liberando GPIOs do ESP32.
- **Sistema de Gestos:** Implementa uma lista de gestos pré-definidos (Mão Aberta, Fechada, Paz, OK, etc.) que podem ser acionados por um simples comando.
- **Modos de Operação:**
  - **Modo Manual:** Controle individual de cada servo ou de todos simultaneamente.
  - **Modo Automático:** Executa um movimento de demonstração contínuo.
- **Recursos de Segurança:**
  - **Botão de Emergência:** Um botão físico que para todos os movimentos e coloca a mão em uma posição neutra.
  - **Habilitar/Desabilitar Servos:** Um botão físico que corta a energia dos servos (via pino `OE` do PCA9685), útil para economizar energia ou ajustar a mão manualmente.
- **Arquitetura com RTOS:** Utiliza uma task dedicada no FreeRTOS para gerenciar a comunicação BLE, garantindo que o controle dos motores e a interface sem fio não interfiram um no outro.
- **Comunicação Estruturada:** Usa **JSON** para enviar pacotes de status completos, facilitando a integração com aplicativos clientes.
- **Feedback Visual:** LEDs de status indicam o estado da conexão, emergência e atividade dos servos.

## 🛠️ Hardware Necessário

- Microcontrolador ESP32 (qualquer Dev Kit).
- Módulo Driver PWM I2C de 16 canais **PCA9685**.
- 5x Servo Motores (e.g., SG90, MG996R).
- Estrutura de mão robótica para acoplar os servos.
- 2x Botões de pressão (Push Buttons) para Emergência e Habilitação.
- 7x LEDs (5 para os dedos, 1 para status, 1 para emergência).
- Resistores apropriados para os LEDs e pull-up para os botões.
- Fonte de alimentação externa para os servos (muito importante para não sobrecarregar o ESP32).
- Protoboard e jumpers para as conexões.

## 🔌 Pinagem (Wiring)

A conexão entre os componentes é definida pelas constantes no topo do código:

| Componente          | Pino no ESP32           | Conectado a...                  | Propósito                                         |
| ------------------- | ----------------------- | ------------------------------- | ------------------------------------------------- |
| **Comunicação I2C** | `GPIO 21 (SDA)`         | Pino SDA do PCA9685             | Linha de dados para o controle dos servos.        |
|                     | `GPIO 22 (SCL)`         | Pino SCL do PCA9685             | Linha de clock para o controle dos servos.        |
| **Driver PCA9685**  | `GPIO 23`               | Pino `OE` (Output Enable)       | Habilita/Desabilita a saída para todos os servos. |
| **Botões Físicos**  | `GPIO 18`               | Botão de Emergência             | Ativa/Desativa a parada de emergência.            |
|                     | `GPIO 19`               | Botão de Habilitação dos Servos | Liga/Desliga a alimentação dos servos.            |
| **LEDs de Dedo**    | `GPIO 4, 5, 12, 13, 14` | LEDs individuais dos dedos      | Indicam atividade nos dedos.                      |
| **LEDs de Status**  | `GPIO 2`                | LED de Emergência               | Pisca quando a emergência está ativa.             |
|                     | `GPIO 15`               | LED de Status Geral             | Indica que o sistema está operante.               |

## 🧠 Como Funciona

### Estrutura do Código

O código é organizado em seções lógicas:

1.  **Constantes e Variáveis Globais:** Define pinos, configurações de servo, UUIDs BLE e variáveis de estado que controlam o comportamento da mão.
2.  **`setup()`:** Inicializa a comunicação serial, o barramento I2C, o driver PCA9685, os pinos (GPIOs), a comunicação BLE e a task do RTOS.
3.  **`loop()`:** O loop principal é simples. Ele é responsável por ler os botões físicos e executar os movimentos (automáticos ou manuais) com base no estado atual.
4.  **Funções de Controle:** Funções específicas como `moveServo()`, `executeGesture()` e `processBluetoothCommand()` encapsulam a lógica principal.
5.  **Task do RTOS (`task_ble`):** Uma tarefa separada que roda em um dos núcleos do ESP32, dedicada exclusivamente a gerenciar a conexão BLE e enviar status periódicos.

### O porquê do RTOS (FreeRTOS)

O ESP32 possui dois núcleos. Usar uma task de RTOS (`xTaskCreatePinnedToCore`) para o BLE é uma prática recomendada para projetos complexos.

- **Estabilidade:** A pilha BLE pode ter momentos de alta atividade que poderiam travar ou atrasar o loop principal, causando movimentos "engasgados" nos servos.
- **Paralelismo:** A `task_ble` é fixada no **Core 0**, enquanto o `setup()` e o `loop()` do Arduino rodam por padrão no **Core 1**. Isso permite que a comunicação sem fio e o controle dos motores ocorram em paralelo, sem que uma afete o desempenho da outra.

### Comunicação BLE e Protocolo

O ESP32 atua como um servidor GATT (Generic Attribute Profile). Ele cria um serviço com uma característica que permite leitura, escrita e notificação.

- **Serviço UUID:** `12345678-1234-1234-1234-123456789abc`
- **Característica UUID:** `87654321-4321-4321-4321-cba987654321`

Um aplicativo cliente (como o **nRF Connect** para Android/iOS) se conecta ao ESP32 e escreve strings de comando nesta característica para controlar a mão.

#### **Comandos Aceitos (API)**

| Comando                | Descrição                                                                                                 | Exemplo de Resposta                 |
| ---------------------- | --------------------------------------------------------------------------------------------------------- | ----------------------------------- |
| `EMERGENCY:ON` / `OFF` | Ativa ou desativa a parada de emergência.                                                                 | `EMERGENCY:ON`                      |
| `SERVOS:ON` / `OFF`    | Habilita ou desabilita os servos através do pino `OE`.                                                    | `SERVOS:OFF`                        |
| `AUTO:ON` / `OFF`      | Liga ou desliga o modo de movimento automático.                                                           | `AUTO:ON`                           |
| `ALL:<angle>`          | Move todos os servos para um ângulo específico (0-180).                                                   | `ALL:90`                            |
| `SERVO:<num>:<angle>`  | Move um servo individual (`num` 0-4) para um ângulo (`angle` 0-180).                                      | `SERVO:1:180`                       |
| `GESTURE:<name>`       | Executa um gesto pré-definido. `name` pode ser: `OPEN`, `CLOSE`, `PEACE`, `OK`, `POINT`, `ROCK`, `RESET`. | `GESTURE:PEACE:OK`                  |
| `STATUS`               | Solicita o estado atual completo da mão.                                                                  | `STATUS:{"servosEnabled":true,...}` |

#### **Resposta de Status (JSON)**

Quando o comando `STATUS` é enviado ou a cada 5 segundos quando conectado, o ESP32 envia uma notificação com um JSON contendo o estado completo:

```json
STATUS:{"servosEnabled":true,"emergencyStop":false,"autoMode":false,"angles":{"0":90,"1":180,"2":180,"3":0,"4":0}}
```

Essa estrutura facilita para o aplicativo cliente exibir informações detalhadas sobre a mão.

### Sistema de Gestos

A implementação dos gestos é feita de forma modular e fácil de expandir através de uma `struct` e um array:

```c++
struct Gesture {
  const char* name;
  int angles[5];
};

const Gesture gestures[] = {
  {"OPEN",  {180, 180, 180, 180, 180}},
  {"CLOSE", {0, 0, 0, 0, 0}},
  // ... outros gestos
};
```

Para adicionar um novo gesto, basta adicionar uma nova linha a este array, sem precisar alterar a lógica de execução. A função `executeGesture()` simplesmente procura o nome do gesto no array e aplica os ângulos correspondentes.

## 🚀 Como Compilar e Usar

1.  **Instale o Ambiente:**

    - Tenha a [Arduino IDE](https://www.arduino.cc/en/software) instalada.
    - Adicione o suporte para placas ESP32 na IDE (via Gerenciador de Placas).

2.  **Instale as Bibliotecas:**

    - No menu da Arduino IDE, vá para `Ferramentas > Gerenciar Bibliotecas...`.
    - Procure e instale as seguintes bibliotecas:
      - `Adafruit PWM Servo Driver Library` por Adafruit.
      - `ArduinoJson` por Benoit Blanchon.

3.  **Carregue o Código:**

    - Abra o arquivo `.ino` na Arduino IDE.
    - Selecione a sua placa ESP32 em `Ferramentas > Placa`.
    - Selecione a porta COM correta em `Ferramentas > Porta`.
    - Clique no botão "Carregar" (seta para a direita).

4.  **Teste a Conexão:**

    - Após carregar o código, abra um aplicativo de scanner BLE (como o **nRF Connect** ou **LightBlue**).
    - Procure por um dispositivo chamado `MaoRobotica_ESP32`.
    - Conecte-se a ele e encontre a característica com o UUID `87654321-...`.
    - Use a opção de escrita (write) para enviar os comandos listados na tabela acima e controlar a mão.

---
