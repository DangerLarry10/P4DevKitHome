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

/*
 * LVGL port configuration.
 * This tells esp_lvgl_port (used inside the BSP) how to run LVGL:
 * - task_priority: FreeRTOS priority (4 is moderate - not too high, not too low)
 * - task_stack: Stack size for the LVGL task (10KB is generous)
 * - task_affinity: -1 means "run on any CPU core" (P4 has dual cores)
 * - task_max_sleep_ms: Max time LVGL task sleeps between frames
 * - timer_period_ms: How often LVGL's internal timer ticks (5ms = 200Hz)
 */
#define LVGL_PORT_INIT_CONFIG() \
    {                               \
        .task_priority = 4,         \
        .task_stack = 10 * 1024,    \
        .task_affinity = -1,        \
        .task_max_sleep_ms = 500,   \
        .timer_period_ms = 5,       \
    }

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
    // - MIPI-DSI bus configuration
    // - JD9365 display driver initialization
    // - GT911 touch controller setup
    // - LVGL display/input device registration
    // - Double-buffered rendering in PSRAM
    ESP_LOGI(TAG, "Initializing display...");

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = LVGL_PORT_INIT_CONFIG(),
        /*
         * buffer_size: How many pixels in the render buffer.
         * Full-screen buffer (800*1280 = 1,024,000 pixels) gives the smoothest
         * rendering with no tearing artifacts. This uses ~4MB of PSRAM per buffer.
         */
        .buffer_size = BSP_LCD_H_RES * BSP_LCD_V_RES,
        /*
         * double_buffer: Use two buffers so LVGL can render to one while
         * the other is being sent to the display. Eliminates flickering.
         */
        .double_buffer = true,
        .hw_cfg = {
            .dsi_bus = {
                .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
                .lane_bit_rate_mbps = BSP_LCD_MIPI_DSI_LANE_BITRATE_MBPS,
            }
        },
        .flags = {
            .buff_dma = false,       // DMA can't access PSRAM on P4
            .buff_spiram = true,     // Buffers go in PSRAM (we have 32MB)
            .sw_rotate = true,       // Rotate from native landscape to portrait
        }
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
     * The parameter is a timeout in milliseconds. 0 = wait forever.
     */
    bsp_display_lock(0);

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
