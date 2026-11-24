#include <M5Unified.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ============================================================
// MATRIX HOME SOC - M5Stack Core2
// FULL REBUILD / 8 PAGES / DOUBLE BUFFER
// THREAT HISTORY / LIVE TRAFFIC MONITOR
// ============================================================

// -------------------- Local credentials --------------------
// Create secrets.h from secrets.example.h and enter your own values there.
#include "secrets.h"

const char* WIFI_SSID = MATRIX_WIFI_SSID;
const char* WIFI_PASSWORD = MATRIX_WIFI_PASSWORD;

// -------------------- MQTT --------------------
const char* MQTT_SERVER = MATRIX_MQTT_SERVER;
const int MQTT_PORT = MATRIX_MQTT_PORT;
const char* MQTT_USER = MATRIX_MQTT_USER;
const char* MQTT_PASSWORD = MATRIX_MQTT_PASSWORD;
const char* MQTT_CLIENT_ID = "MATRIX-CORE2";

// -------------------- MQTT Topics --------------------
const char* TOPIC_STATUS = "matrix/status";
const char* TOPIC_EVENTS = "matrix/events";
const char* TOPIC_INVENTORY = "matrix/inventory";
const char* TOPIC_THREATS_REQUEST = "matrix/requests/threats";
const char* TOPIC_THREATS_HISTORY = "matrix/threats/history";
const char* TOPIC_TRAFFIC = "matrix/traffic";

// -------------------- Colors --------------------
#define C_BLACK   0x0000
#define C_PANEL   0x0841
#define C_GREEN   0x07E0
#define C_NEON    0x37E8
#define C_WHITE   0xFFFF
#define C_GRAY    0x8410
#define C_RED     0xF800
#define C_YELLOW  0xFFE0
#define C_BORDER  0x03A0

// -------------------- Display --------------------
static const int SCREEN_W = 320;
static const int SCREEN_H = 240;

static const int HEADER_LINE_Y = 29;
static const int NAV_LINE_Y = 210;

M5Canvas canvas(&M5.Display);

// -------------------- Network --------------------
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// -------------------- Pages --------------------
enum Page {
  HOME,
  NETWORK,
  THREATS,
  SYSTEM,
  DEVICES,
  TRAFFIC,
  POWER,
  SETTINGS
};

Page currentPage = HOME;
int navGroup = 0;

// -------------------- Status --------------------
bool piStatusReceived = false;
bool inventoryReceived = false;

String piStatus = "WAITING FOR PI";
String latestEvent = "NO EVENTS RECEIVED";

// -------------------- Inventory --------------------
int onlineDevices = 0;
int offlineDevices = 0;
int registeredDevices = 0;
int unknownDevices = 0;

unsigned long inventoryReceivedAt = 0;

// -------------------- Traffic --------------------
float trafficRxMbps = 0.0;
float trafficTxMbps = 0.0;
float trafficRxTotalMB = 0.0;
float trafficTxTotalMB = 0.0;

bool trafficReceived = false;
unsigned long trafficReceivedAt = 0;

const unsigned long TRAFFIC_TIMEOUT = 10000;

// -------------------- Threat History --------------------
const int MAX_EVENTS = 10;

String eventMessages[MAX_EVENTS];
String eventLevels[MAX_EVENTS];
String eventDetails[MAX_EVENTS];
String eventSids[MAX_EVENTS];

int eventCount = 0;

bool historyLoaded = false;
bool historyReceiving = false;
unsigned long historyRequestAt = 0;

// -------------------- Battery --------------------
int batteryLevel = -1;
int batteryVoltage = -1;
int batteryCurrent = 0;
bool batteryCharging = false;
unsigned long lastBatteryRead = 0;

// -------------------- Timers --------------------
unsigned long lastTouch = 0;
unsigned long lastWiFiAttempt = 0;
unsigned long lastMQTTAttempt = 0;

// -------------------- Rendering --------------------
bool redrawRequested = true;

// ============================================================
// TEXT ENGINE
// ============================================================

void drawText(const String& value,
              int x, int y,
              uint16_t color,
              int size = 1,
              int maxChars = 36) {

  String output = value;

  output.replace("\r", " ");
  output.replace("\n", " ");

  if (maxChars > 0 && output.length() > maxChars) {
    if (maxChars > 3) {
      output = output.substring(0, maxChars - 3) + "...";
    } else {
      output = output.substring(0, maxChars);
    }
  }

  canvas.setFont(&fonts::Font2);
  canvas.setTextSize(size);
  canvas.setTextColor(color);
  canvas.setTextDatum(TL_DATUM);
  canvas.drawString(output, x, y);
}

void drawText(const char* value,
              int x, int y,
              uint16_t color,
              int size = 1,
              int maxChars = 36) {
  drawText(String(value), x, y, color, size, maxChars);
}

