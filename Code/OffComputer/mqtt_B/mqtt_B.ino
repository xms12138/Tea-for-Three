#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

// ========= Project =========
const char* mqtt_client_id = "tea-for-three-B";

// ========= WiFi =========
char ssid[] = "CE-Hub-Student";
char pass[] = "casa-ce-gagarin-public-service";

// ========= MQTT =========
const char* mqtt_host = "mqtt.cetools.org";
const int   mqtt_port = 1884;
const char* mqtt_user = "student";
const char* mqtt_pass = "ce2021-mqtt-forget-whale";

// ========= Topics =========
const char* pub_topic_B = "student/MUJI/hzh/B";
const char* sub_topic_A = "student/MUJI/hzh/A";
const char* sub_topic_C = "student/MUJI/hzh/C";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ========= FSR =========
const int FSR_PIN = A6;

// Pressure detected  -> ON
// No pressure        -> OFF
const int PRESS_ON_THRESHOLD    = 50;
const int RELEASE_OFF_THRESHOLD = 35;

const int REQUIRED_STABLE_COUNT = 3;

const int SAMPLE_COUNT = 10;
const int SAMPLE_DELAY_MS = 3;

// ========= Original LEDs =========
#define LED1_PIN 6
#define LED2_PIN 7
#define LED_COUNT 8

Adafruit_NeoPixel led1(LED_COUNT, LED1_PIN, NEO_GRB + NEO_KHZ800);  // controlled by topic A
Adafruit_NeoPixel led2(LED_COUNT, LED2_PIN, NEO_GRB + NEO_KHZ800);  // controlled by topic C

// ========= New B Strip =========
#define LED3_PIN 8
#define LED3_COUNT 10

Adafruit_NeoPixel led3(LED3_COUNT, LED3_PIN, NEO_GRB + NEO_KHZ800); // B board special strip

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

// LED2 -> topic C -> blue
LedMode led2Mode = LED_OFF;
unsigned long led2BlinkStartMs = 0;
unsigned long led2LastToggleMs = 0;
bool led2BlinkVisible = false;

const unsigned long BLINK_DURATION_MS = 3000;
const unsigned long BLINK_INTERVAL_MS = 250;

// ========= Topic states =========
bool stateAOn = false;
bool stateCOn = false;

// ========= B strip state =========
enum BStripMode {
  BSTRIP_OFF,
  BSTRIP_FLASHING,
  BSTRIP_SOLID
};

BStripMode bStripMode = BSTRIP_OFF;

unsigned long bFlashStartMs = 0;
unsigned long bLastToggleMs = 0;
bool bFlashVisible = false;

const int B_FLASH_COUNT = 2;
const unsigned long B_FLASH_INTERVAL_MS = 180;

// ========= FSR State =========
bool currentStateBOn = false;
bool lastSentStateBOn = false;

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
// ===== Warm tea ambience effect for LED3 =====
// ===== Rich warm tea ambience effect for LED3 =====
uint32_t warmTeaColor(int breathPhase, int colorPhase) {
  breathPhase &= 255;
  colorPhase  &= 255;

  // ===== 1. 强呼吸（幅度更大）=====
  int breath;
  if (breathPhase < 128) {
    breath = 50 + (breathPhase * 205) / 127;   // 50 -> 255
  } else {
    breath = 50 + ((255 - breathPhase) * 205) / 127; // 255 -> 50
  }

  // ===== 2. 红-琥珀-橙 茶色渐变 =====
  int rBase, gBase, bBase;

  if (colorPhase < 85) {
    // 深红 → 琥珀
    int t = colorPhase;
    rBase = 255;
    gBase = 60  + (t * 80) / 84;   // 60 → 140
    bBase = 5   + (t * 10) / 84;   // 5  → 15（几乎无蓝）
  } 
  else if (colorPhase < 170) {
    // 琥珀 → 暖橙
    int t = colorPhase - 85;
    rBase = 255;
    gBase = 140 + (t * 40) / 84;   // 140 → 180
    bBase = 15  + (t * 10) / 84;   // 15 → 25
  } 
  else {
    // 暖橙 → 深红（回环）
    int t = colorPhase - 170;
    rBase = 255;
    gBase = 180 - (t * 120) / 85;  // 180 → 60
    bBase = 25  - (t * 20) / 85;   // 25 → 5
  }

  // ===== 3. 呼吸亮度叠加 =====
  int r = (rBase * breath) / 255;
  int g = (gBase * breath) / 255;
  int b = (bBase * breath) / 255;

  return led3.Color(r, g, b);
}

