# P4DevKitHome - Project Status

**Last Updated: 2026-03-02**

## Current State: Initial Implementation Complete (Untested)

All source files have been created. The project needs to be built and tested on hardware.

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

## Build & Flash Instructions

```bash
cd firmware
idf.py set-target esp32p4    # Only needed once
idf.py build                 # Compile everything
idf.py -p COMx flash monitor # Flash and watch serial output
```

**Note:** Connect via the Type-C **UART** port (not the USB-OTG port).

## Known Issues / Potential Problems

1. **BSP package name**: Updated to `waveshare/esp32_p4_platform` to match the existing Waveshare P4 reference product in `docs/esp-brookesia_example`. This still needs a real hardware build to confirm there are no BSP API differences in the custom firmware.
2. **Display rotation**: Using `sw_rotate = true` to get portrait mode. If display appears rotated wrong, try toggling this.
3. **Canvas memory**: Drawing app allocates ~3.7MB in PSRAM for the canvas buffer. Should be fine with 32MB PSRAM but watch for allocation failures.
4. **Clock**: Shows 00:00 on boot since there's no NTP (no WiFi yet). Will show uptime-based time.

## Architecture Overview

```
main.cpp
  └── app_main()
       ├── BSP init (display + touch)
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

## What's NOT Implemented Yet

- WiFi connectivity / ESP32-C6 integration
- Gesture navigation (swipe to go back)
- App settings/preferences persistence
- Actual app icons (using colored circles with initials)
- NTP time sync
