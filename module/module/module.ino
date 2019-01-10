/**

  BRUTUS V2 - MODULE

**/

#include <Wire.h>

// Build configs
#define MOCK_DATA  true

int dataCounter = 0;
String lastData = "";

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

  Wire.begin(8);
  Wire.onRequest(requestEvent);

}

void loop() {

  Json firstProperty;
  firstProperty.putString("type", "INFO");
  firstProperty.putString("id", "led_module");
  firstProperty.putString("name", "LED Module");
  firstProperty.putString("color", "#55B4B0");

  Json command1Property;
  command1Property.putString("type", "ACTION_SWITCH");
  command1Property.putString("label", "Acionar LED");
  command1Property.putString("command", "action_led");
  command1Property.putBool("value", false);

  JsonArray jsonArray;
  jsonArray.putElement(firstProperty.generate());
  jsonArray.putElement(command1Property.generate());

  String data = jsonArray.generate();
  Serial.println(data);

  if (dataCounter == 0) {
    lastData = data;
  }

  delay(500);

}

void requestEvent() {

  if (31 * dataCounter > lastData.length()) {
    dataCounter = 0;
    Serial.println("");
  }

  String dataBuffer = lastData.substring(0 + 31 * dataCounter, 32 + 31 * dataCounter);

  dataCounter++;

  byte buffer[32];
  dataBuffer.toCharArray(buffer, 32);
  Wire.write(buffer, 32);
  Serial.print(dataBuffer);
}
