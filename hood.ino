#include <ArduinoOTA.h>
#include <Homie.h>

#define FW_NAME "HoodControl"
#define FW_VERSION "0.0.2"

// input lines
const int inputPin = 13;
// buttons
const int buttonPin1 = 12; //D6
const int buttonPin2 = 14; //D5
// leds
const int ledPin1 = 5; //D1
const int ledPin2 = 4; //D2

unsigned long WiFifix = 0;
unsigned long problemDetected = 0;
int problemCount = 0;
int deleteConfig = 0;
String problemCause;

HomieNode control("hoodcontrol", "Hood Control", "control");

volatile boolean ledStates[8];
volatile int resync;
volatile int ledStateCounter[8];
volatile int buttonStateCounter[8];

bool ventilationHandler(const HomieRange& range, const String& value) {
  long id = value.toInt();
  if (id >= 1 && id <= 4) {
    Serial.println("Set ventilation");
    buttonStateCounter[id] = 100;
  }
  if (id == 0) {
    for (int i = 1; i <= 4; i++) {
      if (ledStates[i]) {
        buttonStateCounter[i] = 100;
      }
    }
  }
  return true;
}

bool lightHandler(const HomieRange& range, const String& value) {
  if ((value == "false" && ledStates[6])
      || (value == "true" && !ledStates[6])) {
    Serial.println("Switch light");
    buttonStateCounter[6] = 100;
  }
  return true;
}

bool timerHandler(const HomieRange& range, const String& value) {
  if ((value == "false" && ledStates[5])
      || (value == "true" && !ledStates[5])) {
    Serial.println("Switch timer");
    buttonStateCounter[5] = 100;
  }
  return true;
}

bool maintenanceHandler(const HomieRange& range, const String& value) {
  if (ledStates[7]) {
    buttonStateCounter[7] = 1500; //~ 6sec (1500*4=6000ms)
  }
  return true;
}


