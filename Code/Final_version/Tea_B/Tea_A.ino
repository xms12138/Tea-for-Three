#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <FlashStorage.h>
#include <Adafruit_NeoPixel.h>

// =========================
// AP Provisioning Settings
// =========================
const char* AP_SSID = "MUJI1234";
const char* AP_PASS = "12345678";   

// =========================
// Project / MQTT
// =========================
const char* mqtt_client_id = "tea-for-three-A";

const char* mqtt_host = "mqtt.cetools.org";
const int   mqtt_port = 1884;
const char* mqtt_user = "student";
const char* mqtt_pass = "ce2021-mqtt-forget-whale";

// Topics
const char* pub_topic_A = "student/MUJI/hzh/A";
const char* sub_topic_B = "student/MUJI/hzh/B";
const char* sub_topic_C = "student/MUJI/hzh/C";

// =========================
// Flash storage structure
// =========================
typedef struct {
  char marker[8];      // "MUJIWF"
  char ssid[33];       // 32 + '\0'
  char pass[65];       // 64 + '\0'
} WifiConfig;

FlashStorage(config_store, WifiConfig);

// =========================
// Globals
// =========================
WiFiServer server(80);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

enum DeviceMode {
  MODE_CONFIG,
  MODE_NORMAL
};

DeviceMode currentMode = MODE_CONFIG;

WifiConfig cfg;
bool configJustSaved = false;

// =========================
// FSR
// =========================
const int FSR_PIN = A6;

// Pressure detected  -> ON
// No pressure        -> OFF
const int PRESS_ON_THRESHOLD    = 50;
const int RELEASE_OFF_THRESHOLD = 35;

const int REQUIRED_STABLE_COUNT = 3;

const int SAMPLE_COUNT = 10;
const int SAMPLE_DELAY_MS = 3;

// =========================
// LED strips
// =========================
#define LED1_PIN 6
#define LED2_PIN 7
#define LED_COUNT 8

Adafruit_NeoPixel led1(LED_COUNT, LED1_PIN, NEO_GRB + NEO_KHZ800);  // controlled by topic B
Adafruit_NeoPixel led2(LED_COUNT, LED2_PIN, NEO_GRB + NEO_KHZ800);  // controlled by topic C

#define LED3_PIN 8
#define LED3_COUNT 10
Adafruit_NeoPixel led3(LED3_COUNT, LED3_PIN, NEO_GRB + NEO_KHZ800); // A board special strip

// =========================
// LED state machine
// =========================
enum LedMode {
  LED_OFF,
  LED_ON,
  LED_BLINKING_OFF
};

// LED1 -> topic B
LedMode led1Mode = LED_OFF;
unsigned long led1BlinkStartMs = 0;
unsigned long led1LastToggleMs = 0;
bool led1BlinkVisible = false;

// LED2 -> topic C
LedMode led2Mode = LED_OFF;
unsigned long led2BlinkStartMs = 0;
unsigned long led2LastToggleMs = 0;
bool led2BlinkVisible = false;

const unsigned long BLINK_DURATION_MS = 3000;
const unsigned long BLINK_INTERVAL_MS = 250;

// =========================
// Remote topic states
// =========================
bool stateBOn = false;
bool stateCOn = false;

// =========================
// A strip state
// =========================
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

// =========================
// FSR state
// =========================
bool currentStateAOn = false;
bool lastSentStateAOn = false;

int pressStableCount = 0;
int releaseStableCount = 0;

unsigned long lastPublishMs = 0;
const unsigned long republishInterval = 5000;

// =========================
// Provisioning LED3 status
// =========================
enum ProvisionLedMode {
  PROV_LED_NONE,
  PROV_LED_BLINK_RED
};

ProvisionLedMode provisionLedMode = PROV_LED_NONE;
unsigned long provisionLedLastToggleMs = 0;
bool provisionLedVisible = false;
const unsigned long PROV_BLINK_INTERVAL_MS = 250;