void renderWarmTeaStrip3(int offset) {
  for (int i = 0; i < LED3_COUNT; i++) {
    // colorPhase 决定每颗灯颜色略有差异，形成流动感
    int colorPhase = offset + i * 20;

    // breathPhase 让整条灯带一起呼吸，但稍微有一点空间差
    int breathPhase = offset * 2 + i * 6;

    led3.setPixelColor(i, warmTeaColor(breathPhase, colorPhase));
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
    setStripColor(led2, LED_COUNT, led2.Color(250, 50, 0));
  }
  else if (led2Mode == LED_OFF) {
    setStripColor(led2, LED_COUNT, led2.Color(0, 0, 0));
  }
  else if (led2Mode == LED_BLINKING_OFF) {
    if (led2BlinkVisible)
      setStripColor(led2, LED_COUNT, led2.Color(250, 50, 0));
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

// ===== B strip control =====
void bStripOff() {
  bStripMode = BSTRIP_OFF;
  setStripColor(led3, LED3_COUNT, led3.Color(0, 0, 0));
}

void bStripStartFlashing() {
  bStripMode = BSTRIP_FLASHING;
  bFlashStartMs = millis();
  bLastToggleMs = millis();
  bFlashVisible = true;
  setStripColor(led3, LED3_COUNT, led3.Color(255, 255, 255));
}

void bStripStartSolid() {
  bStripMode = BSTRIP_SOLID;
}

void updateBStripState() {
  if (!currentStateBOn) {
    bStripOff();
    return;
  }

  // 只有 A、B、C 都 ON 时，进入常亮动态效果
  if (stateAOn && stateCOn) {
    if (bStripMode != BSTRIP_SOLID) {
      bStripStartSolid();
    }
    return;
  }

  if (bStripMode == BSTRIP_SOLID) {
    bStripOff();
  }
}

void updateBStripEffect() {
  static int flowOffset = 0;
  unsigned long now = millis();

  if (bStripMode == BSTRIP_FLASHING) {
    if (now - bLastToggleMs >= B_FLASH_INTERVAL_MS) {
      bFlashVisible = !bFlashVisible;
      bLastToggleMs = now;

      if (bFlashVisible) {
        setStripColor(led3, LED3_COUNT, led3.Color(255, 255, 255));
      } else {
        setStripColor(led3, LED3_COUNT, led3.Color(0, 0, 0));
      }
    }

    
    if (now - bFlashStartMs >= (unsigned long)(B_FLASH_COUNT * 2 * B_FLASH_INTERVAL_MS)) {
      if (stateAOn && stateCOn && currentStateBOn) {
        bStripStartSolid();
      } else {
        bStripOff();
      }
    }
  }
  else if (bStripMode == BSTRIP_SOLID) {
    if (stateAOn && stateCOn && currentStateBOn) {
      renderWarmTeaStrip3(flowOffset);
      flowOffset = (flowOffset + 2) & 255;
    } else {
      bStripOff();
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

  // Topic C controls LED2
  if (topicStr == sub_topic_C) {
    bool oldStateCOn = stateCOn;

    if (msg == "on") {
      stateCOn = true;

      if (!oldStateCOn) {
        led2TurnOn();
        stateChanged = true;
      }
    }
    else if (msg == "off") {
      stateCOn = false;

      if (oldStateCOn) {
        if (led2Mode == LED_ON) {
          led2StartBlinkingOff();
        }
        stateChanged = true;
      }
    }
  }

  // 只有 A/C 状态真的变化时，才更新 B 灯带状态
  if (stateChanged) {
    updateBStripState();
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

// ===== Publish B =====
void publishStateB(bool stateOn) {
  const char* payload = stateOn ? "on" : "off";

  Serial.print("Publish B = ");
  Serial.println(payload);

  mqttClient.publish(pub_topic_B, payload, true);
}

// ===== FSR debounce logic =====
void updateFSRState(int fsrValue) {
  if (!currentStateBOn) {
    if (fsrValue >= PRESS_ON_THRESHOLD) {
      pressStableCount++;
      releaseStableCount = 0;

      if (pressStableCount >= REQUIRED_STABLE_COUNT) {
        currentStateBOn = true;
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
        currentStateBOn = false;
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
  led3.setBrightness(180);

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

  currentStateBOn = (fsrValue >= midThreshold);
  lastSentStateBOn = currentStateBOn;

  publishStateB(currentStateBOn);
  lastPublishMs = millis();

  renderLed1();
  renderLed2();
  updateBStripState();
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

  bool prevStateBOn = currentStateBOn;
  updateFSRState(fsrValue);

  unsigned long now = millis();

  if (currentStateBOn != lastSentStateBOn) {
    publishStateB(currentStateBOn);
    lastSentStateBOn = currentStateBOn;
    lastPublishMs = now;

    if (!prevStateBOn && currentStateBOn) {
      if (stateAOn && stateCOn) {
        bStripStartSolid();
      } else {
        bStripStartFlashing();
      }
    }

    if (prevStateBOn && !currentStateBOn) {
      bStripOff();
    }
  }

  if (now - lastPublishMs >= republishInterval) {
    publishStateB(currentStateBOn);
    lastPublishMs = now;
  }

  updateBlinkingLEDs();
  updateBStripEffect();

  delay(10);
}