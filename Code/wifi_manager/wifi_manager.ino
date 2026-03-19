#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <FlashStorage.h>

// =========================
// User-visible settings
// =========================
const char* AP_SSID = "MUJI1234";
const char* AP_PASS = "12345678";   // 至少8位

const char* mqtt_host = "mqtt.cetools.org";
const int   mqtt_port = 1884;
const char* mqtt_user = "student";
const char* mqtt_pass = "ce2021-mqtt-forget-whale";
const char* pub_topic_C = "student/MUJI/hzh/C";

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
unsigned long lastPublishMs = 0;
bool configJustSaved = false;

// =========================
// Helpers
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

  while (millis() - start < 8000) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected.");
      printWiFiStatus();
      return true;
    }
    delay(200);
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
      delay(1000);
    }
  }

  delay(3000);
  server.begin();

  IPAddress ip = WiFi.localIP();
  Serial.print("AP IP: ");
  Serial.println(ip);
  Serial.println("Use phone to connect AP, then open: http://192.168.4.1");
}

bool connectMQTT() {
  mqttClient.setServer(mqtt_host, mqtt_port);

  if (mqttClient.connected()) return true;

  Serial.print("Connecting MQTT... ");
  bool ok = mqttClient.connect("MRK1010-C", mqtt_user, mqtt_pass);

  if (ok) {
    Serial.println("connected");
  } else {
    Serial.print("failed, state=");
    Serial.println(mqttClient.state());
  }
  return ok;
}

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
  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("New config client");
  String reqLine = "";
  String headerBuffer = "";
  bool firstLineRead = false;

  unsigned long start = millis();
  while (client.connected() && millis() - start < 3000) {
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

          mqttClient.disconnect();
          WiFi.end();
          delay(1500);

          WiFi.begin(ssid.c_str(), pass.c_str());

          unsigned long wifiStart = millis();
          while (millis() - wifiStart < 15000) {
            if (WiFi.status() == WL_CONNECTED) {
              Serial.println("User WiFi connected. Saving config...");
              saveConfig(ssid, pass);
              configJustSaved = true;
              currentMode = MODE_NORMAL;
              printWiFiStatus();
              return;
            }
            delay(250);
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
// Setup / Loop
// =========================
void setup() {
  clearConfig();
  Serial.begin(115200);
  while (!Serial) {}

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

    if (connectToSavedWiFi()) {
      currentMode = MODE_NORMAL;
    } else {
      startConfigPortal();
    }
  } else {
    Serial.println("No saved WiFi config.");
    startConfigPortal();
  }
}

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
      if (!connectToSavedWiFi()) {
        Serial.println("Saved WiFi failed. Reboot and enter config mode.");
        rebootBoard();
        return;
      }
    }

    if (!mqttClient.connected()) {
      connectMQTT();
    }

    mqttClient.loop();

    if (millis() - lastPublishMs > 5000) {
      lastPublishMs = millis();

      String payload = "hello from MRK1010";
      bool ok = mqttClient.publish(pub_topic_C, payload.c_str());

      Serial.print("Publish to ");
      Serial.print(pub_topic_C);
      Serial.print(" -> ");
      Serial.println(ok ? "OK" : "FAILED");
    }
  }
}