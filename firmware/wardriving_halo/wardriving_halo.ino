/*
 * Wardriving Halo — ESP32-S3  (Heltec WiFi LoRa 32 V3)
 *
 * Transports:
 *   WiFi AP  "WardrivingHalo" — HTTP API + hotspot for the Android app
 *   BLE      "WardrivingHalo" — Nordic UART Service (NUS) for any BLE terminal
 *   LoRa     SX1262 @ 915 MHz — raw LoRa packet mesh
 *
 * Messages received on any transport are relayed to the other two.
 *
 * HTTP endpoints:
 *   GET  /data/aircraft.json   — WiFi scan results (dump1090 format)
 *   GET  /data/signals.json    — same
 *   GET  /json/fromradio       — [] (Meshtastic compat stub)
 *   GET  /battery              — {"level":N,"voltage":V}
 *   POST /gps                  — receive GPS from phone
 *   GET  /msg                  — JSON array of recent comms messages
 *   POST /msg                  — {"text":"..."} relay to BLE + LoRa
 *
 * OLED pages (PRG button cycles):
 *   0 Main  1 Satellite  2 Networks  3 Comms
 */

// ── USB JTAG TX FIFO ───────────────────────────────────────────────────────
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

// ── Includes ───────────────────────────────────────────────────────────────
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <SPI.h>
#include <RadioLib.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ── Hardware pins ──────────────────────────────────────────────────────────
#define PIN_VEXT         36
#define PIN_OLED_SDA     17
#define PIN_OLED_SCL     18
#define PIN_OLED_RST     21
#define PIN_BATT_ADC      1
#define PIN_BATT_CTRL    37
#define PIN_BTN           0   // PRG button, active LOW

// LoRa SX1262 (Heltec WiFi LoRa 32 V3 pinout)
#define PIN_LORA_NSS      8
#define PIN_LORA_DIO1    14
#define PIN_LORA_RST     12
#define PIN_LORA_BUSY    13
#define PIN_LORA_SCK      9
#define PIN_LORA_MISO    11
#define PIN_LORA_MOSI    10

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

// ── Message queue (shared across all transports) ───────────────────────────
#define MSG_MAX  20
#define MSG_LEN  120

struct Msg {
    char text[MSG_LEN];
    char via[6];        // "WIFI", "BLE", "LORA"
    unsigned long ts;
};

static Msg   msgBuf[MSG_MAX];
static int   msgHead  = 0;
static int   msgCount = 0;

static portMUX_TYPE msgMux = portMUX_INITIALIZER_UNLOCKED;

// Forward declarations for relay
static void loraSendRaw(const char *text);
static void bleSend(const char *text);

static void addMessage(const char *text, const char *via) {
    portENTER_CRITICAL(&msgMux);
    int idx = msgHead;
    strlcpy(msgBuf[idx].text, text, MSG_LEN);
    strlcpy(msgBuf[idx].via,  via,  sizeof(msgBuf[idx].via));
    msgBuf[idx].ts = millis();
    msgHead = (msgHead + 1) % MSG_MAX;
    if (msgCount < MSG_MAX) msgCount++;
    portEXIT_CRITICAL(&msgMux);

    if (strcmp(via, "BLE")  != 0) bleSend(text);
    if (strcmp(via, "LORA") != 0) loraSendRaw(text);
}

// ── LoRa SX1262 ────────────────────────────────────────────────────────────
SPIClass loraSPI(FSPI);
SX1262 radio = new Module(PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RST, PIN_LORA_BUSY, loraSPI);

volatile bool loraRxFlag = false;
volatile bool loraTxBusy = false;
bool          loraOk     = false;

void IRAM_ATTR onLoRaDio1() { loraRxFlag = true; }

static void loraStartRx() {
    int s = radio.startReceive();
    if (s != RADIOLIB_ERR_NONE) {
        char b[32]; snprintf(b, sizeof(b), "LoRa RX err %d\n", s);
        usjWrite(b);
    }
}

static void loraSendRaw(const char *text) {
    if (!loraOk || loraTxBusy) return;
    loraTxBusy = true;
    radio.clearDio1Action();
    radio.transmit((uint8_t *)text, strlen(text));
    loraRxFlag = false;
    radio.setDio1Action(onLoRaDio1);
    loraStartRx();
    loraTxBusy = false;
}

// ── BLE Nordic UART Service ────────────────────────────────────────────────
#define BLE_SVC_UUID  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_RX_UUID   "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_TX_UUID   "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer         *pBLEServer = nullptr;
BLECharacteristic *pBLETx    = nullptr;
bool               bleConnected    = false;
bool               bleWasConnected = false;

