# Wardriving Halo

ESP32-S3 wardriving firmware + companion Android app for passive WiFi/Bluetooth network mapping.

## Hardware

**Board: Spec5 / Spectre 2 MK2**
- ESP32-S3 (QFN56 rev 0.2), 16 MB flash, native USB CDC
- SSD1306 OLED display (128×64, I2C)
- LiPo battery with ADC monitoring
- VID 0x303a / PID 0x1001 (Espressif native USB)

## How It Works

The ESP32 creates a WiFi access point that your phone connects to. The phone runs the **WiFi Halo** app which does wardriving (WiFi + Bluetooth scanning with GPS). The ESP32 simultaneously scans nearby WiFi networks and serves them to the app as ADS-B-style JSON over HTTP so they appear on the radar view.

```
Phone (WiFi Halo app)
  │  connects via WiFi
  ▼
ESP32 AP: WardrivingHalo
  │  HTTP 192.168.4.1
  ├── /data/aircraft.json   ← ESP32 WiFi scan results
  ├── /data/signals.json    ← same data, signals view
  ├── /json/fromradio       ← Meshtastic compat (empty [])
  └── /battery              ← {"level": N, "voltage": 0.0}
```

## WiFi AP Credentials

| Field    | Value            |
|----------|------------------|
| SSID     | WardrivingHalo   |
| Password | wardhalo1        |
| IP       | 192.168.4.1      |

## App Setup

1. Install `app/WardrivingHalo.apk` on your Android phone
2. In the app **Settings**:
   - **ADS-B Endpoint** → `http://192.168.4.1/data/aircraft.json`
   - **Meshtastic Node IP** → `192.168.4.1`
3. Connect your phone WiFi to **WardrivingHalo** (password: `wardhalo1`)
4. Open the app — wardriving starts automatically

## Flashing the Firmware

### Requirements
- Linux PC with Python 3, `esptool`, `pyusb`
- Arduino IDE / arduino-cli with ESP32 board support + Heltec ESP32 Dev-Boards library

### Compile
```bash
arduino-cli compile \
  --fqbn "esp32:esp32:heltec_wifi_lora_32_V3" \
  --build-property "build.cdc_on_boot=1" \
  --build-property "build.usb_mode=1" \
  --output-dir firmware/build \
  firmware/wardriving_halo/wardriving_halo.ino
```

### Flash (pre-built binaries included)
```bash
cd firmware
sudo bash flash.sh
```

The flash script handles the full sequence:
1. Puts device in download mode via USB vendor command
2. Flashes all 4 regions (bootloader, partitions, boot_app0, firmware)
3. Triggers watchdog reset to boot into user firmware
4. Verifies `BAT:` output on serial

### Serial Output
Connect at 115200 baud. You will see:
```
=== Wardriving Halo ===
AP IP: 192.168.4.1
BAT:0
SCAN:41
BAT:0
...
```
`BAT:0` = USB power only (no LiPo attached). `SCAN:N` = N networks found in last scan.

## OLED Display

| Line | Content |
|------|---------|
| 0    | Wardriving Halo |
| 1    | Bat: XX% |
| 2    | Battery bar |
| 3    | AP: WardrivingHalo [OTG if USB host connected] |
| 4    | N nets \| M seen |

## Pin Map (Spec5 / Spectre 2 MK2)

| Pin | Function |
|-----|----------|
| GPIO 1  | Battery ADC (divider 4.9:1) |
| GPIO 17 | OLED SDA |
| GPIO 18 | OLED SCL |
| GPIO 21 | OLED RST |
| GPIO 36 | Vext power (LOW = ON) |

## Notes

- OTG detection uses the USB SOF interrupt register (0x60038008 bit 1) — fires every 1ms when a USB host is connected
- USB serial TX uses direct FIFO writes to 0x60038000 (bypasses `usb_serial_jtag_is_connected()` which always returns false in the Arduino boot path)
- WiFi mode is `WIFI_AP_STA` — runs AP and background scan simultaneously
- Scan interval: 10 seconds, async (does not block the HTTP server)
- Max 64 networks stored per scan cycle
