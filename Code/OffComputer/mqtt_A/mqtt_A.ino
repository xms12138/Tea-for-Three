#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

// ========= Project =========
const char* mqtt_client_id = "tea-for-three-A";

// ========= WiFi =========
char ssid[] = "CE-Hub-Student";
char pass[] = "casa-ce-gagarin-public-service";

// ========= MQTT =========
const char* mqtt_host = "mqtt.cetools.org";
const int   mqtt_port = 1884;
const char* mqtt_user = "student";
const char* mqtt_pass = "ce2021-mqtt-forget-whale";

// ========= Topics =========
const char* pub_topic_A = "student/MUJI/hzh/A";
const char* sub_topic_B = "student/MUJI/hzh/B";
const char* sub_topic_C = "student/MUJI/hzh/C";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ========= FSR =========
const int FSR_PIN = A6;

// Pressure detected  -> ON
// No pressure        -> OFF
const int PRESS_ON_THRESHOLD    = 320;
const int RELEASE_OFF_THRESHOLD = 260;

const int REQUIRED_STABLE_COUNT = 3;

const int SAMPLE_COUNT = 10;
const int SAMPLE_DELAY_MS = 3;

// ========= Original LED strips =========
#define LED1_PIN 6
#define LED2_PIN 7
#define LED_COUNT 8

Adafruit_NeoPixel led1(LED_COUNT, LED1_PIN, NEO_GRB + NEO_KHZ800);  // controlled by topic B
Adafruit_NeoPixel led2(LED_COUNT, LED2_PIN, NEO_GRB + NEO_KHZ800);  // controlled by topic C

// ========= New A strip =========
#define LED3_PIN 8
#define LED3_COUNT 10

Adafruit_NeoPixel led3(LED3_COUNT, LED3_PIN, NEO_GRB + NEO_KHZ800); // A board special strip

// ========= LED state machine =========
enum LedMode {
  LED_OFF,
  LED_ON,
  LED_BLINKING_OFF
};

// LED1 -> topic B -> green
LedMode led1Mode = LED_OFF;
unsigned long led1BlinkStartMs = 0;
unsigned long led1LastToggleMs = 0;
bool led1BlinkVisible = false;

// LED2 -> topic C -> blue
LedMode led2Mode = LED_OFF;
unsigned long led2BlinkStartMs = 0;
unsigned long led2LastToggleMs = 0;
bool led2BlinkVisible = false;

const unsigned long BLINK_DURATION_MS = 3000;
const unsigned long BLINK_INTERVAL_MS = 250;

// ========= Remote topic states =========
bool stateBOn = false;
bool stateCOn = false;

// ========= A strip state =========
enum AStripMode {
  ASTRIP_OFF,
  ASTRIP_FLASHING,
  ASTRIP_SOLID
};

AStripMode aStripMode = ASTRIP_OFF;

unsigned long aFlashStartMs = 0;
unsigned long aLastToggleMs = 0;
bool aFlashVisible = false;

const int A_FLASH_COUNT = 2;
const unsigned long A_FLASH_INTERVAL_MS = 180;

// ========= FSR state =========
bool currentStateAOn = false;
bool lastSentStateAOn = false;

int pressStableCount = 0;
int releaseStableCount = 0;

unsigned long lastPublishMs = 0;
const unsigned long republishInterval = 5000;

