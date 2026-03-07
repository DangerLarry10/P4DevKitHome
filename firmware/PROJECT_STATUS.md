# P4DevKitHome - Project Status

**Last Updated: 2026-03-07**

## Current State: Running on Hardware

First successful flash and boot on 2026-03-06. Home screen, status bar, and both apps functional.

## What's Working

- Home screen with scrollable app list (two cards: Calculator, Drawing)
- Status bar with clock (top-left) and WiFi/battery icons (top-right)
- Calculator app launches and works (UI elements are small — needs sizing tuning)
- Drawing app launches and works (refresh rate is slow — canvas optimization needed)
- Back button returns to home screen from both apps
- Touch input is aligned correctly (no coordinate mapping issues)

## What's Done

- ESP-IDF project skeleton (`CMakeLists.txt`, `sdkconfig.defaults`, `partitions.csv`)
- Component dependencies (`idf_component.yml` - Waveshare BSP + LVGL v9)
- Theme system (`theme.h` - all colors, fonts, sizes in one place)
- App framework:
  - `AppBase` - base class all apps inherit from (screen-per-app isolation)
  - `AppManager` - app registration, launching, closing, back button
- Home screen:
  - `StatusBar` - clock, WiFi/battery placeholders
  - `AppList` - scrollable card-style launcher
  - `HomeScreen` - coordinator with 30s clock timer
- Apps:
  - `CalculatorApp` - adapted from esp-brookesia calculator
  - `DrawingApp` - custom finger-painting canvas with color/size palette

## Key Fix Applied

- **Removed software rotation** (`ESP_LV_ADAPTER_ROTATE_90` → `ESP_LV_ADAPTER_ROTATE_0`).
  The BSP already defines the 10.1" panel as 800x1280 (portrait). The rotation was
  unnecessary and triggered a known PPA freeze bug on ESP32-P4 with TRIPLE_PARTIAL mode.

## Build & Flash Instructions

```bash
cd firmware
idf.py set-target esp32p4      # Only needed once
idf.py build                   # Compile everything
idf.py -p COM20 flash monitor  # Flash and watch serial output
```

**Note:** Connect via the Type-C **UART** port (not the USB-OTG port).

## Known Issues

1. **Calculator UI too small**: Button and text sizing needs to be scaled up for the 10.1" display.
2. **Drawing app slow refresh**: Canvas uses ARGB8888 (4 bytes/pixel, ~3.7MB) and opens/closes a draw layer per dot. Should switch to RGB565 and batch draw operations.
3. **Clock shows 00:00**: No NTP (no WiFi yet). Shows epoch time on boot.
4. **Hardcoded screen dimensions**: `theme.h` uses `SCREEN_WIDTH=800` / `SCREEN_HEIGHT=1280` constants instead of querying the display at runtime.

## Architecture Overview

```
main.cpp
  └── app_main()
       ├── BSP init (display + touch, no rotation)
       ├── AppManager
       │    ├── registerApp(Calculator)
       │    └── registerApp(Drawing)
       └── HomeScreen
            ├── StatusBar (clock, icons)
            └── AppList (tappable app cards)
                 └── tap → AppManager.launchApp()
                      ├── creates new LVGL screen
                      ├── calls app.onRun()
                      └── adds back button
```

## Adding a New App

1. Create `firmware/main/apps/myapp/myapp.h` and `.cpp`
2. Inherit from `AppBase`, implement `onRun()` (and optionally `onClose()`, `onBack()`)
3. Add source files to `firmware/main/CMakeLists.txt` SRCS list
4. Add `#include` and `registerApp()` call in `main.cpp`

## Next Major Feature: Security Map App (MQTT)

**Goal:** Display a floor plan PNG with colored overlays showing motion/distance sensor status.

**Architecture decisions made:**
- P4 renders its own map locally (not streamed from PC/Pi)
- Subscribes to zigbee2mqtt MQTT topics directly
- Read-only dashboard (no device control yet)
- 3-5 motion/distance sensors to start

**Network stack needed:**
- WiFi via ESP32-C6 coprocessor: add `esp_wifi_remote` + `esp_hosted` to `idf_component.yml`
- MQTT client: already compiled in (CONFIG_MQTT_PROTOCOL_311=y in sdkconfig)
- C6 connects to P4 via SDIO (pins GPIO39-44) — shares pins with TF card slot

**Home Assistant / MQTT broker:**
- HAOS on repurposed laptop at 192.168.1.103:8123
- MQTT broker on same host (default Mosquitto port 1883)
- Topic structure: `zigbee2mqtt/<device_name>` with JSON payloads

**Prerequisite from user:** Floor plan PNG image (to be created)

**Implementation approach:**
1. Add WiFi components and get network connectivity working first
2. Build MQTT service layer (connect, subscribe, parse JSON payloads)
3. Build the map app: embed PNG in flash, overlay LVGL indicator objects at sensor positions
4. Sensor positions hardcoded initially (can add config screen later)

## What's NOT Implemented Yet

- WiFi connectivity / ESP32-C6 integration (next step for map app)
- Gesture navigation (swipe to go back)
- App settings/preferences persistence
- Actual app icons (using colored circles with initials)
- NTP time sync (will come free with WiFi)
- Overlay manager (for quick controls, slide-over panels)
