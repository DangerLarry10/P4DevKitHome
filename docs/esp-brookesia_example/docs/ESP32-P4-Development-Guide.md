# ESP32-P4-WIFI6-DEV-KIT Development Guide

This document contains critical development information for future Claude sessions and the developer.

## Quick Reference

| Item | Value |
|------|-------|
| Board | Waveshare ESP32-P4-WIFI6-DEV-KIT (Dev-KitC with 10.1" display) |
| COM Port | COM11 (verify with Device Manager) |
| Flash Size | 16MB |
| PSRAM | 32MB |
| ESP-IDF Version | v5.5.1 (installed at C:\Espressif\) |
| Display | 10.1" MIPI-DSI touchscreen |
| Display Driver | jd9365_10_1 (NOT ili9881c - that's for smaller displays) |

---

## esptool Commands for ESP32-P4

### Backup Entire Flash (Demo Firmware)

```bash
# Read entire 16MB flash to backup file
esptool.py --chip esp32p4 --port COM11 --baud 460800 read_flash 0 0x1000000 backup_full_16mb.bin

# Alternative: Use ALL keyword (auto-detects flash size)
esptool.py --chip esp32p4 --port COM11 --baud 460800 read_flash 0 ALL backup_full.bin
```

**Important Notes:**
- Use `--chip esp32p4` explicitly
- 16MB = 0x1000000 in hex
- Lower baud rate (460800 or 115200) if you get errors
- Reading 16MB takes several minutes

### Read Partition Table

```bash
# Partition table is typically at 0x8000, size 0x1000 (4KB)
esptool.py --chip esp32p4 --port COM11 read_flash 0x8000 0x1000 partition_table.bin

# View partition table
python -m esptool --chip esp32p4 partition_table partition_table.bin
```

### Restore Flash Backup

```bash
# Write backup back to flash (must write to address 0 if read from 0)
esptool.py --chip esp32p4 --port COM11 --baud 460800 write_flash 0x0 backup_full.bin
```

### Other Useful Commands

```bash
# Get flash info
esptool.py --chip esp32p4 --port COM11 flash_id

# Read MAC address
esptool.py --chip esp32p4 --port COM11 read_mac

# Erase flash (careful!)
esptool.py --chip esp32p4 --port COM11 erase_flash
```

---

## Graphics Library Decision

### Why NOT LovyanGFX

As of early 2025, LovyanGFX has **incomplete MIPI-DSI support** for ESP32-P4:
- Experimental support only on develop/M5GFX_Backport branches
- Compilation errors with Arduino 3.3.4 and IDF 5.5.1
- GitHub Issues: [#788](https://github.com/lovyan03/LovyanGFX/issues/788), [#782](https://github.com/lovyan03/LovyanGFX/issues/782)

### Recommended Stack

Use the **ESP-IDF native approach**:

```
┌─────────────────────────────┐
│          LVGL v9.x          │  ← UI Framework
├─────────────────────────────┤
│         esp_lcd API         │  ← ESP-IDF display abstraction
├─────────────────────────────┤
│   jd9365_10_1 driver        │  ← 10.1" display driver
├─────────────────────────────┤
│      MIPI-DSI Hardware      │  ← ESP32-P4 built-in
└─────────────────────────────┘
```

**Components to add via idf_component.yml:**
```yaml
dependencies:
  lvgl/lvgl: "^9.3.0"
  espressif/esp_lcd_touch_gt911: "*"  # Touch controller
  # Display driver - check Waveshare's component registry
```

---

## Display Configuration (10.1" DSI-TOUCH-A)

### Display Specifications

| Parameter | Value |
|-----------|-------|
| Size | 10.1 inches |
| Interface | MIPI-DSI 2-lane |
| Resolution | 1280x800 (typical for 10.1") |
| Touch Controller | GT911 (capacitive) |
| Driver IC | JD9365 |
| Color Depth | RGB888, RGB666, RGB565 |

### Touch Controller (GT911)

The 10.1" display uses the **GT911** capacitive touch controller via I2C:

| Signal | GPIO |
|--------|------|
| SDA | GPIO7 |
| SCL | GPIO8 |
| INT | Check schematic |
| RST | Check schematic |

**ESP-IDF Component:** `espressif/esp_lcd_touch_gt911`

---

## ESP-IDF Project Setup

### Initialize Environment (Windows CMD)

```batch
:: Initialize ESP-IDF environment
call "C:\Espressif\idf_cmd_init.bat"

:: Verify
idf.py --version
```

### Create New Project

```bash
# Option 1: Copy from template
cd C:\Users\PCMan\Projects\ESP32-P4-WIFI6-DEV-KIT
mkdir my_project
cd my_project

# Option 2: Create from scratch
idf.py create-project my_project
cd my_project

# Set target to ESP32-P4
idf.py set-target esp32p4
```

### Project Structure

```
my_project/
├── CMakeLists.txt
├── sdkconfig
├── sdkconfig.defaults          # Board-specific defaults
├── main/
│   ├── CMakeLists.txt
│   ├── idf_component.yml       # Component dependencies
│   ├── main.c
│   └── lv_conf.h               # LVGL configuration
├── components/                  # Custom components
└── managed_components/          # Downloaded components
```

### Required idf_component.yml

```yaml
dependencies:
  # LVGL UI framework
  lvgl/lvgl: "^9.3.0"

  # Touch driver for GT911
  espressif/esp_lcd_touch_gt911: "*"

  # WiFi 6 via ESP32-C6 coprocessor
  espressif/esp_wifi_remote: "*"
  espressif/esp_hosted: "*"

  # Audio codec (when using speaker)
  espressif/es8311: "*"
```

---

## OTA Update Implementation

### Why OTA is Different on ESP32-P4

The ESP32-P4 has **no built-in WiFi**. WiFi comes from the ESP32-C6 coprocessor via SDIO. This means:

1. Use `esp_wifi_remote` component (not standard `esp_wifi`)
2. Initialize the C6 coprocessor first
3. Then WiFi API is similar to standard ESP-IDF

### OTA Components Needed

```yaml
dependencies:
  espressif/esp_wifi_remote: "*"
  espressif/esp_hosted: "*"
  # Standard OTA components are built into ESP-IDF
```

### Partition Table for OTA

Use a partition table with two OTA slots:

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
phy_init, data, phy,     0xf000,  0x1000,
ota_0,    app,  ota_0,   0x10000, 0x1E0000,
ota_1,    app,  ota_1,   0x1F0000,0x1E0000,
storage,  data, spiffs,  0x3D0000,0x30000,
```

### Basic OTA Flow

1. Connect to WiFi (via esp_wifi_remote)
2. Check for updates on server
3. Download to inactive OTA partition
4. Verify and switch boot partition
5. Reboot

---

## Build and Flash Commands

```bash
# Build
idf.py build

# Flash (uses COM11)
idf.py -p COM11 flash

# Monitor serial output
idf.py -p COM11 monitor

# Combined build, flash, monitor
idf.py -p COM11 build flash monitor

# Menuconfig (graphical configuration)
idf.py menuconfig
```

### Common menuconfig Settings for This Board

Navigate to these settings:
- `Component config → ESP32P4-Specific → CPU frequency` → 360 MHz
- `Component config → LVGL → Display` → Set resolution
- `Component config → ESP-Hosted` → Enable SDIO interface

---

## Waveshare Resources

### Direct Download Links

| Resource | URL |
|----------|-----|
| Demo Package | https://files.waveshare.com/wiki/ESP32-P4-WIFI6-DEV-KIT/ESP32-P4-NANO_Demo.zip |
| Schematic | https://files.waveshare.com/wiki/ESP32-P4-WIFI6-DEV-KIT/ESP32-P4-WIFI6-DEV-KIT-datasheet.pdf |
| ESP32-P4 Datasheet | https://files.waveshare.com/wiki/common/Esp32-p4_datasheet_en.pdf |
| Technical Reference | https://files.waveshare.com/wiki/common/Esp32-p4_technical_reference_manual_en.pdf |
| Wiki Page | https://www.waveshare.com/wiki/ESP32-P4-WIFI6-DEV-KIT |

### Display Drivers by Size

| Display Size | Driver Component |
|--------------|------------------|
| 5", 7", 8" | ili9881c |
| **10.1"** | **jd9365_10_1** |

---

## Troubleshooting

### "No module named 'esp_idf_monitor'"
**Cause:** ESP-IDF environment not initialized
**Fix:** Run `call "C:\Espressif\idf_cmd_init.bat"` first

### Flash read/write errors
**Cause:** Baud rate too high or poor USB connection
**Fix:** Lower baud to 115200, try different USB cable

### Display not working
**Checklist:**
1. Correct driver (jd9365_10_1 for 10.1")
2. FPC cable properly seated
3. Display power enabled in code

### Touch not responding
**Checklist:**
1. GT911 driver initialized
2. I2C pins correct (GPIO7/8)
3. Touch interrupt pin configured

### WiFi not connecting
**Remember:** ESP32-P4 uses esp_wifi_remote, not esp_wifi
**Checklist:**
1. esp_hosted component initialized
2. ESP32-C6 firmware loaded
3. Using esp_wifi_remote API

---

## Sample LVGL Two-Page App Structure

```c
// Minimal structure for two pages with navigation

// Page handles
static lv_obj_t *page1;
static lv_obj_t *page2;

// Button callbacks
static void goToPage2(lv_event_t *e) {
    lv_obj_add_flag(page1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(page2, LV_OBJ_FLAG_HIDDEN);
}

static void goToPage1(lv_event_t *e) {
    lv_obj_add_flag(page2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(page1, LV_OBJ_FLAG_HIDDEN);
}

void createUI(void) {
    // Page 1
    page1 = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page1, LV_PCT(100), LV_PCT(100));

    lv_obj_t *btn1 = lv_btn_create(page1);
    lv_obj_t *label1 = lv_label_create(btn1);
    lv_label_set_text(label1, "Go to Page 2");
    lv_obj_add_event_cb(btn1, goToPage2, LV_EVENT_CLICKED, NULL);

    // Page 2 (initially hidden)
    page2 = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page2, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(page2, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *btn2 = lv_btn_create(page2);
    lv_obj_t *label2 = lv_label_create(btn2);
    lv_label_set_text(label2, "Go to Page 1");
    lv_obj_add_event_cb(btn2, goToPage1, LV_EVENT_CLICKED, NULL);
}
```

---

## SquareLine Studio Compatibility

LVGL v9.x projects can export to SquareLine Studio format. When setting up:

1. **Board Settings:**
   - Resolution: 1280x800 (for 10.1" display)
   - Color depth: 16-bit (RGB565) for performance, or 24-bit (RGB888)
   - LVGL version: 9.x

2. **Export Settings:**
   - Platform: ESP-IDF
   - Generate: UI files only (integrate into your project)

3. **Integration:**
   - Copy generated `ui.c`, `ui.h`, and assets to your project
   - Call `ui_init()` after LVGL and display initialization

---

**Document Version:** 1.0
**Last Updated:** 2026-02-01
**ESP-IDF Version:** v5.5.1