// ===== Helper: set full strip color =====
void setStripColor(Adafruit_NeoPixel &strip, int count, uint32_t color) {
  for (int i = 0; i < count; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

// ===== Color generator for LED3 flowing effect =====
uint32_t cyberColor(int x) {
  x = x % 256;

  if (x < 85) {
    return led3.Color(0, x * 2, 255);
  } else if (x < 170) {
    x -= 85;
    return led3.Color(x * 3, 0, 255);
  } else {
    x -= 170;
    return led3.Color(255, 0, 255 - x * 3);
  }
}

// ===== Render colorful flowing effect on LED3 =====
void renderFancyStrip3(int offset) {
  for (int i = 0; i < LED3_COUNT; i++) {
    int colorIndex = ((i * 80) + offset) & 255;
    led3.setPixelColor(i, cyberColor(colorIndex));
  }
  led3.show();
}

// ===== Render LED1 based on current mode =====
void renderLed1() {
  if (led1Mode == LED_ON) {
    setStripColor(led1, LED_COUNT, led1.Color(0, 255, 0));
  }
  else if (led1Mode == LED_OFF) {
    setStripColor(led1, LED_COUNT, led1.Color(0, 0, 0));
  }
  else if (led1Mode == LED_BLINKING_OFF) {
    if (led1BlinkVisible)
      setStripColor(led1, LED_COUNT, led1.Color(0, 255, 0));
    else
      setStripColor(led1, LED_COUNT, led1.Color(0, 0, 0));
  }
}

// ===== Render LED2 based on current mode =====
void renderLed2() {
  if (led2Mode == LED_ON) {
    setStripColor(led2, LED_COUNT, led2.Color(0, 0, 255));
  }
  else if (led2Mode == LED_OFF) {
    setStripColor(led2, LED_COUNT, led2.Color(0, 0, 0));
  }
  else if (led2Mode == LED_BLINKING_OFF) {
    if (led2BlinkVisible)
      setStripColor(led2, LED_COUNT, led2.Color(0, 0, 255));
    else
      setStripColor(led2, LED_COUNT, led2.Color(0, 0, 0));
  }
}

// ===== Topic B received ON =====
void led1TurnOn() {
  led1Mode = LED_ON;
  led1BlinkVisible = true;
  renderLed1();
}

// ===== Topic B received OFF =====
void led1StartBlinkingOff() {
  led1Mode = LED_BLINKING_OFF;
  led1BlinkStartMs = millis();
  led1LastToggleMs = millis();
  led1BlinkVisible = true;
  renderLed1();
}

// ===== Topic C received ON =====
void led2TurnOn() {
  led2Mode = LED_ON;
  led2BlinkVisible = true;
  renderLed2();
}

// ===== Topic C received OFF =====
void led2StartBlinkingOff() {
  led2Mode = LED_BLINKING_OFF;
  led2BlinkStartMs = millis();
  led2LastToggleMs = millis();
  led2BlinkVisible = true;
  renderLed2();
}

// ===== A strip control =====
void aStripOff() {
  aStripMode = ASTRIP_OFF;
  setStripColor(led3, LED3_COUNT, led3.Color(0, 0, 0));
}

void aStripStartFlashing() {
  aStripMode = ASTRIP_FLASHING;
  aFlashStartMs = millis();
  aLastToggleMs = millis();
  aFlashVisible = true;
  setStripColor(led3, LED3_COUNT, led3.Color(255, 255, 255));
}

void aStripStartSolid() {
  aStripMode = ASTRIP_SOLID;
}

void updateAStripState() {
  if (!currentStateAOn) {
    aStripOff();
    return;
  }

  // If A is ON and both B and C are already ON,
  // directly enter the solid flowing effect
  if (stateBOn && stateCOn) {
    aStripStartSolid();
    return;
  }

  // If A is ON but B and C are not both ON,
  // flash twice first
  if (aStripMode == ASTRIP_OFF) {
    aStripStartFlashing();
  }
}

void updateAStripEffect() {
  static int flowOffset = 0;
  unsigned long now = millis();

  if (aStripMode == ASTRIP_FLASHING) {
    if (now - aLastToggleMs >= A_FLASH_INTERVAL_MS) {
      aFlashVisible = !aFlashVisible;
      aLastToggleMs = now;

      if (aFlashVisible) {
        setStripColor(led3, LED3_COUNT, led3.Color(255, 255, 255));
      } else {
        setStripColor(led3, LED3_COUNT, led3.Color(0, 0, 0));
      }
    }

    // Two flashes = on/off/on/off = 4 intervals
    if (now - aFlashStartMs >= (unsigned long)(A_FLASH_COUNT * 2 * A_FLASH_INTERVAL_MS)) {
      if (stateBOn && stateCOn && currentStateAOn) {
        aStripStartSolid();
      } else {
        aStripOff();
      }
    }
  }
  else if (aStripMode == ASTRIP_SOLID) {
    if (stateBOn && stateCOn && currentStateAOn) {
      renderFancyStrip3(flowOffset);
      flowOffset = (flowOffset + 20) & 255;
    } else {
      aStripOff();
    }
  }
}

// ===== Update blinking logic for LED1 and LED2 =====
void updateBlinkingLEDs() {
  unsigned long now = millis();

  // LED1 blinking-off logic
  if (led1Mode == LED_BLINKING_OFF) {
    if (now - led1BlinkStartMs >= BLINK_DURATION_MS) {
      led1Mode = LED_OFF;
      led1BlinkVisible = false;
      renderLed1();
    }
    else if (now - led1LastToggleMs >= BLINK_INTERVAL_MS) {
      led1BlinkVisible = !led1BlinkVisible;
      led1LastToggleMs = now;
      renderLed1();
    }
  }

  // LED2 blinking-off logic
  if (led2Mode == LED_BLINKING_OFF) {
    if (now - led2BlinkStartMs >= BLINK_DURATION_MS) {
      led2Mode = LED_OFF;
      led2BlinkVisible = false;
      renderLed2();
    }
    else if (now - led2LastToggleMs >= BLINK_INTERVAL_MS) {
      led2BlinkVisible = !led2BlinkVisible;
      led2LastToggleMs = now;
      renderLed2();
    }
  }
}

// ===== MQTT callback =====
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  String msg = "";

  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  msg.trim();

  Serial.print("MQTT ");
  Serial.print(topicStr);
  Serial.print(" = ");
  Serial.println(msg);

  // Topic B controls LED1
  if (topicStr == sub_topic_B) {
    if (msg == "on") {
      stateBOn = true;
      led1TurnOn();
    }
    else if (msg == "off") {
      stateBOn = false;
      if (led1Mode == LED_ON) {
        led1StartBlinkingOff();
      }
    }
  }

  // Topic C controls LED2
  if (topicStr == sub_topic_C) {
    if (msg == "on") {
      stateCOn = true;
      led2TurnOn();
    }
    else if (msg == "off") {
      stateCOn = false;
      if (led2Mode == LED_ON) {
        led2StartBlinkingOff();
      }
    }
  }

  // Re-evaluate A strip state after receiving B/C updates
  updateAStripState();
}

// ===== Connect to WiFi =====
void connectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting WiFi...");
    WiFi.begin(ssid, pass);
    delay(2000);
  }

  Serial.println("WiFi connected");
}