class BLESrvCB : public BLEServerCallbacks {
    void onConnect(BLEServer *)    override { bleConnected = true;  }
    void onDisconnect(BLEServer *) override { bleConnected = false; }
};

class BLERxCB : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *c) override {
        String val = c->getValue().c_str();
        if (val.length() > 0) addMessage(val.c_str(), "BLE");
    }
};

static void bleSend(const char *text) {
    if (!pBLETx || !bleConnected) return;
    pBLETx->setValue((uint8_t *)text, strlen(text));
    pBLETx->notify();
}

static void bleInit() {
    BLEDevice::init("WardrivingHalo");
    pBLEServer = BLEDevice::createServer();
    pBLEServer->setCallbacks(new BLESrvCB());

    BLEService *svc = pBLEServer->createService(BLE_SVC_UUID);

    pBLETx = svc->createCharacteristic(BLE_TX_UUID,
                 BLECharacteristic::PROPERTY_NOTIFY);
    pBLETx->addDescriptor(new BLE2902());

    BLECharacteristic *pRx = svc->createCharacteristic(BLE_RX_UUID,
                 BLECharacteristic::PROPERTY_WRITE |
                 BLECharacteristic::PROPERTY_WRITE_NR);
    pRx->setCallbacks(new BLERxCB());

    svc->start();

    BLEAdvertising *adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(BLE_SVC_UUID);
    adv->setScanResponse(true);
    adv->setMinPreferred(0x06);
    BLEDevice::startAdvertising();
    usjWrite("BLE ready\n");
}

// ── WiFi AP + HTTP ─────────────────────────────────────────────────────────
#define AP_SSID "WardrivingHalo"
#define AP_PASS "wardhalo1"

WebServer httpServer(80);

#define MAX_NETS 64
struct WifiNet { char hex[13]; char ssid[65]; int rssi; int channel; };
static WifiNet  nets[MAX_NETS];
static int      netCount  = 0;
static uint32_t scanMsgs  = 0;
static bool     scanning  = false;

struct GpsInfo {
    double lat = 0.0; double lon = 0.0;
    float  acc = 0.0f; float alt = 0.0f;
    float  spd = 0.0f; float brg = 0.0f;
    int    sats = 0;
    bool   valid = false;
    unsigned long updatedAt = 0;
};
static GpsInfo gpsInfo;

static String jsonEsc(const char *s) {
    String o; o.reserve(strlen(s) + 4);
    for (; *s; s++) {
        unsigned char c = *s;
        if      (c == '"')  o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else if (c == '\r') o += "\\r";
        else if (c >= 0x20) o += (char)c;
    }
    return o;
}

static String buildAircraftJson() {
    String j; j.reserve(128 + netCount * 110);
    j  = "{\"now\":";  j += String((double)millis()/1000.0, 1);
    j += ",\"messages\":"; j += scanMsgs;
    j += ",\"aircraft\":[";
    for (int i = 0; i < netCount; i++) {
        if (i) j += ",";
        j += "{\"hex\":\"";      j += nets[i].hex;
        j += "\",\"flight\":\""; j += jsonEsc(nets[i].ssid);
        j += "\",\"alt_baro\":"; j += nets[i].rssi;
        j += ",\"gs\":";         j += nets[i].channel;
        j += ",\"track\":0,\"lat\":0.0,\"lon\":0.0,\"messages\":1,\"seen\":0}";
    }
    j += "]}"; return j;
}

static void sendJson(const String &body) {
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    httpServer.sendHeader("Cache-Control", "no-cache");
    httpServer.send(200, "application/json", body);
}

static float jsonFloat(const String &b, const char *key) {
    int i = b.indexOf(key); if (i < 0) return 0.0f;
    i = b.indexOf(':', i);  if (i < 0) return 0.0f;
    i++;
    while (i < (int)b.length() && b[i] == ' ') i++;
    return b.substring(i).toFloat();
}

void handleAircraftJson() { sendJson(buildAircraftJson()); }
void handleSignalsJson()   { sendJson(buildAircraftJson()); }
void handleFromRadio()     { sendJson("[]"); }
void handleBattery()       { sendJson("{\"level\":" + String(readBattPct()) + ",\"voltage\":" + String(battVoltage,2) + "}"); }
void handleNotFound()      { httpServer.send(404, "text/plain", "Not found"); }

