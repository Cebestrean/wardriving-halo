/*
 * Wardriving Halo — ESP32-S3
 * WiFi AP hotspot + HTTP wardriving server for WiFi Halo app.
 * - Scans WiFi networks every 10s and serves results as dump1090 aircraft.json
 * - /data/aircraft.json  — WiFi scan results (dump1090 format)
 * - /data/signals.json   — same data for signal view
 * - /json/fromradio      — empty Meshtastic packet array []
 * - /battery             — {"level":N,"voltage":0.0}
 * USB CDC BAT: output maintained for PC monitoring.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"

// ── USB JTAG TX FIFO — direct register access ──────────────────────────────
// Bypasses usb_serial_jtag_is_connected() which always returns false in the
// Arduino boot path (IDF driver context never initialized).
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
#define PIN_VEXT      36
#define PIN_OLED_SDA  17
#define PIN_OLED_SCL  18
#define PIN_OLED_RST  21
#define PIN_BATT_ADC   1

// ── Battery ────────────────────────────────────────────────────────────────
#define BATT_RATIO   4.9f
#define BATT_FULL    4.20f
#define BATT_EMPTY   3.00f

int readBattPct() {
    analogSetPinAttenuation(PIN_BATT_ADC, ADC_11db);
    int raw = analogRead(PIN_BATT_ADC);
    float v_adc  = raw * 3.3f / 4095.0f;
    float v_batt = v_adc * BATT_RATIO;
    if (v_batt < 0.5f) return 0;
    int pct = (int)((v_batt - BATT_EMPTY) / (BATT_FULL - BATT_EMPTY) * 100.0f);
    return constrain(pct, 0, 100);
}

// ── WiFi AP ────────────────────────────────────────────────────────────────
#define AP_SSID "WardrivingHalo"
#define AP_PASS "wardhalo1"

WebServer httpServer(80);

// ── Scan data ──────────────────────────────────────────────────────────────
#define MAX_NETS 64

struct WifiNet {
    char hex[13];   // 12 hex chars + null
    char ssid[65];  // SSID + null
    int  rssi;
    int  channel;
};

static WifiNet  nets[MAX_NETS];
static int      netCount  = 0;
static uint32_t scanMsgs  = 0;
static bool     scanning  = false;

// ── OLED ───────────────────────────────────────────────────────────────────
SSD1306Wire oled(0x3c, 700000, PIN_OLED_SDA, PIN_OLED_SCL, GEOMETRY_128_64, PIN_OLED_RST);
bool oledOk = false;

int  battPct      = 0;
bool otgConnected = false;

unsigned long lastSofSeen    = 0;
unsigned long lastBatSend    = 0;
unsigned long lastOledUpdate = 0;
unsigned long lastScanStart  = 0;

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
        json += "{\"hex\":\"";    json += nets[i].hex;
        json += "\",\"flight\":\""; json += jsonEsc(nets[i].ssid);
        json += "\",\"alt_baro\":"; json += nets[i].rssi;
        json += ",\"gs\":";       json += nets[i].channel;
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
void handleBattery()       { sendJson("{\"level\":" + String(battPct) + ",\"voltage\":0.0}"); }
void handleNotFound()      { httpServer.send(404, "text/plain", "Not found"); }

// ── I2C recovery + OLED ────────────────────────────────────────────────────
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

void drawOled() {
    if (!oledOk) return;
    oled.clear();
    oled.setFont(ArialMT_Plain_10);
    oled.setTextAlignment(TEXT_ALIGN_LEFT);

    oled.drawString(0,  0, "Wardriving Halo");
    oled.drawString(0, 13, "Bat: " + String(battPct) + "%");

    oled.drawRect(0, 26, 100, 10);
    int barW = (battPct * 98) / 100;
    if (barW > 0) oled.fillRect(1, 27, barW, 8);

    String apLine = "AP: " AP_SSID;
    if (otgConnected) apLine += " OTG";
    oled.drawString(0, 40, apLine);
    oled.drawString(0, 52, String(netCount) + " nets | " + String(scanMsgs) + " seen");

    oled.display();
}

// ── setup ──────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(100);

    pinMode(PIN_VEXT, OUTPUT);
    digitalWrite(PIN_VEXT, LOW);
    delay(150);

    pinMode(PIN_OLED_RST, OUTPUT);
    digitalWrite(PIN_OLED_RST, LOW); delay(20);
    digitalWrite(PIN_OLED_RST, HIGH); delay(20);

    i2cRecover();
    delay(50);

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

    // AP+STA: AP hosts the phone; STA mode allows background WiFi scanning
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

    httpServer.on("/data/aircraft.json", HTTP_GET, handleAircraftJson);
    httpServer.on("/data/signals.json",  HTTP_GET, handleSignalsJson);
    httpServer.on("/json/fromradio",     HTTP_GET, handleFromRadio);
    httpServer.on("/battery",            HTTP_GET, handleBattery);
    httpServer.onNotFound(handleNotFound);
    httpServer.begin();

    WiFi.scanNetworks(/*async=*/true, /*show_hidden=*/true);
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
    bool wasConnected = otgConnected;
    otgConnected = (now - lastSofSeen) < 3000UL;

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

    // Rescan every 10s
    if (!scanning && (now - lastScanStart >= 10000UL)) {
        lastScanStart = now;
        WiFi.scanNetworks(true, true);
        scanning = true;
    }

    httpServer.handleClient();

    // Redraw OLED on OTG change or every 5s
    if (otgConnected != wasConnected || now - lastOledUpdate >= 5000UL) {
        lastOledUpdate = now;
        drawOled();
    }
}
