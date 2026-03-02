/*
 * status_bar.cpp - Status bar with clock and status indicators
 *
 * NOTE ON TIME: The ESP32-P4 doesn't have an RTC battery, so the clock
 * starts at 00:00 on boot. Once we add WiFi (via ESP32-C6), we can
 * sync time with NTP (internet time servers). For now it just shows
 * the system uptime-based time.
 */
#include "status_bar.h"
#include "theme.h"
#include <time.h>
#include <stdio.h>

StatusBar::StatusBar()
    : _container(nullptr), _clockLabel(nullptr),
      _wifiIcon(nullptr), _batteryIcon(nullptr)
{
}

StatusBar::~StatusBar()
{
    // LVGL objects are deleted when their parent screen is deleted,
    // so we don't need to manually free them here.
}

void StatusBar::create(lv_obj_t* parent)
{
    // Create the bar container - full width, STATUS_BAR_HEIGHT tall
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, SCREEN_WIDTH, STATUS_BAR_HEIGHT);
    lv_obj_align(_container, LV_ALIGN_TOP_MID, 0, 0);

    // Style: white background, no border radius, subtle bottom line
    lv_obj_set_style_bg_color(_container, THEME_STATUS_BAR_BG, 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(_container, 0, 0);
    lv_obj_set_style_border_width(_container, 0, 0);
    lv_obj_set_style_pad_all(_container, 0, 0);

    // Add a subtle bottom border to separate from content below
    lv_obj_set_style_border_side(_container, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(_container, 1, 0);
    lv_obj_set_style_border_color(_container, THEME_DIVIDER_COLOR, 0);

    // Don't let the status bar scroll
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);

    // === Clock label (left side) ===
    _clockLabel = lv_label_create(_container);
    lv_obj_set_style_text_font(_clockLabel, THEME_FONT_CLOCK, 0);
    lv_obj_set_style_text_color(_clockLabel, THEME_STATUS_BAR_TEXT, 0);
    lv_label_set_text(_clockLabel, "00:00");
    // Position: vertically centered, 16px from left edge
    lv_obj_align(_clockLabel, LV_ALIGN_LEFT_MID, 16, 0);

    // === WiFi icon (right side, placeholder) ===
    // LV_SYMBOL_WIFI is a built-in LVGL icon from the symbol font
    _wifiIcon = lv_label_create(_container);
    lv_obj_set_style_text_color(_wifiIcon, THEME_TEXT_SECONDARY, 0);
    lv_label_set_text(_wifiIcon, LV_SYMBOL_WIFI);
    lv_obj_align(_wifiIcon, LV_ALIGN_RIGHT_MID, -48, 0);

    // === Battery icon (far right, placeholder) ===
    _batteryIcon = lv_label_create(_container);
    lv_obj_set_style_text_color(_batteryIcon, THEME_TEXT_SECONDARY, 0);
    lv_label_set_text(_batteryIcon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_align(_batteryIcon, LV_ALIGN_RIGHT_MID, -16, 0);
}

void StatusBar::updateClock()
{
    if (_clockLabel == nullptr) return;

    /*
     * Get current system time.
     * time() returns seconds since epoch. localtime_r() converts to
     * hours/minutes/seconds in the device's timezone.
     *
     * On fresh boot without NTP, this will show 00:00 (epoch).
     * That's fine for now - we'll fix it when WiFi is added.
     */
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    // Format as HH:MM (24-hour). You could change to 12-hour with AM/PM if preferred.
    char timeBuf[16];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    lv_label_set_text(_clockLabel, timeBuf);
}
