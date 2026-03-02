# ESP32-P4-WIFI6-DEV-KIT Hardware Reference

## Overview

The ESP32-P4-WIFI6-DEV-KIT is Waveshare's development board based on the ESP32-P4 chip, a high-performance SoC designed for HMI (Human-Machine Interface) applications with rich display and multimedia capabilities.

### What Makes This Board Special

- **Display-First Design**: Built around a MIPI-DSI interface for high-resolution displays (your 10.1" display uses this)
- **WiFi 6 Support**: Uses an ESP32-C6 coprocessor to add WiFi 6 (2.4GHz) and Bluetooth 5 capabilities
- **Video Capabilities**: Hardware H.264 encoding at 1080p 30fps, integrated camera ISP
- **Audio Processing**: Onboard codec (ES8311) with amplifier for speakers and microphone support
- **Graphics Acceleration**: Dedicated 2D graphics acceleration and pixel processing hardware

### Key ESP32-P4 Chip Capabilities

| Feature | Specification |
|---------|---------------|
| Main CPU | RISC-V 32-bit dual-core @ 400MHz (currently limited to 360MHz in SDK) |
| Low-Power CPU | RISC-V 32-bit single-core |
| RAM | 768KB L2 memory + 32KB low-power SRAM + 8KB TCM |
| ROM | 128KB high-performance + 16KB low-power system ROM |
| Flash (onboard) | 16MB NOR Flash |
| PSRAM (onboard) | 32MB stacked PSRAM |
| WiFi/Bluetooth | **NOT built into P4 chip** - provided by ESP32-C6 coprocessor |

**IMPORTANT**: Unlike ESP32, ESP32-S3, etc., the ESP32-P4 chip itself has NO WiFi or Bluetooth. This board uses an ESP32-C6 connected via SDIO to provide WiFi 6 (2.4GHz only) and Bluetooth 5.

## Power Requirements

| Parameter | Specification | Notes |
|-----------|---------------|-------|
| Input Voltage | 5V DC | Via USB Type-C |
| Power Sources | 2 options | (1) Type-C UART port OR (2) Type-C USB 2.0 port |
| Optional PoE | Supported | Via external PoE module header |
| RTC Battery | CR1220 holder | **Use rechargeable batteries ONLY** |
| Speaker Power | 2W max | Via onboard amplifier (NS4150B) |

**Power Supply Notes**:
- You can power the board from either Type-C port
- Do not use non-rechargeable batteries in the RTC holder (may damage charging circuit)
- PoE module sold separately if needed

## Display Specifications (10.1" DSI-TOUCH-A)

Your Dev-KitC version includes the 10.1 inch MIPI-DSI touchscreen display.

### MIPI-DSI Interface

| Parameter | Specification |
|-----------|---------------|
| Interface Type | MIPI-DSI (Display Serial Interface) |
| Data Lanes | 2-lane |
| Standard | D-PHY v1.1 |
| Speed | Up to 1.5Gbps per lane (3Gbps total bandwidth) |
| Connection | 15-pin FPC cable (included with display) |

### Display Capabilities

The ESP32-P4 includes:
- **PPA (Pixel Processing Accelerator)**: Hardware-accelerated pixel operations
- **2D Graphics Controller**: Dedicated 2D graphics acceleration for UI rendering
- **LVGL Support**: Optimized for LVGL v9.3.0+ GUI library

### Supported Display Resolutions

The board supports these Waveshare MIPI-DSI displays:
- 5-inch DSI-TOUCH-A
- 7-inch DSI-TOUCH-A (720×1280)
- 8-inch DSI-TOUCH-A
- 10.1-inch DSI-TOUCH-A (your display)

**Note**: Exact resolution for 10.1" display not specified in documentation, but MIPI-DSI supports high-resolution panels.

## Camera Module (Not Yet Connected)

### Camera Interface

| Parameter | Specification |
|-----------|---------------|
| Interface Type | MIPI-CSI (Camera Serial Interface) |
| Data Lanes | 2-lane |
| Maximum Resolution | 2MP (limited by H.264 encoder performance) |
| Connection | FPC connector on board |

### Supported Camera Modules

Waveshare has tested these camera modules:
- **OV5647**: 5MP camera sensor (Raspberry Pi Camera v1 compatible)
- **SC2336**: 2MP camera sensor

### Video Encoding Capabilities

| Feature | Specification |
|---------|---------------|
| H.264 Encoding | 1080p @ 30fps |
| JPEG Encoding | Supported |
| Image Signal Processor | Integrated ISP for image enhancement |

**Why 2MP limit?**: While cameras like the OV5647 support 5MP, the ESP32-P4's H.264 hardware encoder is optimized for real-time 1080p video, so 2MP (1920×1080) is the practical maximum for video applications.

### Camera Connection Pinout

The MIPI-CSI interface uses dedicated hardware lanes. When connecting a camera:
- Use the 15-pin FPC connector labeled "CAM" on the board
- Camera modules connect directly via ribbon cable
- No GPIO configuration needed (hardware interface)

## Audio/Speaker Interface (Not Yet Connected)

### Audio Hardware

| Component | Part Number | Function |
|-----------|-------------|----------|
| Audio Codec | ES8311 | I2S audio codec with ADC/DAC |
| Amplifier | NS4150B | Class D amplifier for speaker output |
| Microphone | SMD onboard | Built-in microphone on PCB |
| Speaker Connector | MX1.25 | 2-pin connector for 8Ω speaker |
| Headphone Jack | 3.5mm | Standard audio jack |

### I2S Audio Pin Mapping

| Signal | GPIO Pin | Function |
|--------|----------|----------|
| MCLK | GPIO13 | Master clock to codec |
| SCLK | GPIO12 | Bit clock (serial clock) |
| ASDOUT | GPIO11 | Audio serial data out (playback) |
| LRCK | GPIO10 | Left/right clock (word select) |
| DSDIN | GPIO9 | Audio serial data in (recording from mic) |
| PA_Ctrl | GPIO53 | Power amplifier control (enable/disable) |

### Speaker Specifications

| Parameter | Value |
|-----------|-------|
| Impedance | 8Ω |
| Maximum Power | 2W |
| Connector | MX1.25 2-pin white connector |

**How to Connect Your Speaker**:
1. Speaker should be 8Ω impedance (4Ω may work but check current draw)
2. Maximum 2W power handling
3. Polarity usually doesn't matter for small speakers, but red=positive is standard
4. The MX1.25 connector is the small white 2-pin connector on the board

**Audio Library**: Use `espressif/es8311` component in ESP-IDF for codec control.

## Pin Definitions

### 40-Pin GPIO Header

The board includes a 40-pin header with partial Raspberry Pi HAT compatibility. Here are the key pins:

**Default I2C Pins**:
| Signal | GPIO | Notes |
|--------|------|-------|
| SDA | GPIO7 | I2C data line |
| SCL | GPIO8 | I2C clock line |

**Note**: Many other GPIOs are available on the 40-pin header, but specific mappings depend on your application. The board reserves certain GPIOs for onboard peripherals (see tables below).

### SD Card Interface Pins

The TF card slot uses 4-wire SDIO mode:

| Signal | GPIO Pin | Function |
|--------|----------|----------|
| CLK | GPIO43 | SD card clock |
| CMD | GPIO44 | SD card command |
| D0 | GPIO39 | Data line 0 |
| D1 | GPIO40 | Data line 1 |
| D2 | GPIO41 | Data line 2 |
| D3 | GPIO42 | Data line 3 |

**SD Card Specs**:
- SDIO 3.0 compatible
- Default speed: 20MHz
- High-speed capable: 40MHz
- Internal pullups supported (no external resistors needed)

### USB and Serial Ports

| Port | Type | Function | Use Case |
|------|------|----------|----------|
| Type-C UART | USB-to-Serial | Programming/Debug | Use this for flashing firmware and serial monitor |
| Type-C USB 2.0 | USB OTG | Device/Host mode | For USB peripherals or USB device emulation |

**USB OTG Mode Selection**:
- Jumper on board selects Host vs Device mode
- High-speed capable (480Mbps)
- Can power the board or be powered by it (depending on mode)

### Boot and Reset Controls

| Button | Function |
|--------|----------|
| BOOT | Hold during power-on or reset to enter download mode |
| RST | Hardware reset button |

**Programming Procedure**:
1. Hold BOOT button
2. Press and release RST button
3. Release BOOT button
4. Board is now in download mode for flashing firmware

**Note**: Some USB-to-serial adapters can automatically trigger download mode, so manual button pressing may not always be needed.

### Reserved Pins (Used by Onboard Hardware)

These GPIO pins are used by onboard peripherals and should not be reassigned:

**Audio I2S** (see Audio section above):
- GPIO9, GPIO10, GPIO11, GPIO12, GPIO13, GPIO53

**SD Card SDIO**:
- GPIO39, GPIO40, GPIO41, GPIO42, GPIO43, GPIO44

**MIPI-DSI Display**:
- Hardware dedicated lanes (not GPIO-accessible)

**MIPI-CSI Camera**:
- Hardware dedicated lanes (not GPIO-accessible)

**ESP32-C6 SDIO Interface** (WiFi 6 module):
- Internal connection via SDIO (specific GPIOs not documented but reserved)

## WiFi 6 Module (ESP32-C6)

### How It Works

The ESP32-P4 chip has no built-in wireless capabilities. Waveshare added an ESP32-C6 chip that acts as a wireless coprocessor:

```
ESP32-P4 <--[SDIO Interface]--> ESP32-C6 <--[Radio]--> WiFi 6 / Bluetooth 5
```

### ESP32-C6 Capabilities

| Feature | Specification |
|---------|---------------|
| WiFi | WiFi 6 (802.11ax) - 2.4GHz ONLY |
| Bluetooth | Bluetooth 5 (LE) |
| Connection to P4 | SDIO 3.0 |
| Frequency | 2.4GHz only (no 5GHz support) |

**Important Limitations**:
- Only 2.4GHz WiFi 6 supported (ESP32-C6 hardware limitation)
- 5GHz WiFi NOT supported
- ESP32-C6 runs its own firmware to manage wireless functions
- Communication happens via SDIO interface (appears similar to SDIO WiFi card)

### Required ESP-IDF Components

To use WiFi 6 on this board, you need these ESP-IDF components:
- `espressif/esp_wifi_remote`: Remote WiFi driver for SDIO-connected wireless
- `espressif/esp_hosted`: ESP-Hosted protocol for C6 coprocessor communication

**Why this matters**: Traditional ESP32 WiFi code won't work directly. You need the remote WiFi driver that communicates with the C6 via SDIO.

## Programming and Development

### Supported Development Environments

| Environment | Support Level | Minimum Version | Notes |
|-------------|---------------|-----------------|-------|
| ESP-IDF | **Recommended** | v5.3.1+ | Full support, best for this board |
| Arduino IDE | Partial | esp32 board v3.2.0+ | Basic support, limited examples |
| PlatformIO | Limited | N/A | Not fully supported yet |
| MicroPython | Limited | N/A | Not fully supported yet |

**Recommendation for Your Projects**: Use ESP-IDF since you're already familiar with it. Arduino support exists but is less mature for P4.

### ESP-IDF Setup (Windows)

You mentioned ESP-IDF is already installed. Here's what you need for this board:

1. **Version Check**: Ensure you have ESP-IDF v5.3.1 or newer
   ```bash
   idf.py --version
   ```

2. **Target Selection**: Set ESP32-P4 as the target
   ```bash
   idf.py set-target esp32p4
   ```

3. **Key Components to Add** (via `idf_component.yml`):
   - `espressif/esp_wifi_remote`: For WiFi 6 via C6 coprocessor
   - `espressif/es8311`: For audio codec control
   - `lvgl/lvgl ^9.3.0`: For display graphics (if using LVGL)

### Important ESP-IDF Notes for This Board

**Current CPU Frequency Limitation**:
The ESP32-P4 is rated for 400MHz, but current SDK (as of documentation date) limits it to 360MHz. This will likely be updated in future SDK versions.

**LVGL Configuration**:
If you're using LVGL for the display:
- Use LVGL v9.3.0 or newer
- Board has hardware acceleration for pixel operations (PPA)
- Dedicated 2D graphics acceleration available
- You'll need proper `lv_conf.h` configuration for the display resolution

**WiFi Remote Driver**:
The WiFi 6 implementation is different from standard ESP32:
- Uses `esp_wifi_remote` component instead of built-in WiFi driver
- ESP32-C6 handles the actual WiFi stack
- API is similar but not identical to standard ESP-IDF WiFi functions
- Check Waveshare examples for proper initialization sequence

### Programming Connection

**Use the Type-C UART port** for programming:
- This is the USB-to-serial bridge
- Connect to your PC via Type-C cable
- Should appear as a COM port in Windows Device Manager
- Use this port for `idf.py flash monitor`

**The Type-C USB 2.0 port** is for USB OTG functionality (host/device mode), not for programming.

## Ethernet Interface

| Feature | Specification |
|---------|---------------|
| Speed | 100M (10/100 Mbps) |
| Connector | RJ45 |
| Use Case | Wired network connection |

This provides a wired alternative to WiFi for network connectivity.

## Important Notes and Gotchas

### 1. ESP32-P4 Has NO Built-in WiFi or Bluetooth

This is the most important thing to understand:
- ESP32-P4 is NOT like ESP32/S2/S3/C3/C6 with integrated wireless
- Wireless comes from the ESP32-C6 coprocessor via SDIO
- You must use the remote WiFi driver, not standard WiFi APIs
- Check Waveshare's examples for proper WiFi initialization

### 2. CPU Speed Currently Limited

- Hardware capable of 400MHz
- Current SDK limits to 360MHz
- Future SDK updates should unlock full 400MHz
- Don't manually try to overclock until official SDK support

### 3. Display Interface is MIPI-DSI, Not SPI

If you're used to SPI displays (like ILI9341, ST7789):
- This is completely different hardware interface
- MIPI-DSI is much faster (3Gbps vs ~40Mbps for SPI)
- You don't configure GPIOs for the display (hardware connection)
- Use ESP-IDF display drivers or LVGL with proper config

### 4. Two Different Type-C Ports

- **Type-C UART**: For programming and serial debug (use this most of the time)
- **Type-C USB 2.0**: For USB host/device functionality (not for programming)
- Don't confuse them when connecting your PC

### 5. Boot Mode Entry

To flash firmware:
- Hold BOOT button
- Press RST button (while holding BOOT)
- Release both buttons
- Board is now in download mode

Some USB-to-serial chips auto-trigger this, but manual method always works.

### 6. RTC Battery Warning

- Only use rechargeable batteries (board has charging circuit)
- Non-rechargeable CR1220 may be damaged or leak
- RTC battery is optional (only needed to maintain clock during power-off)

### 7. PlatformIO and MicroPython Support

- Currently limited or non-existent for ESP32-P4
- Stick with ESP-IDF or Arduino IDE for now
- Support will likely improve as the chip matures

### 8. SDIO Pin Conflict

The SD card uses 6 GPIOs (39-44). If you need these pins for other purposes, you cannot use the SD card slot simultaneously.

### 9. Camera Resolution vs Capabilities

- Camera sensors may support higher resolution (e.g., OV5647 = 5MP)
- Video encoder limits real-time encoding to 1080p (2MP)
- You can capture still images at higher resolution
- For video streaming, stick to 2MP or lower

### 10. Audio Amplifier Enable

The amplifier has a control pin (GPIO53) that must be enabled for speaker output:
- Set GPIO53 HIGH to enable amplifier
- Set GPIO53 LOW to disable (power saving)
- Don't forget this or you won't hear audio from the speaker

## Block Diagram Summary

```
┌─────────────────────────────────────────────────────┐
│              ESP32-P4-WIFI6-DEV-KIT                 │
│                                                     │
│  ┌──────────────┐         ┌──────────────┐        │
│  │   ESP32-P4   │         │  ESP32-C6    │        │
│  │  (Main CPU)  │◄─SDIO──►│ (WiFi 6/BT5) │        │
│  │   400MHz*    │         │   2.4GHz     │        │
│  └──────────────┘         └──────────────┘        │
│         │                                           │
│         ├─ MIPI-DSI ──► 10.1" Display (connected)  │
│         ├─ MIPI-CSI ──► Camera (not connected)     │
│         ├─ I2S ────────► Audio Codec (ES8311)      │
│         │                  └─► Speaker (not conn.) │
│         ├─ SDIO ───────► SD Card Slot              │
│         ├─ UART ───────► Type-C (programming)      │
│         ├─ USB 2.0 ────► Type-C OTG                │
│         ├─ I2C/GPIO ───► 40-pin Header             │
│         └─ RMII ───────► Ethernet RJ45             │
│                                                     │
└─────────────────────────────────────────────────────┘

* Currently limited to 360MHz in SDK
```

## Quick Start Checklist

For your specific setup (Dev-KitC with 10.1" display connected):

- [x] Display connected and ready to use
- [ ] Camera module to be connected later (use CAM FPC connector)
- [ ] Speaker to be connected later (MX1.25 white connector, 8Ω speaker)
- [ ] Microphone already onboard (ready when audio codec initialized)

**To start developing**:
1. Use Type-C UART port to connect to PC
2. Set target: `idf.py set-target esp32p4`
3. For display: Configure LVGL for MIPI-DSI and your resolution
4. For WiFi 6: Add `esp_wifi_remote` component
5. For audio: Add `es8311` component when ready to use speaker

## Resources

- **Manufacturer**: Waveshare
- **Product Page**: https://www.waveshare.com/wiki/ESP32-P4-WIFI6-DEV-KIT
- **ESP-IDF**: v5.3.1 or newer required
- **Key Libraries**:
  - `lvgl/lvgl` - GUI library (v9.3.0+)
  - `espressif/esp_wifi_remote` - WiFi 6 via C6 coprocessor
  - `espressif/es8311` - Audio codec driver
  - `espressif/esp_hosted` - ESP-Hosted protocol

**Waveshare Example Code**: Check the wiki page for official examples demonstrating:
- Display initialization with LVGL
- Camera capture and video encoding
- Audio playback and recording
- WiFi 6 connectivity

---

**Document Version**: 1.0
**Last Updated**: 2026-02-01
**Board Revision**: Dev-KitC (with 10.1" display)
