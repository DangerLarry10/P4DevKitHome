# Build Environment Notes

**Last Updated: 2026-03-03**

## Quick Start

From this directory (`firmware/`), just run:

```cmd
build.bat              # Build the project
build.bat flash COM5   # Build + flash + serial monitor
build.bat monitor COM5 # Just open serial monitor
build.bat clean        # Full clean rebuild
```

## ESP-IDF Installation

| Component | Path |
|-----------|------|
| ESP-IDF v5.5.1 | `C:\Espressif\frameworks\esp-idf-v5.5.1` |
| Tools | `C:\Espressif\tools\` |
| Python 3.11 venv | `C:\Espressif\python_env\idf5.5_py3.11_env` |

## About the Python Virtual Environment

The ESP-IDF build system uses a Python virtual environment (venv) that contains
all the build tools (esptool, idf-component-manager, etc.). Here's what you need
to know:

### Why it lives at `C:\Espressif\` and not in this project

- The venv is **shared across all ESP-IDF projects** on your machine. Moving it
  would break any other ESP-IDF project.
- Python venvs have **hardcoded absolute paths** baked into their scripts. You
  can't just copy/move the folder — it would need to be recreated from scratch.
- The venv is ~200MB+ and would bloat the git repo.

### What's in the venv

Key packages (as of 2026-03-03):
- `esptool 4.12.dev1` — flashes firmware to ESP32 chips
- `idf-component-manager 2.4.7` — downloads managed components (LVGL, BSP, etc.)
- `esp-idf-monitor 1.9.0` — serial monitor for `idf.py monitor`
- `esp-idf-kconfig 2.5.3` — handles `menuconfig` / `sdkconfig`

### If the venv breaks or needs updating

```cmd
cd C:\Espressif\frameworks\esp-idf-v5.5.1
C:\Espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe tools\idf_tools.py install-python-env
```

This re-installs/updates all Python packages to match ESP-IDF v5.5.1's requirements.

### Why we can't use Python 3.14

As of March 2026, the `windows-curses` package (needed by `esp-idf-kconfig` for
`menuconfig`) doesn't support Python 3.14. The venv uses Python 3.11.2 which
works fine.

## MSYS / Git Bash Warning

ESP-IDF's `export.bat` and build tools **do not work in Git Bash or MSYS shells**.
They detect the MSYS environment and refuse to run. The `build.bat` script handles
this by clearing the `MSYSTEM` environment variable automatically.

If you need to run ESP-IDF commands manually, use **Command Prompt** or
**PowerShell**, not Git Bash.

## Component Versions (from dependencies.lock)

| Component | Version |
|-----------|---------|
| lvgl/lvgl | 9.5.0 |
| espressif/esp_lvgl_adapter | 0.1.4 |
| waveshare/esp32_p4_platform | 1.0.6 |
| espressif/esp_lcd_touch_gt911 | 1.2.0~1 |
| waveshare/esp_lcd_jd9365 | 1.0.6 |