// ============================================================
// PANEL ENGINE
// ============================================================

void drawPanel(int x, int y, int w, int h) {
  canvas.fillRoundRect(x, y, w, h, 5, C_PANEL);
  canvas.drawRoundRect(x, y, w, h, 5, C_BORDER);
}

void drawPanelTitle(const String& title, int x, int y) {
  drawText(title, x, y, C_GRAY, 1, 34);
}

// ============================================================
// DISPLAY INITIALIZATION
// ============================================================

void setLandscape() {
  for (int i = 0; i < 4; i++) {
    M5.Display.setRotation(i);

    if (M5.Display.width() > M5.Display.height()) {
      return;
    }
  }

  M5.Display.setRotation(0);
}

// ============================================================
// BATTERY
// ============================================================

uint16_t getBatteryColor() {
  if (batteryLevel < 0) return C_GRAY;
  if (batteryLevel <= 15) return C_RED;
  if (batteryLevel <= 30) return C_YELLOW;

  return C_GREEN;
}

void updateBattery() {
  int newLevel = M5.Power.getBatteryLevel();
  int newVoltage = M5.Power.getBatteryVoltage();
  int newCurrent = M5.Power.getBatteryCurrent();
  bool newCharging = M5.Power.isCharging();

  if (newLevel != batteryLevel ||
      newVoltage != batteryVoltage ||
      newCurrent != batteryCurrent ||
      newCharging != batteryCharging) {

    batteryLevel = newLevel;
    batteryVoltage = newVoltage;
    batteryCurrent = newCurrent;
    batteryCharging = newCharging;

    redrawRequested = true;
  }
}

void drawBatteryHeader() {
  uint16_t color = getBatteryColor();

  canvas.drawRoundRect(263, 8, 18, 10, 2, color);
  canvas.fillRect(281, 11, 2, 4, color);

  if (batteryLevel >= 0) {
    int fillWidth = batteryLevel * 12 / 100;

    if (fillWidth > 0) {
      canvas.fillRect(266, 11, fillWidth, 4, color);
    }
  }

  String value;

  if (batteryLevel < 0) {
    value = "--%";
  } else {
    value = String(batteryLevel) + "%";
  }

  drawText(value, 286, 6, color, 1, 4);

  if (batteryCharging) {
    drawText("+", 307, 6, C_GREEN);
  }
}

// ============================================================
// COMMON HEADER
// ============================================================

void drawHeader(const char* pageName) {
  canvas.fillScreen(C_BLACK);

  drawText("MATRIX", 9, 6, C_NEON, 1, 8);
  drawText(pageName, 105, 6, C_WHITE, 1, 18);

  drawBatteryHeader();

  canvas.drawFastHLine(
    7, HEADER_LINE_Y, 306, C_BORDER
  );
}

// ============================================================
// NAVIGATION
// ============================================================

void drawNavigation() {
  canvas.drawFastHLine(
    7, NAV_LINE_Y, 306, C_BORDER
  );

  const char* labels[4];

  if (navGroup == 0) {
    labels[0] = "HOME";
    labels[1] = "NETWORK";
    labels[2] = "THREATS";
    labels[3] = "MORE";
  }
  else if (navGroup == 1) {
    labels[0] = "DEVICES";
    labels[1] = "TRAFFIC";
    labels[2] = "SYSTEM";
    labels[3] = "MORE";
  }
  else {
    labels[0] = "POWER";
    labels[1] = "SETTINGS";
    labels[2] = "HOME";
    labels[3] = "BACK";
  }

  const int xs[4] = {7, 85, 163, 241};

  for (int i = 0; i < 4; i++) {
    bool selected = false;

    if (navGroup == 0) {
      selected =
        (i == 0 && currentPage == HOME) ||
        (i == 1 && currentPage == NETWORK) ||
        (i == 2 && currentPage == THREATS);
    }
    else if (navGroup == 1) {
      selected =
        (i == 0 && currentPage == DEVICES) ||
        (i == 1 && currentPage == TRAFFIC) ||
        (i == 2 && currentPage == SYSTEM);
    }
    else {
      selected =
        (i == 0 && currentPage == POWER) ||
        (i == 1 && currentPage == SETTINGS);
    }

    if (selected) {
      canvas.fillRoundRect(
        xs[i], 215, 70, 21, 3, C_PANEL
      );

      canvas.drawRoundRect(
        xs[i], 215, 70, 21, 3, C_GREEN
      );

      drawText(
        labels[i], xs[i] + 5, 218,
        C_NEON, 1, 9
      );
    }
    else {
      drawText(
        labels[i], xs[i] + 5, 218,
        C_GRAY, 1, 9
      );
    }
  }
}

// ============================================================
// INVENTORY STATUS
// ============================================================

