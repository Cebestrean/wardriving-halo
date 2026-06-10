#!/bin/bash
# Flash Wardriving Halo firmware and boot into user firmware.
# Handles the full sequence: download mode → flash → watchdog boot.
set -e

BUILD="/home/w8str/wardriving_halo/build"
BOOT_APP0="/home/w8str/.arduino15/packages/esp32/hardware/esp32/3.3.10/tools/partitions/boot_app0.bin"

find_usb_path() {
    for d in /sys/bus/usb/devices/*/; do
        d="${d%/}"; d="${d##*/}"
        vid=$(cat /sys/bus/usb/devices/$d/idVendor 2>/dev/null)
        pid=$(cat /sys/bus/usb/devices/$d/idProduct 2>/dev/null)
        [ "$vid" = "303a" ] && [ "$pid" = "1001" ] && echo "$d" && return
    done
}

rebind() {
    local path=$(find_usb_path)
    [ -z "$path" ] && { echo "ESP32 not found on USB"; exit 1; }
    echo "${path}:1.0" | sudo tee /sys/bus/usb/drivers/cdc_acm/bind 2>/dev/null || true
    sleep 1
    ls /dev/ttyACM0 &>/dev/null || { echo "cdc_acm bind failed"; exit 1; }
}

echo "=== Step 1: entering download mode ==="
sudo python3 - << 'PY'
import usb.core, usb.util
dev = usb.core.find(idVendor=0x303a, idProduct=0x1001)
if not dev: print("Device not found"); exit(1)
for cfg in dev:
    for intf in cfg:
        try:
            if dev.is_kernel_driver_active(intf.bInterfaceNumber):
                dev.detach_kernel_driver(intf.bInterfaceNumber)
        except: pass
dev.ctrl_transfer(bmRequestType=0x41, bRequest=1, wValue=0, wIndex=0, data_or_wLength=0)
usb.util.dispose_resources(dev)
print("Download mode triggered")
PY
sleep 1.5
rebind

echo "=== Step 2: flashing ==="
sudo esptool --chip esp32s3 --port /dev/ttyACM0 --baud 460800 --no-stub \
    --before default-reset write-flash \
    --flash-mode dio --flash-freq 80m --flash-size 16MB \
    0x0     "$BUILD/wardriving_halo.ino.bootloader.bin" \
    0x8000  "$BUILD/wardriving_halo.ino.partitions.bin" \
    0xe000  "$BOOT_APP0" \
    0x10000 "$BUILD/wardriving_halo.ino.bin"

sleep 1
rebind

echo "=== Step 3: watchdog boot into user firmware ==="
sudo python3 - << 'PY'
import sys
sys.path.insert(0, '/usr/local/lib/python3.13/dist-packages')
from esptool.targets.esp32s3 import ESP32S3ROM
import serial, time

BASE   = 0x60008000
WDT0   = BASE + 0x0098
WDT1   = BASE + 0x009C
WDTPRO = BASE + 0x00B0
WKEY   = 0x50D83AA1

p = serial.Serial('/dev/ttyACM0', 115200, timeout=1.5)
e = ESP32S3ROM(p, baud=115200)
e.connect(mode='no_reset')
e.write_reg(WDTPRO, WKEY)
e.write_reg(WDT1, 2000)
e.write_reg(WDT0, (1<<31)|(5<<28)|(1<<8)|2)
e.write_reg(WDTPRO, 0)
time.sleep(0.6)
p.close()
print("Boot triggered")
PY

sleep 2

# Find and rebind to newly enumerated device
USB_PATH=$(for d in /sys/bus/usb/devices/*/; do
    d="${d%/}"; d="${d##*/}"
    vid=$(cat /sys/bus/usb/devices/$d/idVendor 2>/dev/null)
    pid=$(cat /sys/bus/usb/devices/$d/idProduct 2>/dev/null)
    [ "$vid" = "303a" ] && [ "$pid" = "1001" ] && echo "$d" && break
done)
[ -n "$USB_PATH" ] && echo "${USB_PATH}:1.0" | sudo tee /sys/bus/usb/drivers/cdc_acm/bind 2>/dev/null || true
sleep 1

echo "=== Step 4: verifying ==="
sudo python3 - << 'PY'
import serial, time
s = serial.Serial('/dev/ttyACM0', 115200, timeout=4.0)
time.sleep(0.2)
d = s.read(256)
s.close()
print("Output:", repr(d))
if b'BAT:' in d:
    print("SUCCESS: firmware running")
else:
    print("WARNING: no BAT output received yet")
PY
