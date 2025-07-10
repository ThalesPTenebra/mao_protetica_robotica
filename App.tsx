import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  Alert,
  ScrollView,
  Switch,
  StatusBar,
  Vibration,
  Platform,
  PermissionsAndroid,
} from 'react-native';
import Slider from '@react-native-community/slider';
import { Buffer } from 'buffer';
import {
  BleManager,
  Device,
  Service,
  Characteristic,
} from 'react-native-ble-plx';

const SERVICE_UUID = '12345678-1234-1234-1234-123456789abc';
const CHARACTERISTIC_UUID = '87654321-4321-4321-4321-cba987654321';
const DEVICE_NAME = 'MaoRobotica_ESP32';

interface ServoAngles {
  [key: number]: number;
}

export default function App() {
  // Estados principais
  const [bleManager] = useState(new BleManager());
  const [device, setDevice] = useState<Device | null>(null);
  const [characteristic, setCharacteristic] = useState<Characteristic | null>(
    null,
  );
  const [isConnected, setIsConnected] = useState(false);
  const [isScanning, setIsScanning] = useState(false);
  const [scanTimeoutId, setScanTimeoutId] = useState<NodeJS.Timeout | null>(
    null,
  );

  // Estados do sistema
  const [servosEnabled, setServosEnabled] = useState(true);
  const [emergencyStop, setEmergencyStop] = useState(false);
  const [autoMode, setAutoMode] = useState(true);
  const [connectionStatus, setConnectionStatus] = useState('Desconectado');

  // Estados dos servos individuais
  const [servoAngles, setServoAngles] = useState<ServoAngles>({
    0: 90, // Polegar
    1: 90, // Indicador
    2: 90, // Médio
    3: 90, // Anelar
    4: 90, // Mindinho
  });

  const fingerNames = ['Polegar', 'Indicador', 'Médio', 'Anelar', 'Mindinho'];

  useEffect(() => {
    // Verificar permissões na inicialização
    checkPermissions();

    return () => {
      if (scanTimeoutId) {
        clearTimeout(scanTimeoutId);
      }
      if (device) {
        device.cancelConnection();
      }
      bleManager.destroy();
    };
  }, []);

  // Função para verificar e solicitar permissões
  const checkPermissions = async () => {
    if (Platform.OS === 'android') {
      try {
        const granted = await PermissionsAndroid.requestMultiple([
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
          PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        ]);

        const allPermissionsGranted = Object.values(granted).every(
          permission => permission === PermissionsAndroid.RESULTS.GRANTED,
        );

        if (!allPermissionsGranted) {
          Alert.alert(
            'Permissões necessárias',
            'O app precisa de permissões de Bluetooth e localização para funcionar corretamente.',
            [{ text: 'OK' }],
          );
        }
      } catch (err) {
        console.warn('Erro ao solicitar permissões:', err);
      }
    }
  };

  // Função para escanear dispositivos BLE
  const scanForDevices = async () => {
    // Verificar se o Bluetooth está habilitado
    const state = await bleManager.state();
    if (state !== 'PoweredOn') {
      Alert.alert(
        'Bluetooth Desabilitado',
        'Por favor, habilite o Bluetooth para continuar.',
        [{ text: 'OK' }],
      );
      return;
    }

    setIsScanning(true);
    setConnectionStatus('Procurando...');

    // Limpar timeout anterior se existir
    if (scanTimeoutId) {
      clearTimeout(scanTimeoutId);
      setScanTimeoutId(null);
    }

    let scanFinished = false;

    const handleDeviceFound = async (scannedDevice: Device) => {
      if (scanFinished) return; // Evita múltiplas tentativas

      scanFinished = true;
      bleManager.stopDeviceScan();
      setIsScanning(false);

      // Limpar timeout imediatamente
      if (scanTimeoutId) {
        clearTimeout(scanTimeoutId);
        setScanTimeoutId(null);
      }

      try {
        await connectToDevice(scannedDevice);
      } catch (error) {
        console.log('Erro na conexão após encontrar dispositivo:', error);
        scanFinished = false; // Permite nova tentativa se falhar
      }
    };

    bleManager.startDeviceScan(null, null, (error, scannedDevice) => {
      if (error) {
        console.log('Erro no scan:', error);
        setIsScanning(false);
        setConnectionStatus('Erro no scan');
        scanFinished = true;
        return;
      }

      if (
        !scanFinished &&
        (scannedDevice?.name === DEVICE_NAME ||
          scannedDevice?.localName === DEVICE_NAME)
      ) {
        handleDeviceFound(scannedDevice);
      }
    });

    // Timeout do scan
    const timeoutId = setTimeout(() => {
      if (!scanFinished) {
        bleManager.stopDeviceScan();
        setIsScanning(false);
        scanFinished = true;
        setConnectionStatus('Dispositivo não encontrado');
      }
      setScanTimeoutId(null);
    }, 10000);

    setScanTimeoutId(timeoutId);
  };

  // Função para conectar ao dispositivo
  const connectToDevice = async (scannedDevice: Device) => {
    try {
      setConnectionStatus('Conectando...');

      const connectedDevice = await scannedDevice.connect();
      setDevice(connectedDevice);

      await connectedDevice.discoverAllServicesAndCharacteristics();

      const services = await connectedDevice.services();
      const targetService = services.find(
        service => service.uuid === SERVICE_UUID,
      );

      if (targetService) {
        const characteristics = await targetService.characteristics();
        const targetCharacteristic = characteristics.find(
          char => char.uuid === CHARACTERISTIC_UUID,
        );

        if (targetCharacteristic) {
          setCharacteristic(targetCharacteristic);

          // IMPORTANTE: Definir como conectado ANTES de outras operações
          setIsConnected(true);
          setConnectionStatus('Conectado');

          // Monitorar desconexões
          connectedDevice.onDisconnected((error, device) => {
            console.log('Dispositivo desconectado:', error);
            setDevice(null);
            setCharacteristic(null);
            setIsConnected(false);
            setConnectionStatus('Desconectado');
          });

          // Monitorar notificações
          targetCharacteristic.monitor((error, char) => {
            if (error) {
              console.log('Erro na notificação:', error);
              return;
            }
            if (char?.value) {
              const response = Buffer.from(char.value, 'base64').toString();
              console.log('Resposta recebida:', response);
              handleBLEResponse(response);
            }
          });

          // Solicitar status inicial
          await sendCommand('STATUS');

          Vibration.vibrate(200);
          Alert.alert('Sucesso', 'Conectado à mão robótica!');
        } else {
          throw new Error('Característica não encontrada');
        }
      } else {
        throw new Error('Serviço não encontrado');
      }
    } catch (error) {
      console.log('Erro na conexão:', error);
      setConnectionStatus('Erro na conexão');
      setIsConnected(false);
      setDevice(null);
      setCharacteristic(null);
      Alert.alert('Erro', 'Falha ao conectar ao dispositivo');
      throw error; // Re-throw para que scanForDevices saiba que falhou
    }
  };

  // Função para desconectar
  const disconnectDevice = async () => {
    try {
      if (device) {
        await device.cancelConnection();
        setDevice(null);
        setCharacteristic(null);
        setIsConnected(false);
        setConnectionStatus('Desconectado');
        Alert.alert('Desconectado', 'Dispositivo desconectado com sucesso');
      }
    } catch (error) {
      console.log('Erro ao desconectar:', error);
    }
  };

  // Função para enviar comandos via BLE
  const sendCommand = async (command: string) => {
    if (!characteristic || !isConnected) {
      console.log('Tentativa de envio sem conexão ativa:', command);
      return false;
    }

    try {
      const commandBuffer = Buffer.from(command, 'utf8');
      const base64Command = commandBuffer.toString('base64');

      await characteristic.writeWithResponse(base64Command);
      console.log('Comando enviado:', command);
      return true;
    } catch (error) {
      console.log('Erro ao enviar comando:', error);
      // Não mostrar alert para evitar spam de erros
      return false;
    }
  };

  // Função para lidar com respostas BLE
  const handleBLEResponse = (response: string) => {
    try {
      if (response.startsWith('STATUS:')) {
        const data = JSON.parse(response.substring(7));
        setServosEnabled(data.servosEnabled);
        setEmergencyStop(data.emergencyStop);
        setAutoMode(data.autoMode);
        if (data.angles) {
          setServoAngles(data.angles);
        }
      } else if (response.startsWith('ERROR:')) {
        const errorMessage = response.substring(6);
        Alert.alert('Erro do dispositivo', errorMessage);
        console.log('Erro do dispositivo:', errorMessage);
      } else if (response.startsWith('UNKNOWN_COMMAND')) {
        Alert.alert(
          'Comando não reconhecido',
          'O dispositivo não reconheceu o comando enviado',
        );
        console.log('Comando não reconhecido pelo dispositivo');
      }
    } catch (error) {
      console.log('Erro ao processar resposta:', error);
    }
  };

  // Função para controlar servo individual
  const controlServo = (servoIndex: number, angle: number) => {
    if (!isConnected) {
      Alert.alert('Erro', 'Dispositivo não conectado');
      return;
    }

    if (emergencyStop) {
      Alert.alert(
        'Parada de emergência',
        'Desative a parada de emergência primeiro',
      );
      return;
    }

    const newAngles = { ...servoAngles, [servoIndex]: angle };
    setServoAngles(newAngles);
    sendCommand(`SERVO:${servoIndex}:${angle}`);
  };

  // Função para controlar todos os servos
  const controlAllServos = (angle: number) => {
    if (!isConnected) {
      Alert.alert('Erro', 'Dispositivo não conectado');
      return;
    }

    if (emergencyStop) {
      Alert.alert(
        'Parada de emergência',
        'Desative a parada de emergência primeiro',
      );
      return;
    }

    const newAngles: ServoAngles = {};
    for (let i = 0; i < 5; i++) {
      newAngles[i] = angle;
    }
    setServoAngles(newAngles);
    sendCommand(`ALL:${angle}`);
  };

  // Gestos pré-definidos - FORMATO ALTERNATIVO
  const executeGesture = (gesture: string) => {
    if (!isConnected) {
      Alert.alert('Erro', 'Dispositivo não conectado');
      return;
    }

    if (emergencyStop) {
      Alert.alert(
        'Parada de emergência',
        'Desative a parada de emergência primeiro',
      );
      return;
    }

    let angles: ServoAngles = {};

    switch (gesture) {
      case 'OPEN':
        angles = { 0: 180, 1: 180, 2: 180, 3: 180, 4: 180 };
        break;
      case 'CLOSE':
        angles = { 0: 0, 1: 0, 2: 0, 3: 0, 4: 0 };
        break;
      case 'PEACE':
        angles = { 0: 0, 1: 180, 2: 180, 3: 0, 4: 0 };
        break;
      case 'OK':
        angles = { 0: 45, 1: 45, 2: 180, 3: 180, 4: 180 };
        break;
      case 'POINT':
        angles = { 0: 0, 1: 180, 2: 0, 3: 0, 4: 0 };
        break;
      case 'ROCK':
        angles = { 0: 0, 1: 180, 2: 0, 3: 0, 4: 180 };
        break;
      default:
        angles = { 0: 90, 1: 90, 2: 90, 3: 90, 4: 90 };
    }

    setServoAngles(angles);
    sendCommand(`GESTURE:${gesture}`);
  };

  // Função para parada de emergência
  const toggleEmergencyStop = () => {
    if (!isConnected) {
      Alert.alert('Erro', 'Dispositivo não conectado');
      return;
    }

    const newState = !emergencyStop;
    setEmergencyStop(newState);
    sendCommand(newState ? 'EMERGENCY:ON' : 'EMERGENCY:OFF');

    if (newState) {
      Vibration.vibrate([200, 100, 200]);
      Alert.alert('Parada de Emergência', 'Todos os servos foram parados!');
    }
  };

  // Função para alternar modo automático
  const toggleAutoMode = () => {
    if (!isConnected) {
      Alert.alert('Erro', 'Dispositivo não conectado');
      return;
    }

    const newState = !autoMode;
    setAutoMode(newState);
    sendCommand(newState ? 'AUTO:ON' : 'AUTO:OFF');
  };

  // Função para habilitar/desabilitar servos
  const toggleServos = () => {
    if (!isConnected) {
      Alert.alert('Erro', 'Dispositivo não conectado');
      return;
    }

    const newState = !servosEnabled;
    setServosEnabled(newState);
    sendCommand(newState ? 'SERVOS:ON' : 'SERVOS:OFF');
  };

  return (
    <View style={styles.container}>
      <StatusBar barStyle="light-content" backgroundColor="#2c3e50" />

      <View style={styles.header}>
        <Text style={styles.title}>Controle Mão Robótica</Text>
        <Text
          style={[
            styles.status,
            { color: isConnected ? '#27ae60' : '#e74c3c' },
          ]}
        >
          {connectionStatus}
        </Text>
      </View>

      <ScrollView
        style={styles.scrollView}
        showsVerticalScrollIndicator={false}
      >
        {/* Seção de Conexão */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Conexão BLE</Text>
          {!isConnected ? (
            <TouchableOpacity
              style={[styles.button, styles.connectButton]}
              onPress={scanForDevices}
              disabled={isScanning}
            >
              <Text style={styles.buttonText}>
                {isScanning ? 'Procurando...' : 'Conectar'}
              </Text>
            </TouchableOpacity>
          ) : (
            <TouchableOpacity
              style={[styles.button, styles.disconnectButton]}
              onPress={disconnectDevice}
            >
              <Text style={styles.buttonText}>Desconectar</Text>
            </TouchableOpacity>
          )}
        </View>

        {/* Controles do Sistema */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Sistema</Text>

          <View style={styles.switchRow}>
            <Text style={styles.switchLabel}>Servos Habilitados</Text>
            <Switch
              value={servosEnabled}
              onValueChange={toggleServos}
              disabled={!isConnected}
              thumbColor={servosEnabled ? '#27ae60' : '#95a5a6'}
              trackColor={{ false: '#bdc3c7', true: '#2ecc71' }}
            />
          </View>

          <View style={styles.switchRow}>
            <Text style={styles.switchLabel}>Modo Automático</Text>
            <Switch
              value={autoMode}
              onValueChange={toggleAutoMode}
              disabled={!isConnected}
              thumbColor={autoMode ? '#3498db' : '#95a5a6'}
              trackColor={{ false: '#bdc3c7', true: '#5dade2' }}
            />
          </View>

          <TouchableOpacity
            style={[
              styles.button,
              emergencyStop
                ? styles.emergencyActiveButton
                : styles.emergencyButton,
            ]}
            onPress={toggleEmergencyStop}
            disabled={!isConnected}
          >
            <Text style={styles.buttonText}>
              {emergencyStop ? 'Desativar Emergência' : 'Parada de Emergência'}
            </Text>
          </TouchableOpacity>
        </View>

        {/* Controles Individuais dos Servos */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Controle Individual</Text>

          {fingerNames.map((finger, index) => (
            <View key={index} style={styles.servoControl}>
              <Text style={styles.servoLabel}>
                {finger}: {servoAngles[index]}°
              </Text>
              <Slider
                style={styles.slider}
                minimumValue={0}
                maximumValue={180}
                value={servoAngles[index]}
                onValueChange={value => controlServo(index, Math.round(value))}
                disabled={!isConnected || emergencyStop || !servosEnabled}
                minimumTrackTintColor="#3498db"
                maximumTrackTintColor="#bdc3c7"
              />
            </View>
          ))}
        </View>

        {/* Controle Geral */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Controle Geral</Text>

          <View style={styles.buttonRow}>
            <TouchableOpacity
              style={[styles.button, styles.secondaryButton]}
              onPress={() => controlAllServos(0)}
              disabled={!isConnected || emergencyStop}
            >
              <Text style={styles.buttonText}>Fechar Tudo</Text>
            </TouchableOpacity>

            <TouchableOpacity
              style={[styles.button, styles.secondaryButton]}
              onPress={() => controlAllServos(180)}
              disabled={!isConnected || emergencyStop}
            >
              <Text style={styles.buttonText}>Abrir Tudo</Text>
            </TouchableOpacity>
          </View>
        </View>

        {/* Gestos Pré-definidos */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Gestos</Text>

          <View style={styles.gestureGrid}>
            <TouchableOpacity
              style={styles.gestureButton}
              onPress={() => executeGesture('OPEN')}
              disabled={!isConnected || emergencyStop}
            >
              <Text style={styles.gestureText}>✋</Text>
              <Text style={styles.gestureLabel}>Abrir</Text>
            </TouchableOpacity>

            <TouchableOpacity
              style={styles.gestureButton}
              onPress={() => executeGesture('CLOSE')}
              disabled={!isConnected || emergencyStop}
            >
              <Text style={styles.gestureText}>✊</Text>
              <Text style={styles.gestureLabel}>Fechar</Text>
            </TouchableOpacity>

            <TouchableOpacity
              style={styles.gestureButton}
              onPress={() => executeGesture('PEACE')}
              disabled={!isConnected || emergencyStop}
            >
              <Text style={styles.gestureText}>✌️</Text>
              <Text style={styles.gestureLabel}>Paz</Text>
            </TouchableOpacity>

            <TouchableOpacity
              style={styles.gestureButton}
              onPress={() => executeGesture('OK')}
              disabled={!isConnected || emergencyStop}
            >
              <Text style={styles.gestureText}>👌</Text>
              <Text style={styles.gestureLabel}>OK</Text>
            </TouchableOpacity>

            <TouchableOpacity
              style={styles.gestureButton}
              onPress={() => executeGesture('POINT')}
              disabled={!isConnected || emergencyStop}
            >
              <Text style={styles.gestureText}>👆</Text>
              <Text style={styles.gestureLabel}>Apontar</Text>
            </TouchableOpacity>

            <TouchableOpacity
              style={styles.gestureButton}
              onPress={() => executeGesture('ROCK')}
              disabled={!isConnected || emergencyStop}
            >
              <Text style={styles.gestureText}>🤘</Text>
              <Text style={styles.gestureLabel}>Rock</Text>
            </TouchableOpacity>
          </View>
        </View>
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#34495e',
  },
  header: {
    backgroundColor: '#2c3e50',
    padding: 20,
    paddingTop: 40,
    alignItems: 'center',
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#ecf0f1',
    marginBottom: 5,
  },
  status: {
    fontSize: 16,
    fontWeight: '500',
  },
  scrollView: {
    flex: 1,
    padding: 20,
  },
  section: {
    backgroundColor: '#ecf0f1',
    borderRadius: 10,
    padding: 15,
    marginBottom: 15,
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#2c3e50',
    marginBottom: 15,
  },
  button: {
    backgroundColor: '#3498db',
    padding: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginVertical: 5,
  },
  connectButton: {
    backgroundColor: '#27ae60',
  },
  disconnectButton: {
    backgroundColor: '#e74c3c',
  },
  emergencyButton: {
    backgroundColor: '#f39c12',
  },
  emergencyActiveButton: {
    backgroundColor: '#e74c3c',
  },
  secondaryButton: {
    backgroundColor: '#95a5a6',
    flex: 1,
    marginHorizontal: 5,
  },
  buttonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '600',
  },
  buttonRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 15,
  },
  switchLabel: {
    fontSize: 16,
    color: '#2c3e50',
  },
  servoControl: {
    marginBottom: 20,
  },
  servoLabel: {
    fontSize: 16,
    color: '#2c3e50',
    marginBottom: 10,
    fontWeight: '500',
  },
  slider: {
    width: '100%',
    height: 40,
  },
  gestureGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
  },
  gestureButton: {
    backgroundColor: '#3498db',
    width: '30%',
    aspectRatio: 1,
    borderRadius: 10,
    justifyContent: 'center',
    alignItems: 'center',
    marginBottom: 10,
  },
  gestureText: {
    fontSize: 30,
    marginBottom: 5,
  },
  gestureLabel: {
    color: '#fff',
    fontSize: 12,
    fontWeight: '500',
  },
});