bool inventoryIsFresh() {
  return inventoryReceived &&
         millis() - inventoryReceivedAt < 900000;
}

// ============================================================
// TRAFFIC STATUS
// ============================================================

bool trafficIsFresh() {
  return trafficReceived &&
         millis() - trafficReceivedAt < TRAFFIC_TIMEOUT;
}

// ============================================================
// HOME PAGE
// ============================================================

void drawHome() {
  drawHeader("DASHBOARD");

  drawPanel(8, 37, 148, 59);
  drawPanelTitle("THREAT LEVEL", 17, 44);

  uint16_t threatColor = C_GREEN;
  String threatText = "LOW";

  if (eventCount > 0) {
    if (eventLevels[0] == "ALERT") {
      threatColor = C_RED;
      threatText = "ALERT";
    }
    else if (eventLevels[0] == "WARNING") {
      threatColor = C_YELLOW;
      threatText = "WARNING";
    }
  }

  drawText(threatText, 17, 62, threatColor, 2, 12);

  drawPanel(164, 37, 148, 59);
  drawPanelTitle("MQTT LINK", 173, 44);

  drawText(
    mqtt.connected() ? "ONLINE" : "OFFLINE",
    173, 62,
    mqtt.connected() ? C_GREEN : C_RED,
    2
  );

  drawPanel(8, 104, 94, 46);
  drawPanelTitle("PI STATUS", 16, 110);

  drawText(
    piStatusReceived ? "RECEIVED" : "WAITING",
    16, 126,
    piStatusReceived ? C_GREEN : C_YELLOW
  );

  drawPanel(113, 104, 94, 46);
  drawPanelTitle("WI-FI", 121, 110);

  drawText(
    WiFi.status() == WL_CONNECTED ? "ONLINE" : "OFFLINE",
    121, 126,
    WiFi.status() == WL_CONNECTED ? C_GREEN : C_RED
  );

  drawPanel(218, 104, 94, 46);
  drawPanelTitle("DEVICES", 226, 110);

  if (inventoryIsFresh()) {
    String value =
      String(onlineDevices) + "/" +
      String(registeredDevices);

    drawText(value, 226, 126, C_GREEN, 2, 10);
  }
  else {
    drawText("--/--", 226, 126, C_YELLOW, 2);
  }

  drawPanel(8, 158, 304, 43);
  drawPanelTitle("LATEST PI MESSAGE", 17, 163);

  drawText(latestEvent, 17, 181, C_YELLOW, 1, 43);

  drawNavigation();
}

// ============================================================
// NETWORK PAGE
// ============================================================

void drawNetwork() {
  drawHeader("NETWORK");

  drawPanel(8, 37, 304, 34);
  drawPanelTitle("NETWORK STATUS", 17, 42);

  drawText(
    WiFi.status() == WL_CONNECTED
      ? "WIFI CONNECTED"
      : "WIFI OFFLINE",
    175, 45,
    WiFi.status() == WL_CONNECTED ? C_GREEN : C_RED,
    1, 16
  );

  drawPanel(8, 80, 148, 49);
  drawPanelTitle("ONLINE", 17, 85);

  drawPanel(164, 80, 148, 49);
  drawPanelTitle("OFFLINE", 173, 85);

  drawPanel(8, 137, 148, 49);
  drawPanelTitle("REGISTERED", 17, 142);

  drawPanel(164, 137, 148, 49);
  drawPanelTitle("UNKNOWN", 173, 142);

  if (inventoryIsFresh()) {
    drawText(String(onlineDevices), 17, 102, C_GREEN, 2);
    drawText(String(offlineDevices), 173, 102, C_GRAY, 2);
    drawText(String(registeredDevices), 17, 159, C_WHITE, 2);

    drawText(
      String(unknownDevices),
      173, 159,
      unknownDevices > 0 ? C_YELLOW : C_GREEN,
      2
    );

    drawText("LIVE INVENTORY: PI / NMAP", 17, 191, C_GREEN);
  }
  else {
    drawText("--", 17, 102, C_YELLOW, 2);
    drawText("--", 173, 102, C_YELLOW, 2);
    drawText("--", 17, 159, C_YELLOW, 2);
    drawText("--", 173, 159, C_YELLOW, 2);

    drawText("WAITING FOR PI INVENTORY", 17, 191, C_YELLOW);
  }

  drawNavigation();
}

// ============================================================
// THREAT EVENT STORAGE
// ============================================================

void clearThreatHistory() {
  eventCount = 0;

  for (int i = 0; i < MAX_EVENTS; i++) {
    eventMessages[i] = "";
    eventLevels[i] = "";
    eventDetails[i] = "";
    eventSids[i] = "";
  }
}

