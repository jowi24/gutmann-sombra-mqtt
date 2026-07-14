#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define FW_NAME    "HoodControl"
#define FW_VERSION "0.0.6"

// Pins
const int inputPin   = 13; // D7
const int buttonPin1 = 12; // D6
const int buttonPin2 = 14; // D5
const int ledPin1    =  5; // D1
const int ledPin2    =  4; // D2

// MQTT config (defaults, overridden by stored config)
char mqtt_host[64]   = "mqtt.local";
char mqtt_port[6]    = "1883";
char mqtt_prefix[64] = "home/kitchen/hood";
char device_name[32] = "hood-control";

// ISR state
volatile boolean ledStates[8];
volatile int resync;
volatile int ledStateCounter[8];
volatile int buttonStateCounter[8];
volatile unsigned long lastIsr = 0;
volatile unsigned long phaseCounter = 0;

WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

// Forward declarations
void checkButtons(int line);
void updateButtonState(int buttonId, int buttonPin);
void checkLeds(int line);
void updateLedStateCounter(int ledId, int ledPinState);
void mqttReconnect();
void publishState(int i);
void saveConfig();
void loadConfig();

// ---- ISR functions (unchanged) ----

ICACHE_RAM_ATTR void IsrResync() {
	if (++resync > 10) {
		unsigned long now = micros();
		if ((now - lastIsr) > 3900 && (now - lastIsr) < 4100) {
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
	switch (phaseCounter % 40) {
	case 1:  checkButtons(1); break;
	case 5:  checkLeds(1);    break;
	case 11: checkButtons(2); break;
	case 15: checkLeds(2);    break;
	case 21: checkButtons(4); break;
	case 25: checkLeds(4);    break;
	case 31: checkButtons(3); break;
	case 35: checkLeds(3);    break;
	case 9: case 19: case 29: case 39:
		pinMode(buttonPin1, INPUT);
		pinMode(buttonPin2, INPUT);
		break;
	}
	phaseCounter++;
}

ICACHE_RAM_ATTR void checkButtons(int line) {
	switch (line) {
	case 1: updateButtonState(2, buttonPin1); updateButtonState(6, buttonPin2); break;
	case 2: updateButtonState(3, buttonPin1); updateButtonState(7, buttonPin2); break;
	case 3: updateButtonState(1, buttonPin1); updateButtonState(5, buttonPin2); break;
	case 4: updateButtonState(4, buttonPin1); break;
	}
}

ICACHE_RAM_ATTR void updateButtonState(int buttonId, int buttonPin) {
	if (buttonStateCounter[buttonId] > 0) {
		pinMode(buttonPin, OUTPUT);
		digitalWrite(buttonPin, HIGH);
		buttonStateCounter[buttonId]--;
	}
}

ICACHE_RAM_ATTR void checkLeds(int line) {
	int s1 = digitalRead(ledPin1);
	int s2 = digitalRead(ledPin2);
	switch (line) {
	case 1: updateLedStateCounter(2, s1); updateLedStateCounter(6, s2); break;
	case 2: updateLedStateCounter(3, s1); updateLedStateCounter(7, s2); break;
	case 3: updateLedStateCounter(1, s1); updateLedStateCounter(5, s2); break;
	case 4: updateLedStateCounter(4, s1); break;
	}
}

ICACHE_RAM_ATTR void updateLedStateCounter(int ledId, int ledPinState) {
	if (ledStates[ledId] == ledPinState) ledStateCounter[ledId]++;
	else ledStateCounter[ledId] = 0;
}

// ---- Config (LittleFS + ArduinoJson) ----

void saveConfig() {
	JsonDocument doc;
	doc["mqtt_host"]   = mqtt_host;
	doc["mqtt_port"]   = mqtt_port;
	doc["mqtt_prefix"] = mqtt_prefix;
	doc["device_name"] = device_name;
	File f = LittleFS.open("/config.json", "w");
	if (f) { serializeJson(doc, f); f.close(); }
}

void loadConfig() {
	if (!LittleFS.exists("/config.json")) return;
	File f = LittleFS.open("/config.json", "r");
	if (!f) return;
	JsonDocument doc;
	if (deserializeJson(doc, f) == DeserializationError::Ok) {
		strlcpy(mqtt_host,   doc["mqtt_host"]   | mqtt_host,   sizeof(mqtt_host));
		strlcpy(mqtt_port,   doc["mqtt_port"]   | mqtt_port,   sizeof(mqtt_port));
		strlcpy(mqtt_prefix, doc["mqtt_prefix"] | mqtt_prefix, sizeof(mqtt_prefix));
		strlcpy(device_name, doc["device_name"] | device_name, sizeof(device_name));
	}
	f.close();
}

// ---- MQTT ----

void mqttCallback(char* topic, byte* payload, unsigned int length) {
	String t   = String(topic);
	String msg = "";
	for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
	String prefix = String(mqtt_prefix);

	if (t == prefix + "/ventilation/set") {
		long id = msg.toInt();
		if (id >= 1 && id <= 4) buttonStateCounter[id] = 100;
		if (id == 0) for (int i = 1; i <= 4; i++) if (ledStates[i]) buttonStateCounter[i] = 100;
	} else if (t == prefix + "/light/set") {
		if ((msg == "false" && ledStates[6]) || (msg == "true" && !ledStates[6]))
			buttonStateCounter[6] = 100;
	} else if (t == prefix + "/timer/set") {
		if ((msg == "false" && ledStates[5]) || (msg == "true" && !ledStates[5]))
			buttonStateCounter[5] = 100;
	} else if (t == prefix + "/maintenance/set") {
		if (ledStates[7]) buttonStateCounter[7] = 1500;
	}
}

void publishState(int i) {
	if (!mqtt.connected()) return;
	String prefix = String(mqtt_prefix);
	switch (i) {
	case 1: case 2: case 3: case 4: {
		String v = ledStates[1] ? "1" : ledStates[2] ? "2" : ledStates[3] ? "3" : ledStates[4] ? "4" : "0";
		mqtt.publish((prefix + "/ventilation/state").c_str(), v.c_str(), true);
		Serial.printf("ventilation: %s\n", v.c_str());
		break;
	}
	case 5:
		mqtt.publish((prefix + "/timer/state").c_str(),       ledStates[5] ? "true" : "false", true);
		Serial.printf("timer: %s\n", ledStates[5] ? "true" : "false");
		break;
	case 6:
		mqtt.publish((prefix + "/light/state").c_str(),       ledStates[6] ? "true" : "false", true);
		Serial.printf("light: %s\n", ledStates[6] ? "true" : "false");
		break;
	case 7:
		mqtt.publish((prefix + "/maintenance/state").c_str(), ledStates[7] ? "true" : "false", true);
		Serial.printf("maintenance: %s\n", ledStates[7] ? "true" : "false");
		break;
	}
}

void publishDiscovery() {
	String prefix = String(mqtt_prefix);
	String dev_id = String(device_name);

	// Device-Block als gemeinsame Basis
	auto addDevice = [&](JsonDocument& doc) {
		JsonObject dev = doc["device"].to<JsonObject>();
		dev["identifiers"][0] = dev_id;
		dev["name"]           = "Abzugshaube";
		dev["model"]          = "DIY ESP8266";
		dev["manufacturer"]   = "Joachim Wilke";
		dev["sw_version"]     = FW_VERSION;
	};

	// Availability topic (online/offline)
	String avail = prefix + "/$online";

	// Helper: publish one discovery config
	auto publish = [&](const char* component, const char* obj_id, JsonDocument& doc) {
		String topic = "homeassistant/";
		topic += component;
		topic += "/";
		topic += dev_id + "_" + obj_id;
		topic += "/config";
		String payload;
		serializeJson(doc, payload);
		mqtt.publish(topic.c_str(), payload.c_str(), true);
	};

	// --- Ventilation (fan mit preset modes: off/1/2/3/4) ---
	{
		JsonDocument doc;
		doc["name"]                    = "Lüftung";
		doc["unique_id"]               = dev_id + "_ventilation";
		// State: "0" = off, "1"-"4" = preset
		doc["state_topic"]             = prefix + "/ventilation/state";
		doc["command_topic"]           = prefix + "/ventilation/set";
		doc["payload_on"]              = "1";
		doc["payload_off"]             = "0";
		doc["state_value_template"]    = "{% if value == '0' %}off{% else %}on{% endif %}";
		doc["preset_mode_state_topic"] = prefix + "/ventilation/state";
		doc["preset_mode_command_topic"] = prefix + "/ventilation/set";
		doc["preset_mode_value_template"] = "{{ value }}";
		doc["preset_modes"][0]         = "1";
		doc["preset_modes"][1]         = "2";
		doc["preset_modes"][2]         = "3";
		doc["preset_modes"][3]         = "4";
		doc["availability_topic"]      = avail;
		doc["payload_available"]       = "true";
		doc["payload_not_available"]   = "false";
		doc["icon"]                    = "mdi:air-filter";
		addDevice(doc);
		publish("fan", "ventilation", doc);
	}

	// --- Light (light entity) ---
	{
		JsonDocument doc;
		doc["name"]            = "Haubenlicht";
		doc["unique_id"]       = dev_id + "_light";
		doc["state_topic"]     = prefix + "/light/state";
		doc["command_topic"]   = prefix + "/light/set";
		doc["payload_on"]      = "true";
		doc["payload_off"]     = "false";
		doc["availability_topic"] = avail;
		doc["payload_available"]  = "true";
		doc["payload_not_available"] = "false";
		doc["icon"]            = "mdi:lightbulb";
		addDevice(doc);
		publish("light", "light", doc);
	}

	// --- Timer (switch) ---
	{
		JsonDocument doc;
		doc["name"]            = "Nachlauf-Timer";
		doc["unique_id"]       = dev_id + "_timer";
		doc["state_topic"]     = prefix + "/timer/state";
		doc["command_topic"]   = prefix + "/timer/set";
		doc["payload_on"]      = "true";
		doc["payload_off"]     = "false";
		doc["availability_topic"] = avail;
		doc["payload_available"]  = "true";
		doc["payload_not_available"] = "false";
		doc["icon"]            = "mdi:timer";
		addDevice(doc);
		publish("switch", "timer", doc);
	}

	// --- Maintenance (binary_sensor, read-only) ---
	{
		JsonDocument doc;
		doc["name"]              = "Filterreinigung";
		doc["unique_id"]         = dev_id + "_maintenance";
		doc["state_topic"]       = prefix + "/maintenance/state";
		doc["payload_on"]        = "true";
		doc["payload_off"]       = "false";
		doc["device_class"]      = "problem";
		doc["availability_topic"] = avail;
		doc["payload_available"]  = "true";
		doc["payload_not_available"] = "false";
		doc["icon"]              = "mdi:air-filter";
		addDevice(doc);
		publish("binary_sensor", "maintenance", doc);
	}

	Serial.println("HA discovery published");
}

void mqttReconnect() {
	String will = String(mqtt_prefix) + "/$online";
	if (mqtt.connect(device_name, will.c_str(), 1, true, "false")) {
		mqtt.publish(will.c_str(), "true", true);
		mqtt.subscribe((String(mqtt_prefix) + "/+/set").c_str());
		mqtt.setBufferSize(1024);
		publishDiscovery();
		Serial.println("MQTT connected");
	}
}

// ---- Setup ----

void setup() {
	Serial.begin(115200);
	Serial.println("\n\n" FW_NAME " v" FW_VERSION);

	LittleFS.begin();
	loadConfig();

	// Halten des Flash-Buttons (GPIO0) beim Start setzt WiFi-Config zurück
	pinMode(0, INPUT_PULLUP);
	if (digitalRead(0) == LOW) {
		WiFiManager wm;
		wm.resetSettings();
		LittleFS.remove("/config.json");
		Serial.println("WiFi + MQTT config reset!");
	}

	WiFiManager wm;
	WiFiManagerParameter p_host  ("mqtt_host",   "MQTT Host",   mqtt_host,   64);
	WiFiManagerParameter p_port  ("mqtt_port",   "MQTT Port",   mqtt_port,    6);
	WiFiManagerParameter p_prefix("mqtt_prefix", "MQTT Prefix", mqtt_prefix, 64);
	WiFiManagerParameter p_name  ("device_name", "Device Name", device_name, 32);
	wm.addParameter(&p_host);
	wm.addParameter(&p_port);
	wm.addParameter(&p_prefix);
	wm.addParameter(&p_name);
	wm.setSaveParamsCallback([&]() {
		strlcpy(mqtt_host,   p_host.getValue(),   sizeof(mqtt_host));
		strlcpy(mqtt_port,   p_port.getValue(),   sizeof(mqtt_port));
		strlcpy(mqtt_prefix, p_prefix.getValue(), sizeof(mqtt_prefix));
		strlcpy(device_name, p_name.getValue(),   sizeof(device_name));
		saveConfig();
	});

	// Blockiert bis WiFi verbunden (oder Konfigurationsportal abgeschlossen)
	wm.autoConnect(FW_NAME "-Setup");
	Serial.println("WiFi connected");

	// WiFi Robustheit bei stabiler Stromversorgung optimieren
	WiFi.setAutoReconnect(true);           // Auto-Reconnect bei Verbindungsverlust
	WiFi.setOutputPower(20.5f);            // Maximale TX-Power (0..20.5 dBm) für besseres Signal
	WiFi.setSleepMode(WIFI_NONE_SLEEP);    // Sleep-Modi deaktivieren

	// OTA - Timer während Upload pausieren, sonst blockiert der ISR den Transfer
	ArduinoOTA.setHostname(device_name);
	ArduinoOTA.onStart([]() {
		timer1_detachInterrupt();
		timer1_disable();
		Serial.println("OTA start - timer stopped");
	});
	ArduinoOTA.onError([](ota_error_t error) {
		// Timer nach Fehler wieder aktivieren
		timer1_isr_init();
		timer1_attachInterrupt(IsrTimer);
		timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
		timer1_write((clockCyclesPerMicrosecond() / 16) * 100);
		Serial.printf("OTA error[%u]\n", error);
	});
	ArduinoOTA.begin();

	// MQTT
	mqtt.setServer(mqtt_host, atoi(mqtt_port));
	mqtt.setCallback(mqttCallback);
	mqtt.setBufferSize(1024);

	// Pins initialisieren
	resync = 0;
	for (int i = 0; i < 8; i++) {
		ledStates[i]        = false;
		ledStateCounter[i]  = 0;
		buttonStateCounter[i] = 0;
	}
	pinMode(inputPin,   INPUT);
	pinMode(buttonPin1, INPUT);
	pinMode(buttonPin2, INPUT);
	pinMode(ledPin1,    INPUT);
	pinMode(ledPin2,    INPUT);

	attachInterrupt(digitalPinToInterrupt(inputPin), IsrResync, RISING);

	timer1_isr_init();
	timer1_attachInterrupt(IsrTimer);
}

// ---- Loop ----

unsigned long lastMqttAttempt = 0;
unsigned long lastWifiCheck = 0;

void loop() {
	ArduinoOTA.handle();

	// WiFi Überwachung: aktiv reconnecten bei Verbindungsverlust
	unsigned long now = millis();
	if (now - lastWifiCheck > 10000) {  // alle 10s prüfen
		lastWifiCheck = now;
		if (WiFi.status() != WL_CONNECTED) {
			Serial.printf("WiFi lost (status=%d), reconnecting...\n", WiFi.status());
			WiFi.reconnect();
		}
	}

	if (!mqtt.connected()) {
		if (now - lastMqttAttempt > 5000) {
			lastMqttAttempt = now;
			if (WiFi.status() == WL_CONNECTED) {
				mqttReconnect();
			} else {
				Serial.println("WiFi not connected, skipping MQTT");
			}
		}
	} else {
		mqtt.loop();
	}

	for (int i = 1; i < 8; i++) {
		if ((ledStateCounter[i] > 20 && i != 7)
		 || (ledStateCounter[i] > 20  && i == 7 && !ledStates[7])
		 || (ledStateCounter[i] > 375 && i == 7 &&  ledStates[7])) {
			ledStates[i]       = !ledStates[i];
			ledStateCounter[i] = 0;
			publishState(i);
		}
	}
}
