#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

// ========= WiFi =========
char ssid[] = "CE-Hub-Student";
char pass[] = "casa-ce-gagarin-public-service";

// ========= MQTT =========
const char* mqtt_host = "mqtt.cetools.org";
const int   mqtt_port = 1884;
const char* mqtt_user = "student";
const char* mqtt_pass = "ce2021-mqtt-forget-whale";

// ========= Topics =========
const char* pub_topic_C = "student/MUJI/hzh/C";
const char* sub_topic_A = "student/MUJI/hzh/A";
const char* sub_topic_B = "student/MUJI/hzh/B";

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

// ========= LED =========
#define LED1_PIN 6
#define LED2_PIN 7
#define LED_COUNT 8

Adafruit_NeoPixel led1(LED_COUNT, LED1_PIN, NEO_GRB + NEO_KHZ800);  // controlled by topic A
Adafruit_NeoPixel led2(LED_COUNT, LED2_PIN, NEO_GRB + NEO_KHZ800);  // controlled by topic B

// ========= LED State Machine =========
enum LedMode {
  LED_OFF,
  LED_ON,
  LED_BLINKING_OFF
};

// LED1 -> topic A -> green
LedMode led1Mode = LED_OFF;
unsigned long led1BlinkStartMs = 0;
unsigned long led1LastToggleMs = 0;
bool led1BlinkVisible = false;

// LED2 -> topic B -> blue
LedMode led2Mode = LED_OFF;
unsigned long led2BlinkStartMs = 0;
unsigned long led2LastToggleMs = 0;
bool led2BlinkVisible = false;

const unsigned long BLINK_DURATION_MS = 3000;
const unsigned long BLINK_INTERVAL_MS = 250;

// ========= FSR State =========
bool currentStateCOn = false;   // true = pressure detected = ON
bool lastSentStateCOn = false;

int pressStableCount = 0;
int releaseStableCount = 0;

unsigned long lastPublishMs = 0;
const unsigned long republishInterval = 5000;

// ===== Helper: set full strip color =====
void setStripColor(Adafruit_NeoPixel &strip, uint32_t color) {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

// ===== Render LED1 based on current mode =====
void renderLed1() {
  if (led1Mode == LED_ON) {
    setStripColor(led1, led1.Color(0, 255, 0));
  }
  else if (led1Mode == LED_OFF) {
    setStripColor(led1, led1.Color(0, 0, 0));
  }
  else if (led1Mode == LED_BLINKING_OFF) {
    if (led1BlinkVisible)
      setStripColor(led1, led1.Color(0, 255, 0));
    else
      setStripColor(led1, led1.Color(0, 0, 0));
  }
}

// ===== Render LED2 based on current mode =====
void renderLed2() {
  if (led2Mode == LED_ON) {
    setStripColor(led2, led2.Color(0, 0, 255));
  }
  else if (led2Mode == LED_OFF) {
    setStripColor(led2, led2.Color(0, 0, 0));
  }
  else if (led2Mode == LED_BLINKING_OFF) {
    if (led2BlinkVisible)
      setStripColor(led2, led2.Color(0, 0, 255));
    else
      setStripColor(led2, led2.Color(0, 0, 0));
  }
}

// ===== Topic A received ON =====
void led1TurnOn() {
  led1Mode = LED_ON;
  led1BlinkVisible = true;
  renderLed1();
}

// ===== Topic A received OFF =====
void led1StartBlinkingOff() {
  led1Mode = LED_BLINKING_OFF;
  led1BlinkStartMs = millis();
  led1LastToggleMs = millis();
  led1BlinkVisible = true;
  renderLed1();
}

// ===== Topic B received ON =====
void led2TurnOn() {
  led2Mode = LED_ON;
  led2BlinkVisible = true;
  renderLed2();
}

// ===== Topic B received OFF =====
void led2StartBlinkingOff() {
  led2Mode = LED_BLINKING_OFF;
  led2BlinkStartMs = millis();
  led2LastToggleMs = millis();
  led2BlinkVisible = true;
  renderLed2();
}

// ===== Update blinking logic in loop =====
void updateBlinkingLEDs() {
  unsigned long now = millis();

  // LED1
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

  // LED2
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

  // Topic A controls LED1
  if (topicStr == sub_topic_A) {
    if (msg == "on") {
      led1TurnOn();
    }
    else if (msg == "off") {
      led1StartBlinkingOff();
    }
  }

  // Topic B controls LED2
  if (topicStr == sub_topic_B) {
    if (msg == "on") {
      led2TurnOn();
    }
    else if (msg == "off") {
      led2StartBlinkingOff();
    }
  }
}

// ===== Connect WiFi =====
void connectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting WiFi...");
    WiFi.begin(ssid, pass);
    delay(2000);
  }

  Serial.println("WiFi connected");
}

// ===== Connect MQTT =====
void connectMQTT() {
  mqttClient.setServer(mqtt_host, mqtt_port);
  mqttClient.setCallback(onMqttMessage);

  while (!mqttClient.connected()) {
    String clientId = "MKR1010-";
    clientId += String((uint32_t)WiFi.localIP(), HEX);

    if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("MQTT connected");
      mqttClient.subscribe(sub_topic_A);
      mqttClient.subscribe(sub_topic_B);
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

// ===== Publish C =====
void publishStateC(bool stateOn) {
  const char* payload = stateOn ? "on" : "off";

  Serial.print("Publish C = ");
  Serial.println(payload);

  mqttClient.publish(pub_topic_C, payload, true);
}

// ===== FSR debounce logic =====
// true  -> pressure detected -> ON
// false -> no pressure       -> OFF
void updateFSRState(int fsrValue) {
  if (!currentStateCOn) {
    // Currently OFF, check whether it should become ON
    if (fsrValue >= PRESS_ON_THRESHOLD) {
      pressStableCount++;
      releaseStableCount = 0;

      if (pressStableCount >= REQUIRED_STABLE_COUNT) {
        currentStateCOn = true;
        pressStableCount = 0;
        releaseStableCount = 0;
      }
    }
    else {
      pressStableCount = 0;
    }
  }
  else {
    // Currently ON, check whether it should become OFF
    if (fsrValue <= RELEASE_OFF_THRESHOLD) {
      releaseStableCount++;
      pressStableCount = 0;

      if (releaseStableCount >= REQUIRED_STABLE_COUNT) {
        currentStateCOn = false;
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
  while (!Serial) {}

  analogReadResolution(10);

  led1.begin();
  led2.begin();

  led1.setBrightness(255);
  led2.setBrightness(255);

  led1.clear();
  led2.clear();
  led1.show();
  led2.show();

  connectWiFi();
  connectMQTT();

  int fsrValue = readFSR();
  int midThreshold = (PRESS_ON_THRESHOLD + RELEASE_OFF_THRESHOLD) / 2;

  currentStateCOn = (fsrValue >= midThreshold);
  lastSentStateCOn = currentStateCOn;

  publishStateC(currentStateCOn);
  lastPublishMs = millis();

  renderLed1();
  renderLed2();
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

  updateFSRState(fsrValue);

  unsigned long now = millis();

  if (currentStateCOn != lastSentStateCOn) {
    publishStateC(currentStateCOn);
    lastSentStateCOn = currentStateCOn;
    lastPublishMs = now;
  }

  if (now - lastPublishMs >= republishInterval) {
    publishStateC(currentStateCOn);
    lastPublishMs = now;
  }

  updateBlinkingLEDs();

  delay(30);
}