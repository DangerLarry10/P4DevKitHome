# Claude Fix Notes

## Purpose

These notes are for the next Claude Code pass on `P4DevKitHome`.

The active codebase is `firmware/`. The `docs/esp-brookesia_example/` tree is a reference source only. Use it for UI concepts and app patterns, not as a drop-in code source.

## Baseline That Is Known Good

- The first successful build is commit `fc7475d` (`successful-first-build`, 2026-03-03 04:01:38 -0700).
- The current `HEAD` (`647847c`) only adds `firmware/build.bat` and `firmware/BUILD_ENVIRONMENT.md`. Firmware source is still on the same buildable baseline.
- The successful build log is `docs/buildAttemptLog1.txt`.
- The built image size in that log is about `0xdef10` bytes (`~912 KB`), leaving ample room in the current `14M` factory partition.

## Actual Toolchain and Locked Versions

- ESP-IDF: `5.5.1`
- Python venv: `C:\Espressif\python_env\idf5.5_py3.11_env`
- Target: `esp32p4`
- Waveshare BSP: `waveshare/esp32_p4_platform 1.0.6`
- LVGL adapter: `espressif/esp_lvgl_adapter 0.1.4`
- LVGL: `9.5.0`
- Touch driver: `espressif/esp_lcd_touch_gt911 1.2.0~1`
- LCD driver: `waveshare/esp_lcd_jd9365 1.0.6`

Do not downgrade or casually reshuffle these versions while fixing runtime issues. The current code already compiles with this set.

## Hardware Reality To Design Around

- Board: Waveshare `ESP32-P4-WIFI6-DEV-KIT`
- Display: `10.1"` panel, selected in `firmware/sdkconfig` as `CONFIG_BSP_LCD_TYPE_800_1280_10_1_INCH_A=y`
- Color format: `RGB565`
- PSRAM: `32 MB`
- The BSP header selected by that config reports `BSP_LCD_H_RES=800` and `BSP_LCD_V_RES=1280`

This matters because the project comments still talk about a native `1280x800` panel rotated into portrait. The installed BSP for this selected panel already exposes `800x1280`.

## Highest-Risk Runtime Issue

### 1. PPA freeze risk on this exact hardware/config

`firmware/main/main.cpp` currently uses:

- `ESP_LV_ADAPTER_ROTATE_90`
- `ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL`

That is the exact combination called out in `firmware/managed_components/espressif__esp_lvgl_adapter/README.md` as an ESP32-P4 freeze case when PPA acceleration is active.

The adapter README says the ESP-IDF PPA patch is required for:

- ESP32-P4
- `TRIPLE_PARTIAL`
- 90/270 degree rotation

The installed IDF file `C:\Espressif\frameworks\esp-idf-v5.5.1\components\esp_driver_ppa\src\ppa_srm.c` does not contain the documented fix marker:

- `PPA.sr_byte_order.sr_macro_bk_ro_bypass = 1;`

So assume the host IDF is unpatched until proven otherwise.

### Recommended fix order

1. Stop treating this as a compile problem. It is now a board bring-up and runtime stability problem.
2. First test removing software rotation, because the selected BSP panel config is already `800x1280`.
3. If portrait is still needed after that, confirm the physical orientation on real hardware before adding any rotation back.
4. If rotation must remain, either:
   - patch the installed ESP-IDF PPA driver exactly as the adapter README describes, or
   - switch away from the `TRIPLE_PARTIAL + rotate` combination while debugging.

The safest short-term path is usually: keep the BSP-selected panel geometry, disable extra rotation, then validate display and touch first.

## Strong Candidate Logic Bug

### 2. Geometry assumptions are hardcoded and can drift from the BSP

`firmware/main/home/theme.h` hardcodes:

- `SCREEN_WIDTH 800`
- `SCREEN_HEIGHT 1280`

That works only as long as the runtime LVGL display resolution exactly matches those constants.

With the current BSP + possible rotation changes, this can become wrong immediately and will break:

- home screen layout sizing
- drawing canvas size
- touch coordinate mapping assumptions

### Fix direction

- Replace hardcoded screen size macros with runtime values captured after display init.
- A small `DisplayMetrics` helper or central getter is enough.
- Make home layout, calculator layout, and drawing canvas query the active LVGL display instead of assuming portrait constants.

