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

#define OBJ_INTERN_LED  "internLed"
#define OBJ_LANTERN  "lantern"
#define OBJ_TEMP  "temp"
#define OBJ_ALARM  "alarm"
#define OBJ_USB_PORT  "usbPort"
#define OBJ_MODULES  "modules"

#define OBJ_MODULE_1  "module1"

#define MOCK_MODULE_1  true

// Variaveis

BluetoothSerial SerialBT;
SSD1306  display(0x3c, 21, 22);

long lastTime = 0;

bool deviceConnected = false;

bool internLeds = false;
bool lantern = false;
float temp = 0;
bool alarmSet = false;
bool usbPort = false;
bool modules = false;

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

  pinMode(22, OUTPUT);
}

void loop() {

  String value = "";
  while (SerialBT.available() > 0) {
    value += (char)SerialBT.read();
  }
  if (value.length() != 0) {
    bool integer = value.charAt(value.length() - 3) == 49;
    if (value.indexOf(OBJ_INTERN_LED) == 0) {
      internLeds = integer;
    } else if (value.indexOf(OBJ_LANTERN) == 0) {
      lantern = integer;
    } else if (value.indexOf(OBJ_ALARM) == 0) {
      alarmSet = integer;
    } else if (value.indexOf(OBJ_USB_PORT) == 0) {
      usbPort = integer;
    } else if (value.indexOf(OBJ_MODULES) == 0) {
      modules = integer;
    } else if (value.indexOf("mocked_controller:multiplier") == 0) {
      multiplier = integer;
    }
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

    String module1 = "[]";
    if (MOCK_MODULE_1 && modules) {
      module1 = getMockedModule1().generate();
    }

    json.putArray(OBJ_MODULE_1, module1);

    SerialBT.println(json.generate());
    Serial.println(json.generate());

    lastTime = actualTime;

    digitalWrite(22, !digitalRead(22));
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