void insertThreat(
  String level,
  String message,
  String sid,
  String details
) {
  for (int i = MAX_EVENTS - 1; i > 0; i--) {
    eventLevels[i] = eventLevels[i - 1];
    eventMessages[i] = eventMessages[i - 1];
    eventDetails[i] = eventDetails[i - 1];
    eventSids[i] = eventSids[i - 1];
  }

  eventLevels[0] = level;
  eventMessages[0] = message;
  eventDetails[0] = details;
  eventSids[0] = sid;

  if (eventCount < MAX_EVENTS) {
    eventCount++;
  }

  latestEvent = message;
}

// ============================================================
// THREAT ROW
// ============================================================

void drawThreatRow(int index, int y) {
  drawPanel(8, y, 304, 29);

  if (index >= eventCount) {
    drawText("--", 17, y + 7, C_GRAY);
    drawText("NO SECURITY EVENTS", 55, y + 7, C_GRAY);
    return;
  }

  uint16_t color = C_GREEN;

  if (eventLevels[index] == "WARNING") {
    color = C_YELLOW;
  }

  if (eventLevels[index] == "ALERT") {
    color = C_RED;
  }

  String title =
    eventLevels[index] +
    " SID:" +
    eventSids[index];

  drawText(title, 17, y + 2, color, 1, 35);

  String description = eventMessages[index];

  if (description.length() == 0) {
    description = eventDetails[index];
  }

  drawText(description, 17, y + 15, C_WHITE, 1, 43);
}

// ============================================================
// THREATS PAGE
// ============================================================

void drawThreats() {
  drawHeader("THREATS");

  drawPanel(8, 36, 304, 36);

  drawText("SOC EVENT CONSOLE", 17, 39, C_NEON, 1, 30);

  String status;

  if (!mqtt.connected()) {
    status = "MQTT OFFLINE";
  }
  else if (historyReceiving) {
    status = "RECEIVING HISTORY...";
  }
  else if (historyLoaded) {
    status = "HISTORY LOADED: " + String(eventCount);
  }
  else {
    status = "WAITING FOR HISTORY";
  }

  drawText(status, 17, 54, C_GRAY, 1, 34);

  drawThreatRow(0, 77);
  drawThreatRow(1, 109);
  drawThreatRow(2, 141);
  drawThreatRow(3, 173);

  drawNavigation();
}

// ============================================================
// SYSTEM PAGE
// ============================================================

void drawSystem() {
  drawHeader("SYSTEM");

  drawPanel(8, 37, 304, 43);
  drawPanelTitle("RASPBERRY PI", 17, 42);

  drawText(
    piStatusReceived ? "STATUS RECEIVED" : "WAITING FOR STATUS",
    17, 59,
    piStatusReceived ? C_GREEN : C_YELLOW
  );

  drawPanel(8, 89, 304, 43);
  drawPanelTitle("NETWORK MONITOR", 17, 94);

  drawText(
    inventoryIsFresh()
      ? "NMAP INVENTORY ACTIVE"
      : "WAITING FOR INVENTORY",
    17, 111,
    inventoryIsFresh() ? C_GREEN : C_YELLOW
  );

  drawPanel(8, 141, 304, 43);
  drawPanelTitle("MQTT CONNECTION", 17, 146);

  drawText(
    mqtt.connected() ? "CONNECTED" : "NOT CONNECTED",
    17, 163,
    mqtt.connected() ? C_GREEN : C_RED
  );

  drawNavigation();
}

// ============================================================
// DEVICES PAGE
// ============================================================

void drawDevices() {
  drawHeader("DEVICES");

  drawPanel(8, 37, 304, 31);
  drawText("DEVICE INVENTORY", 17, 46, C_NEON);

  drawPanel(8, 76, 304, 105);

  if (inventoryIsFresh()) {
    char value[48];

    snprintf(value, sizeof(value),
      "ONLINE DEVICES     : %d", onlineDevices);
    drawText(value, 18, 88, C_GREEN, 1, 36);

    snprintf(value, sizeof(value),
      "OFFLINE DEVICES    : %d", offlineDevices);
    drawText(value, 18, 110, C_GRAY, 1, 36);

    snprintf(value, sizeof(value),
      "REGISTERED DEVICES : %d", registeredDevices);
    drawText(value, 18, 132, C_WHITE, 1, 36);

    snprintf(value, sizeof(value),
      "UNKNOWN DEVICES    : %d", unknownDevices);
    drawText(value, 18, 154,
      unknownDevices > 0 ? C_YELLOW : C_GREEN, 1, 36);
  }
  else {
    drawText("WAITING FOR PI INVENTORY", 18, 95, C_YELLOW);
    drawText("DEVICE DETAILS NOT RECEIVED", 18, 119, C_GRAY);
  }

  drawNavigation();
}

// ============================================================
// TRAFFIC PAGE
// ============================================================

