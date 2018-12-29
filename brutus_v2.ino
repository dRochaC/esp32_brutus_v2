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

#define OBJ_INTERN_LED  "internLed"
#define OBJ_LANTERN  "lantern"
#define OBJ_TEMP  "temp"
#define OBJ_ALARM  "alarm"
#define OBJ_USB_PORT  "usbPort"
#define OBJ_MODULE_1  "module1"

#define MOCK_MODULE_1  true

// Variaveis

BluetoothSerial SerialBT;

long lastTime = 0;

bool deviceConnected = false;

bool internLeds = false;
bool lantern = false;
float temp = 0;
bool alarmSet = false;
bool usbPort = false;

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
  SerialBT.begin("BrutusV2");;
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
    }
    Serial.println(value);
  }

  long actualTime = millis();
  if (actualTime > lastTime + 1000) {
    temp = random(5) + 25;

    Json json;
    json.putBool(OBJ_INTERN_LED, internLeds);
    json.putBool(OBJ_LANTERN, lantern);
    json.putBool(OBJ_TEMP, temp);
    json.putBool(OBJ_ALARM, alarmSet);
    json.putBool(OBJ_USB_PORT, usbPort);

    if (MOCK_MODULE_1) {
      json.putArray(OBJ_MODULE_1, getMockedModule1());
    }

    SerialBT.println(json.generate());
    Serial.println(json.generate());

    lastTime = actualTime;
  }
}

JsonArray getMockedModule1() {

  Json firstProperty;
  firstProperty.putString("id", "mocked_controller");
  firstProperty.putString("name", "Mocked Controller");
  firstProperty.putString("type", "INFO");

  Json command1Property;
  command1Property.putString("type", "OUTPUT_INT");
  command1Property.putString("value", "10");

  Json command2Property;
  command2Property.putString("type", "ACTION_SWITCH");
  command2Property.putString("command", "multiplier");

  JsonArray jsonArray;
  jsonArray.putElement(firstProperty.generate());
  jsonArray.putElement(command1Property.generate());
  jsonArray.putElement(command2Property.generate());

  return jsonArray;
}

