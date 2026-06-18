/*
 * Wardriving Halo — ESP32-S3
 * WiFi AP hotspot + HTTP wardriving server for WiFi Halo app.
 * - Scans WiFi networks every 10s and serves results as dump1090 aircraft.json
 * - /data/aircraft.json  — WiFi scan results (dump1090 format)
 * - /data/signals.json   — same data for signal view
 * - /json/fromradio      — empty Meshtastic packet array []
 * - /battery             — {"level":N,"voltage":V}
 * - /gps  (POST)         — receive satellite/location data from phone
 * PRG button (GPIO0) cycles OLED pages: Main → Satellite → Networks
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"

// ── USB JTAG TX FIFO — direct register access ──────────────────────────────
#define USJ_EP1_REG      (*((volatile uint32_t *)0x60038000))
#define USJ_EP1_CONF_REG (*((volatile uint32_t *)0x60038004))
#define USJ_INT_RAW_REG  (*((volatile uint32_t *)0x60038008))
#define USJ_INT_CLR_REG  (*((volatile uint32_t *)0x60038014))
#define USJ_TX_FREE      (0x2u)
#define USJ_WR_DONE      (0x1u)
#define USJ_SOF_INT      (0x2u)

static void usjWrite(const char *s) {
    while (*s) {
        if (!(USJ_EP1_CONF_REG & USJ_TX_FREE)) break;
        USJ_EP1_REG = (uint8_t)*s++;
    }
    USJ_EP1_CONF_REG |= USJ_WR_DONE;
}

// ── Hardware pins ──────────────────────────────────────────────────────────
#define PIN_VEXT         36
#define PIN_OLED_SDA     17
#define PIN_OLED_SCL     18
#define PIN_OLED_RST     21
#define PIN_BATT_ADC      1
#define PIN_BATT_CTRL    37   // LOW = enable battery voltage divider
#define PIN_BTN           0   // PRG button, active LOW

// ── Battery ────────────────────────────────────────────────────────────────
float battVoltage = 0.0f;

#define BATT_RATIO   4.9f
#define BATT_FULL    4.20f
#define BATT_EMPTY   3.00f

int readBattPct() {
    pinMode(PIN_BATT_CTRL, OUTPUT);
    digitalWrite(PIN_BATT_CTRL, LOW);
    delay(5);
    analogSetPinAttenuation(PIN_BATT_ADC, ADC_11db);
    uint32_t mv = 0;
    for (int i = 0; i < 8; i++) mv += analogReadMilliVolts(PIN_BATT_ADC);
    mv /= 8;
    digitalWrite(PIN_BATT_CTRL, HIGH);
    battVoltage = (mv / 1000.0f) * BATT_RATIO;
    if (battVoltage < 0.5f) return 0;
    int pct = (int)((battVoltage - BATT_EMPTY) / (BATT_FULL - BATT_EMPTY) * 100.0f);
    return constrain(pct, 0, 100);
}

// ── WiFi AP ────────────────────────────────────────────────────────────────
#define AP_SSID "WardrivingHalo"
#define AP_PASS "wardhalo1"

WebServer httpServer(80);

// ── Scan data ──────────────────────────────────────────────────────────────
#define MAX_NETS 64

struct WifiNet {
    char hex[13];
    char ssid[65];
    int  rssi;
    int  channel;
};

static WifiNet  nets[MAX_NETS];
static int      netCount  = 0;
static uint32_t scanMsgs  = 0;
static bool     scanning  = false;

// ── GPS / Satellite data (posted by phone via /gps) ────────────────────────
struct GpsInfo {
    double lat       = 0.0;
    double lon       = 0.0;
    float  accuracy  = 0.0f;
    float  altitude  = 0.0f;
    float  speed     = 0.0f;
    float  bearing   = 0.0f;
    int    sats      = 0;
    bool   valid     = false;
    unsigned long updatedAt = 0;
};
static GpsInfo gpsInfo;

// ── OLED + menu ────────────────────────────────────────────────────────────
SSD1306Wire oled(0x3c, 700000, PIN_OLED_SDA, PIN_OLED_SCL, GEOMETRY_128_64, PIN_OLED_RST);
bool oledOk = false;

#define PAGE_MAIN  0
#define PAGE_SAT   1
#define PAGE_NETS  2
#define NUM_PAGES  3

int  currentPage  = PAGE_MAIN;
bool lastBtnState = HIGH;

int   battPct      = 0;
bool  otgConnected = false;

unsigned long lastSofSeen    = 0;
unsigned long lastBatSend    = 0;
unsigned long lastOledUpdate = 0;
unsigned long lastScanStart  = 0;
unsigned long lastOledRetry  = 0;
unsigned long lastBtnMs      = 0;

// ── JSON string escape ─────────────────────────────────────────────────────
static String jsonEsc(const char *s) {
    String out;
    out.reserve(strlen(s) + 4);
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c < 0x20)  {}
        else                out += (char)c;
    }
    return out;
}

// ── Build aircraft.json body ───────────────────────────────────────────────
static String buildAircraftJson() {
    String json;
    json.reserve(128 + netCount * 110);
    json  = "{\"now\":";
    json += String((double)millis() / 1000.0, 1);
    json += ",\"messages\":";
    json += String(scanMsgs);
    json += ",\"aircraft\":[";
    for (int i = 0; i < netCount; i++) {
        if (i > 0) json += ",";
        json += "{\"hex\":\"";      json += nets[i].hex;
        json += "\",\"flight\":\""; json += jsonEsc(nets[i].ssid);
        json += "\",\"alt_baro\":"; json += nets[i].rssi;
        json += ",\"gs\":";         json += nets[i].channel;
        json += ",\"track\":0,\"lat\":0.0,\"lon\":0.0,\"messages\":1,\"seen\":0}";
    }
    json += "]}";
    return json;
}

// ── HTTP handlers ──────────────────────────────────────────────────────────
static void sendJson(const String &body) {
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    httpServer.sendHeader("Cache-Control", "no-cache");
    httpServer.send(200, "application/json", body);
}

void handleAircraftJson() { sendJson(buildAircraftJson()); }
void handleSignalsJson()   { sendJson(buildAircraftJson()); }
void handleFromRadio()     { sendJson("[]"); }
void handleBattery()       { sendJson("{\"level\":" + String(battPct) + ",\"voltage\":" + String(battVoltage, 2) + "}"); }
void handleNotFound()      { httpServer.send(404, "text/plain", "Not found"); }

// Simple float extractor from JSON string (no library needed)
static float jsonFloat(const String &body, const char *key) {
    int idx = body.indexOf(key);
    if (idx < 0) return 0.0f;
    idx = body.indexOf(':', idx);
    if (idx < 0) return 0.0f;
    idx++;
    while (idx < (int)body.length() && body[idx] == ' ') idx++;
    return body.substring(idx).toFloat();
}

void handleGpsPost() {
    if (httpServer.method() != HTTP_POST) {
        httpServer.send(405, "text/plain", "POST only");
        return;
    }
    String body = httpServer.arg("plain");
    gpsInfo.lat      = (double)jsonFloat(body, "\"lat\"");
    gpsInfo.lon      = (double)jsonFloat(body, "\"lon\"");
    gpsInfo.accuracy = jsonFloat(body, "\"acc\"");
    gpsInfo.altitude = jsonFloat(body, "\"alt\"");
    gpsInfo.speed    = jsonFloat(body, "\"spd\"");
    gpsInfo.bearing  = jsonFloat(body, "\"brg\"");
    gpsInfo.sats     = (int)jsonFloat(body, "\"sats\"");
    gpsInfo.valid    = true;
    gpsInfo.updatedAt = millis();
    sendJson("{\"ok\":true}");
}

// ── I2C recovery ───────────────────────────────────────────────────────────
void i2cRecover() {
    pinMode(PIN_OLED_SCL, OUTPUT);
    pinMode(PIN_OLED_SDA, OUTPUT);
    for (int i = 0; i < 9; i++) {
        digitalWrite(PIN_OLED_SCL, HIGH); delayMicroseconds(5);
        digitalWrite(PIN_OLED_SCL, LOW);  delayMicroseconds(5);
    }
    digitalWrite(PIN_OLED_SDA, LOW);  delayMicroseconds(5);
    digitalWrite(PIN_OLED_SCL, HIGH); delayMicroseconds(5);
    digitalWrite(PIN_OLED_SDA, HIGH); delayMicroseconds(5);
}

// ── OLED page draws ────────────────────────────────────────────────────────
static void drawHeader(const char *title) {
    oled.drawString(0, 0, title);
    oled.drawLine(0, 11, 127, 11);
}

static void drawPageMain() {
    drawHeader("Wardriving Halo");
    oled.drawString(0, 14, "Bat: " + String(battPct) + "% (" + String(battVoltage, 2) + "V)");
    oled.drawRect(0, 27, 100, 8);
    int barW = (battPct * 98) / 100;
    if (barW > 0) oled.fillRect(1, 28, barW, 6);
    String apLine = "AP: " AP_SSID;
    if (otgConnected) apLine += " OTG";
    oled.drawString(0, 38, apLine);
    oled.drawString(0, 51, String(netCount) + " nets | " + String(scanMsgs) + " seen");
}

static void drawPageSat() {
    drawHeader("-- Satellite --");
    if (!gpsInfo.valid) {
        oled.drawString(0, 20, "No GPS data yet.");
        oled.drawString(0, 34, "Open app & scan.");
        return;
    }
    unsigned long age = (millis() - gpsInfo.updatedAt) / 1000;
    char buf[32];
    snprintf(buf, sizeof(buf), "Lat: %.5f", gpsInfo.lat);
    oled.drawString(0, 14, buf);
    snprintf(buf, sizeof(buf), "Lon: %.5f", gpsInfo.lon);
    oled.drawString(0, 25, buf);
    snprintf(buf, sizeof(buf), "Sats:%d  Acc:%.0fm", gpsInfo.sats, gpsInfo.accuracy);
    oled.drawString(0, 36, buf);
    snprintf(buf, sizeof(buf), "Alt:%.0fm  %lus ago", gpsInfo.altitude, age);
    oled.drawString(0, 47, buf);
}

static void drawPageNets() {
    drawHeader("-- Networks --");
    if (netCount == 0) {
        oled.drawString(0, 24, "Scanning...");
        return;
    }
    int show = min(netCount, 4);
    for (int i = 0; i < show; i++) {
        char ssid[14];
        snprintf(ssid, sizeof(ssid), "%-13s", nets[i].ssid);
        char line[22];
        snprintf(line, sizeof(line), "%s%4d", ssid, nets[i].rssi);
        oled.drawString(0, 14 + i * 13, line);
    }
}

void drawOled() {
    if (!oledOk) return;
    oled.displayOn();
    oled.clear();
    oled.setFont(ArialMT_Plain_10);
    oled.setTextAlignment(TEXT_ALIGN_LEFT);

    switch (currentPage) {
        case PAGE_SAT:  drawPageSat();  break;
        case PAGE_NETS: drawPageNets(); break;
        default:        drawPageMain(); break;
    }

    oled.display();
}

// ── setup ──────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(100);

    pinMode(PIN_BTN, INPUT_PULLUP);

    pinMode(PIN_VEXT, OUTPUT);
    digitalWrite(PIN_VEXT, LOW);
    delay(500);

    pinMode(PIN_OLED_RST, OUTPUT);
    digitalWrite(PIN_OLED_RST, LOW); delay(50);
    digitalWrite(PIN_OLED_RST, HIGH); delay(50);

    i2cRecover();
    delay(100);

    oledOk = oled.init();
    if (oledOk) {
        oled.flipScreenVertically();
        oled.clear();
        oled.setFont(ArialMT_Plain_10);
        oled.drawString(0, 0, "Wardriving Halo");
        oled.drawString(0, 13, "Starting AP...");
        oled.display();
    }

    battPct = readBattPct();
    char buf[40];
    snprintf(buf, sizeof(buf), "BAT:%d\n", battPct);
    usjWrite(buf);
    usjWrite("=== Wardriving Halo ===\n");
    lastBatSend = millis();

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);
    IPAddress ip = WiFi.softAPIP();

    if (oledOk) {
        oled.clear();
        oled.drawString(0,  0, "Wardriving Halo");
        oled.drawString(0, 13, "AP: " AP_SSID);
        oled.drawString(0, 26, "IP: " + ip.toString());
        oled.drawString(0, 40, "Scanning WiFi...");
        oled.display();
    }

    char ipbuf[48];
    snprintf(ipbuf, sizeof(ipbuf), "AP IP: %s\n", ip.toString().c_str());
    usjWrite(ipbuf);

    httpServer.on("/data/aircraft.json", HTTP_GET,  handleAircraftJson);
    httpServer.on("/data/signals.json",  HTTP_GET,  handleSignalsJson);
    httpServer.on("/json/fromradio",     HTTP_GET,  handleFromRadio);
    httpServer.on("/battery",            HTTP_GET,  handleBattery);
    httpServer.on("/gps",                HTTP_POST, handleGpsPost);
    httpServer.onNotFound(handleNotFound);
    httpServer.begin();

    WiFi.scanNetworks(true, true);
    scanning = true;
    lastScanStart = millis();
}

// ── loop ───────────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();

    // SOF fires every 1ms when a USB host is connected
    if (USJ_INT_RAW_REG & USJ_SOF_INT) {
        USJ_INT_CLR_REG = USJ_SOF_INT;
        lastSofSeen = now;
    }
    otgConnected = (now - lastSofSeen) < 3000UL;

    // PRG button — cycle pages on press with 200ms debounce
    bool btnNow = digitalRead(PIN_BTN);
    if (lastBtnState == HIGH && btnNow == LOW && now - lastBtnMs > 200UL) {
        lastBtnMs = now;
        currentPage = (currentPage + 1) % NUM_PAGES;
        lastOledUpdate = 0;  // force immediate redraw
    }
    lastBtnState = btnNow;

    battPct = readBattPct();

    if (now - lastBatSend >= 3000UL) {
        lastBatSend = now;
        char buf[16];
        snprintf(buf, sizeof(buf), "BAT:%d\n", battPct);
        usjWrite(buf);
    }

    // Check async scan result
    if (scanning) {
        int result = WiFi.scanComplete();
        if (result >= 0) {
            int n = min(result, MAX_NETS);
            netCount = n;
            for (int i = 0; i < n; i++) {
                uint8_t *mac = WiFi.BSSID(i);
                snprintf(nets[i].hex, sizeof(nets[i].hex),
                         "%02x%02x%02x%02x%02x%02x",
                         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                String ssid = WiFi.SSID(i);
                strlcpy(nets[i].ssid, ssid.c_str(), sizeof(nets[i].ssid));
                nets[i].rssi    = WiFi.RSSI(i);
                nets[i].channel = WiFi.channel(i);
                scanMsgs++;
            }
            WiFi.scanDelete();
            scanning = false;

            char dbg[32];
            snprintf(dbg, sizeof(dbg), "SCAN:%d\n", netCount);
            usjWrite(dbg);
        } else if (result == WIFI_SCAN_FAILED) {
            WiFi.scanDelete();
            scanning = false;
        }
    }

    if (!scanning && now - lastScanStart >= 10000UL) {
        lastScanStart = now;
        WiFi.scanNetworks(true, true);
        scanning = true;
    }

    httpServer.handleClient();

    // Retry OLED init every 5s if boot-time init failed
    if (!oledOk && now - lastOledRetry >= 5000UL) {
        lastOledRetry = now;
        i2cRecover();
        delay(50);
        oledOk = oled.init();
        if (oledOk) oled.flipScreenVertically();
    }

    // Redraw every 2s
    if (now - lastOledUpdate >= 2000UL) {
        lastOledUpdate = now;
        drawOled();
    }
}