void drawTraffic() {
  drawHeader("TRAFFIC");

  drawPanel(8, 37, 304, 31);
  drawText("NETWORK TRAFFIC MONITOR", 17, 46, C_NEON);

  drawPanel(8, 76, 304, 105);

  if (trafficIsFresh()) {
    drawText("INTERFACE : wlan0", 18, 81, C_GREEN);

    char rxRate[32];
    char txRate[32];
    char rxTotal[32];
    char txTotal[32];

    snprintf(
      rxRate, sizeof(rxRate),
      "RX SPEED  : %.3f Mbps",
      trafficRxMbps
    );

    snprintf(
      txRate, sizeof(txRate),
      "TX SPEED  : %.3f Mbps",
      trafficTxMbps
    );

    snprintf(
      rxTotal, sizeof(rxTotal),
      "RX TOTAL  : %.2f MB",
      trafficRxTotalMB
    );

    snprintf(
      txTotal, sizeof(txTotal),
      "TX TOTAL  : %.2f MB",
      trafficTxTotalMB
    );

    drawText(rxRate, 18, 101, C_GREEN, 1, 36);
    drawText(txRate, 18, 121, C_NEON, 1, 36);
    drawText(rxTotal, 18, 141, C_WHITE, 1, 36);
    drawText(txTotal, 18, 161, C_WHITE, 1, 36);
  }
  else {
    drawText("INTERFACE : wlan0", 18, 87, C_GRAY);
    drawText("RX SPEED  : WAITING", 18, 109, C_GRAY);
    drawText("TX SPEED  : WAITING", 18, 131, C_GRAY);
    drawText("RX / TX TOTAL : WAITING", 18, 153, C_GRAY);
  }

  drawText(
    trafficIsFresh() ? "LIVE DATA" :
    trafficReceived ? "STALE DATA" : "NO TRAFFIC DATA",
    18, 187,
    trafficIsFresh() ? C_GREEN : C_YELLOW,
    1, 24
  );

  drawNavigation();
}

// ============================================================
// POWER PAGE
// ============================================================

void drawPower() {
  drawHeader("POWER");

  drawPanel(8, 37, 304, 62);
  drawPanelTitle("BATTERY LEVEL", 18, 44);

  uint16_t color = getBatteryColor();

  if (batteryLevel >= 0) {
    drawText(
      String(batteryLevel) + "%",
      18, 63, color, 2
    );

    canvas.drawRoundRect(112, 63, 180, 17, 3, color);

    int width = batteryLevel * 174 / 100;

    if (width > 0) {
      canvas.fillRect(115, 66, width, 11, color);
    }
  }
  else {
    drawText("UNKNOWN", 18, 63, C_GRAY, 2);
  }

  drawPanel(8, 108, 148, 38);
  drawPanelTitle("CHARGING", 17, 113);

  drawText(
    batteryCharging ? "YES" : "NO",
    17, 129,
    batteryCharging ? C_GREEN : C_YELLOW
  );

  drawPanel(164, 108, 148, 38);
  drawPanelTitle("VOLTAGE", 173, 113);

  if (batteryVoltage > 0) {
    char voltage[20];

    snprintf(
      voltage, sizeof(voltage),
      "%.2f V",
      batteryVoltage / 1000.0
    );

    drawText(voltage, 173, 129, C_WHITE);
  }
  else {
    drawText("--", 173, 129, C_GRAY);
  }

  drawPanel(8, 154, 304, 39);
  drawPanelTitle("BATTERY CURRENT", 17, 159);

  char current[24];

  snprintf(
    current, sizeof(current),
    "%d mA",
    batteryCurrent
  );

  drawText(current, 17, 175, C_WHITE);

  drawNavigation();
}

// ============================================================
// SETTINGS PAGE
// ============================================================

void drawSettings() {
  drawHeader("SETTINGS");

  drawPanel(8, 37, 304, 43);
  drawPanelTitle("WI-FI", 17, 42);

  drawText(
    WiFi.status() == WL_CONNECTED
      ? "CONNECTED"
      : "DISCONNECTED",
    17, 59,
    WiFi.status() == WL_CONNECTED ? C_GREEN : C_RED
  );

  drawPanel(8, 89, 304, 43);
  drawPanelTitle("MQTT SERVER", 17, 94);

  drawText(MQTT_SERVER, 17, 111, C_WHITE, 1, 34);

  drawPanel(8, 141, 304, 43);
  drawPanelTitle("MQTT TOPICS", 17, 146);

  drawText(
    "status /events /inventory /traffic",
    17, 163, C_GREEN, 1, 36
  );

  drawNavigation();
}

// ============================================================
// PAGE RENDERER
// ============================================================