This is the main structural fix that will make the code robust when you add overlays later.

## Reference Repo Warning

### 3. Do not copy the `esp-brookesia_example` product `main.cpp` blindly

The reference file:

- `docs/esp-brookesia_example/products/phone/phone_waveshare_p4_wifi6/main/main.cpp`

still uses the old `bsp_display_cfg_t` field layout (`lvgl_port_cfg`, `buffer_size`, `double_buffer`, etc.).

That file is not trustworthy as a direct implementation source against the currently installed BSP 1.0.6 headers used by this project.

Use the reference repo for:

- app-flow ideas
- system UI concepts
- stylesheet/layout behavior

Do not copy its low-level display bring-up code without reconciling it against the local managed component headers first.

## Code Fixes Still Worth Doing

### 4. Calculator cleanup

The current calculator builds, but it still has embedded-system hygiene issues:

- `isStartZero()` logic is fragile and should be simplified/retested.
- `FORMULA_LEN_MAX` protects the formula label, but history growth is still effectively unbounded over long sessions.
- Division-by-zero currently returns `0`, which is misleading UI behavior.

Recommended changes:

- Make token parsing explicit instead of relying on character-class edge cases.
- Cap history length or rotate old entries out.
- Show an error state (`ERR`) for divide-by-zero instead of silently returning zero.

## Performance / Memory Fixes

### 5. Drawing app is expensive for this board even if PSRAM is large enough

The drawing app currently:

- allocates a full-screen-ish canvas in PSRAM
- uses `LV_COLOR_FORMAT_ARGB8888` even though the panel is `RGB565`
- draws lines by calling `drawDot()` for every step
- opens and closes a canvas layer inside every `drawDot()`

This is functional, but it is much heavier than it needs to be.

### Fix direction

- Prefer `RGB565` canvas storage unless alpha is truly needed.
- Avoid `lv_canvas_init_layer()` / `lv_canvas_finish_layer()` per pixel.
- Batch draw per stroke, or write directly into the backing buffer and invalidate only the touched region.
- Allocate with capabilities that are explicitly CPU-safe for byte access, not only `MALLOC_CAP_SPIRAM`, if you see access issues.

This will matter once overlays, animations, and more apps are added.

## Touch / Orientation Validation

### 6. Touch mapping must be validated after any display rotation change

The code currently assumes:

- canvas starts at `STATUS_BAR_HEIGHT`
- X is unshifted
- touch transform flags are all zero

That is only correct if the final display orientation and the GT911 mapping match that assumption.

After any rotation change, validate:

- tap targets line up with visuals
- drawing happens directly under the finger
- back button hitbox is correct

If not, fix the touch transform in the BSP config first, not with ad hoc coordinate hacks in each app.

## Architecture Guidance For The Actual Product Goal

The real product is not just a launcher. It is a home screen with overlays that control other apps.

Do not model every future surface as a full-screen app.

### Recommended architecture

- Keep the current screen-per-app isolation for heavyweight apps.
- Add a separate overlay manager for:
  - quick controls
  - slide-over panels
  - modal sheets
  - app-specific floating controls
- Implement overlays as child containers on the home/root screen, not as separate LVGL screens.

That gives you:

- lower churn than screen swapping
- easier animation
- better fit for control panels and future “widget” behavior

## Documentation Drift To Clean Up

- `README.md` still says the project is “not yet built or tested on hardware,” but a successful build already exists.
- Some comments still describe the old display-orientation model.

Clean this up after the runtime issues are resolved so the docs stop pushing future edits in the wrong direction.

## Practical Work Order

1. Stabilize display bring-up on the real board:
   - remove extra rotation first
   - verify display orientation
   - verify touch alignment
2. Resolve the PPA freeze risk:
   - either patch ESP-IDF or avoid the risky render-mode/rotation combo
3. Remove hardcoded screen geometry from app/layout code
4. Add an overlay manager on top of the home screen instead of turning overlays into full apps
5. Then clean up calculator and drawing performance issues

## Short Version

The firmware compiles. The likely problems now are hardware bring-up mismatches, especially display rotation, touch alignment, and the known ESP32-P4 PPA freeze path caused by `TRIPLE_PARTIAL + 90-degree rotation` on an unpatched ESP-IDF 5.5.1 install. Fix those first, then make layout sizing dynamic, then add overlays cleanly on top of the home screen.
