/**
 * BasicHTTPClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
//#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>


#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <RCSwitch.h>

//#include "FdIrRemoteEsp8266.h"



 // Comment out DEBUG options
#define DEBUG_SERIAL true
#define DEBUG_PRESET_WIFI true

#ifdef DEBUG_SERIAL
#define sPrintln(a) (Serial.println(a))
#define sPrint(a) (Serial.print(a))
#else
#define sPrintln(a) 
#define sPrint(a) 
#endif



// ESP-12
const int PIN_BUTTON = 5;
const int PIN_PIR = 12;
const int PIN_TEMP = A0;  // ADC, what pin is this?
const int PIN_LIGHT = 4;

const int PIN_LED_STAT1 = 2;
const int PIN_LED_STAT2 = 16;
const int PIN_IR = 13;
const int PIN_RF = 16;



#ifdef DEBUG_PRESET_WIFI
const char* ssid = "xxxxx";
const char* password = "xxxxx";
#endif

const char* mqtt_server = "xxx.xxx.xxx.xxx";
const char* mqtt_user = "xxxxx";
const char* mqtt_pass = "xxxxx";

const String mqtt_topic_root = "xxxxx/xxxxx";
const String device_id = "1";



// Runtime vars

//FdIrRemoteEsp8266 irRemote(PIN_IR);

ESP8266WiFiMulti WiFiMulti;

WiFiClient espClient;
PubSubClient pubsubClient(espClient);
RCSwitch rcSwitch = RCSwitch();

IRsend irSend(PIN_IR);





// --- Debug --- //

// Debug messages get sent to the serial port.
void debug(String str) {
  #ifdef DEBUG_SERIAL
  uint32_t now = millis();
  Serial.printf("%07u.%03u: %s\n", now / 1000, now % 1000, str.c_str());
  #endif  // DEBUG_SERIAL
}





// --- Wifi Setup --- //

void wifi_setup() {
  
  debug("WiFi Setup...");
  
  #ifdef DEBUG_PRESET_WIFI
  
    debug("Connecting to preset");
    WiFiMulti.addAP(ssid, password);
    while (WiFiMulti.run() != WL_CONNECTED) {
      debug(".");
      digitalWrite(PIN_LED_STAT1, HIGH); // LED_STAT1 OFF
      delay(100);
      digitalWrite(PIN_LED_STAT1, LOW); // LED_STAT1 ON
      delay(400);
    }
    debug("Done.");
    
  #else
    
    debug("TODO add wifi manager");
    
  #endif
  
}







void setup() {
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_TEMP, INPUT);
  pinMode(PIN_LIGHT, INPUT);
  
  pinMode(PIN_LED_STAT1, OUTPUT);
  pinMode(PIN_LED_STAT2, OUTPUT);
  
  pinMode(PIN_LED_STAT1, OUTPUT);
  pinMode(PIN_LED_STAT2, OUTPUT);
  pinMode(PIN_IR, OUTPUT);
  pinMode(PIN_RF, OUTPUT);
  
  digitalWrite(PIN_LED_STAT1, LOW); // LED_STAT1 ON

  
  #ifdef DEBUG_SERIAL
  Serial.begin(115200);
  #endif  // DEBUG


  wifi_setup();
  pubsubClient.setServer(mqtt_server, 1883);
  pubsubClient.setCallback(mqtt_callback);
  
  rcSwitch.enableTransmit(PIN_RF);
  randomSeed(micros());
  
  digitalWrite(PIN_LED_STAT1, HIGH); // LED_STAT1 OFF
  
}


char* stringToChar(String str) {

  int str_len = str.length() + 1; 
  char output_char[str_len];
  str.toCharArray(output_char, str_len);

  return output_char;
  
}


int cSize(char* ch){
  int tmp=0;
  while (*ch) {
    *ch++;
    tmp++;
  }
  return tmp;
}
char* cJoin(char* ca1, char* ca2) {

  int size = cSize(ca1) + cSize(ca2);
  char result[size];

  strcat(result, ca1);
  strcat(result, ca2);

  return result;
  
}





void mqtt_publish(char* subTopic, char* value) {

  String mqtt_topic = mqtt_topic_root + "/" + device_id + "/";
  
  String mqtt_topic_str = mqtt_topic + subTopic;

  pubsubClient.publish(mqtt_topic_str.c_str(), value);
  
}
void mqtt_publish(char* subTopic, int ivalue) {
  char value[6];
  sprintf(value,"%d",ivalue);
  mqtt_publish(subTopic, value);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {

  char message[length];
  memcpy(message, payload, length);
  message[length] = '\0';
  
  sPrint("Message arrived [");
  sPrint(topic);
  sPrint("] ");
  sPrint(message);
  sPrintln();

  
  String mqtt_topic = mqtt_topic_root + "/" + device_id + "/";
  String mqtt_topic_led = mqtt_topic + "led";
  String mqtt_topic_rf = mqtt_topic + "ir";
  String mqtt_topic_ir = mqtt_topic + "rf";

  if (strcmp(topic, mqtt_topic_led.c_str()) == 0) {
    if (strcmp(message, "on") == 0) {
      digitalWrite(PIN_LED_STAT1, LOW);
    } else if (strcmp(message, "off") == 0) {
      digitalWrite(PIN_LED_STAT1, HIGH);
    }
  }
  if (strcmp(topic, mqtt_topic_rf.c_str()) == 0) {
    sendIrByString(message);
  }
  if (strcmp(topic, mqtt_topic_ir.c_str()) == 0) {
    sendRfByString(message);
  }

}

void mqtt_reconnect() {
  // Loop until we're reconnected
  while (!pubsubClient.connected()) {
    sPrint("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (pubsubClient.connect(clientId.c_str(), "fox", "IjkKN3R8xxyDgCF")) {
      sPrintln("connected");

      String mqtt_topic = mqtt_topic_root + "/" + device_id + "/";
      String mqtt_topic_led = mqtt_topic + "led";
      String mqtt_topic_rf = mqtt_topic + "ir";
      String mqtt_topic_ir = mqtt_topic + "rf";
      
      // Once connected, publish an announcement...
      mqtt_publish("status", "connected");

      // ... and resubscribe
      pubsubClient.subscribe(mqtt_topic_led.c_str());
      pubsubClient.subscribe(mqtt_topic_rf.c_str());
      pubsubClient.subscribe(mqtt_topic_ir.c_str());
      
    } else {
      sPrint("failed, rc=");
      sPrint(pubsubClient.state());
      sPrintln(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



// IO

// - Inputs / Sensors
int reading_button = 0;
int lastReading_button = 0;
int readButton() {
  lastReading_button = reading_button;
  return reading_button = (!digitalRead(PIN_BUTTON));
}
int hasChanged_button() {
  return (reading_button != lastReading_button);
}
int getReadingValue_button() {
  return reading_button;
}

int reading_motion = 0;
int lastReading_motion = 0;
int readMotion() {
  lastReading_motion = reading_motion;
  return reading_motion = digitalRead(PIN_PIR);
}
int hasChanged_motion() {
  return (reading_motion != lastReading_motion);
}
int getReadingValue_motion() {
  return reading_motion;
}

int reading_temp = 0;
int lastReading_temp = 0;
int readTemp() {
  lastReading_temp = reading_temp;
  
  int current_temp = analogRead(PIN_TEMP);
  int diff = reading_temp - current_temp;
  
  if (diff > 4 || diff < -4) {
    reading_temp = current_temp;  // Only change reading after significant change.
  }
  
//  sPrint("diff = ");
//  sPrint(diff);
//  sPrint(" current = ");
//  sPrint(current_temp);
//  sPrint(" reading_temp = ");
//  sPrint(reading_temp);
//  sPrint(" lastReading_temp = ");
//  sPrint(lastReading_temp);
  
  return reading_temp;
}
int hasChanged_temp() {
  return (reading_temp != lastReading_temp);
}
int getReadingValue_temp() {
  // Convert raw value
  return reading_temp;
}

int reading_light = 0;
int lastReading_light = 0;
int readLight() {
  lastReading_light = reading_light;
  
  // send pulse, time response.
  pinMode(PIN_LIGHT, OUTPUT);
  digitalWrite(PIN_LIGHT, HIGH);
  delay(5);
  pinMode(PIN_LIGHT, INPUT);

  int counter = 0;
  while(digitalRead(PIN_LIGHT)) {
    counter++;
    if (counter > 99999) {
      break;
    }
  }
  int percent = 100 - (counter/1000);

  // reduce resolution so it doesn't constantly spam mqtt
  percent = percent/5;
  percent = percent*5;
  
  return reading_light = percent;
}
int hasChanged_light() {
  return (reading_light != lastReading_light);  // TODO apply percentage threshold.
}
int getReadingValue_light() {
  // Convert raw value
  return reading_light;
}



void readInputs() {
  readButton();
  readMotion();
  readTemp();
  readLight();
}


void sensorTest() {
  readInputs();

  sPrint("button ");
  sPrintln(getReadingValue_button());
  
  sPrint("motion ");
  sPrintln(getReadingValue_motion());
  
  sPrint("temp ");
  sPrintln(getReadingValue_temp());
  
  sPrint("light ");
  sPrintln(getReadingValue_light());
}


// - Outputs

void sendIrByString(char* command) {
  // Split string into protcol,signal e.g. NEC,32,16769056

  String protocol = "";
  String bits = "";
  String data = "";

  int i;
  int segment = 0;
  for (i = 0; command[i] != '\0'; i++){
    if (command[i] == ','){
      segment++;
      continue;
    }
    
    if (segment == 0) { protocol += command[i]; }
    else if (segment == 1) { bits += command[i]; }
    else if (segment == 2) { data += command[i]; }
  }
  
//  sPrint("sendIrByString: ");
//  sPrint(protocol);
//  sPrint(",");
//  sPrint(bits);
//  sPrint(",");
//  sPrint(data);
//  sPrintln(".");

  if (protocol.equals("NEC")){
    if (bits.equals("") || data.equals("")) { return; } // Require bits and data.
    irSend.sendNEC(atol(data.c_str()), atoi(bits.c_str()));
  }
  
  //irSend.sendNEC(16769056, 32);
  
}

void sendRfByString(char* command) {
  // Split string into protcol,signal e.g. 24,1397077
  
  String bits = "";
  String data = "";

  int i;
  int segment = 0;
  for (i = 0; command[i] != '\0'; i++){
    if (command[i] == ','){
      segment++;
      continue;
    }
    
    if (segment == 0) { bits += command[i]; }
    else if (segment == 1) { data += command[i]; }
  }
  
//  sPrint("sendRfByString: ");
//  sPrint(bits);
//  sPrint(",");
//  sPrint(data);
//  sPrintln(".");

  if (bits.equals("") || data.equals("")) { return; } // Require bits and data.
  rcSwitch.setRepeatTransmit(5);
  rcSwitch.send(atol(data.c_str()), atoi(bits.c_str()));

//  rcSwitch.send(1397077, 24);

}





int intervalMilis = 2000;  // 1s
long lastPublishMilis = 0;
int value = 0;

void loop() {

  // MQTT connection
  if (!pubsubClient.connected()) {
    mqtt_reconnect();
  }
  pubsubClient.loop();


  // Send sensor readings.
  
  
  //sensorTest();

  // Slow down the rate we send/check for updates
  long now = millis();
  if (now - lastPublishMilis > intervalMilis) {
    lastPublishMilis = now;

    sPrintln("r");
    
    readInputs();

    if (hasChanged_button()) { mqtt_publish("button", getReadingValue_button()); }
    if (hasChanged_motion()) { mqtt_publish("motion", getReadingValue_motion()); }
    if (hasChanged_temp()) { mqtt_publish("temp", getReadingValue_temp()); }
    if (hasChanged_light()) { mqtt_publish("light", getReadingValue_light()); }

  
//    ++value;
//    snprintf (msg, 75, "hello world #%ld", value);
//    sPrint("Publish message: ");
//    sPrintln(msg);
//    pubsubClient.publish("outTopic", msg);
  }
  
}











