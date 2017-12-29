#include <PubSubClient.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// local file that defines the uppercase placeholders below
#include "config.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* mqtt_server = MQTT_HOSTNAME;

// input lines
const int inputPin = 13; 
// buttons
const int buttonPin1 = 12; //D6
const int buttonPin2 = 14; //D5
// leds
const int ledPin1 = 5; //D1
const int ledPin2 = 4; //D2

// variables will change:
WiFiClient espClient;
PubSubClient client(espClient);

volatile boolean ledStates[8];
volatile int resync;
volatile int ledStateCounter[8];
volatile int buttonStateCounter[8];

void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");
  resync = 0;
  for (int i=0; i<8; i++) {
    ledStates[i] = false;
    ledStateCounter[i] = 0;
    buttonStateCounter[i] = 0;
  }
  pinMode(inputPin, INPUT);
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(ledPin1, INPUT);
  pinMode(ledPin2, INPUT);

  attachInterrupt(digitalPinToInterrupt(inputPin), IsrResync, RISING);

  timer1_isr_init();
  timer1_attachInterrupt(IsrTimer);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setHostname("esp8266-hood-control");
  ArduinoOTA.setPassword((const char *)OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("Ready :)");
}

volatile unsigned long t = 0;
volatile unsigned long lastIsr = 0;
volatile int timerCounter = 0;
volatile unsigned long phaseCounter = 0;

ICACHE_RAM_ATTR void IsrResync() {
  if (++resync > 10) {
    unsigned long now = micros();
    if ((now - lastIsr) > 3900 && (now - lastIsr) < 4100) {
      //restart timers
      phaseCounter = 1;
      resync = 0;
      timer1_disable();
      timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP); 
      timer1_write((clockCyclesPerMicrosecond() / 16) * 100);
    }
    lastIsr = micros();
  }
}

ICACHE_RAM_ATTR void IsrTimer() {
  // hood triggers lines: 1,2,4,3,...
  switch (phaseCounter%40) {
    case 1:
      checkButtons(1);
    break;
    case 5:
      checkLeds(1);
    break;
    case 11:
      checkButtons(2);
    break;
    case 15:
      checkLeds(2);
    break;
    case 21:
      checkButtons(4);
    break;
    case 25:
      checkLeds(4);
    break;
    case 31:
      checkButtons(3);
    break;
    case 35:
      checkLeds(3);
    break;
    case 9:
    case 19:
    case 29:
    case 39:
      pinMode(buttonPin1, INPUT);
      pinMode(buttonPin2, INPUT);
    break;
  }
  phaseCounter++;
}

ICACHE_RAM_ATTR void checkButtons(int inputPin) {
  switch(inputPin) {
    // input line 1
    case 1:
      updateButtonState(2, buttonPin1);
      updateButtonState(6, buttonPin2);
    break;
    // input line 2
    case 2:
      updateButtonState(3, buttonPin1);
      updateButtonState(7, buttonPin2);
    break;
    // input line 3
    case 3:
      updateButtonState(1, buttonPin1);
      updateButtonState(5, buttonPin2);
    break;
    // input line 4
    case 4:
      updateButtonState(4, buttonPin1);
    break;
  }
}

ICACHE_RAM_ATTR void updateButtonState(int buttonId, int buttonPin) {
  if (buttonStateCounter[buttonId] > 0) {
    pinMode(buttonPin, OUTPUT);
    digitalWrite(buttonPin, HIGH);
    buttonStateCounter[buttonId]--;
  } 
}

ICACHE_RAM_ATTR void checkLeds(int inputPin) {
  // LED 2/3/1/4
  int ledPin1State = digitalRead(ledPin1);
  // LED 6/7/5/-
  int ledPin2State = digitalRead(ledPin2);
         
  switch(inputPin) {
    // input line 1
    case 1:
      updateLedStateCounter(2, ledPin1State);
      updateLedStateCounter(6, ledPin2State);
    break;
    // input line 2
    case 2:
      updateLedStateCounter(3, ledPin1State);
      updateLedStateCounter(7, ledPin2State);
    break;
    // input line 3
    case 3:
      updateLedStateCounter(1, ledPin1State);
      updateLedStateCounter(5, ledPin2State);
    break;
    // input line 4
    case 4:
      updateLedStateCounter(4, ledPin1State);
    break;
  }
}


ICACHE_RAM_ATTR void updateLedStateCounter(int ledId, int ledPinState) {
  if (ledStates[ledId] == ledPinState) {
    ledStateCounter[ledId]++;
  } else {
    ledStateCounter[ledId] = 0;  
  }
}