void drawPage() {
  switch (currentPage) {
    case HOME:     drawHome(); break;
    case NETWORK:  drawNetwork(); break;
    case THREATS:  drawThreats(); break;
    case SYSTEM:   drawSystem(); break;
    case DEVICES:  drawDevices(); break;
    case TRAFFIC:  drawTraffic(); break;
    case POWER:    drawPower(); break;
    case SETTINGS: drawSettings(); break;
  }

  canvas.pushSprite(0, 0);
}

// ============================================================
// STRING HELPERS
// ============================================================

String shortenMessage(String message, int maxLength) {
  message.replace("\n", " ");
  message.replace("\r", " ");

  if (message.length() > maxLength) {
    message = message.substring(0, maxLength - 3) + "...";
  }

  return message;
}

// ============================================================
// JSON PARSER
// ============================================================

String jsonString(const String& json, const char* key) {
  String searchKey = "\"" + String(key) + "\"";

  int pos = json.indexOf(searchKey);

  if (pos < 0) return "";

  pos = json.indexOf(':', pos + searchKey.length());

  if (pos < 0) return "";

  pos++;

  while (pos < json.length() &&
         (json[pos] == ' ' || json[pos] == '\t')) {
    pos++;
  }

  if (pos >= json.length() || json[pos] != '"') {
    return "";
  }

  pos++;

  String result;
  bool escaped = false;

  while (pos < json.length()) {
    char c = json[pos++];

    if (escaped) {
      if (c == 'n') result += '\n';
      else if (c == 'r') result += '\r';
      else result += c;

      escaped = false;
    }
    else if (c == '\\') {
      escaped = true;
    }
    else if (c == '"') {
      break;
    }
    else {
      result += c;
    }
  }

  return result;
}

int jsonInt(const String& json, const char* key) {
  String searchKey = "\"" + String(key) + "\"";

  int pos = json.indexOf(searchKey);

  if (pos < 0) return -1;

  pos = json.indexOf(':', pos + searchKey.length());

  if (pos < 0) return -1;

  pos++;

  while (pos < json.length() &&
         (json[pos] == ' ' || json[pos] == '\t')) {
    pos++;
  }

  int end = pos;

  if (end < json.length() && json[end] == '-') {
    end++;
  }

  while (end < json.length() && isDigit(json[end])) {
    end++;
  }

  if (end == pos) return -1;

  return json.substring(pos, end).toInt();
}

// ============================================================
// FLOAT JSON PARSER
// ============================================================

float jsonFloat(const String& json, const char* key) {
  String searchKey = "\"" + String(key) + "\"";

  int pos = json.indexOf(searchKey);

  if (pos < 0) return -1.0;

  pos = json.indexOf(':', pos + searchKey.length());

  if (pos < 0) return -1.0;

  pos++;

  while (pos < json.length() &&
         (json[pos] == ' ' || json[pos] == '\t')) {
    pos++;
  }

  int end = pos;

  if (end < json.length() && json[end] == '-') {
    end++;
  }

  while (end < json.length() &&
         (isDigit(json[end]) || json[end] == '.')) {
    end++;
  }

  if (end == pos) return -1.0;

  return json.substring(pos, end).toFloat();
}

// ============================================================
// THREAT HISTORY REQUEST
// ============================================================

void requestThreatHistory() {
  if (!mqtt.connected()) {
    Serial.println("History request skipped: MQTT offline");
    return;
  }

  clearThreatHistory();

  historyLoaded = false;
  historyReceiving = true;
  historyRequestAt = millis();

  bool published = mqtt.publish(
    TOPIC_THREATS_REQUEST,
    "LATEST",
    false
  );

  Serial.println(
    published
      ? "Threat history requested"
      : "Threat history request failed"
  );

  redrawRequested = true;
}

// ============================================================
// PROCESS THREAT HISTORY
// ============================================================

void processThreatHistory(String message) {
  String type = jsonString(message, "type");

  if (type == "event") {
    String level = jsonString(message, "level");
    String signature = jsonString(message, "signature");

    String srcIP = jsonString(message, "src_ip");
    String destIP = jsonString(message, "dest_ip");
    String proto = jsonString(message, "proto");

    String sid = String(jsonInt(message, "sid"));
    int port = jsonInt(message, "dest_port");

    if (level != "ALERT" &&
        level != "WARNING" &&
        level != "INFO") {
      level = "INFO";
    }

    String details = srcIP + " > " + destIP;

    if (port > 0) {
      details += ":" + String(port);
    }

    if (proto.length() > 0) {
      details += " " + proto;
    }

    insertThreat(
      level,
      shortenMessage(signature, 80),
      sid,
      details
    );

    Serial.printf(
      "History event: SID%s [%s]\n",
      sid.c_str(),
      level.c_str()
    );

    redrawRequested = true;
  }
  else if (type == "end") {
    int count = jsonInt(message, "count");

    historyReceiving = false;
    historyLoaded = true;

    Serial.printf(
      "Threat history completed: %d events\n",
      count
    );

    redrawRequested = true;
  }
}

