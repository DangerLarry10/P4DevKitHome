# P4DevKitHome

Custom home screen framework for the **Waveshare ESP32-P4-WIFI6-DEV-KIT** with 10.1" MIPI-DSI touchscreen.

## Hardware

| Component | Specification |
|-----------|---------------|
| Board | Waveshare ESP32-P4-WIFI6-DEV-KIT (Dev-KitC) |
| MCU | ESP32-P4 dual-core RISC-V @ 400MHz |
| Coprocessor | ESP32-C6 (WiFi 6 + BLE 5, not yet integrated) |
| Display | 10.1" MIPI-DSI, 1280x800 native (portrait 800x1280 via sw_rotate) |
| Display Driver | JD9365 |
| Touch | GT911 capacitive, I2C |
| Flash | 16MB |
| PSRAM | 32MB |

## Software Stack

- **ESP-IDF v5.5+** - Espressif IoT Development Framework
- **LVGL v9.2** - Light and Versatile Graphics Library
- **Waveshare BSP** (`waveshare/esp32_p4_platform ^1.0.6`) - Board Support Package

## Project Structure

```
P4DevKitHome/
├── firmware/                  # ESP-IDF project (the actual code)
│   ├── main/
│   │   ├── main.cpp           # Entry point
│   │   ├── home/              # Home screen framework
│   │   │   ├── theme.h        # All colors, fonts, sizes
│   │   │   ├── home_screen.*  # Screen coordinator
│   │   │   ├── status_bar.*   # Clock + status icons
│   │   │   ├── app_list.*     # Scrollable app launcher
│   │   │   └── app_manager.*  # App lifecycle management
│   │   └── apps/              # Individual apps
│   │       ├── app_base.h     # Base class for all apps
│   │       ├── calculator/    # Calculator (adapted from esp-brookesia)
│   │       └── drawing/       # Finger drawing canvas
│   ├── sdkconfig.defaults     # ESP32-P4 build config
│   ├── partitions.csv         # 16MB flash layout
│   └── PROJECT_STATUS.md      # Detailed build/arch docs
├── docs/                      # Reference material
│   ├── ESP32-P4-WIFI6-DEV-KIT-Reference.md
│   └── esp-brookesia_example/ # Trimmed brookesia reference
│       ├── core/              # Framework patterns
│       ├── apps/              # Calculator, settings, timer reference
│       └── products/phone/phone_waveshare_p4_wifi6/  # Our board's reference
└── PLAN_RECOMMENDATIONS.md    # Codex review notes
```

## Quick Start

```bash
# Prerequisites: ESP-IDF v5.5+ installed and sourced

cd firmware
idf.py set-target esp32p4     # Configure for ESP32-P4 (once)
idf.py build                  # Compile
idf.py -p COMx flash monitor  # Flash via UART Type-C port
```

## Current Status

- All source files written and committed
- **Not yet built or tested on hardware**
- See `firmware/PROJECT_STATUS.md` for detailed status and known issues

## Adding a New App

1. Create `firmware/main/apps/myapp/myapp.h` and `.cpp`
2. Inherit from `AppBase`, implement `onRun()` at minimum
3. Add source files to `firmware/main/CMakeLists.txt`
4. Add `#include` and `appManager.registerApp()` in `main.cpp`

See `firmware/main/apps/app_base.h` for the full API with comments.
