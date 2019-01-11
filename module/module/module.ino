/**

  BRUTUS V2 - MODULE

**/

#include <Wire.h>

// Build configs
#define MOCK_DATA  true
#define PRINT_ALL  false

#define MODULE_1_ADDRESS  0x08
#define MODULE_NAME "Led Module"
#define MODULE_ID "led_module"

#define OBJ_ACTION_LED "a_led"

#define ORANGE_COLOR "#FF5733"
#define GREEN_COLOR "#97FF6E"

int dataCounter = 0;
String lastData = "";
String actualData = "";

String color = ORANGE_COLOR;
bool ledStatus = false;

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

  Wire.begin(MODULE_1_ADDRESS);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
}

void loop() {

  Json firstProperty;
  firstProperty.putString("type", "INFO");
  firstProperty.putString("id", MODULE_ID);
  firstProperty.putString("name", "LED Module");
  firstProperty.putString("color", color);

  Json command1Property;
  command1Property.putString("type", "ACTION_SWITCH");
  command1Property.putString("label", "Acionar LED");
  command1Property.putString("command", OBJ_ACTION_LED);
  command1Property.putBool("value", ledStatus);

  JsonArray jsonArray;
  jsonArray.putElement(firstProperty.generate());
  jsonArray.putElement(command1Property.generate());

  actualData = jsonArray.generate();

  if (lastData.equals("")) {
    lastData = actualData;
  }

  delay(100);

  String moduleData = "";
  while (Wire.available()) {
    char c = Wire.read();
    moduleData += String(c);
  }

  if (moduleData.length() != 0) {
    checkBoolCommand(moduleData, OBJ_ACTION_LED, ledStatus);

    appPrintln(moduleData);
  }

}

void receiveEvent(int howMany) {
  // unused
}

void requestEvent() {

  if (31 * dataCounter > lastData.length()) {
    dataCounter = 0;
    lastData = actualData;
    appPrintln("");
  }

  String dataBuffer = lastData.substring(0 + 31 * dataCounter, 32 + 31 * dataCounter);

  dataCounter++;

  byte buffer[32];
  dataBuffer.toCharArray(buffer, 32);
  Wire.write(buffer, 32);

  String logger = "requestEvent:" + String(millis());
  appPrintln(logger);
}

bool checkBoolCommand(String value, String command, bool& var) {

  bool integer = value.charAt(value.length() - 3) == 49;

  String completeCommand = String(MODULE_ID) + ":" + command;

  if (value.indexOf(completeCommand) == 0) {
    var = integer;
  }
}

bool checkIntCommand(String value, String command, int& var) {

  int integer = String(value.charAt(value.length() - 3)).toInt();

  String completeCommand = String(MODULE_ID) + ":" + command;

  if (value.indexOf(completeCommand) == 0) {
    var = integer;
  }
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
