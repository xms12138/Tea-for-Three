#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

// ========= Project =========
const char* mqtt_client_id = "tea-for-three-C";

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

// ========= Original LEDs =========
#define LED1_PIN 6
#define LED2_PIN 7
#define LED_COUNT 8

Adafruit_NeoPixel led1(LED_COUNT, LED1_PIN, NEO_GRB + NEO_KHZ800);  // controlled by topic A
Adafruit_NeoPixel led2(LED_COUNT, LED2_PIN, NEO_GRB + NEO_KHZ800);  // controlled by topic B

// ========= New C Strip =========
#define LED3_PIN 8
#define LED3_COUNT 10

Adafruit_NeoPixel led3(LED3_COUNT, LED3_PIN, NEO_GRB + NEO_KHZ800); // C board special strip

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

// ========= Topic states =========
bool stateAOn = false;
bool stateBOn = false;

// ========= C strip state =========
enum CStripMode {
  CSTRIP_OFF,
  CSTRIP_FLASHING,
  CSTRIP_SOLID
};

CStripMode cStripMode = CSTRIP_OFF;

unsigned long cFlashStartMs = 0;
unsigned long cLastToggleMs = 0;
bool cFlashVisible = false;

const int C_FLASH_COUNT = 2;                   
const unsigned long C_FLASH_INTERVAL_MS = 180; 

// ========= FSR State =========
bool currentStateCOn = false;
bool lastSentStateCOn = false;

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

// ===== Fancy effect for LED3 =====
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
    setStripColor(led1, LED_COUNT, led1.Color(50, 250, 50));
  }
  else if (led1Mode == LED_OFF) {
    setStripColor(led1, LED_COUNT, led1.Color(0, 0, 0));
  }
  else if (led1Mode == LED_BLINKING_OFF) {
    if (led1BlinkVisible)
      setStripColor(led1, LED_COUNT, led1.Color(50, 250, 50));
    else
      setStripColor(led1, LED_COUNT, led1.Color(0, 0, 0));
  }
}

// ===== Render LED2 based on current mode =====
void renderLed2() {
  if (led2Mode == LED_ON) {
    setStripColor(led2, LED_COUNT, led2.Color(255, 50, 0));
  }
  else if (led2Mode == LED_OFF) {
    setStripColor(led2, LED_COUNT, led2.Color(0, 0, 0));
  }
  else if (led2Mode == LED_BLINKING_OFF) {
    if (led2BlinkVisible)
      setStripColor(led2, LED_COUNT, led2.Color(255, 50, 0));
    else
      setStripColor(led2, LED_COUNT, led2.Color(0, 0, 0));
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

// ===== C strip control =====
void cStripOff() {
  cStripMode = CSTRIP_OFF;
  setStripColor(led3, LED3_COUNT, led3.Color(0, 0, 0));
}

void cStripStartFlashing() {
  cStripMode = CSTRIP_FLASHING;
  cFlashStartMs = millis();
  cLastToggleMs = millis();
  cFlashVisible = true;
  setStripColor(led3, LED3_COUNT, led3.Color(255, 255, 255));
}

void cStripStartSolid() {
  cStripMode = CSTRIP_SOLID;
}

void updateCStripState() {
  if (!currentStateCOn) {
    cStripOff();
    return;
  }

  // 只有 A、B、C 都 ON 时，进入常亮动态效果
  if (stateAOn && stateBOn) {
    if (cStripMode != CSTRIP_SOLID) {
      cStripStartSolid();
    }
    return;
  }

  // 条件不满足时，不要因为 A/B 状态变化而重新闪两下
  // 如果之前是常亮，就关掉
  if (cStripMode == CSTRIP_SOLID) {
    cStripOff();
  }
}

void updateCStripEffect() {
  static int flowOffset = 0;
  unsigned long now = millis();

  if (cStripMode == CSTRIP_FLASHING) {
    if (now - cLastToggleMs >= C_FLASH_INTERVAL_MS) {
      cFlashVisible = !cFlashVisible;
      cLastToggleMs = now;

      if (cFlashVisible) {
        setStripColor(led3, LED3_COUNT, led3.Color(255, 255, 255));
      } else {
        setStripColor(led3, LED3_COUNT, led3.Color(0, 0, 0));
      }
    }

    // 亮灭亮灭 = 4个间隔
    if (now - cFlashStartMs >= (unsigned long)(C_FLASH_COUNT * 2 * C_FLASH_INTERVAL_MS)) {
      if (stateAOn && stateBOn && currentStateCOn) {
        cStripStartSolid();
      } else {
        cStripOff();
      }
    }
  }
  else if (cStripMode == CSTRIP_SOLID) {
    
    if (stateAOn && stateBOn && currentStateCOn) {
      renderFancyStrip3(flowOffset);
      flowOffset = (flowOffset + 20) & 255;
    } else {
      cStripOff();
    }
  }
}

// ===== Update blinking logic in loop =====
void updateBlinkingLEDs() {
  unsigned long now = millis();

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

  bool stateChanged = false;

  // Topic A controls LED1
  if (topicStr == sub_topic_A) {
    bool oldStateAOn = stateAOn;

    if (msg == "on") {
      stateAOn = true;

      if (!oldStateAOn) {
        led1TurnOn();
        stateChanged = true;
      }
    }
    else if (msg == "off") {
      stateAOn = false;

      if (oldStateAOn) {
        if (led1Mode == LED_ON) {
          led1StartBlinkingOff();
        }
        stateChanged = true;
      }
    }
  }

  // Topic B controls LED2
  if (topicStr == sub_topic_B) {
    bool oldStateBOn = stateBOn;

    if (msg == "on") {
      stateBOn = true;

      if (!oldStateBOn) {
        led2TurnOn();
        stateChanged = true;
      }
    }
    else if (msg == "off") {
      stateBOn = false;

      if (oldStateBOn) {
        if (led2Mode == LED_ON) {
          led2StartBlinkingOff();
        }
        stateChanged = true;
      }
    }
  }

  // 只有 A/B 状态真的变化时，才更新 C strip
  if (stateChanged) {
    Serial.println("A/B state changed -> updateCStripState()");
    updateCStripState();
  } else {
    Serial.println("Repeated MQTT message ignored for C strip");
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
    if (mqttClient.connect(mqtt_client_id, mqtt_user, mqtt_pass)) {
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
void updateFSRState(int fsrValue) {
  if (!currentStateCOn) {
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

  currentStateCOn = (fsrValue >= midThreshold);
  lastSentStateCOn = currentStateCOn;

  publishStateC(currentStateCOn);
  lastPublishMs = millis();

  renderLed1();
  renderLed2();
  updateCStripState();
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

  bool prevStateCOn = currentStateCOn;
  updateFSRState(fsrValue);

  unsigned long now = millis();

  if (currentStateCOn != lastSentStateCOn) {
    publishStateC(currentStateCOn);
    lastSentStateCOn = currentStateCOn;
    lastPublishMs = now;

    if (!prevStateCOn && currentStateCOn) {
      if (stateAOn && stateBOn) {
        cStripStartSolid();
      } else {
        cStripStartFlashing();
      }
    }

    if (prevStateCOn && !currentStateCOn) {
      cStripOff();
    }
  }

  if (now - lastPublishMs >= republishInterval) {
    publishStateC(currentStateCOn);
    lastPublishMs = now;
  }

  updateBlinkingLEDs();
  updateCStripEffect();

  delay(10);
}