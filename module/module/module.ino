/**

  BRUTUS V2 - MODULE

**/

#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <MemoryFree.h>

// Build configs
#define MOCK_DATA  true
#define PRINT_ALL  true

#define MODULE_1_ADDRESS  0x08
#define MODULE_NAME "Led Module"
#define NUMPIXELS 16
#define BUFFER 32

#define OBJ_ACTION_LED "Acionar LED"
#define OBJ_ACTION_RANDOM "Aleatório"

// Default commands
#define OBJ_ALARM  "alarm"
#define OBJ_ALARM_STATUS  "alarmStatus"

#define ORANGE_COLOR "#FF5733"
#define GREEN_COLOR "#97FF6E"
#define BLUE_COLOR "#4C4CFF"
#define NORMAL_COLOR "#A88AD4"

int dataCounter = 0;
int lastDataCounter = 0;
byte buffer[32];

String color = NORMAL_COLOR;
bool ledStatus = false;
bool randomColor = false;
int ledBrightness = 50;
int colorCount = 0;

bool alarmSet = false;
bool alarmStatus = false;

long lastTime = 0;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, 12, NEO_GRB + NEO_KHZ800);

// classes auxiliares

class JsonArray {
  private:
    int size = 0;
    String root[5];

  public:
    void putElement(String value) {

      root[size] = value;

      size++;
    }

    String generate() {

      String complete = "[";
      for (int i = 0; i < size; i++) {

        if (i != 0 && i < size) {
          complete.concat(",");
        }

        complete.concat(root[i]);
      }
      complete.concat("]");

      return complete;
    }

    int length() {

      int length = 2;
      for (int i = 0; i < size; i++) {
        length += root[i].length();

        if (i != 0 && i < size - 1) {
          length += 1;
        }
      }
      return length;
    }

    void clear() {

      for (int i = 0; i < size; i++) {
        root[i] = "";
      }
      size = 0;
    }

    String toBuffer(byte buffer[], int bufferSize, int counter) {

      String dataBuffer = generate().substring(0 + 31 * dataCounter, 32 + 31 * dataCounter);
      dataBuffer.toCharArray(buffer, bufferSize);

      /*String dataBuffer = "";

        int firstIndex = (bufferSize - 1) * counter;
        int secondIndex = (bufferSize - 1) * counter + bufferSize - 1;

        int incresedSize = 0;

        Serial.print("Counter: ");
        Serial.println(counter);

        if (size > 1) {
        for (int i = 0; i < size; i++) {

          String part1 = root[i] + ",";
          String part2 = root[i + 1];
          int partsSize = part1.length() + part2.length();

          int nFirstIndex = firstIndex - incresedSize;
          int nSecondIndex = secondIndex - incresedSize;

          Serial.print(nFirstIndex);
          Serial.print(" ");
          Serial.print(nSecondIndex);
          Serial.print(" - ");
          Serial.println(partsSize);

          if (nFirstIndex < partsSize && nSecondIndex < partsSize) {

            if (nSecondIndex < part1.length()) {
              dataBuffer.concat(part1.substring(nFirstIndex, nSecondIndex));
            } else {
              dataBuffer.concat(part1.substring(nFirstIndex, part1.length()));
              dataBuffer.concat(part2.substring(0, nSecondIndex - part1.length()));
            }
            break;
          }

          incresedSize += part1.length();
        }
        }

        dataBuffer.toCharArray(buffer, bufferSize);*/
      return dataBuffer;
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

JsonArray jsonArray;

void setup() {

  Serial.begin(9600);

  Wire.begin(MODULE_1_ADDRESS);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);

  pixels.begin();
  pixels.setBrightness(ledBrightness);
}

void loop() {

  Json firstProperty;
  firstProperty.putString("type", "INFO");
  firstProperty.putString("name", MODULE_NAME);
  firstProperty.putString("color", ORANGE_COLOR);

  Json cmdProp1;
  cmdProp1.putString("type", "SWITCH");
  cmdProp1.putString("label", OBJ_ACTION_LED);
  cmdProp1.putBool("value", ledStatus);

  Json cmdProp2;
  cmdProp2.putString("type", "SWITCH");
  cmdProp2.putString("label", OBJ_ACTION_RANDOM);
  cmdProp2.putBool("value", randomColor);

  jsonArray.clear();
  jsonArray.putElement(firstProperty.generate());
  jsonArray.putElement(cmdProp1.generate());
  jsonArray.putElement(cmdProp2.generate());

  if (lastDataCounter != dataCounter) {

    lastDataCounter = dataCounter;

    if (31 * dataCounter > jsonArray.generate().length()) {
      dataCounter = 0;
      appPrintln("finish");
    }

    String result = jsonArray.toBuffer(buffer, 32, dataCounter);
    //appPrintln(jsonArray.generate());
  }

  String moduleData = "";
  while (Wire.available()) {
    char c = Wire.read();
    moduleData += String(c);
  }

  if (moduleData.length() != 0) {
    checkBoolCommand(moduleData, OBJ_ACTION_LED, ledStatus);
    checkBoolCommand(moduleData, OBJ_ACTION_RANDOM, randomColor);

    checkBoolCommand(moduleData, OBJ_ALARM, alarmSet);
    checkBoolCommand(moduleData, OBJ_ALARM_STATUS, alarmStatus);

    appPrintln(moduleData);
  }


  long actual = millis();
  if (actual - lastTime > 80) {

    drawLeds();

    lastTime = actual;
  }

}

void drawLeds() {
  for (int i = 0; i < NUMPIXELS; i++) {

    int redColor = 0;
    int greenColor = 0;
    int blueColor = 0;

    if (colorCount < 255) {
      redColor = colorCount;
    } else   if (colorCount < 255 * 2) {
      greenColor = colorCount;
    } else   if (colorCount < 255 * 3) {
      blueColor = colorCount;
    } else {
      colorCount = 0;
    }

    if (randomColor) {
      redColor = random(255);
      greenColor = random(255);
      blueColor = random(255);
    }

    if (alarmSet) {
      redColor = 255;
      greenColor = 0;
      blueColor = 0;
    }

    if (alarmStatus) {
      redColor = 0;
      greenColor = 255;
      blueColor = 0;
    }

    if (!ledStatus) {
      redColor = 0;
      greenColor = 0;
      blueColor = 0;
    }

    pixels.setPixelColor(i, pixels.Color(redColor, greenColor, blueColor));

    colorCount++;
  }

  pixels.show();
}

void receiveEvent(int howMany) {
  // unused
}

void requestEvent() {

  dataCounter++;

  Wire.write(buffer, 32);

  //appPrintln(getFreeMemory());
}

bool checkBoolCommand(String value, String command, bool& var) {

  int twoDotsIndex = value.indexOf(":");
  String completeCommand = String(MODULE_NAME) + "." + command;
  if (twoDotsIndex != -1 && (value.indexOf(completeCommand) == 0 || value.indexOf(command) == 0)) {
    String valueSub = value.substring(twoDotsIndex + 1, value.length());
    var = valueSub.toInt() == 1;
  }
}

bool checkIntCommand(String value, String command, int& var) {

  int twoDotsIndex = value.indexOf(":");
  String completeCommand = String(MODULE_NAME) + "." + command;
  if (twoDotsIndex != -1 && (value.indexOf(completeCommand) == 0 || value.indexOf(command) == 0)) {
    String valueSub = value.substring(twoDotsIndex + 1, value.length());
    var = valueSub.toInt();
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
