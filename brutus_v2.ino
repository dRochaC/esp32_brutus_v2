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

  Pinos 34 & 35 são apenas para input

**/

#include <BluetoothSerial.h>
#include <Wire.h>
#include <SSD1306.h>
#include <Wtv020sd16p.h>
#include <WiFi.h>
#include <I2Cdev.h>
#include <MPU6050.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <DFRobotDFPlayerMini.h>

// Build configs
#define MOCK_MODULE_1 false
#define MOCK_MODULE_1_COMMAND "mocked_controller:multiplier"
#define BLUETOOTH_NAME "BrutusV2"
#define PRINT_ALL true
#define PRINT_JSON false
#define SEND_DELAY 300
#define RAW_CURRENT false

#define MODULE_1_ADDRESS  0x08

#define OBJ_INTERN_LED  "internLed"
#define OBJ_AUTO_INTERN_LED  "autoInternLed"
#define OBJ_BACK_LANTERN  "backLantern"
#define OBJ_TEMP  "temp"
#define OBJ_ALARM  "alarm"
#define OBJ_ALARM_STATUS  "alarmStatus"
#define OBJ_USB_PORT  "usbPort"
#define OBJ_MODULES  "modules"
#define OBJ_VOLUME  "volume"
#define OBJ_STOP_MUSIC  "stopMusic"
#define OBJ_DF_PLAYER  "dfPlayer"
#define OBJ_WIFI  "wifi"
#define OBJ_WIFI_NAME  "wifiName"
#define OBJ_OTA  "ota"
#define OBJ_BACKPACK_CONSUMPTION  "backpackConsumption"
#define OBJ_BACKPACK_CONSUMPTION_RAW  "backpackConsumptionRaw"
#define OBJ_SOLAR_CONSUMPTION  "solarConsumption"
#define OBJ_SOLAR_CONSUMPTION_RAW  "solarConsumptionRaw"
#define OBJ_RESTART  "restartEsp"
#define OBJ_IRON_MAN  "ironManMode"
#define OBJ_PLAY_SONG  "playSong"

#define OBJ_MODULE_1  "module1"

#define ACS_OFFSET 2060
#define ACS_VOLTAGE_PER_AMP 185

#define INTERN_LIGHT_SENSOR_THRESHOLD 3400

#define IMU_THRESHOLD 2000

// Portas
#define INTERN_LED_PIN  27
#define INTERN_LED_LIGHT_SENSOR_PIN  34
#define PORT_BACK_LANTERN_PIN  26
#define AMPLIFIER_PIN 25
#define PORT_USB_PORT_PIN  32
#define PORT_MODULES_PIN  15

#define ALL_CURRENT_PIN 36
#define ALL_SOLAR_CURRENT_PIN 39

// Variaveis

MPU6050 accelgyro;

BluetoothSerial SerialBT;

SSD1306  display(0x3c, 21, 22);

DFRobotDFPlayerMini myDFPlayer;

long lastTime = 0;

bool deviceConnected = false;

bool internLeds = false;
bool autoInternLed = false;
int internLightSensor = 99999;
bool backLantern = false;
float temp = 0;
bool alarmSet = false;
bool alarmConfigured = false;
bool alarmStatus = false;
bool alarmStart = false;
bool usbPort = false;
bool modules = false;
bool wifiEnabled = false;
bool otaEnabled = false;
int volume = 30; // 50%
float backpackConsumption = 0;
float backpackConsumptionRaw = 0;
float solarConsumption = 0;
float solarConsumptionRaw = 0;

long actualTime = 0;

// Hardware Status
bool isPlayerOk = false;
bool isIMUOk = false;

// Modulo
String moduleData = "";
String completeModuleData = "";
bool isModuleConnected = false;

// Modulo mockado
bool multiplier = false;

// Sensores inerciais
int16_t lastAx = 0, lastAy = 0, lastAz = 0;

// Wifi
const char* ssid = "NETvirtua102";
const char* password =  "wificastro";

// classes auxiliares

class Average {
  private:
    int numReadings = 20;
    int readings[20];      // the readings from the analog input
    int readIndex = 0;              // the index of the current reading
    int total = 0;                  // the running total
    int average = 0;