void handleGpsPost() {
    if (httpServer.method() != HTTP_POST) { httpServer.send(405,"text/plain","POST only"); return; }
    String b = httpServer.arg("plain");
    gpsInfo.lat  = (double)jsonFloat(b,"\"lat\"");
    gpsInfo.lon  = (double)jsonFloat(b,"\"lon\"");
    gpsInfo.acc  = jsonFloat(b,"\"acc\"");
    gpsInfo.alt  = jsonFloat(b,"\"alt\"");
    gpsInfo.spd  = jsonFloat(b,"\"spd\"");
    gpsInfo.brg  = jsonFloat(b,"\"brg\"");
    gpsInfo.sats = (int)jsonFloat(b,"\"sats\"");
    gpsInfo.valid = true; gpsInfo.updatedAt = millis();
    sendJson("{\"ok\":true}");
}

void handleMsgGet() {
    String j = "[";
    portENTER_CRITICAL(&msgMux);
    int start = (msgCount < MSG_MAX) ? 0 : msgHead;
    int cnt   = msgCount;
    portEXIT_CRITICAL(&msgMux);
    for (int i = 0; i < cnt; i++) {
        int idx = (start + i) % MSG_MAX;
        if (i) j += ",";
        j += "{\"text\":\""; j += jsonEsc(msgBuf[idx].text);
        j += "\",\"via\":\""; j += msgBuf[idx].via;
        j += "\",\"ts\":"; j += msgBuf[idx].ts; j += "}";
    }
    j += "]";
    sendJson(j);
}

void handleMsgPost() {
    if (httpServer.method() != HTTP_POST) { httpServer.send(405,"text/plain","POST only"); return; }
    String b = httpServer.arg("plain");
    String text;
    int ti = b.indexOf("\"text\"");
    if (ti >= 0) {
        ti = b.indexOf('"', ti + 6) + 1;
        int te = b.indexOf('"', ti);
        text = b.substring(ti, te);
    } else {
        text = b;
    }
    text.trim();
    if (text.length() == 0) { httpServer.send(400,"text/plain","empty"); return; }
    addMessage(text.c_str(), "WIFI");
    sendJson("{\"ok\":true}");
}

// ── OLED ───────────────────────────────────────────────────────────────────
SSD1306Wire oled(0x3c, 700000, PIN_OLED_SDA, PIN_OLED_SCL, GEOMETRY_128_64, PIN_OLED_RST);
bool oledOk = false;

#define PAGE_MAIN   0
#define PAGE_SAT    1
#define PAGE_NETS   2
#define PAGE_COMMS  3
#define NUM_PAGES   4

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

static void drawHeader(const char *t) {
    oled.drawString(0, 0, t);
    oled.drawLine(0, 11, 127, 11);
}

static void drawPageMain() {
    drawHeader("Wardriving Halo");
    oled.drawString(0, 14, "Bat: " + String(battPct) + "% (" + String(battVoltage,2) + "V)");
    oled.drawRect(0, 27, 100, 8);
    int bw = (battPct * 98) / 100;
    if (bw > 0) oled.fillRect(1, 28, bw, 6);
    String ap = "AP: " AP_SSID; if (otgConnected) ap += " OTG";
    oled.drawString(0, 38, ap);
    oled.drawString(0, 51, String(netCount) + " nets | " + String(scanMsgs) + " scanned");
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
    snprintf(buf, sizeof(buf), "Lat: %.5f", gpsInfo.lat);      oled.drawString(0, 14, buf);
    snprintf(buf, sizeof(buf), "Lon: %.5f", gpsInfo.lon);      oled.drawString(0, 25, buf);
    snprintf(buf, sizeof(buf), "Sats:%d  Acc:%.0fm", gpsInfo.sats, gpsInfo.acc); oled.drawString(0, 36, buf);
    snprintf(buf, sizeof(buf), "Alt:%.0fm  %lus ago", gpsInfo.alt, age);          oled.drawString(0, 47, buf);
}

static void drawPageNets() {
    drawHeader("-- Networks --");
    if (netCount == 0) { oled.drawString(0, 24, "Scanning..."); return; }
    int show = min(netCount, 4);
    for (int i = 0; i < show; i++) {
        char line[22];
        snprintf(line, sizeof(line), "%-13s%4d", nets[i].ssid, nets[i].rssi);
        oled.drawString(0, 14 + i * 13, line);
    }
}