// =========================
// Helper: set full strip color
// =========================
void setStripColor(Adafruit_NeoPixel &strip, int count, uint32_t color) {
  for (int i = 0; i < count; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

// =========================
// LED3 provisioning status
// =========================
void startProvisionBlinkRed() {
  provisionLedMode = PROV_LED_BLINK_RED;
  provisionLedLastToggleMs = millis();
  provisionLedVisible = true;
  setStripColor(led3, LED3_COUNT, led3.Color(255, 0, 0));
}

void stopProvisionLed() {
  provisionLedMode = PROV_LED_NONE;
  provisionLedVisible = false;
  setStripColor(led3, LED3_COUNT, led3.Color(0, 0, 0));
}

void updateProvisionLed() {
  if (provisionLedMode != PROV_LED_BLINK_RED) return;

  unsigned long now = millis();
  if (now - provisionLedLastToggleMs >= PROV_BLINK_INTERVAL_MS) {
    provisionLedLastToggleMs = now;
    provisionLedVisible = !provisionLedVisible;

    if (provisionLedVisible) {
      setStripColor(led3, LED3_COUNT, led3.Color(255, 0, 0));
    } else {
      setStripColor(led3, LED3_COUNT, led3.Color(0, 0, 0));
    }
  }
}

void flashProvisionSuccessGreen() {
  for (int i = 0; i < 3; i++) {
    setStripColor(led3, LED3_COUNT, led3.Color(0, 255, 0));
    delay(250);
    setStripColor(led3, LED3_COUNT, led3.Color(0, 0, 0));
    delay(150);
  }
}

// =========================
// Flash helpers
// =========================
void clearConfig() {
  WifiConfig emptyCfg;
  memset(&emptyCfg, 0, sizeof(emptyCfg));
  config_store.write(emptyCfg);
}

bool hasSavedConfig() {
  cfg = config_store.read();

  if (strncmp(cfg.marker, "MUJIWF", 6) != 0) return false;
  if (strlen(cfg.ssid) == 0) return false;

  return true;
}

void saveConfig(const String& ssid, const String& pass) {
  WifiConfig newCfg;
  memset(&newCfg, 0, sizeof(newCfg));

  strncpy(newCfg.marker, "MUJIWF", sizeof(newCfg.marker) - 1);
  strncpy(newCfg.ssid, ssid.c_str(), sizeof(newCfg.ssid) - 1);
  strncpy(newCfg.pass, pass.c_str(), sizeof(newCfg.pass) - 1);

  config_store.write(newCfg);
  cfg = newCfg;
}

void rebootBoard() {
  Serial.println("Rebooting board...");
  delay(1000);
  NVIC_SystemReset();
}

// =========================
// Web helpers
// =========================
String htmlEscape(const String& s) {
  String out = s;
  out.replace("&", "&amp;");
  out.replace("<", "&lt;");
  out.replace(">", "&gt;");
  out.replace("\"", "&quot;");
  return out;
}

String urlDecode(const String& input) {
  String out = "";
  for (unsigned int i = 0; i < input.length(); i++) {
    char c = input[i];
    if (c == '+') {
      out += ' ';
    } else if (c == '%' && i + 2 < input.length()) {
      char h1 = input[i + 1];
      char h2 = input[i + 2];
      char hex[3] = { h1, h2, '\0' };
      out += (char) strtol(hex, NULL, 16);
      i += 2;
    } else {
      out += c;
    }
  }
  return out;
}

String getQueryParam(const String& reqLine, const String& key) {
  int qMark = reqLine.indexOf('?');
  if (qMark < 0) return "";

  int httpPos = reqLine.indexOf(" HTTP/");
  if (httpPos < 0) return "";

  String query = reqLine.substring(qMark + 1, httpPos);

  String pattern = key + "=";
  int start = query.indexOf(pattern);
  if (start < 0) return "";

  start += pattern.length();
  int end = query.indexOf('&', start);
  if (end < 0) end = query.length();

  return urlDecode(query.substring(start, end));
}

// =========================
// WiFi helpers
// =========================
void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
}

bool connectToSavedWiFi() {
  Serial.println("Trying saved WiFi (one attempt)...");

  if (strlen(cfg.ssid) == 0) {
    Serial.println("No saved SSID.");
    return false;
  }

  mqttClient.disconnect();

  WiFi.end();
  delay(1500);

  WiFi.begin(cfg.ssid, cfg.pass);

  unsigned long start = millis();

  while (millis() - start < 10000) {
    updateProvisionLed();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected.");
      printWiFiStatus();
      return true;
    }
    delay(50);
  }

  Serial.println("WiFi connect failed (single attempt).");
  return false;
}