  public:
    Average() {
      for (int thisReading = 0; thisReading < numReadings; thisReading++) {
        readings[thisReading] = 0;
      }
    }
    void update(float value) {
      // subtract the last reading:
      total = total - readings[readIndex];
      // read from the sensor:
      readings[readIndex] = value;
      // add the reading to the total:
      total = total + readings[readIndex];
      // advance to the next position in the array:
      readIndex = readIndex + 1;

      // if we're at the end of the array...
      if (readIndex >= numReadings) {
        // ...wrap around to the beginning:
        readIndex = 0;
      }

      // calculate the average:
      average = total / numReadings;
    }
    int getAverage() {
      return average;
    }
};

class IMU {
  public:
    int16_t gx, gy, gz;
    int16_t ax, ay, az;

    void update(int16_t _ax, int16_t _ay, int16_t _az) {
      ax = _ax;
      ay = _ay;
      az = _az;
    }

};

IMU imu;

Average allCurrent;
Average solarCurrent;

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

  Serial.begin(9600);

  SerialBT.begin(BLUETOOTH_NAME);

  display.init();
  display.drawString(0, 0, "booting...");
  display.display();

  appPrintln("booting...");

  // Sound System initialization
  isPlayerOk = myDFPlayer.begin(Serial);
  delay(500);

  String playerMessage = isPlayerOk ? "Audio System...[OK]" : "Audio System...[ERROR]";
  appPrintln(playerMessage);
  display.drawString(0, 12, playerMessage);
  display.display();

  myDFPlayer.setTimeOut(500);
  myDFPlayer.volume(volume);
  myDFPlayer.EQ(0);

  // IMU initialization
  accelgyro.initialize();
  isIMUOk = accelgyro.testConnection();
  String mpuMessage = isIMUOk ? "InertialSensors...[OK]" : "InertialSensors...[ERROR]";
  appPrintln(mpuMessage);
  display.drawString(0, 24, mpuMessage);
  display.display();

  analogReadResolution(12);

  pinMode(INTERN_LED_PIN, OUTPUT);
  pinMode(PORT_BACK_LANTERN_PIN, OUTPUT);
  pinMode(PORT_USB_PORT_PIN, OUTPUT);
  pinMode(PORT_MODULES_PIN, OUTPUT);
  pinMode(AMPLIFIER_PIN, OUTPUT);

  Wire.begin();

  play(1);
}

void loop() {

  actualTime = millis();
  bool oneSecond = oneSecondDelay();

  handleLoop();
  handleTime();

  if (alarmStatus) {

    appPrint("AlarmStart: ");
    appPrintln(alarmStart);

    if (alarmStart) {
      myDFPlayer.volume(30);
      play(3);
      alarmStart = false;
    }

    if (oneSecond) {
      digitalWrite(PORT_BACK_LANTERN_PIN, !digitalRead(PORT_BACK_LANTERN_PIN));
    }

    return;
  }

  handleDigitalPortStatus(INTERN_LED_PIN, internLeds);
  handleDigitalPortStatus(PORT_BACK_LANTERN_PIN, backLantern);
  handleDigitalPortStatus(PORT_USB_PORT_PIN, usbPort);
  handleDigitalPortStatus(PORT_MODULES_PIN, modules);

  ArduinoOTA.handle();

  delay(100);
}

void handleTime() {

  if (oneSecondDelay()) {

    sendData();

    lastTime = actualTime;
  }
}

bool oneSecondDelay() {
  return actualTime > lastTime + SEND_DELAY;
}

void handleAlarm() {

  if (alarmSet) {

    // Salva valores default
    if (!alarmConfigured) {
      alarmConfigured = true;

      lastAx = imu.ax;
      lastAy = imu.ay;
      lastAz = imu.az;
    }

    if ((checkAxisThreshold(lastAx, imu.ax) ||
         checkAxisThreshold(lastAy, imu.ay) ||
         checkAxisThreshold(lastAz, imu.az)) &&
        !alarmStatus) {

      // Ativa alarme
      alarmStatus = true;
      alarmStart = true;
    }

  } else {
    alarmConfigured = false;
    alarmStatus = false;
    alarmStart = false;
  }
}

