/*
 * main.cpp - Entry point for P4DevKitHome
 *
 * This is where everything starts. The app_main() function is called by
 * ESP-IDF's FreeRTOS after the system boots up. Here we:
 *
 * 1. Initialize the display and touch using the Waveshare BSP
 * 2. Set up LVGL (the GUI library)
 * 3. Create our apps and register them with the AppManager
 * 4. Show the home screen
 *
 * IMPORTANT: LVGL is NOT thread-safe. Any time you touch LVGL objects
 * from code that might run on a different thread, you MUST lock first
 * using bsp_display_lock() / bsp_display_unlock(). The BSP creates an
 * LVGL task that runs the rendering loop, and we need to coordinate with it.
 */
#include "bsp/esp-bsp.h"        // Waveshare BSP - hardware init
#include "lvgl.h"                // LVGL GUI library
#include "esp_log.h"             // ESP-IDF logging

// Our framework
#include "home/home_screen.h"
#include "home/app_manager.h"

// Our apps
#include "apps/calculator/calculator_app.h"
#include "apps/drawing/drawing_app.h"

static const char* TAG = "Main";

// Static instances - these live for the entire program lifetime
static AppManager appManager;
static HomeScreen homeScreen;
static CalculatorApp calculatorApp;
static DrawingApp drawingApp;

/*
 * app_main() - The entry point called by ESP-IDF after boot.
 *
 * This is like setup() in Arduino, except it only runs once and then
 * the FreeRTOS scheduler takes over (LVGL runs in its own task).
 */
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "=== P4DevKitHome Starting ===");
    ESP_LOGI(TAG, "Custom home screen for Waveshare ESP32-P4 DevKit");

    // ──────────────────────────────────────────────
    // STEP 1: Initialize the display hardware
    // ──────────────────────────────────────────────
    // The BSP handles ALL the low-level display setup:
    // - MIPI-DSI bus configuration (lanes, clock, etc.)
    // - JD9365 display driver initialization
    // - GT911 touch controller setup via I2C
    // - LVGL display/input device registration
    // - Frame buffer allocation in PSRAM
    ESP_LOGI(TAG, "Initializing display...");

    /*
     * bsp_display_cfg_t has these fields (from the Waveshare BSP headers):
     *   .lv_adapter_cfg  - LVGL task settings (stack, priority, timing)
     *   .rotation        - Software rotation (0, 90, 180, 270 degrees)
     *   .tear_avoid_mode - Frame tearing prevention strategy
     *   .touch_flags     - Touch coordinate transformations
     *
     * ESP_LV_ADAPTER_DEFAULT_CONFIG() fills in sensible defaults for the
     * LVGL task (8KB stack, priority 6, 1ms tick, -1 core affinity).
     *
     * ESP_LV_ADAPTER_ROTATE_90 rotates the display 90 degrees clockwise,
     * turning the native 1280x800 landscape into 800x1280 portrait.
     *
     * TRIPLE_PARTIAL uses 3 framebuffers for tear-free rendering on MIPI-DSI.
     * This is the recommended mode for our display type.
     */
    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg  = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation        = ESP_LV_ADAPTER_ROTATE_90,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,
        .touch_flags = {
            .swap_xy  = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    lv_display_t* disp = bsp_display_start_with_config(&cfg);
    if (disp == nullptr) {
        ESP_LOGE(TAG, "Failed to start display!");
        return;
    }
    ESP_LOGI(TAG, "Display initialized: %dx%d", BSP_LCD_H_RES, BSP_LCD_V_RES);

    // Turn on the backlight
    esp_err_t ret = bsp_display_backlight_on();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to turn on backlight: %s", esp_err_to_name(ret));
    }

    // ──────────────────────────────────────────────
    // STEP 2: Lock LVGL and build our UI
    // ──────────────────────────────────────────────
    /*
     * bsp_display_lock() / bsp_display_unlock() are CRITICAL.
     * The BSP runs LVGL in a separate FreeRTOS task. If we modify LVGL
     * objects from app_main() at the same time, we get crashes.
     * The lock is a mutex (mutual exclusion) - only one thread can hold it.
     *
     * The BSP's bsp_display_lock() wraps esp_lv_adapter_lock() which uses
     * -1 for infinite wait (portMAX_DELAY). Since the BSP declares uint32_t,
     * we pass UINT32_MAX which converts to -1 in the adapter's int32_t param.
     * Using 0 would be a non-blocking try that could fail silently!
     * We also check the return value - if we can't get the lock, don't proceed.
     */
    if (bsp_display_lock(UINT32_MAX) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to acquire display lock!");
        return;
    }

    ESP_LOGI(TAG, "Registering apps...");

    // Register our apps with the manager
    appManager.registerApp(&calculatorApp);
    appManager.registerApp(&drawingApp);

    // Build and show the home screen
    ESP_LOGI(TAG, "Creating home screen...");
    homeScreen.create(&appManager);
    homeScreen.show();

    // Release the lock so LVGL can render
    bsp_display_unlock();

    ESP_LOGI(TAG, "=== P4DevKitHome Ready ===");
    ESP_LOGI(TAG, "Registered %d apps", appManager.getAppCount());

    /*
     * That's it! At this point:
     * - The LVGL task (created by BSP) is running and rendering frames
     * - The home screen is visible with status bar and app list
     * - Touch events are being processed by LVGL
     * - Our clock timer fires every 30 seconds
     *
     * app_main() returns here, but the program keeps running because
     * FreeRTOS tasks (like LVGL's) continue in the background.
     */
}