// ===== Connect to MQTT broker =====
void connectMQTT() {
  mqttClient.setServer(mqtt_host, mqtt_port);
  mqttClient.setCallback(onMqttMessage);

  while (!mqttClient.connected()) {
    if (mqttClient.connect(mqtt_client_id, mqtt_user, mqtt_pass)) {
      Serial.println("MQTT connected");
      mqttClient.subscribe(sub_topic_B);
      mqttClient.subscribe(sub_topic_C);
    }
    else {
      Serial.print("MQTT failed rc=");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

// ===== Read averaged FSR value =====
int readFSR() {
  long sum = 0;

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sum += analogRead(FSR_PIN);
    delay(SAMPLE_DELAY_MS);
  }

  return sum / SAMPLE_COUNT;
}

// ===== Publish A state =====
void publishStateA(bool stateOn) {
  const char* payload = stateOn ? "on" : "off";

  Serial.print("Publish A = ");
  Serial.println(payload);

  mqttClient.publish(pub_topic_A, payload, true);
}

// ===== FSR debounce logic =====
// true  -> pressure detected -> ON
// false -> no pressure       -> OFF
void updateFSRState(int fsrValue) {
  if (!currentStateAOn) {
    if (fsrValue >= PRESS_ON_THRESHOLD) {
      pressStableCount++;
      releaseStableCount = 0;

      if (pressStableCount >= REQUIRED_STABLE_COUNT) {
        currentStateAOn = true;
        pressStableCount = 0;
        releaseStableCount = 0;
      }
    }
    else {
      pressStableCount = 0;
    }
  }
  else {
    if (fsrValue <= RELEASE_OFF_THRESHOLD) {
      releaseStableCount++;
      pressStableCount = 0;

      if (releaseStableCount >= REQUIRED_STABLE_COUNT) {
        currentStateAOn = false;
        pressStableCount = 0;
        releaseStableCount = 0;
      }
    }
    else {
      releaseStableCount = 0;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(10);

  led1.begin();
  led2.begin();
  led3.begin();

  led1.setBrightness(255);
  led2.setBrightness(255);
  led3.setBrightness(150);

  led1.clear();
  led2.clear();
  led3.clear();

  led1.show();
  led2.show();
  led3.show();

  connectWiFi();
  connectMQTT();

  int fsrValue = readFSR();
  int midThreshold = (PRESS_ON_THRESHOLD + RELEASE_OFF_THRESHOLD) / 2;

  currentStateAOn = (fsrValue >= midThreshold);
  lastSentStateAOn = currentStateAOn;

  publishStateA(currentStateAOn);
  lastPublishMs = millis();

  renderLed1();
  renderLed2();
  updateAStripState();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED)
    connectWiFi();

  if (!mqttClient.connected())
    connectMQTT();

  mqttClient.loop();

  int fsrValue = readFSR();

  Serial.print("FSR = ");
  Serial.println(fsrValue);

  bool prevStateAOn = currentStateAOn;
  updateFSRState(fsrValue);

  unsigned long now = millis();

  if (currentStateAOn != lastSentStateAOn) {
    publishStateA(currentStateAOn);
    lastSentStateAOn = currentStateAOn;
    lastPublishMs = now;

    if (!prevStateAOn && currentStateAOn) {
      // A has just been pressed
      if (stateBOn && stateCOn) {
        aStripStartSolid();
      } else {
        aStripStartFlashing();
      }
    }

    if (prevStateAOn && !currentStateAOn) {
      aStripOff();
    }
  }

  if (now - lastPublishMs >= republishInterval) {
    publishStateA(currentStateAOn);
    lastPublishMs = now;
  }

  updateBlinkingLEDs();
  updateAStripEffect();

  delay(10);
}