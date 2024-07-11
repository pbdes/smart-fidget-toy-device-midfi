#include <Adafruit_NeoPixel.h>
#include <PulseSensorPlayground.h>
#include <Adafruit_SPIFlash.h>
#include <SPIMemory.h>
#include <ArduinoJson.h>
#include <RTCZero.h>

int minHB = 0, maxHB = 0, avgHB = 0, beats = 0, times = 0, id = 0;

//Time
RTCZero rtc;
/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 20;
const byte hours = 20;

/* Change these values to set the current initial date */
const byte day = 8;
const byte month = 4;
const byte year = 24; // Last two digits of the year

//NeoPixel
#define PIN 11
#define NUMPIXELS 1
Adafruit_NeoPixel strip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Rotary Encoder
#define CLK 0
#define DT 1
#define SW 2

int counterCCW = 0;
int counterCW = 0;
int counterP = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir = "";
unsigned long lastButtonPress = 0;
unsigned long firstActivityTime = 0;
unsigned long duration = 0;
bool active = false;
unsigned long lastActivityTime = 0; // The last time the button was pressed
const long timeout = 5000; // Activity timeout period (1 minute = 60000 milliseconds)

enum Action {
  CCW,
  CW,
  P
};

Action act = CCW;

//State of the Device
enum State {
  COLLECT,
  SEND,
  DELETE
};

State currentState = COLLECT;

String received, startTime, endTime, curDate;
bool showed = false; //to track if data was shown to the user in this connection

// Pulse Sensor Inputs
const int PulseWire = A3;
int Threshold = 550;
PulseSensorPlayground pulseSensor;

//Flash memory
#define PIN_FLASH_CS 17
Adafruit_FlashTransport_SPI flashTransport(PIN_FLASH_CS, SPI1);
Adafruit_SPIFlash flash(&flashTransport);
uint32_t address = 0; // Start address for JSON data
uint32_t currentAddress = address;
int memorySize = 0;

void setup() {
  strip.begin();
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  delay(500);
  Serial.println("Serial started");

  //Configure time
  rtc.begin();
  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(day, month, year);

  //Configure Rotary Encoder
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);
  lastStateCLK = digitalRead(CLK);
  Serial.println("Rotary encoder started");

  // Configure the PulseSensor object
  pulseSensor.analogInput(PulseWire);
  pulseSensor.setThreshold(Threshold);
  pulseSensor.begin();
  if (pulseSensor.begin()) {
    Serial.println("We created a pulseSensor Object!");
  }

  //Configure Flash Chip
  if (!flash.begin()) {
    Serial.println("Could not find a valid SPI flash chip!");
  } else {
    Serial.println("Found SPI flash chip!");
    memorySize = flash.size() / 1024;
    Serial.print("JEDEC ID: 0x"); Serial.println(flash.getJEDECID(), HEX);
    Serial.print("Flash size: "); Serial.print(flash.size() / 1024); Serial.println(" KB");
  }
}

void loop() {
  static String serialCommand;
  if (Serial.available()) {
    received = Serial.readStringUntil('\n');
    Serial.println(received);
      if (received == "S") {             //S stay for send
        // Serial.println("we are live");
        currentState = SEND;
      } else if (received == "D") {      // D stays for disconnected
        currentState = DELETE;
      } else {
        currentState = COLLECT;
      }
    }

  switch (currentState) {
    case COLLECT:
      collect();
      break;
    case SEND:
      send(counterCCW);
      break;
    case DELETE:
      clear();
      currentState = COLLECT;
      break;
  }
}


void collect() {
  unsigned long currentMillis = millis();
  strip.setPixelColor(0, strip.Color(0, 0, 255));
  strip.show();
  // Read the current state of CLK
  currentStateCLK = digitalRead(CLK);

  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {
    // If the DT state is different than the CLK state then
    // the encoder is rotating CCW so decrement
    setActive();
    if (digitalRead(DT) != currentStateCLK) {
      counterCW++;
      Action act = CW;
      Serial.println("cw");
    } else {
      // Encoder is rotating CCW so increment
      counterCCW++;
      Action act = CCW;
      Serial.println("ccw");
    }
    heartBeat();
    lastActivityTime = currentMillis;
  }

  // Remember last CLK state
  lastStateCLK = currentStateCLK;

  int btnState = digitalRead(SW);
  if (btnState == LOW) {
    setActive();
    //if 50ms have passed since last LOW pulse, it means that the
    //button has been pressed, released and pressed again
    if (millis() - lastButtonPress > 500) {
      // Remember last button press event
      lastButtonPress = millis();
      counterP++;
      Action act = P;
      Serial.println("check P");
    }
    heartBeat();
    lastActivityTime = currentMillis;
  }

  if ((currentMillis - lastActivityTime) > timeout && counterP > 0) {
    endSession();
  } else if ((currentMillis - lastActivityTime) > timeout && counterCW > 0) {
    endSession();
  } else if ((currentMillis - lastActivityTime) > timeout && counterCCW > 0) {
    endSession();
  }

  if (!counterP && !counterCW && !counterCCW) {
    startTime = getTime();
    curDate = getDate();
  }
}

