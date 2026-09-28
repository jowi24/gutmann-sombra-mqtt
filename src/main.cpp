#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define FW_NAME    "HoodControl"
#define FW_VERSION "0.0.9"

const char* const CONFIG_PORTAL_SSID     = FW_NAME "-Setup";
// Override both via build_flags, see docs/OPERATIONS.md
#ifndef PORTAL_PASSWORD
#define PORTAL_PASSWORD "hoodcontrol"  // at least 8 characters
#endif
#ifndef MQTT_HOST
#define MQTT_HOST "mqtt.local"
#endif

const char* const CONFIG_PORTAL_PASSWORD = PORTAL_PASSWORD;
const unsigned long WIFI_RETRY_INTERVAL  = 10000;
const unsigned long PORTAL_RETRY_INTERVAL = 30000;
const unsigned long PORTAL_START_DELAY   = 20000;
const unsigned long PORTAL_CLOSE_DELAY   = 60000;
const unsigned long DIAGNOSTICS_INTERVAL = 60000;
const unsigned long STATE_RETRY_INTERVAL = 5000;
const unsigned long MQTT_RETRY_MIN       = 5000;
const unsigned long MQTT_RETRY_MAX       = 300000;
const size_t EVENT_LOG_SIZE              = 16;
const size_t EVENT_LENGTH                 = 80;

// Pins
const int inputPin    = 13; // D7
const int buttonPin1  = 14; // D5 - swapped like the LED pins, see below
const int buttonPin2  = 12; // D6
const int ledPin1     =  4; // D2 - swapped vs. the perfboard: on the PCB, J1/1+2 arrive the other way round
const int ledPin2     =  5; // D1
const int statusLedPin =  2; // D4, onboard blue LED (ESP-12), active LOW

// MQTT config (defaults, overridden by stored config)
char mqtt_host[64]   = MQTT_HOST;
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
WiFiManager  wifiManager;

WiFiManagerParameter p_host  ("mqtt_host",   "MQTT Host",   mqtt_host,   64);
WiFiManagerParameter p_port  ("mqtt_port",   "MQTT Port",   mqtt_port,    6);
WiFiManagerParameter p_prefix("mqtt_prefix", "MQTT Prefix", mqtt_prefix, 64);
WiFiManagerParameter p_name  ("device_name", "Device Name", device_name, 32);

char mqttClientId[48];
char previousBootStage[32] = "unknown";
char lastError[80] = "none";
char eventLog[EVENT_LOG_SIZE][EVENT_LENGTH];
size_t eventLogStart = 0;
size_t eventLogCount = 0;
unsigned long wifiReconnects = 0;
unsigned long mqttReconnects = 0;
unsigned long mqttFailures = 0;
unsigned long mqttRetryInterval = MQTT_RETRY_MIN;
unsigned long disconnectedSince = 0;
unsigned long wifiConnectedSince = 0;
unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastDiagnostics = 0;
unsigned long lastStateAttempt = 0;
unsigned long configSavedAt = 0;
bool wasWifiConnected = false;
bool wasMqttConnected = false;
bool stateSyncPending = true;
bool discoveryPending = true;

// Forward declarations
void checkButtons(int line);
void updateButtonState(int buttonId, int buttonPin);
void checkLeds(int line);
void updateLedStateCounter(int ledId, int ledPinState);
void mqttReconnect();
bool publishState(int i);
bool publishAllStates();
bool publishDiagnostics();
void publishEventLog();
void processNetwork();
void startConfigPortal();
void addEvent(const char* message);
void setLastError(const char* message);
void loadPreviousBootStage();
void recordBootStage(const char* stage);
void saveConfig();
void loadConfig();
void setStatusLed(bool on);
void updateStatusLed();

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
		// Release: drive LOW. U3 (74HCT08) sits between ESP and hood; a
		// floating HCT input would make it press keys at random. The diode
		// behind U3 blocks LOW, so the hood's line is left alone.
		digitalWrite(buttonPin1, LOW);
		digitalWrite(buttonPin2, LOW);
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
	if (!f) {
		setLastError("Unable to open config.json for writing");
		return;
	}
	if (serializeJson(doc, f) == 0) setLastError("Unable to write config.json");
	f.close();
}