void startConfigPortal() {
  currentMode = MODE_CONFIG;

  Serial.println("\n=== CONFIG MODE ===");
  Serial.print("Starting AP: ");
  Serial.println(AP_SSID);

  mqttClient.disconnect();

  WiFi.end();
  delay(1500);

  int status = WiFi.beginAP(AP_SSID, AP_PASS);
  if (status != WL_AP_LISTENING) {
    Serial.println("Failed to start AP.");
    while (true) {
      startProvisionBlinkRed();
      updateProvisionLed();
      delay(20);
    }
  }

  delay(3000);
  server.begin();

  IPAddress ip = WiFi.localIP();
  Serial.print("AP IP: ");
  Serial.println(ip);
  Serial.println("Use phone to connect AP, then open: http://192.168.4.1");

  startProvisionBlinkRed();
}

// =========================
// MQTT
// =========================
bool connectMQTT() {
  mqttClient.setServer(mqtt_host, mqtt_port);
  mqttClient.setCallback(onMqttMessage);

  if (mqttClient.connected()) return true;

  Serial.print("Connecting MQTT... ");
  bool ok = mqttClient.connect(mqtt_client_id, mqtt_user, mqtt_pass);

  if (ok) {
    Serial.println("connected");
    mqttClient.subscribe(sub_topic_B);
    mqttClient.subscribe(sub_topic_C);
  } else {
    Serial.print("failed, state=");
    Serial.println(mqttClient.state());
  }
  return ok;
}

// =========================
// Web page
// =========================
void sendHttpHeader(WiFiClient& client, const char* contentType = "text/html") {
  client.println("HTTP/1.1 200 OK");
  client.print("Content-Type: ");
  client.println(contentType);
  client.println("Connection: close");
  client.println();
}

