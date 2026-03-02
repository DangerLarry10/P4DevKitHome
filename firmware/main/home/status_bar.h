/*
 * status_bar.h - Top status bar showing clock and status icons
 *
 * Layout (800px wide, 48px tall):
 * ┌──────────────────────────────────────┐
 * │  12:34 PM              WiFi  Battery │
 * └──────────────────────────────────────┘
 *
 * The clock is updated by an LVGL timer every 30 seconds.
 * WiFi and battery are placeholders for now - they'll show static icons
 * until we integrate the ESP32-C6 coprocessor for actual WiFi status.
 */
#pragma once

#include "lvgl.h"

class StatusBar {
public:
    StatusBar();
    ~StatusBar();

    /*
     * create() - Build the status bar as a child of `parent`.
     * Call this once during home screen setup.
     */
    void create(lv_obj_t* parent);

    /*
     * updateClock() - Refresh the clock display.
     * Called by an LVGL timer callback.
     */
    void updateClock();

    // Get the bar's LVGL object (for layout purposes)
    lv_obj_t* getContainer() const { return _container; }

private:
    lv_obj_t* _container;       // The bar itself (white rectangle)
    lv_obj_t* _clockLabel;      // "12:34 PM" text
    lv_obj_t* _wifiIcon;        // WiFi status icon (placeholder)
    lv_obj_t* _batteryIcon;     // Battery icon (placeholder)
};