void callback(char* topic, byte* payload, unsigned int length) {
 Serial.print("Message arrived [");
 Serial.print(topic);
 Serial.print(", ");
 payload[length]=0;
 Serial.print((const char*)payload);
 Serial.println("] ");
 if (strcmp(topic, "home/kitchen/hood/buttons/toggle") == 0) {
  int id = atoi((const char*)payload);
   Serial.println("Press a button");
   if (id >= 0 && id <= 7) {
     buttonStateCounter[id] = 100; //~ 100*4=400ms
   }
 }
 if (strcmp(topic, "home/kitchen/hood/ventilation/set") == 0) {
   int id = atoi((const char*)payload);
   if (id >= 1 && id <= 4) {
     Serial.println("Set ventilation");
     buttonStateCounter[id] = 100;
   }
   if (id == 0) {
    for (int i=1; i<=4; i++) {
      if (ledStates[i]) {
        buttonStateCounter[i] = 100;
      }
    }
   }
 }
 if (strcmp(topic, "home/kitchen/hood/light/set") == 0) {
    if ((strcmp((const char*)payload, "OFF")==0 && ledStates[6]) || (strcmp((const char*)payload, "ON")==0 && !ledStates[6])) {
      Serial.println("Switch light");
      buttonStateCounter[6] = 100;
    }
 }
 if (strcmp(topic, "home/kitchen/hood/timer/set") == 0) {
    if ((strcmp((const char*)payload, "OFF")==0 && ledStates[5]) || (strcmp((const char*)payload, "ON")==0 && !ledStates[5])) {
      Serial.println("Switch timer");
      buttonStateCounter[5] = 100;
    }
 }
}

void reconnect() {
 // Loop until we're reconnected
 while (!client.connected()) {
 Serial.print("Attempting MQTT connection...");
 // Attempt to connect
 if (client.connect("ESP8266 Hood Client", "home/kitchen/hood/controller/state", 1, true, "OFFLINE")) {
  Serial.println("connected");
  // ... and subscribe to topic
  client.subscribe("home/kitchen/hood/#", 1);
  client.publish("home/kitchen/hood/controller/state", "ONLINE", true);
 } else {
  Serial.print("failed, rc=");
  Serial.print(client.state());
  Serial.println(" try again in 5 seconds");
  // Wait 5 seconds before retrying
  delay(5000);
  }
 }
}

unsigned long lastUptimeTransmit = 0;

void loop() {
  ArduinoOTA.handle();
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastUptimeTransmit > 60000) {
    char buffer[8];
    itoa(now/60000,buffer,10);
    client.publish("home/kitchen/hood/uptime/state", buffer, true);
    lastUptimeTransmit = now;
  }

  // check ledStateCounters and switch state if neccessary
  for (int i=1; i<8; i++) {
    if (ledStateCounter[i] > 20) {
      ledStates[i] = !ledStates[i];
      ledStateCounter[i] = 0;
      Serial.print(i);
      Serial.print(ledStates[i]);
      Serial.print(" ");
      switch(i) {
        case 1:
        case 2:
        case 3:
        case 4:
          Serial.print("Update home/kitchen/hood/ventilation/state ");
          Serial.println(ledStates[1] ? "1" : (ledStates[2] ? "2" : (ledStates[3] ? "3" : (ledStates[4] ? "4" : "0"))));
          client.publish("home/kitchen/hood/ventilation/state", ledStates[1] ? "1" : (ledStates[2] ? "2" : (ledStates[3] ? "3" : (ledStates[4] ? "4" : "0"))), true);
        break;
        case 5:
          Serial.print("Update home/kitchen/hood/timer/state ");
          Serial.println(ledStates[5] ? "ON" : "OFF");
          client.publish("home/kitchen/hood/timer/state", ledStates[5] ? "ON" : "OFF", true);
        break;
        case 6:
          Serial.print("Update home/kitchen/hood/light/state ");
          Serial.println(ledStates[6] ? "ON" : "OFF");
          client.publish("home/kitchen/hood/light/state", ledStates[6] ? "ON" : "OFF", true);
        break;
        case 7:
          Serial.print("Update home/kitchen/hood/maintenance/state ");
          Serial.println(ledStates[7] ? "ON" : "OFF");
          //client.publish("home/kitchen/hood/maintenance/state", ledStates[7] ? "ON" : "OFF", true);
        break;
      }
    }
  }
  
  yield();
}
