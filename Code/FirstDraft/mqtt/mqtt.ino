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
const char* pub_topic_B = "student/MUJI/hzh/B";
const char* sub_topic_A = "student/MUJI/hzh/A";
const char* sub_topic_C = "student/MUJI/hzh/C";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ========= FSR =========
const int FSR_PIN = A6;

const int PRESS_ON_THRESHOLD   = 320;
const int RELEASE_ON_THRESHOLD = 260;

const int REQUIRED_STABLE_COUNT = 3;

const int SAMPLE_COUNT = 10;
const int SAMPLE_DELAY_MS = 3;

// ========= LED =========
#define LED1_PIN 6
#define LED2_PIN 7

Adafruit_NeoPixel led1(8, LED1_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led2(8, LED2_PIN, NEO_GRB + NEO_KHZ800);

// ========= 状态 =========
bool stateAOn = false;
bool stateCOn = false;

// B 的当前状态
bool currentStateBOn = true;
bool lastSentStateBOn = true;

int pressStableCount = 0;
int releaseStableCount = 0;

unsigned long lastPublishMs = 0;
const unsigned long republishInterval = 5000;

// ===== 更新 LED =====
void updateLEDs() {

  // LED1 -> A
  for(int i=0;i<8;i++)
  {
    if (stateAOn)
      led1.setPixelColor(i, led1.Color(0,255,0));
    else
      led1.setPixelColor(i, led1.Color(0,0,0));
  }

  led1.show();

  // LED2 -> C
  for(int i=0;i<8;i++)
  {
    if (stateCOn)
      led2.setPixelColor(i, led2.Color(0,0,255));
    else
      led2.setPixelColor(i, led2.Color(0,0,0));
  }

  led2.show();
}

// ===== MQTT 回调 =====
void onMqttMessage(char* topic, byte* payload, unsigned int length) {

  String topicStr = String(topic);
  String msg = "";

  for (unsigned int i = 0; i < length; i++)
    msg += (char)payload[i];

  msg.trim();

  Serial.print("MQTT ");
  Serial.print(topicStr);
  Serial.print(" = ");
  Serial.println(msg);

  if (topicStr == sub_topic_A) {

    stateAOn = (msg == "on");
    updateLEDs();

  }

  if (topicStr == sub_topic_C) {

    stateCOn = (msg == "on");
    updateLEDs();

  }
}

// ===== WiFi =====
void connectWiFi() {

  while (WiFi.status() != WL_CONNECTED) {

    Serial.print("Connecting WiFi...");
    WiFi.begin(ssid, pass);
    delay(2000);

  }

  Serial.println("WiFi connected");
}

// ===== MQTT =====
void connectMQTT() {

  mqttClient.setServer(mqtt_host, mqtt_port);
  mqttClient.setCallback(onMqttMessage);

  while (!mqttClient.connected()) {

    String clientId = "MKR1010-";
    clientId += String((uint32_t)WiFi.localIP(), HEX);

    if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {

      Serial.println("MQTT connected");

      mqttClient.subscribe(sub_topic_A);
      mqttClient.subscribe(sub_topic_C);

    } else {

      Serial.print("MQTT failed rc=");
      Serial.println(mqttClient.state());
      delay(2000);

    }
  }
}

// ===== FSR 平均采样 =====
int readFSR() {

  long sum = 0;

  for (int i = 0; i < SAMPLE_COUNT; i++) {

    sum += analogRead(FSR_PIN);
    delay(SAMPLE_DELAY_MS);

  }

  return sum / SAMPLE_COUNT;
}

// ===== 发布 B =====
void publishStateB(bool stateOn) {

  const char* payload = stateOn ? "on" : "off";

  Serial.print("Publish B = ");
  Serial.println(payload);

  mqttClient.publish(pub_topic_B, payload, true);
}

// ===== FSR 抗抖动 =====
void updateFSRState(int fsrValue) {

  if (currentStateBOn == true) {

    if (fsrValue >= PRESS_ON_THRESHOLD) {

      pressStableCount++;
      releaseStableCount = 0;

      if (pressStableCount >= REQUIRED_STABLE_COUNT) {

        currentStateBOn = false;
        pressStableCount = 0;
        releaseStableCount = 0;

      }

    } else pressStableCount = 0;

  }
  else {

    if (fsrValue <= RELEASE_ON_THRESHOLD) {

      releaseStableCount++;
      pressStableCount = 0;

      if (releaseStableCount >= REQUIRED_STABLE_COUNT) {

        currentStateBOn = true;
        pressStableCount = 0;
        releaseStableCount = 0;

      }

    } else releaseStableCount = 0;

  }
}

void setup() {

  Serial.begin(115200);
  while (!Serial) { delay(10); }

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

  int midThreshold = (PRESS_ON_THRESHOLD + RELEASE_ON_THRESHOLD)/2;

  currentStateBOn = (fsrValue < midThreshold);
  lastSentStateBOn = currentStateBOn;

  publishStateB(currentStateBOn);

  lastPublishMs = millis();

  updateLEDs();
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

  if (currentStateBOn != lastSentStateBOn) {

    publishStateB(currentStateBOn);
    lastSentStateBOn = currentStateBOn;
    lastPublishMs = now;

  }

  if (now - lastPublishMs >= republishInterval) {

    publishStateB(currentStateBOn);
    lastPublishMs = now;

  }

  delay(80);
}