void sendConfigPage(WiFiClient& client, const String& msg = "") {
  sendHttpHeader(client);

  client.println("<!DOCTYPE html><html><head><meta charset='utf-8'>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
  client.println("<title>MKR1010 WiFi Setup</title>");
  client.println("<style>");
  client.println("body{font-family:Arial,sans-serif;max-width:420px;margin:30px auto;padding:0 16px;}");
  client.println("input{width:100%;padding:12px;margin:8px 0;box-sizing:border-box;}");
  client.println("button{width:100%;padding:12px;margin-top:10px;}");
  client.println(".box{border:1px solid #ccc;border-radius:10px;padding:16px;}");
  client.println(".msg{margin-bottom:12px;padding:10px;border-radius:8px;background:#f3f3f3;}");
  client.println("</style></head><body>");
  client.println("<div class='box'>");
  client.println("<h2>MKR1010 WiFi Setup</h2>");

  if (msg.length() > 0) {
    client.print("<div class='msg'>");
    client.print(htmlEscape(msg));
    client.println("</div>");
  }

  client.println("<form action='/save' method='GET'>");
  client.println("<label>WiFi SSID</label>");
  client.println("<input name='ssid' maxlength='32' required>");
  client.println("<label>WiFi Password</label>");
  client.println("<input name='pass' type='password' maxlength='64'>");
  client.println("<button type='submit'>Save and Connect</button>");
  client.println("</form>");

  client.println("<p style='margin-top:16px;font-size:14px;'>");
  client.println("After success, the device will remember this WiFi and auto-connect next time.");
  client.println("</p>");

  client.println("</div></body></html>");
}

void handleConfigPortal() {
  updateProvisionLed();

  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("New config client");
  String reqLine = "";
  String headerBuffer = "";
  bool firstLineRead = false;

  unsigned long start = millis();
  while (client.connected() && millis() - start < 3000) {
    updateProvisionLed();

    while (client.available()) {
      char c = client.read();

      if (!firstLineRead) {
        if (c == '\n') {
          firstLineRead = true;
          reqLine.trim();
          Serial.print("Request: ");
          Serial.println(reqLine);
        } else if (c != '\r') {
          reqLine += c;
        }
      }

      headerBuffer += c;

      if (headerBuffer.endsWith("\r\n\r\n")) {
        if (reqLine.startsWith("GET /save?")) {
          String ssid = getQueryParam(reqLine, "ssid");
          String pass = getQueryParam(reqLine, "pass");

          if (ssid.length() == 0) {
            sendConfigPage(client, "SSID cannot be empty.");
            client.stop();
            Serial.println("Config client done");
            return;
          }

          sendHttpHeader(client);
          client.println("<!DOCTYPE html><html><body style='font-family:Arial;padding:24px;'>");
          client.println("<h2>Trying to connect...</h2>");
          client.print("<p>SSID: ");
          client.print(htmlEscape(ssid));
          client.println("</p>");
          client.println("<p>Please wait. The device may disconnect from this AP if connection succeeds.</p>");
          client.println("</body></html>");
          delay(200);
          client.stop();

          Serial.print("Received SSID: ");
          Serial.println(ssid);
          Serial.println("Trying user WiFi...");

          startProvisionBlinkRed();

          mqttClient.disconnect();
          WiFi.end();
          delay(1500);

          WiFi.begin(ssid.c_str(), pass.c_str());

          unsigned long wifiStart = millis();
          while (millis() - wifiStart < 15000) {
            updateProvisionLed();

            if (WiFi.status() == WL_CONNECTED) {
              Serial.println("User WiFi connected. Saving config...");
              saveConfig(ssid, pass);
              configJustSaved = true;
              currentMode = MODE_NORMAL;

              stopProvisionLed();
              flashProvisionSuccessGreen();

              printWiFiStatus();
              return;
            }
            delay(50);
          }

          Serial.println("Entered WiFi connect failed, back to AP mode.");
          rebootBoard();
          return;
        } else {
          sendConfigPage(client, "Connect your device to home WiFi.");
          client.stop();
          Serial.println("Config client done");
          return;
        }
      }
    }
  }

  client.stop();
}

// =========================
// Product helper: LED3 warm tea effect
// =========================
uint32_t warmTeaColor(int breathPhase, int colorPhase) {
  breathPhase &= 255;
  colorPhase  &= 255;

  int breath;
  if (breathPhase < 128) {
    breath = 50 + (breathPhase * 205) / 127;   // 50 -> 255
  } else {
    breath = 50 + ((255 - breathPhase) * 205) / 127; // 255 -> 50
  }

  int rBase, gBase, bBase;

  if (colorPhase < 85) {
    int t = colorPhase;
    rBase = 255;
    gBase = 60  + (t * 80) / 84;   // 60 -> 140
    bBase = 5   + (t * 10) / 84;   // 5 -> 15
  } 
  else if (colorPhase < 170) {
    int t = colorPhase - 85;
    rBase = 255;
    gBase = 140 + (t * 40) / 84;   // 140 -> 180
    bBase = 15  + (t * 10) / 84;   // 15 -> 25
  } 
  else {
    int t = colorPhase - 170;
    rBase = 255;
    gBase = 180 - (t * 120) / 85;  // 180 -> 60
    bBase = 25  - (t * 20) / 85;   // 25 -> 5
  }

  int r = (rBase * breath) / 255;
  int g = (gBase * breath) / 255;
  int b = (bBase * breath) / 255;

  return led3.Color(r, g, b);
}

void renderWarmTeaStrip3(int offset) {
  for (int i = 0; i < LED3_COUNT; i++) {
    int colorPhase = offset + i * 20;
    int breathPhase = offset * 2 + i * 6;
    led3.setPixelColor(i, warmTeaColor(breathPhase, colorPhase));
  }
  led3.show();
}

// =========================
// Product helper: LED1 / LED2 rendering
// =========================
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

// =========================
// Product helper: topic-triggered LED logic
// =========================
void led1TurnOn() {
  led1Mode = LED_ON;
  led1BlinkVisible = true;
  renderLed1();
}

void led1StartBlinkingOff() {
  led1Mode = LED_BLINKING_OFF;
  led1BlinkStartMs = millis();
  led1LastToggleMs = millis();
  led1BlinkVisible = true;
  renderLed1();
}

void led2TurnOn() {
  led2Mode = LED_ON;
  led2BlinkVisible = true;
  renderLed2();
}

void led2StartBlinkingOff() {
  led2Mode = LED_BLINKING_OFF;
  led2BlinkStartMs = millis();
  led2LastToggleMs = millis();
  led2BlinkVisible = true;
  renderLed2();
}

// =========================
// Product helper: A strip control
// =========================
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

  if (stateBOn && stateCOn) {
    if (aStripMode != ASTRIP_SOLID) {
      aStripStartSolid();
    }
    return;
  }

  if (aStripMode == ASTRIP_SOLID) {
    aStripOff();
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
      renderWarmTeaStrip3(flowOffset);
      flowOffset = (flowOffset + 2) & 255;
    } else {
      aStripOff();
    }
  }
}

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