volatile unsigned long lastIsr = 0;
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
	switch (phaseCounter % 40) {
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
	switch (inputPin) {
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

	switch (inputPin) {
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

void onHomieEvent(const HomieEvent &event)
{
  // The device rebooted when attachInterrupt was called in setup()
  // before Wifi was connected and interrupts were already coming in.
  switch (event.type)
  {
  case HomieEventType::WIFI_CONNECTED:
    timer1_isr_init();
    timer1_attachInterrupt(IsrTimer);
    //attachInterrupt(PIN_OPTOCOUPLER, onOptoCouplerPulse, FALLING);
    break;
  case HomieEventType::WIFI_DISCONNECTED:
    timer1_detachInterrupt();
    //detachInterrupt(PIN_OPTOCOUPLER);
    break;
  default:
    break;
  }
}

void setup() {
  Homie_setBrand(FW_NAME);
  Homie_setFirmware(FW_NAME, FW_VERSION);
  Serial.begin(115200);
  Serial << endl
         << endl;
  Homie.getLogger() << "System started" << endl;
  control.advertise("ventilation").setDatatype("integer").settable(ventilationHandler);
  control.advertise("timer").setDatatype("boolean").settable(timerHandler);
  control.advertise("light").setDatatype("boolean").settable(lightHandler);
  control.advertise("maintenance").setDatatype("boolean").settable(maintenanceHandler);

  Homie.disableResetTrigger();
  Homie.onEvent(onHomieEvent);
  Homie.setup();
  ArduinoOTA.setHostname(Homie.getConfiguration().deviceId);
  ArduinoOTA.begin();

	resync = 0;
	for (int i = 0; i < 8; i++) {
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


}

void fixWiFi()
{
  // Posts every 10 seconds the state of WiFi.status(), Homie.getMqttClient().connected() and Homie.isConfigured()
  // Within this interval the connectivity is checked and logged if a problem is detected
  // Then it disconnects Wifi, if Wifi or MQTT is not connected for 1 Minute (but only if Homie is configured)
  if (WiFifix == 0 || ((millis() - WiFifix) > 10000))
  {

    float rssi = WiFi.RSSI();
    //relayNode.setProperty("quality").send(String(rssi));
    Homie.getLogger() << "Wifi-state:" << WiFi.status() << " | Wi-Fi signal quality: " << rssi << " | MQTT-state:" << Homie.getMqttClient().connected() << " | HomieConfig-state:" << Homie.isConfigured() << endl;

    if (!Homie.getMqttClient().connected() || WiFi.status() != 3)
    {
      if (0 == problemDetected)
      {
        if (WiFi.status() != 3)
        {
          problemCause = "WiFi: Disconnected ";
        }
        if (!Homie.getMqttClient().connected())
        {
          problemCause += "MQTT: Disconnected";
        }
        Homie.getLogger() << "Connectivity in problematic state --> " << problemCause << endl;
        problemDetected = millis();
      }
      else if ((millis() - problemDetected) > 120000 && (problemCount >= 5))
      {
        Homie.getLogger() << "Connectivity in problematic state --> This remained for 10 minutes. Rebooting!" << endl;
        Homie.reboot();
      }
      else if ((millis() - problemDetected) > 120000 && problemCount < 5)
      {
        problemCount = (problemCount + 1);
        Homie.getLogger() << "Connectivity in problematic state --> " << problemCause << "/n This remained for 2 minutes. Disconnecting WiFi to start over." << endl;
        problemDetected = 0;
        problemCause = "";
        if (WiFi.status() != 0)
        {
          WiFi.disconnect();
        }
        if (WiFi.status() == 0)
        {
          WiFi.begin();
        }
      }
    }
    else if (problemCount != 0 && Homie.getMqttClient().connected() || WiFi.status() == 3)
    {
      problemCount = 0;
      ArduinoOTA.setHostname(Homie.getConfiguration().deviceId);
      ArduinoOTA.begin();
    }
  }
  WiFifix = millis();
}

unsigned long lastPublish = 0;
int counter = 0;

void loop() {
  ArduinoOTA.handle();
  Homie.loop();
  if (Homie.isConfigured() == 1)
  {
    fixWiFi();
  }

	// check ledStateCounters and switch state if necessary
	for (int i = 1; i < 8; i++) {
		if ((ledStateCounter[i] > 20 && i != 7)
				|| (ledStateCounter[i] > 20 && i == 7 && ledStates[7] == false)
				|| (ledStateCounter[i] > 375 && i == 7 && ledStates[7] == true)) {
			ledStates[i] = !ledStates[i];
			ledStateCounter[i] = 0;
			Serial.print(i);
			Serial.print(ledStates[i]);
			Serial.print(" ");
			switch (i) {
			case 1:
			case 2:
			case 3:
			case 4: {
				String ledState =
						ledStates[1] ?
								"1" :
								(ledStates[2] ?
										"2" :
										(ledStates[3] ?
												"3" : (ledStates[4] ? "4" : "0")));
				Serial.print("Update home/kitchen/hood/ventilation/state ");
				Serial.println(ledState);
        control.setProperty("ventilation").send(ledState);
				break;
			}
			case 5: {
				String ledState = ledStates[5] ? "true": "false";
				Serial.print("Update home/kitchen/hood/timer/state ");
				Serial.println(ledState);
        control.setProperty("timer").send(ledState);
				break;
			}
			case 6: {
				String ledState = ledStates[6] ? "true": "false";
				Serial.print("Update home/kitchen/hood/light/state ");
				Serial.println(ledState);
        control.setProperty("light").send(ledState);
				break;
			}
			case 7: {
				String ledState = ledStates[7] ? "true": "false";
				Serial.print("Update home/kitchen/hood/maintenance/state ");
				Serial.println(ledState);
        control.setProperty("maintenance").send(ledState);
				break;
			}
			}
		}
	}

}