bool checkAxisThreshold(int16_t lastAxis, int16_t actualAxis) {
  int16_t diff = abs(lastAxis - actualAxis);
  return diff > IMU_THRESHOLD;
}

void handleSensorData() {

  internLightSensor = analogRead(INTERN_LED_LIGHT_SENSOR_PIN);
  if (autoInternLed) {
    internLeds = internLightSensor > INTERN_LIGHT_SENSOR_THRESHOLD;
  }

  allCurrent.update(analogRead(ALL_CURRENT_PIN) * 3300 / 4096);
  backpackConsumptionRaw = allCurrent.getAverage();
  backpackConsumption = allCurrent.getAverage() - ACS_OFFSET;
  backpackConsumption = adjustCurrent(backpackConsumption);

  solarCurrent.update(analogRead(ALL_SOLAR_CURRENT_PIN) * 3300 / 4096);
  solarConsumptionRaw = solarCurrent.getAverage();
  solarConsumption = solarCurrent.getAverage() - ACS_OFFSET;
  solarConsumption = adjustCurrent(solarConsumption);

  if (isIMUOk) {
    int16_t ax, ay, az, gx, gy, gz;
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    temp = accelgyro.getTemperature() / 340.0 + 36.53;

    imu.update(ax, ay, az);
  }
}

int adjustCurrent(int averageValue) {

  if (RAW_CURRENT) {
    return averageValue;
  }

  if (averageValue >= 50 && averageValue < 100) {
    averageValue = 50;
  } else if (averageValue >= 100 && averageValue < 150) {
    averageValue = 100;
  } else if (averageValue >= 150 && averageValue < 200) {
    averageValue = 150;
  } else if (averageValue >= 200 && averageValue < 250) {
    averageValue = 200;
  } else if (averageValue >= 250 && averageValue < 300) {
    averageValue = 250;
  } else if (averageValue < 50) {
    averageValue = 0;
  }
}

void handleLoop() {

  handleModuleData();
  handleSensorData();
  handleAlarm();
  handleBtData();
}

void handleBtData() {

  String value = "";
  while (SerialBT.available() > 0) {
    value += (char)SerialBT.read();
  }
  if (value.length() != 0) {

    checkBoolCommand(value, OBJ_INTERN_LED, internLeds);
    checkBoolCommand(value, OBJ_BACK_LANTERN, backLantern);
    checkBoolCommand(value, OBJ_ALARM, alarmSet);
    checkBoolCommand(value, OBJ_USB_PORT, usbPort);
    checkBoolCommand(value, OBJ_MODULES, modules);
    checkBoolCommand(value, OBJ_AUTO_INTERN_LED, autoInternLed);

    bool lastAlarmStatus = alarmStatus;
    checkBoolCommand(value, OBJ_ALARM_STATUS, alarmStatus);
    if (!alarmStatus && lastAlarmStatus != alarmStatus) {
      myDFPlayer.stop();
    }

    bool restart = false;
    checkBoolCommand(value, OBJ_RESTART, restart);
    restart ? esp_restart() : yield();

    checkIntCommand(value, OBJ_VOLUME, volume);
    myDFPlayer.volume(volume);

    bool stopMusic = false;
    checkBoolCommand(value, OBJ_STOP_MUSIC, stopMusic);
    stopMusic ? myDFPlayer.stop() : yield();

    checkBoolCommand(value, OBJ_WIFI, wifiEnabled);
    wifiEnabled ? connectToWifi() : disableWifi();

    checkBoolCommand(value, OBJ_OTA, otaEnabled);
    otaEnabled ? enableOTA() : disableOTA();

    bool ironMan = false;
    checkBoolCommand(value, OBJ_IRON_MAN, ironMan);
    ironMan ? play(3) : yield();

    int playSongPos = 0;
    checkIntCommand(value, OBJ_PLAY_SONG, playSongPos);
    playSongPos > 0 ? play(playSongPos) : yield();

    // mock
    checkBoolCommand(value, MOCK_MODULE_1_COMMAND, multiplier);

    appPrintln(value);

    int bufferSize = value.length();
    if (bufferSize > 32) {
      bufferSize = 32;
    }

    // Manda dado pro módulo conectado
    uint8_t buffer[bufferSize];
    for (int i = 0; i < bufferSize; i++) {
      buffer[i] = value.charAt(i);
    }

    Wire.beginTransmission(MODULE_1_ADDRESS);
    Wire.write(buffer, bufferSize);
    Wire.endTransmission(true);
  }
}