// =========================
// MQTT callback
// =========================
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

  if (topicStr == sub_topic_B) {
    bool oldStateBOn = stateBOn;

    if (msg == "on") {
      stateBOn = true;

      if (!oldStateBOn) {
        led1TurnOn();
        stateChanged = true;
      }
    }
    else if (msg == "off") {
      stateBOn = false;

      if (oldStateBOn) {
        if (led1Mode == LED_ON) {
          led1StartBlinkingOff();
        }
        stateChanged = true;
      }
    }
  }

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

  if (stateChanged) {
    updateAStripState();
  }
}

// =========================
// FSR helpers
// =========================
int readFSR() {
  long sum = 0;

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sum += analogRead(FSR_PIN);
    delay(SAMPLE_DELAY_MS);
  }

  return sum / SAMPLE_COUNT;
}

void publishStateA(bool stateOn) {
  const char* payload = stateOn ? "on" : "off";

  Serial.print("Publish A = ");
  Serial.println(payload);

  mqttClient.publish(pub_topic_A, payload, true);
}

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

// =========================
// Setup
// =========================
void setup() {
  clearConfig();
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

  Serial.println("\nBooting MRK1010...");

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi module not found.");
    while (true) delay(1000);
  }

  String fv = WiFi.firmwareVersion();
  Serial.print("NINA firmware: ");
  Serial.println(fv);

  if (hasSavedConfig()) {
    Serial.print("Saved SSID found: ");
    Serial.println(cfg.ssid);

    startProvisionBlinkRed();

    if (connectToSavedWiFi()) {
      stopProvisionLed();
      flashProvisionSuccessGreen();
      currentMode = MODE_NORMAL;
    } else {
      startConfigPortal();
      return;
    }
  } else {
    Serial.println("No saved WiFi config.");
    startConfigPortal();
    return;
  }

  if (!connectMQTT()) {
    Serial.println("Initial MQTT connect failed.");
  }

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

// =========================
// Loop
// =========================
void loop() {
  if (currentMode == MODE_CONFIG) {
    handleConfigPortal();
    return;
  }

  if (currentMode == MODE_NORMAL) {
    if (configJustSaved) {
      Serial.println("Entered NORMAL mode after successful provisioning.");
      configJustSaved = false;
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi lost. Trying saved WiFi...");
      startProvisionBlinkRed();

      if (!connectToSavedWiFi()) {
        Serial.println("Saved WiFi failed. Reboot and enter config mode.");
        rebootBoard();
        return;
      }

      stopProvisionLed();
      flashProvisionSuccessGreen();
    }

    if (!mqttClient.connected()) {
      connectMQTT();
    }

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
}