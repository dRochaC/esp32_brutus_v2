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

#include <BluetoothSerial.h>
#include <Wire.h>
#include <SSD1306.h>
#include <Wtv020sd16p.h>
#include <WiFi.h>
#include <I2Cdev.h>
#include <MPU6050.h>
#include <EEPROM.h>
//#include <ArduinoOTA.h>

// Build configs
#define MOCK_MODULE_1  true
#define MOCK_MODULE_1_COMMAND  "mocked_controller:multiplier"
#define BLUETOOTH_NAME  "BrutusV2"
#define PRINT_ALL  true

#define OBJ_INTERN_LED  "internLed"
#define OBJ_LANTERN  "lantern"
#define OBJ_TEMP  "temp"
#define OBJ_ALARM  "alarm"
#define OBJ_USB_PORT  "usbPort"
#define OBJ_MODULES  "modules"
#define OBJ_VOLUME  "volume"
#define OBJ_STOP_MUSIC  "stopMusic"
#define OBJ_IS_PLAYING  "isPlaying"
#define OBJ_WIFI  "wifi"
#define OBJ_WIFI_NAME  "wifiName"
#define OBJ_OTA  "ota"

#define OBJ_MODULE_1  "module1"

// Portas
#define INTERN_LED_PIN  16
#define PORT_LANTERN_PIN  17
#define PORT_USB_PORT_PIN  32
#define PORT_MODULES_PIN  33

#define AUDIO_RESET_PIN 19
#define AUDIO_CLOCK_PIN 18
#define AUDIO_DATA_PIN 15
#define AUDIO_BUSY_PIN 5

#define MPU 0x68

// Variaveis

MPU6050 accelgyro;

BluetoothSerial SerialBT;

SSD1306  display(0x3c, 21, 22);

Wtv020sd16p wtv020sd16p(AUDIO_RESET_PIN, AUDIO_CLOCK_PIN, AUDIO_DATA_PIN, AUDIO_BUSY_PIN);

long lastTime = 0;

bool deviceConnected = false;

bool internLeds = false;
bool lantern = false;
float temp = 0;
bool alarmSet = false;
bool usbPort = false;
bool modules = false;
bool wifiEnabled = false;
bool otaEnabled = false;
int volume = 4; // 50%

// Modulo mockado
bool multiplier = false;

// Sensores inerciais
int16_t ax, ay, az;
int16_t gx, gy, gz;

// Wifi
const char* ssid = "NETvirtua102";
const char* password =  "wificastro";

// classes auxiliares

class JsonArray {
  private:
    String root = "";
    int first = true;

  public:
    void putElement(String value) {
      if (!first) {
        root += ",";
      }

      root += value;

      first = false;
    }
    String generate() {
      return first ? "" : "[" + root + "]";
    }
};

class Json {
  private:
    String root = "";
    int first = true;
    void internPutValue(String key, String value) {

      if (!first) {
        root += ",";
      }

      root += "\"" + key + "\":" + value;

      first = false;
    }
    String putMarks(String value) {
      return "\"" + value + "\"";
    }

  public:
    void putArray(String key, JsonArray jsonArray) {
      internPutValue(key, jsonArray.generate());
    }
    void putArray(String key, String jsonArray) {
      internPutValue(key, jsonArray);
    }
    void putString(String key, String value) {
      internPutValue(key, putMarks(value));
    }
    void putFloat(String key, float value) {
      internPutValue(key, String(value));
    }
    void putInt(String key, int value) {
      internPutValue(key, String(value));
    }
    void putBool(String key, bool value) {
      String nValue = "false";
      if (value) {
        nValue = "true";
      }
      internPutValue(key, putMarks(nValue));
    }
    String generate() {
      return first ? "" : "{" + root + "}";
    }
};

void setup() {

  // Desabilita wifi
  WiFi.mode(WIFI_MODE_NULL);
  //btStop();

  Serial.begin(115200);

  display.init();
  display.drawString(0, 0, "booting...");
  display.display();

  // Som de inicialização
  wtv020sd16p.reset();
  delay(1000);
  appPrintln("booting...");
  playAudio(0);

  SerialBT.begin(BLUETOOTH_NAME);

  accelgyro.initialize();
  String mpuMessage = accelgyro.testConnection() ? "InertialSensors...[OK]" : "InertialSensors...[ERROR]";
  appPrintln(mpuMessage);
  display.drawString(0, 12, mpuMessage);
  display.display();

  pinMode(INTERN_LED_PIN, OUTPUT);
  pinMode(PORT_LANTERN_PIN, OUTPUT);
  pinMode(PORT_USB_PORT_PIN, OUTPUT);
  pinMode(PORT_MODULES_PIN, OUTPUT);
}