void loadConfig() {
	if (!LittleFS.exists("/config.json")) return;
	File f = LittleFS.open("/config.json", "r");
	if (!f) {
		setLastError("Unable to open config.json");
		return;
	}
	JsonDocument doc;
	if (deserializeJson(doc, f) == DeserializationError::Ok) {
		strlcpy(mqtt_host,   doc["mqtt_host"]   | mqtt_host,   sizeof(mqtt_host));
		strlcpy(mqtt_port,   doc["mqtt_port"]   | mqtt_port,   sizeof(mqtt_port));
		strlcpy(mqtt_prefix, doc["mqtt_prefix"] | mqtt_prefix, sizeof(mqtt_prefix));
		strlcpy(device_name, doc["device_name"] | device_name, sizeof(device_name));
	} else setLastError("Invalid config.json");
	f.close();
}

void loadPreviousBootStage() {
	if (!LittleFS.exists("/boot-stage.txt")) return;
	File f = LittleFS.open("/boot-stage.txt", "r");
	if (!f) return;
	size_t length = f.readBytes(previousBootStage, sizeof(previousBootStage) - 1);
	previousBootStage[length] = '\0';
	f.close();
}

void recordBootStage(const char* stage) {
	File f = LittleFS.open("/boot-stage.txt", "w");
	if (!f) {
		setLastError("Unable to persist boot stage");
		return;
	}
	f.print(stage);
	f.close();
}

// ---- Status LED (onboard blue LED, GPIO2/D4, active LOW) ----
//
// Blink pattern shows what boot/connection stage we're in; once WiFi+MQTT
// are both up it goes mostly dark with a brief periodic "heartbeat" blip
// so a glance confirms the device is alive without being distracting.

void setStatusLed(bool on) {
	digitalWrite(statusLedPin, on ? LOW : HIGH);
}

void updateStatusLed() {
	unsigned long onMs, offMs;

	if (wifiManager.getConfigPortalActive()) {
		onMs = 1000; offMs = 1000;               // slow blink: waiting for setup
	} else if (WiFi.status() != WL_CONNECTED) {
		onMs = 150;  offMs = 150;                 // fast blink: connecting to WiFi
	} else if (!mqtt.connected()) {
		onMs = 400;  offMs = 400;                 // medium blink: connecting to MQTT
	} else {
		onMs = 40;   offMs = 2960;                // heartbeat: everything is fine
	}

	static bool ledOn = false;
	static unsigned long lastToggle = 0;
	unsigned long now = millis();
	if (now - lastToggle >= (ledOn ? onMs : offMs)) {
		ledOn = !ledOn;
		lastToggle = now;
		setStatusLed(ledOn);
	}
}

void setLastError(const char* message) {
	strlcpy(lastError, message, sizeof(lastError));
	Serial.printf("ERROR: %s\n", lastError);
	addEvent(lastError);
}