// ============================================================
// INVENTORY JSON
// ============================================================

int getJsonInt(const String& json, const char* key) {
  String searchKey = String("\"") + key + "\":";

  int position = json.indexOf(searchKey);

  if (position < 0) return -1;

  position += searchKey.length();

  while (position < json.length() &&
         (json[position] == ' ' || json[position] == '\t')) {
    position++;
  }

  int endPosition = position;

  while (endPosition < json.length() &&
         isDigit(json[endPosition])) {
    endPosition++;
  }

  if (endPosition == position) return -1;

  return json.substring(position, endPosition).toInt();
}

void processInventory(String message) {
  int online = getJsonInt(message, "online");
  int offline = getJsonInt(message, "offline");
  int registered = getJsonInt(message, "registered");
  int unknown = getJsonInt(message, "unknown");

  if (online < 0 ||
      offline < 0 ||
      registered < 0 ||
      unknown < 0 ||
      online + offline != registered ||
      unknown > online) {

    Serial.println("Invalid inventory message");
    return;
  }

  bool changed =
    online != onlineDevices ||
    offline != offlineDevices ||
    registered != registeredDevices ||
    unknown != unknownDevices ||
    !inventoryReceived;

  onlineDevices = online;
  offlineDevices = offline;
  registeredDevices = registered;
  unknownDevices = unknown;

  inventoryReceived = true;
  inventoryReceivedAt = millis();

  Serial.printf(
    "Inventory: ONLINE=%d OFFLINE=%d TOTAL=%d UNKNOWN=%d\n",
    online, offline, registered, unknown
  );

  if (changed) {
    redrawRequested = true;
  }
}

// ============================================================
// PROCESS TRAFFIC
// ============================================================

void processTraffic(String message) {
  String interfaceName = jsonString(message, "interface");

  if (interfaceName != "wlan0") {
    return;
  }

  float rx = jsonFloat(message, "rx_mbps");
  float tx = jsonFloat(message, "tx_mbps");
  float rxTotal = jsonFloat(message, "rx_total_mb");
  float txTotal = jsonFloat(message, "tx_total_mb");

  if (rx < 0 ||
      tx < 0 ||
      rxTotal < 0 ||
      txTotal < 0) {

    Serial.println("Invalid traffic message");
    return;
  }

  trafficRxMbps = rx;
  trafficTxMbps = tx;
  trafficRxTotalMB = rxTotal;
  trafficTxTotalMB = txTotal;

  trafficReceived = true;
  trafficReceivedAt = millis();

  redrawRequested = true;
}

// ============================================================
// REALTIME EVENTS
// ============================================================

void addEvent(String level, String message) {
  insertThreat(
    level,
    shortenMessage(message, 80),
    "--",
    "LIVE MQTT EVENT"
  );

  Serial.printf(
    "New event [%s]: %s\n",
    level.c_str(),
    message.c_str()
  );

  redrawRequested = true;
}

// ============================================================
// MQTT CALLBACK
// ============================================================

void mqttCallback(
  char* topic,
  byte* payload,
  unsigned int length
) {
  String message;

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (strcmp(topic, TOPIC_STATUS) == 0) {
    String newStatus = shortenMessage(message, 34);

    if (newStatus != piStatus || !piStatusReceived) {
      piStatus = newStatus;
      piStatusReceived = true;
      latestEvent = piStatus;

      redrawRequested = true;
    }
  }
  else if (strcmp(topic, TOPIC_EVENTS) == 0) {
    int separator = message.indexOf('|');

    if (separator >= 0) {
      String level = message.substring(0, separator);
      String detail = message.substring(separator + 1);

      level.trim();
      detail.trim();
      level.toUpperCase();

      if (level != "INFO" &&
          level != "WARNING" &&
          level != "ALERT") {
        level = "INFO";
      }

      addEvent(level, detail);
    }
    else {
      addEvent("INFO", message);
    }
  }
  else if (strcmp(topic, TOPIC_INVENTORY) == 0) {
    processInventory(message);
  }
  else if (strcmp(topic, TOPIC_THREATS_HISTORY) == 0) {
    processThreatHistory(message);
  }
  else if (strcmp(topic, TOPIC_TRAFFIC) == 0) {
    processTraffic(message);
  }
}