void loop() {

  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  temp = accelgyro.getTemperature() / 340.0 + 36.53;

  String value = "";
  while (SerialBT.available() > 0) {
    value += (char)SerialBT.read();
  }
  if (value.length() != 0) {

    checkBoolCommand(value, OBJ_INTERN_LED, internLeds);
    checkBoolCommand(value, OBJ_LANTERN, lantern);
    checkBoolCommand(value, OBJ_ALARM, alarmSet);
    checkBoolCommand(value, OBJ_USB_PORT, usbPort);
    checkBoolCommand(value, OBJ_MODULES, modules);
    checkIntCommand(value, OBJ_VOLUME, volume);

    bool stopMusic = false;
    checkBoolCommand(value, OBJ_STOP_MUSIC, stopMusic);
    stopMusic ? playAudio(1) : yield();

    checkBoolCommand(value, OBJ_WIFI, wifiEnabled);
    wifiEnabled ? connectToWifi() : disableWifi();

    checkBoolCommand(value, OBJ_OTA, otaEnabled);
    otaEnabled ? enableOTA() : disableOTA();

    // mock
    checkBoolCommand(value, MOCK_MODULE_1_COMMAND, multiplier);

    appPrintln(value);
  }

  long actualTime = millis();
  if (actualTime > lastTime + 1000) {

    Json json;
    json.putBool(OBJ_INTERN_LED, internLeds);
    json.putBool(OBJ_LANTERN, lantern);
    json.putFloat(OBJ_TEMP, temp);
    json.putBool(OBJ_ALARM, alarmSet);
    json.putBool(OBJ_USB_PORT, usbPort);
    json.putBool(OBJ_MODULES, modules);
    json.putInt(OBJ_VOLUME, volume);
    json.putBool(OBJ_IS_PLAYING, (digitalRead(AUDIO_BUSY_PIN) == HIGH));
    json.putBool(OBJ_OTA, otaEnabled);
    json.putBool(OBJ_WIFI, wifiEnabled);
    json.putString(OBJ_WIFI_NAME, wifiEnabled ? ssid : " ");

    String module1 = "[]";
    if (MOCK_MODULE_1 && modules) {
      module1 = getMockedModule1().generate();
    }

    json.putArray(OBJ_MODULE_1, module1);

    SerialBT.println(json.generate());
    appPrintln(json.generate());

    lastTime = actualTime;
  }

  handleDigitalPortStatus(INTERN_LED_PIN, internLeds);
  handleDigitalPortStatus(PORT_LANTERN_PIN, lantern);
  handleDigitalPortStatus(PORT_USB_PORT_PIN, usbPort);
  handleDigitalPortStatus(PORT_MODULES_PIN, modules);

  //ArduinoOTA.handle();
}

void connectToWifi() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    appPrintln("Establishing connection to WiFi..");
  }

  appPrintln("connected");
}

void disableWifi() {
  WiFi.mode(WIFI_MODE_NULL);
}

void enableOTA() {
  /*ArduinoOTA.begin();
  ArduinoOTA.onStart([]() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(display.getWidth() / 2, display.getHeight() / 2 - 10, "OTA Update");
    display.display();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    display.drawProgressBar(4, 32, 120, 8, progress / (total / 100) );
    display.display();
  });

  ArduinoOTA.onEnd([]() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, "Restart");
    display.display();
  });*/
}

void disableOTA() {
  // usused
}

void playAudio(int file) {
  if (digitalRead(AUDIO_BUSY_PIN) != HIGH) {
    wtv020sd16p.reset();
    delay(200);
    wtv020sd16p.setVolume(volume);
    delay(200);
    wtv020sd16p.asyncPlayVoice(0);
    delay(200);
  }
}

bool checkBoolCommand(String value, String command, bool& var) {

  bool integer = value.charAt(value.length() - 3) == 49;

  if (value.indexOf(command) == 0) {
    var = integer;
  }
}

bool checkIntCommand(String value, String command, int& var) {

  int integer = String(value.charAt(value.length() - 3)).toInt();

  if (value.indexOf(command) == 0) {
    var = integer;
  }
}

void handleDigitalPortStatus(int port, bool var) {
  digitalWrite(port, var);
}

JsonArray getMockedModule1() {

  Json firstProperty;
  firstProperty.putString("type", "INFO");
  firstProperty.putString("id", "mocked_controller");
  firstProperty.putString("name", "Mocked Controller");
  firstProperty.putString("color", "#c9c9ff");

  String value = "10";
  if (multiplier) {
    value = "99999";
  }

  Json command1Property;
  command1Property.putString("type", "OUTPUT_VALUE");
  command1Property.putString("label", "Valor de saída");
  command1Property.putString("value", value);

  Json command2Property;
  command2Property.putString("type", "ACTION_SWITCH");
  command2Property.putString("label", "Multiplicador");
  command2Property.putString("command", "multiplier");
  command2Property.putBool("value", multiplier);

  JsonArray jsonArray;
  jsonArray.putElement(firstProperty.generate());
  jsonArray.putElement(command1Property.generate());
  jsonArray.putElement(command2Property.generate());

  return jsonArray;
}

void appPrint(int value) {
  if (PRINT_ALL) {
    Serial.print(value);
  }
}

void appPrintln(int value) {
  if (PRINT_ALL) {
    Serial.println(value);
  }
}

void appPrint(String value) {
  if (PRINT_ALL) {
    Serial.print(value);
  }
}

void appPrintln(String value) {
  if (PRINT_ALL) {
    Serial.println(value);
  }
}

