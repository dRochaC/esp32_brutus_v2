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

// Build configs
#define MOCK_MODULE_1  true
#define MOCK_MODULE_1_COMMAND  "mocked_controller:multiplier"

#define OBJ_INTERN_LED  "internLed"
#define OBJ_LANTERN  "lantern"
#define OBJ_TEMP  "temp"
#define OBJ_ALARM  "alarm"
#define OBJ_USB_PORT  "usbPort"
#define OBJ_MODULES  "modules"
#define OBJ_VOLUME  "volume"
#define OBJ_STOP_MUSIC  "stopMusic"
#define OBJ_IS_PLAYING  "isPlaying"

#define OBJ_MODULE_1  "module1"

// Portas
#define PORT_INTERN_LED  16
#define PORT_LANTERN  17
#define PORT_USB_PORT  32
#define PORT_MODULES  33

#define resetPin 19  //Pino Reset
#define clockPin 18  //Pino clock
#define dataPin 15   //Pino data (DI)
#define busyPin 5    //Pino busy

// Variaveis

BluetoothSerial SerialBT;
SSD1306  display(0x3c, 21, 22);
Wtv020sd16p wtv020sd16p(resetPin, clockPin, dataPin, busyPin);

long lastTime = 0;

bool deviceConnected = false;

bool internLeds = false;
bool lantern = false;
float temp = 0;
bool alarmSet = false;
bool usbPort = false;
bool modules = false;
int volume = 4; // 50%

// Modulo mockado
bool multiplier = false;

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

  Serial.begin(115200);

  SerialBT.begin("BrutusV2");

  display.init();
  display.drawString(0, 0, "Brutus_v2");
  display.drawString(0, 12, "Initializing...");
  display.display();

  wtv020sd16p.reset();
  delay(1000);
  Serial.println("playing");
  playAudio(0);

  pinMode(PORT_INTERN_LED, OUTPUT);
  pinMode(PORT_LANTERN, OUTPUT);
  pinMode(PORT_USB_PORT, OUTPUT);
  pinMode(PORT_MODULES, OUTPUT);
}

void loop() {

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
    stopMusic ? playAudio(1) : delay(0);

    // mock
    checkBoolCommand(value, MOCK_MODULE_1_COMMAND, multiplier);

    Serial.println(value);
  }

  long actualTime = millis();
  if (actualTime > lastTime + 1000) {
    temp = random(5) + 25;

    Json json;
    json.putBool(OBJ_INTERN_LED, internLeds);
    json.putBool(OBJ_LANTERN, lantern);
    json.putFloat(OBJ_TEMP, temp);
    json.putBool(OBJ_ALARM, alarmSet);
    json.putBool(OBJ_USB_PORT, usbPort);
    json.putBool(OBJ_MODULES, modules);
    json.putInt(OBJ_VOLUME, volume);
    json.putBool(OBJ_IS_PLAYING, (digitalRead(busyPin) == HIGH));

    String module1 = "[]";
    if (MOCK_MODULE_1 && modules) {
      module1 = getMockedModule1().generate();
    }

    json.putArray(OBJ_MODULE_1, module1);

    SerialBT.println(json.generate());
    Serial.println(json.generate());

    lastTime = actualTime;
  }

  handleDigitalPortStatus(PORT_INTERN_LED, internLeds);
  handleDigitalPortStatus(PORT_LANTERN, lantern);
  handleDigitalPortStatus(PORT_USB_PORT, usbPort);
  handleDigitalPortStatus(PORT_MODULES, modules);
}

void playAudio(int file) {
  if (digitalRead(busyPin) != HIGH) {
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

void nothing();