void addEvent(const char* message) {
	size_t index = (eventLogStart + eventLogCount) % EVENT_LOG_SIZE;
	if (eventLogCount == EVENT_LOG_SIZE) {
		eventLogStart = (eventLogStart + 1) % EVENT_LOG_SIZE;
		index = (eventLogStart + eventLogCount - 1) % EVENT_LOG_SIZE;
	} else {
		eventLogCount++;
	}
	snprintf(eventLog[index], EVENT_LENGTH, "%lus: %s", millis() / 1000, message);
	Serial.println(eventLog[index]);
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

bool publishState(int i) {
	if (!mqtt.connected()) {
		stateSyncPending = true;
		return false;
	}
	String prefix = String(mqtt_prefix);
	bool published = false;
	switch (i) {
	case 1: case 2: case 3: case 4: {
		String v = ledStates[1] ? "1" : ledStates[2] ? "2" : ledStates[3] ? "3" : ledStates[4] ? "4" : "0";
		published = mqtt.publish((prefix + "/ventilation/state").c_str(), v.c_str(), true);
		Serial.printf("ventilation: %s\n", v.c_str());
		break;
	}
	case 5:
		published = mqtt.publish((prefix + "/timer/state").c_str(), ledStates[5] ? "true" : "false", true);
		Serial.printf("timer: %s\n", ledStates[5] ? "true" : "false");
		break;
	case 6:
		published = mqtt.publish((prefix + "/light/state").c_str(), ledStates[6] ? "true" : "false", true);
		Serial.printf("light: %s\n", ledStates[6] ? "true" : "false");
		break;
	case 7:
		published = mqtt.publish((prefix + "/maintenance/state").c_str(), ledStates[7] ? "true" : "false", true);
		Serial.printf("maintenance: %s\n", ledStates[7] ? "true" : "false");
		break;
	}
	if (!published) {
		stateSyncPending = true;
		setLastError("MQTT state publish failed");
	}
	return published;
}

bool publishAllStates() {
	bool success = publishState(1);
	for (int i = 5; i <= 7; i++) success = publishState(i) && success;
	stateSyncPending = !success;
	return success;
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
	bool success = true;
	auto publish = [&](const char* component, const char* obj_id, JsonDocument& doc) {
		String topic = "homeassistant/";
		topic += component;
		topic += "/";
		topic += dev_id + "_" + obj_id;
		topic += "/config";
		String payload;
		serializeJson(doc, payload);
		success = mqtt.publish(topic.c_str(), payload.c_str(), true) && success;
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
		// Template muss payload_on/payload_off liefern, nicht "on"/"off"
		doc["state_value_template"]    = "{% if value == '0' %}0{% else %}1{% endif %}";
		doc["preset_mode_state_topic"] = prefix + "/ventilation/state";
		doc["preset_mode_command_topic"] = prefix + "/ventilation/set";
		// "0" ist kein Preset; "None" (payload_reset_preset_mode) setzt es zurück
		doc["preset_mode_value_template"] = "{% if value == '0' %}None{% else %}{{ value }}{% endif %}";
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

	discoveryPending = !success;
	if (success) Serial.println("HA discovery published");
	else setLastError("HA discovery publish failed");
}

void mqttReconnect() {
	String will = String(mqtt_prefix) + "/$online";
	if (mqtt.connect(mqttClientId, will.c_str(), 1, true, "false")) {
		bool onlinePublished = mqtt.publish(will.c_str(), "true", true);
		bool subscribed = mqtt.subscribe((String(mqtt_prefix) + "/+/set").c_str(), 1);
		if (!onlinePublished || !subscribed) {
			mqttFailures++;
			setLastError("MQTT initialization publish/subscribe failed");
			mqtt.disconnect();
			mqttRetryInterval = min(mqttRetryInterval * 2, MQTT_RETRY_MAX);
			return;
		}
		mqttReconnects++;
		mqttRetryInterval = MQTT_RETRY_MIN;
		stateSyncPending = true;
		discoveryPending = true;
		lastDiagnostics = millis() - DIAGNOSTICS_INTERVAL;
		addEvent("MQTT connected");
		Serial.printf("MQTT connected as %s\n", mqttClientId);
	} else {
		mqttFailures++;
		snprintf(lastError, sizeof(lastError), "MQTT connect failed, state=%d", mqtt.state());
		addEvent(lastError);
		mqttRetryInterval = min(mqttRetryInterval * 2, MQTT_RETRY_MAX);
	}
}

bool publishDiagnostics() {
	if (!mqtt.connected()) return false;
	JsonDocument doc;
	doc["firmware"] = FW_VERSION;
	doc["uptime_s"] = millis() / 1000;
	doc["rssi"] = WiFi.RSSI();
	doc["wifi_status"] = WiFi.status();
	doc["ip"] = WiFi.localIP().toString();
	doc["mqtt_state"] = mqtt.state();
	doc["wifi_reconnects"] = wifiReconnects;
	doc["mqtt_reconnects"] = mqttReconnects;
	doc["mqtt_failures"] = mqttFailures;
	doc["free_heap"] = ESP.getFreeHeap();
	doc["reset_reason"] = ESP.getResetReason();
	doc["previous_boot_stage"] = previousBootStage;
	doc["last_error"] = lastError;
	doc["portal_active"] = wifiManager.getConfigPortalActive();
	String payload;
	serializeJson(doc, payload);
	String prefix = String(mqtt_prefix);
	String uptime = String(millis() / 1000);
	String rssi = String(WiFi.RSSI());
	String ip = WiFi.localIP().toString();
	String resetReason = ESP.getResetReason();
	bool success = mqtt.publish((prefix + "/diagnostics").c_str(), payload.c_str(), true);
	success = mqtt.publish((prefix + "/$wifi_rssi").c_str(), rssi.c_str(), true) && success;
	success = mqtt.publish((prefix + "/$uptime").c_str(), uptime.c_str(), true) && success;
	success = mqtt.publish((prefix + "/$reset_reason").c_str(), resetReason.c_str(), true) && success;
	success = mqtt.publish((prefix + "/$firmware").c_str(), FW_VERSION, true) && success;
	success = mqtt.publish((prefix + "/$ip").c_str(), ip.c_str(), true) && success;
	success = mqtt.publish((prefix + "/$last_error").c_str(), lastError, true) && success;
	if (!success) setLastError("MQTT diagnostics publish failed");
	return success;
}

void publishEventLog() {
	while (mqtt.connected() && eventLogCount > 0) {
		if (!mqtt.publish((String(mqtt_prefix) + "/events").c_str(),
				eventLog[eventLogStart], false)) {
			setLastError("MQTT event publish failed");
			return;
		}
		eventLogStart = (eventLogStart + 1) % EVENT_LOG_SIZE;
		eventLogCount--;
	}
}

void startConfigPortal() {
	if (wifiManager.getConfigPortalActive()) return;
	addEvent("Starting protected config portal at 192.168.4.1");
	wifiManager.startConfigPortal(CONFIG_PORTAL_SSID, CONFIG_PORTAL_PASSWORD);

	// WiFiManager disables STA when it starts a portal without a connection.
	WiFi.mode(WIFI_AP_STA);
	WiFi.begin();
	lastWifiAttempt = millis();
}

void processNetwork() {
	unsigned long now = millis();
	bool connected = WiFi.status() == WL_CONNECTED;

	if (connected && !wasWifiConnected) {
		wifiConnectedSince = now;
		disconnectedSince = 0;
		mqttRetryInterval = MQTT_RETRY_MIN;
		lastMqttAttempt = now - MQTT_RETRY_MIN;
		addEvent("WiFi connected");
		Serial.printf("IP=%s RSSI=%d\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
		ArduinoOTA.setHostname(device_name);
		ArduinoOTA.begin();
	} else if (!connected && wasWifiConnected) {
		wifiReconnects++;
		disconnectedSince = now;
		snprintf(lastError, sizeof(lastError), "WiFi disconnected, status=%d", WiFi.status());
		addEvent(lastError);
	}
	wasWifiConnected = connected;

	if (wifiManager.getConfigPortalActive()) wifiManager.process();

	if (!connected) {
		if (disconnectedSince == 0) disconnectedSince = now;
		unsigned long retryInterval = wifiManager.getConfigPortalActive()
			? PORTAL_RETRY_INTERVAL : WIFI_RETRY_INTERVAL;
		if (now - lastWifiAttempt >= retryInterval) {
			lastWifiAttempt = now;
			WiFi.mode(wifiManager.getConfigPortalActive() ? WIFI_AP_STA : WIFI_STA);
			WiFi.begin();
			addEvent("WiFi reconnect requested");
		}
		if (now - disconnectedSince >= PORTAL_START_DELAY) startConfigPortal();
	} else if (wifiManager.getConfigPortalActive()
			&& now - wifiConnectedSince >= PORTAL_CLOSE_DELAY) {
		wifiManager.stopConfigPortal();
		WiFi.mode(WIFI_STA);
		addEvent("Config portal stopped");
	}
}

// ---- Setup ----

void setup() {
	Serial.begin(115200);
	Serial.println("\n\n" FW_NAME " v" FW_VERSION);

	// Button outputs LOW as early as possible (they feed U3's inputs)
	pinMode(buttonPin1, OUTPUT);
	digitalWrite(buttonPin1, LOW);
	pinMode(buttonPin2, OUTPUT);
	digitalWrite(buttonPin2, LOW);

	// Status LED: three quick blips to mark power-on / start of setup()
	pinMode(statusLedPin, OUTPUT);
	setStatusLed(false);
	for (int i = 0; i < 3; i++) {
		setStatusLed(true);
		delay(80);
		setStatusLed(false);
		delay(80);
	}

	if (!LittleFS.begin()) Serial.println("ERROR: LittleFS mount failed");
	loadPreviousBootStage();
	recordBootStage("setup");
	loadConfig();
	Serial.printf("Reset reason: %s; previous boot stage: %s\n",
		ESP.getResetReason().c_str(), previousBootStage);

	// Halten des Flash-Buttons (GPIO0) beim Start setzt WiFi-Config zurück
	pinMode(0, INPUT_PULLUP);
	if (digitalRead(0) == LOW) {
		wifiManager.resetSettings();
		LittleFS.remove("/config.json");
		Serial.println("WiFi + MQTT config reset!");
	}

	p_host.setValue(mqtt_host, sizeof(mqtt_host));
	p_port.setValue(mqtt_port, sizeof(mqtt_port));
	p_prefix.setValue(mqtt_prefix, sizeof(mqtt_prefix));
	p_name.setValue(device_name, sizeof(device_name));
	wifiManager.addParameter(&p_host);
	wifiManager.addParameter(&p_port);
	wifiManager.addParameter(&p_prefix);
	wifiManager.addParameter(&p_name);
	wifiManager.setSaveParamsCallback([]() {
		strlcpy(mqtt_host,   p_host.getValue(),   sizeof(mqtt_host));
		strlcpy(mqtt_port,   p_port.getValue(),   sizeof(mqtt_port));
		strlcpy(mqtt_prefix, p_prefix.getValue(), sizeof(mqtt_prefix));
		strlcpy(device_name, p_name.getValue(),   sizeof(device_name));
		saveConfig();
		configSavedAt = millis();
		addEvent("Configuration saved; restart scheduled");
	});
	wifiManager.setConfigPortalBlocking(false);
	wifiManager.setConnectTimeout(10);
	wifiManager.setSaveConnectTimeout(10);
	wifiManager.setWiFiAutoReconnect(true);
	wifiManager.setHostname(device_name);

	// WiFi Robustheit bei stabiler Stromversorgung optimieren
	WiFi.setAutoReconnect(true);
	WiFi.setOutputPower(20.5f);
	WiFi.setSleepMode(WIFI_NONE_SLEEP);
	WiFi.mode(WIFI_STA);
	WiFi.begin();
	disconnectedSince = millis();
	lastWifiAttempt = millis();
	snprintf(mqttClientId, sizeof(mqttClientId), "%s-%06x",
		device_name, ESP.getChipId());
	addEvent("WiFi connection started");

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
	// MQTT
	mqtt.setServer(mqtt_host, atoi(mqtt_port));
	mqtt.setCallback(mqttCallback);
	mqtt.setBufferSize(1024);
	mqtt.setKeepAlive(15);
	mqtt.setSocketTimeout(3);

	// Pins initialisieren
	resync = 0;
	for (int i = 0; i < 8; i++) {
		ledStates[i]        = false;
		ledStateCounter[i]  = 0;
		buttonStateCounter[i] = 0;
	}
	pinMode(inputPin,   INPUT);
	pinMode(ledPin1,    INPUT);
	pinMode(ledPin2,    INPUT);

	attachInterrupt(digitalPinToInterrupt(inputPin), IsrResync, RISING);

	timer1_isr_init();
	timer1_attachInterrupt(IsrTimer);
	recordBootStage("running");
}

// ---- Loop ----

void loop() {
	ArduinoOTA.handle();
	processNetwork();
	updateStatusLed();

	unsigned long now = millis();
	if (!mqtt.connected()) {
		if (wasMqttConnected) {
			snprintf(lastError, sizeof(lastError), "MQTT disconnected, state=%d", mqtt.state());
			addEvent(lastError);
		}
		if (WiFi.status() == WL_CONNECTED
				&& now - lastMqttAttempt >= mqttRetryInterval) {
			lastMqttAttempt = now;
			mqttReconnect();
		}
	} else {
		if (!mqtt.loop()) {
			snprintf(lastError, sizeof(lastError), "MQTT loop failed, state=%d", mqtt.state());
			addEvent(lastError);
		}
		if ((stateSyncPending || discoveryPending)
				&& now - lastStateAttempt >= STATE_RETRY_INTERVAL) {
			lastStateAttempt = now;
			if (discoveryPending) publishDiscovery();
			if (stateSyncPending) publishAllStates();
		}
		if (now - lastDiagnostics >= DIAGNOSTICS_INTERVAL) {
			lastDiagnostics = now;
			publishDiagnostics();
		}
		publishEventLog();
	}
	wasMqttConnected = mqtt.connected();

	if (configSavedAt != 0 && now - configSavedAt >= 2000) {
		recordBootStage("config-restart");
		ESP.restart();
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