static void drawPageComms() {
    drawHeader("-- Comms --");
    oled.drawString(0, 14, bleConnected ? "BLE: Connected  " : "BLE: Advertising");
    oled.drawString(0, 25, loraOk      ? "LoRa:915MHz SF9 " : "LoRa: init fail ");
    portENTER_CRITICAL(&msgMux);
    int cnt     = msgCount;
    int lastIdx = (msgHead - 1 + MSG_MAX) % MSG_MAX;
    portEXIT_CRITICAL(&msgMux);
    if (cnt > 0) {
        char preview[22];
        snprintf(preview, sizeof(preview), "[%s]%.15s", msgBuf[lastIdx].via, msgBuf[lastIdx].text);
        oled.drawString(0, 38, preview);
        oled.drawString(0, 51, String(cnt) + " message(s)");
    } else {
        oled.drawString(0, 38, "No messages yet.");
        oled.drawString(0, 51, "Waiting...");
    }
}

void drawOled() {
    if (!oledOk) return;
    oled.displayOn();
    oled.clear();
    oled.setFont(ArialMT_Plain_10);
    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    switch (currentPage) {
        case PAGE_SAT:   drawPageSat();   break;
        case PAGE_NETS:  drawPageNets();  break;
        case PAGE_COMMS: drawPageComms(); break;
        default:         drawPageMain();  break;
    }
    oled.display();
}

// ── I2C recovery ───────────────────────────────────────────────────────────
void i2cRecover() {
    pinMode(PIN_OLED_SCL, OUTPUT); pinMode(PIN_OLED_SDA, OUTPUT);
    for (int i = 0; i < 9; i++) {
        digitalWrite(PIN_OLED_SCL, HIGH); delayMicroseconds(5);
        digitalWrite(PIN_OLED_SCL, LOW);  delayMicroseconds(5);
    }
    digitalWrite(PIN_OLED_SDA, LOW);  delayMicroseconds(5);
    digitalWrite(PIN_OLED_SCL, HIGH); delayMicroseconds(5);
    digitalWrite(PIN_OLED_SDA, HIGH); delayMicroseconds(5);
}

// ── setup ──────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(100);
    pinMode(PIN_BTN, INPUT_PULLUP);

    // Power OLED
    pinMode(PIN_VEXT, OUTPUT);
    digitalWrite(PIN_VEXT, LOW);
    delay(500);

    // Reset OLED
    pinMode(PIN_OLED_RST, OUTPUT);
    digitalWrite(PIN_OLED_RST, LOW); delay(50);
    digitalWrite(PIN_OLED_RST, HIGH); delay(50);
    i2cRecover(); delay(100);

    oledOk = oled.init();
    if (oledOk) {
        oled.flipScreenVertically();
        oled.clear();
        oled.setFont(ArialMT_Plain_10);
        oled.drawString(0, 0,  "Wardriving Halo");
        oled.drawString(0, 13, "Starting up...");
        oled.display();
    }

    // ── LoRa SX1262 init ────────────────────────────────────────────────
    loraSPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_NSS);
    delay(100);
    // begin(freq MHz, bw kHz, sf, cr, syncWord, power dBm, preamble, tcxoVoltage V)
    int loraState = radio.begin(915.0, 125.0, 9, 7, 0x12, 14, 8, 1.8);
    if (loraState == RADIOLIB_ERR_NONE) {
        loraOk = true;
        radio.setDio1Action(onLoRaDio1);
        loraStartRx();
        usjWrite("LoRa OK 915MHz SF9\n");
    } else {
        char b[40]; snprintf(b, sizeof(b), "LoRa FAIL err=%d\n", loraState);
        usjWrite(b);
    }

    // ── BLE NUS init ─────────────────────────────────────────────────────
    bleInit();

    // ── Battery initial read ──────────────────────────────────────────────
    battPct = readBattPct();
    char batbuf[20]; snprintf(batbuf, sizeof(batbuf), "BAT:%d\n", battPct);
    usjWrite(batbuf);
    lastBatSend = millis();

    // ── WiFi AP ───────────────────────────────────────────────────────────
    if (oledOk) {
        oled.clear();
        oled.drawString(0, 0,  "Wardriving Halo");
        oled.drawString(0, 13, "Starting WiFi AP...");
        oled.display();
    }
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);
    IPAddress ip = WiFi.softAPIP();
    char ipbuf[48]; snprintf(ipbuf, sizeof(ipbuf), "AP: %s\n", ip.toString().c_str());
    usjWrite(ipbuf);

    // ── HTTP routes ───────────────────────────────────────────────────────
    httpServer.on("/data/aircraft.json", HTTP_GET,  handleAircraftJson);
    httpServer.on("/data/signals.json",  HTTP_GET,  handleSignalsJson);
    httpServer.on("/json/fromradio",     HTTP_GET,  handleFromRadio);
    httpServer.on("/battery",            HTTP_GET,  handleBattery);
    httpServer.on("/gps",                HTTP_POST, handleGpsPost);
    httpServer.on("/msg",                HTTP_GET,  handleMsgGet);
    httpServer.on("/msg",                HTTP_POST, handleMsgPost);
    httpServer.onNotFound(handleNotFound);
    httpServer.begin();

    // ── Start WiFi scan ───────────────────────────────────────────────────
    WiFi.scanNetworks(true, true);
    scanning = true; lastScanStart = millis();

    // ── Boot splash ───────────────────────────────────────────────────────
    if (oledOk) {
        oled.clear();
        oled.drawString(0,  0, "Wardriving Halo");
        oled.drawString(0, 13, "AP: " AP_SSID);
        oled.drawString(0, 26, ip.toString());
        oled.drawString(0, 39, loraOk ? "LoRa:915MHz OK"  : "LoRa:init fail");
        oled.drawString(0, 52, "BLE: advertising");
        oled.display();
        delay(2000);
    }
}