void send(int count) {
  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();
  if(!showed) {
    logFileData();
    showed = true;
  }
}

void clear() {
  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.show();
  showed = false;
  id = 0;
  delay(50);
  flash.eraseSector(address);
  currentAddress = address;
  logFileData();
  currentState = COLLECT;
}

int readPulse() {
  if (pulseSensor.sawStartOfBeat()) {
    return pulseSensor.getBeatsPerMinute();
  } else {
    return 0; // Return 0 if no beat detected
  }
}

//the function to write the values of the session
void writeFile() {
  DynamicJsonDocument doc(memorySize);
  
  byte buffer[memorySize];
  memset(buffer, 0, sizeof(buffer));
  
  // Read existing JSON from flash
  flash.readBuffer(address, buffer, sizeof(buffer));
  DeserializationError error = deserializeJson(doc, buffer);
  
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    // Reinitialize the document to ensure it's always in a valid state
    doc.to<JsonObject>().clear(); // Clear any existing content in the document
    // Ensure "data" is always an array, even after clearing the document
    doc.createNestedArray("data");
  }

  // At this point, doc contains either the deserialized JSON or an empty document with a "data" array
  JsonArray data = doc["data"];

  JsonObject newData = data.createNestedObject();
  newData["date"] = curDate;
  newData["startTime"] = startTime;
  newData["endTime"] = endTime;
  newData["duration"] = String(duration);
  newData["min"] = minHB;
  newData["max"] = maxHB;
  newData["avg"] = avgHB;
  newData["p"] = counterP;
  newData["ccw"] = counterCCW;
  newData["cw"] = counterCW;
  newData["id"] = id;
  
  // Serialize JSON to buffer
  size_t bytesWritten = serializeJson(doc, buffer, sizeof(buffer));
  
  // Erase sector before writing
  flash.eraseSector(address);
  if (flash.writeBuffer(address, buffer, bytesWritten)) {
    Serial.println(F("Data written to flash."));
    currentAddress = address + bytesWritten;
  } else {
    Serial.println(F("Failed to write data."));
  }
}

//the function to show JSON data in Serial Port
void logFileData() {
    byte buffer[memorySize]; // Ensure this buffer is large enough for your data
    memset(buffer, 0, sizeof(buffer)); // Clear the buffer
    
    // Read the data back from flash
    flash.readBuffer(address, buffer, currentAddress);
    // Serial.println(F("Data read from flash: "));
    Serial.println((char*)buffer);
}

void endSession() {
    Serial.println("Session ended");
    duration = millis() - firstActivityTime;
    endTime = getTime();
    id++;
    writeFile();
    active = false;
    minHB = 0;
    maxHB = 0;
    avgHB = 0;
    beats = 0;
    times = 0;
    counterCCW = 0;
    counterCW =0;
    counterP = 0;
}

String getTime() {
  String time = String(rtc.getHours()) + ":" + String(rtc.getMinutes()) + ":" + String(rtc.getSeconds());
  return time;
}

String getDate() {
  String date = String(rtc.getDay()) + "." + String(rtc.getMonth()) + "." + String(rtc.getYear());
  return date;
}

void heartBeat() {
  int hb = readPulse();
  if(hb > 0) {
    times++;
    beats+=hb;
    if(!minHB && !maxHB) {
      minHB = hb;
      maxHB = hb;
    } else if (hb > maxHB) {
      maxHB = hb;
    } else if(hb < minHB) {
      minHB = hb;
    }
    avgHB = beats/times;
  }
    Serial.print("avg:");
    Serial.print(avgHB);
    Serial.print("max:");
    Serial.print(maxHB);
    Serial.print("min:");
    Serial.println(minHB);
}

void setActive() {
  if(active = false) {
    active = true;
    firstActivityTime = millis();
  }
}