// ============================================================
// WIFI CONNECTION
// ============================================================

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (lastWiFiAttempt != 0 &&
      millis() - lastWiFiAttempt < 10000) {
    return;
  }

  lastWiFiAttempt = millis();

  Serial.println("Connecting to Wi-Fi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

// ============================================================
// MQTT CONNECTION
// ============================================================

void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (mqtt.connected()) {
    return;
  }

  if (lastMQTTAttempt != 0 &&
      millis() - lastMQTTAttempt < 5000) {
    return;
  }

  lastMQTTAttempt = millis();

  Serial.println("Connecting to MQTT...");

  if (mqtt.connect(
        MQTT_CLIENT_ID,
        MQTT_USER,
        MQTT_PASSWORD
      )) {

    Serial.println("MQTT connected!");

    mqtt.subscribe(TOPIC_STATUS);
    mqtt.subscribe(TOPIC_EVENTS);
    mqtt.subscribe(TOPIC_INVENTORY);
    mqtt.subscribe(TOPIC_THREATS_HISTORY);
    mqtt.subscribe(TOPIC_TRAFFIC);

    Serial.println("Subscribed to threat history");
    Serial.println("Subscribed to traffic");

    redrawRequested = true;

    if (currentPage == THREATS) {
      requestThreatHistory();
    }
  }
  else {
    Serial.printf(
      "MQTT failed: %d\n",
      mqtt.state()
    );
  }
}

// ============================================================
// CONNECTION MONITOR
// ============================================================

void checkConnectionChanges() {
  static bool previousWiFi = false;
  static bool previousMQTT = false;

  bool currentWiFi =
    WiFi.status() == WL_CONNECTED;

  bool currentMQTT =
    mqtt.connected();

  if (currentWiFi != previousWiFi ||
      currentMQTT != previousMQTT) {

    previousWiFi = currentWiFi;
    previousMQTT = currentMQTT;

    redrawRequested = true;
  }
}

// ============================================================
// TOUCH NAVIGATION
// ============================================================

void handleTouch() {
  auto detail = M5.Touch.getDetail();

  if (!detail.wasPressed()) {
    return;
  }

  if (millis() - lastTouch < 250) {
    return;
  }

  lastTouch = millis();

  int x = detail.x;
  int y = detail.y;

  if (y < 210) {
    return;
  }

  int button = x / 80;

  if (button > 3) {
    button = 3;
  }

  Page previousPage = currentPage;

  if (navGroup == 0) {
    if (button == 0) currentPage = HOME;
    if (button == 1) currentPage = NETWORK;
    if (button == 2) currentPage = THREATS;
    if (button == 3) navGroup = 1;
  }
  else if (navGroup == 1) {
    if (button == 0) currentPage = DEVICES;
    if (button == 1) currentPage = TRAFFIC;
    if (button == 2) currentPage = SYSTEM;
    if (button == 3) navGroup = 2;
  }
  else {
    if (button == 0) currentPage = POWER;
    if (button == 1) currentPage = SETTINGS;

    if (button == 2) {
      currentPage = HOME;
      navGroup = 0;
    }

    if (button == 3) {
      navGroup = 1;
    }
  }

  if (previousPage != THREATS &&
      currentPage == THREATS) {
    requestThreatHistory();
  }

  redrawRequested = true;
}

// ============================================================
// SETUP
// ============================================================

void setup() {
  auto config = M5.config();

  M5.begin(config);

  Serial.begin(115200);

  setLandscape();

  M5.Display.setBrightness(180);

  canvas.setColorDepth(16);

  if (!canvas.createSprite(SCREEN_W, SCREEN_H)) {
    Serial.println("ERROR: Canvas allocation failed");

    while (true) {
      delay(1000);
    }
  }

  canvas.fillScreen(C_BLACK);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  mqtt.setBufferSize(1024);
  mqtt.setKeepAlive(30);

  updateBattery();

  redrawRequested = true;

  Serial.println("--------------------------------");
  Serial.println("MATRIX HOME SOC");
  Serial.println("CORE2 FULL REBUILD");
  Serial.println("8 PAGES READY");
  Serial.println("TRAFFIC MONITOR READY");
  Serial.println("--------------------------------");
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {
  M5.update();

  connectWiFi();
  connectMQTT();

  if (mqtt.connected()) {
    mqtt.loop();
  }

  checkConnectionChanges();
  handleTouch();

  if (millis() - lastBatteryRead >= 5000) {
    lastBatteryRead = millis();
    updateBattery();
  }

  if (historyReceiving &&
      millis() - historyRequestAt > 10000) {

    historyReceiving = false;

    Serial.println("Threat history request timeout");

    redrawRequested = true;
  }

  // Refresh traffic page when its data becomes stale.
  if (currentPage == TRAFFIC && trafficReceived) {
    static bool trafficWasFresh = true;
    bool nowFresh = trafficIsFresh();

    if (trafficWasFresh != nowFresh) {
      trafficWasFresh = nowFresh;
      redrawRequested = true;
    }
  }

  if (redrawRequested) {
    drawPage();
    redrawRequested = false;
  }

  delay(20);
}