void handleModuleData() {

  int result = Wire.requestFrom(MODULE_1_ADDRESS, 32);

  isModuleConnected = result > 0;
  if (!isModuleConnected) {
    moduleData = "";
    completeModuleData = "";
  }

  while (Wire.available()) {
    char c = Wire.read();
    moduleData += String(c);
  }

  if (moduleData.length() != 0) {
    int firstPos = moduleData.indexOf("[");
    int lastPos = moduleData.indexOf("]", firstPos);

    if (firstPos != -1 && lastPos != -1 && firstPos < lastPos) {

      completeModuleData = moduleData.substring(firstPos, lastPos + 1);
      moduleData = "";
    }
  }

  if (moduleData.length() > 500) {
    moduleData = "";
  }
}

void play(int pos) {
  myDFPlayer.play(pos);
}

void sendData() {

  if (MOCK_MODULE_1) {
    temp = random(100) / 20.0 + 25;
    backpackConsumption = random(100) / 20.0 + 100;
    solarConsumption = random(50) / 20.0 + 20;
  }

  Json json;
  json.putBool(OBJ_INTERN_LED, internLeds);
  json.putBool(OBJ_BACK_LANTERN, backLantern);
  json.putBool(OBJ_AUTO_INTERN_LED, autoInternLed);
  json.putFloat(OBJ_TEMP, temp);
  json.putBool(OBJ_ALARM, alarmSet);
  json.putBool(OBJ_ALARM_STATUS, alarmStatus);
  json.putBool(OBJ_USB_PORT, usbPort);
  json.putBool(OBJ_MODULES, modules);
  json.putInt(OBJ_VOLUME, volume);
  json.putBool(OBJ_DF_PLAYER, myDFPlayer.available());
  json.putBool(OBJ_OTA, otaEnabled);
  json.putBool(OBJ_WIFI, wifiEnabled);
  json.putString(OBJ_WIFI_NAME, wifiEnabled ? ssid : " ");
  json.putFloat(OBJ_BACKPACK_CONSUMPTION, backpackConsumption);
  json.putFloat(OBJ_SOLAR_CONSUMPTION, solarConsumption);
  json.putFloat(OBJ_BACKPACK_CONSUMPTION_RAW, backpackConsumptionRaw);
  json.putFloat(OBJ_SOLAR_CONSUMPTION_RAW, solarConsumptionRaw);

  String module1 = "[]";
  if (modules && isModuleConnected && completeModuleData.length() > 0) {
    module1 = completeModuleData;
  }
  if (modules && MOCK_MODULE_1) {
    module1 = getMockedModule1().generate();
  }

  json.putArray(OBJ_MODULE_1, module1);

  SerialBT.println(json.generate());

  if (PRINT_JSON) {
    appPrintln(json.generate());
  }
}

void connectToWifi() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    appPrint(".");

    handleLoop();
  }

  appPrintln("connected");
  wifiEnabled = true;
}

void disableWifi() {
  WiFi.mode(WIFI_MODE_NULL);
  wifiEnabled = false;
}

void enableOTA() {
  ArduinoOTA.begin();
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
  });
}

void disableOTA() {
  // usused
}

bool checkBoolCommand(String value, String command, bool& var) {

  int twoDotsIndex = value.indexOf(":");
  if (twoDotsIndex != -1 && value.indexOf(command) == 0) {
    String valueSub = value.substring(twoDotsIndex + 1, value.length());
    var = valueSub.toInt() == 1;
  }
}

bool checkIntCommand(String value, String command, int& var) {

  int twoDotsIndex = value.indexOf(":");
  if (twoDotsIndex != -1 && value.indexOf(command) == 0) {
    String valueSub = value.substring(twoDotsIndex + 1, value.length());
    var = valueSub.toInt();
  }
}

void handleDigitalPortStatus(int port, bool var) {
  digitalWrite(port, var ? HIGH : LOW);
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
