# Plan Recommendations

## Current Reality

- There are no markdown files directly in the project root.
- The active custom project is `firmware/`.
- The copied `esp-brookesia` tree under `docs/esp-brookesia_example/` already contains a Waveshare-specific reference target at `products/phone/phone_waveshare_p4_wifi6/`.
- `docs/esp-brookesia_example/.claude/project-state.md` is stale and has been marked for deletion instead of being removed.

## Recommended Plan Changes

1. Treat `firmware/` as the primary implementation and `docs/esp-brookesia_example/` as a trimmed reference only.
2. Use the confirmed hardware target everywhere: Waveshare `ESP32-P4-WIFI6-DEV-KIT` Dev-KitC with the 10.1 inch `1280x800` MIPI-DSI display.
3. Standardize on the Waveshare BSP used by the existing reference product: `waveshare/esp32_p4_platform`.
4. Keep the custom firmware scope narrow until hardware validation passes: home screen, calculator, drawing, touch, display rotation, and memory stability.
5. Defer WiFi, OTA, camera, speaker, and broader `esp-brookesia` feature-porting until the current firmware builds and runs on the board.
6. Keep only the hardware-relevant parts of the copied `esp-brookesia` tree so it stays usable as reference without the unrelated board targets, speaker product, CI files, and agent code.

## Immediate Validation Steps

1. Run `idf.py set-target esp32p4` in `firmware/`.
2. Run `idf.py build` and confirm the Waveshare BSP resolves with the updated manifest.
3. Flash on the UART Type-C port and verify display init, touch input, and portrait rotation.
4. If the display comes up with the wrong orientation or touch mapping, compare the custom `firmware/main/main.cpp` init with `docs/esp-brookesia_example/products/phone/phone_waveshare_p4_wifi6/main/main.cpp`.
