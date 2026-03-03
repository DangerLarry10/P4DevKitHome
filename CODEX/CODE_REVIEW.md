# Code Review

Scope: reviewed all files under `firmware/` for build issues, API misuse, missing includes, and likely runtime bugs. I did not modify any source files.

I could not run a real ESP-IDF build in this shell because `idf.py` is not installed here, so the findings below are from source review plus checking the vendored reference headers under `docs/esp-brookesia_example/.../managed_components/`.

## Findings

1. **Build blocker: `main.cpp` initializes `bsp_display_cfg_t` with fields that do not exist in the pinned BSP headers.**
   `firmware/main/main.cpp:77` through `firmware/main/main.cpp:100` uses `.lvgl_port_cfg`, `.buffer_size`, `.double_buffer`, `.hw_cfg`, and `.flags`, and the nested macro at `firmware/main/main.cpp:40` through `firmware/main/main.cpp:47` uses `.task_stack`, `.task_affinity`, `.task_max_sleep_ms`, and `.timer_period_ms`.

   The vendored `waveshare/esp32_p4_platform` 1.0.6 headers in this repo define `bsp_display_cfg_t` differently: `docs/esp-brookesia_example/products/phone/phone_waveshare_p4_wifi6/managed_components/waveshare__esp32_p4_platform/include/bsp/esp32_p4_platform.h:249` only has `lv_adapter_cfg`, `rotation`, `tear_avoid_mode`, and `touch_flags`. The nested `esp_lv_adapter_config_t` in `docs/esp-brookesia_example/products/phone/phone_waveshare_p4_wifi6/managed_components/espressif__esp_lvgl_adapter/include/esp_lv_adapter.h:30` uses `task_stack_size`, `task_core_id`, `tick_period_ms`, `task_min_delay_ms`, and `task_max_delay_ms`.

   As written, this should fail to compile against the dependency versions declared in `firmware/main/idf_component.yml`.

2. **API misuse in LVGL locking: `bsp_display_lock(0)` is treated as a zero-timeout try-lock, not “wait forever”, and the return value is ignored.**
   `firmware/main/main.cpp:125` says `0 = wait forever`, and `firmware/main/main.cpp:127` calls `bsp_display_lock(0);` before creating the UI.

   The vendored adapter API says otherwise: `docs/esp-brookesia_example/products/phone/phone_waveshare_p4_wifi6/managed_components/espressif__esp_lvgl_adapter/include/esp_lv_adapter.h:125` documents “use `-1` for infinite wait”, and the implementation at `.../esp_lv_adapter.c:209` converts only negative values to `portMAX_DELAY`. A `0` becomes `pdMS_TO_TICKS(0)`, which is a non-blocking take. Because the code also does not check the returned `esp_err_t`, UI creation can proceed without owning the LVGL mutex, which is exactly the race this lock is supposed to prevent.

3. **Calculator leading-zero suppression is broken due to an impossible condition.**
   In `firmware/main/apps/calculator/calculator_app.cpp:213` through `firmware/main/apps/calculator/calculator_app.cpp:214`, `isStartZero()` checks:

   `text[len - 2] > '9' && text[len - 2] < '0'`

   No character can be both greater than `'9'` and less than `'0'` at the same time, so that branch can never be true. The function therefore only recognizes the literal `"0"` case and fails to detect `"1+0"`, `"2-0"`, etc. That breaks the intended “replace the leading zero in the current number segment” behavior and allows inputs like `1+05`.

4. **The calculator defines a max formula length but never enforces it, so text buffers can grow without bound.**
   `firmware/main/apps/calculator/calculator_app.cpp:31` defines `FORMULA_LEN_MAX` as 256, but none of the insertion paths check it before appending more text:

   - `firmware/main/apps/calculator/calculator_app.cpp:378`
   - `firmware/main/apps/calculator/calculator_app.cpp:391`
   - `firmware/main/apps/calculator/calculator_app.cpp:399`

   This means repeated input can keep expanding the label text allocation indefinitely. On an embedded UI, that is an avoidable memory-growth path and can eventually lead to fragmentation or allocation failure.

5. **Missing direct include: `calculator_app.cpp` uses `isdigit()` without including `<cctype>`.**
   `firmware/main/apps/calculator/calculator_app.cpp:271` calls `isdigit(input[i])`, but the file only includes `<math.h>`, `<vector>`, `<cstring>`, and `<cstdio>` at `firmware/main/apps/calculator/calculator_app.cpp:20` through `firmware/main/apps/calculator/calculator_app.cpp:23`.

   This may compile only because of transitive includes elsewhere, but the file itself is missing the standard header that declares `isdigit()`. That is a portability/build-hygiene issue and can turn into a compile error depending on toolchain and include order.

## Notes

- I did not find obvious issues in the rest of the `firmware/main/home/*` or `firmware/main/apps/drawing/*` files that were as severe as the items above.
- The local ESP-Brookesia example under `docs/` appears to contain the same stale display-config pattern as `firmware/main/main.cpp`, so this code was likely copied from a sample that is out of sync with the vendored BSP package.