// ── loop ───────────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();

    // USB SOF / OTG presence
    if (USJ_INT_RAW_REG & USJ_SOF_INT) { USJ_INT_CLR_REG = USJ_SOF_INT; lastSofSeen = now; }
    otgConnected = (now - lastSofSeen) < 3000UL;

    // PRG button — cycle OLED page
    bool btnNow = digitalRead(PIN_BTN);
    if (lastBtnState == HIGH && btnNow == LOW && now - lastBtnMs > 200UL) {
        lastBtnMs = now;
        currentPage = (currentPage + 1) % NUM_PAGES;
        lastOledUpdate = 0;
    }
    lastBtnState = btnNow;

    // Battery
    if (now - lastBatSend >= 3000UL) {
        lastBatSend = now;
        battPct = readBattPct();
        char b[16]; snprintf(b, sizeof(b), "BAT:%d\n", battPct);
        usjWrite(b);
    }

    // ── LoRa RX ───────────────────────────────────────────────────────────
    if (loraOk && loraRxFlag && !loraTxBusy) {
        loraRxFlag = false;
        String pkt;
        int state = radio.readData(pkt);
        if (state == RADIOLIB_ERR_NONE && pkt.length() > 0) {
            char buf[MSG_LEN];
            pkt.toCharArray(buf, MSG_LEN - 1);
            buf[MSG_LEN - 1] = '\0';
            addMessage(buf, "LORA");
            char dbg[MSG_LEN + 16];
            snprintf(dbg, sizeof(dbg), "LoRa RX: %s\n", buf);
            usjWrite(dbg);
        }
        loraStartRx();
    }

    // ── BLE re-advertise after disconnect ─────────────────────────────────
    if (bleWasConnected && !bleConnected) {
        bleWasConnected = false;
        delay(100);
        BLEDevice::startAdvertising();
    }
    if (bleConnected) bleWasConnected = true;

    // ── WiFi scan results ─────────────────────────────────────────────────
    if (scanning) {
        int r = WiFi.scanComplete();
        if (r >= 0) {
            int n = min(r, MAX_NETS); netCount = n;
            for (int i = 0; i < n; i++) {
                uint8_t *mac = WiFi.BSSID(i);
                snprintf(nets[i].hex, sizeof(nets[i].hex),
                         "%02x%02x%02x%02x%02x%02x",
                         mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
                strlcpy(nets[i].ssid, WiFi.SSID(i).c_str(), sizeof(nets[i].ssid));
                nets[i].rssi = WiFi.RSSI(i); nets[i].channel = WiFi.channel(i);
                scanMsgs++;
            }
            WiFi.scanDelete(); scanning = false;
            char dbg[24]; snprintf(dbg, sizeof(dbg), "SCAN:%d\n", netCount);
            usjWrite(dbg);
        } else if (r == WIFI_SCAN_FAILED) {
            WiFi.scanDelete(); scanning = false;
        }
    }
    if (!scanning && now - lastScanStart >= 10000UL) {
        lastScanStart = now;
        WiFi.scanNetworks(true, true);
        scanning = true;
    }

    httpServer.handleClient();

    // OLED retry every 5s if init failed
    if (!oledOk && now - lastOledRetry >= 5000UL) {
        lastOledRetry = now;
        i2cRecover(); delay(50);
        oledOk = oled.init();
        if (oledOk) oled.flipScreenVertically();
    }

    // Redraw OLED every 2s
    if (now - lastOledUpdate >= 2000UL) {
        lastOledUpdate = now;
        drawOled();
    }
}
