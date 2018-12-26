/**

  BRUTUS V2

  // Como criar threads no ESP32
  xTaskCreatePinnedToCore(
  Task1,                  pvTaskCode
  "Workload1",            pcName
  1000,                   usStackDepth
  NULL,                   pvParameters
  1,                      uxPriority
  &TaskA,                 pxCreatedTask
  0);                     xCoreID

**/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <SimpleBLE.h>
#include <BluetoothSerial.h>

#define SERVICE_UUID   "ab0828b1-198e-4351-b779-901fa0e0371e"
#define CHARACTERISTIC_UUID_RX  "4ac8a682-9736-4e5d-932b-e9b31405049c"
#define CHARACTERISTIC_UUID_TX  "0972EF8C-7613-4075-AD52-756F33D4DA91"

// Variaveis

TaskHandle_t TaskA, TaskB;
BLECharacteristic *characteristicTX;

//SimpleBLE ble;
BluetoothSerial SerialBT;

bool deviceConnected = false;
bool usbPort = false;
bool lantern = false;
bool internLeds = false;
bool alarm = false;
bool soundVolume = 0;

// classes auxiliares

// callback para receber os eventos de conexão de dispositivos
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

//callback para eventos das características
class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) {

      //retorna ponteiro para o registrador contendo o valor atual da caracteristica
      std::string rxValue = characteristic->getValue();

      //verifica se existe dados (tamanho maior que zero)
      if (rxValue.length() > 0) {

      }
    }//onWrite
};

void setup() {

  Serial.begin(115200);

  SerialBT.begin("BrutusV2");

  //initBLE();
  initThreads();
}

void initBLE() {

  // Create the BLE Device
  BLEDevice::init("brutus"); // nome do dispositivo bluetooth

  // Create the BLE Server
  BLEServer *server = BLEDevice::createServer(); //cria um BLE server

  server->setCallbacks(new ServerCallbacks()); //seta o callback do server

  // Create the BLE Service
  BLEService *service = server->createService(SERVICE_UUID);

  // Create a BLE Characteristic para envio de dados
  characteristicTX = service->createCharacteristic(
                       CHARACTERISTIC_UUID_TX,
                       BLECharacteristic::PROPERTY_NOTIFY
                     );

  characteristicTX->addDescriptor(new BLE2902());
}

void initThreads() {

  xTaskCreatePinnedToCore(Task1, "Workload1", 1000, NULL, 1, &TaskA, 0);
  xTaskCreatePinnedToCore(Task2, "Workload2", 1000, NULL, 1, &TaskB, 1);
}

void loop() {

  //ble.begin("Hello world");

  String value = "";
  while (SerialBT.available() > 0) {
    value += (char)SerialBT.read();
  }
  if (value.length() != 0) {
    Serial.println(value);
  }

}

void Task1(void * parameter) {

  for (;;) {

    float randomTemp = random(5) + 25;
    
    SerialBT.println("iled0;lant1;temp" + String(randomTemp) + ";");

    delay(100) ;
  }
}

void Task2(void * parameter) {

  for (;;) {
    unsigned long start = millis();   // ref: https://github.com/espressif/arduino-esp32/issues/384

    Serial.println("Task 2 complete running on Core " + String(xPortGetCoreID()));

    delay(1000) ;
  }
}
