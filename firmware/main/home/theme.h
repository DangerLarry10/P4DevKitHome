/*
 * theme.h - Central place for ALL visual constants
 *
 * WHY one file? So you can tweak colors, fonts, and sizes in ONE place
 * instead of hunting through multiple files. If you want a dark theme later,
 * just change these values.
 */
#pragma once

#include "lvgl.h"

// === Screen dimensions (800x1280 portrait) ===
// These match the Waveshare 10.1" MIPI-DSI display in portrait orientation.
#define SCREEN_WIDTH            800
#define SCREEN_HEIGHT           1280

// === Layout sizes ===
#define STATUS_BAR_HEIGHT       48      // Top bar with clock/icons
#define APP_LIST_ITEM_HEIGHT    80      // Each app row in the launcher
#define APP_ICON_SIZE           48      // Icon square size
#define APP_LIST_PAD            16      // Padding around list items
#define BACK_BTN_SIZE           36      // Back button in app screens

// === Light theme colors ===
// All colors are defined as hex values. lv_color_hex() converts them
// to LVGL's internal color format at compile time.
#define THEME_BG_COLOR              lv_color_hex(0xF0F0F0)  // Light gray background
#define THEME_CARD_COLOR            lv_color_hex(0xFFFFFF)  // White cards/surfaces
#define THEME_TEXT_PRIMARY          lv_color_hex(0x1A1A1A)  // Near-black text
#define THEME_TEXT_SECONDARY        lv_color_hex(0x666666)  // Gray descriptions
#define THEME_ACCENT_COLOR          lv_color_hex(0x2196F3)  // Material blue accent
#define THEME_STATUS_BAR_BG         lv_color_hex(0xFFFFFF)  // White status bar
#define THEME_STATUS_BAR_TEXT       lv_color_hex(0x333333)  // Dark status bar text
#define THEME_DIVIDER_COLOR         lv_color_hex(0xE0E0E0)  // Subtle dividers
#define THEME_PRESSED_COLOR         lv_color_hex(0xE3F2FD)  // Light blue on press
#define THEME_BACK_BTN_COLOR        lv_color_hex(0x757575)  // Gray back button

// === Calculator-specific colors ===
#define THEME_CALC_BG               lv_color_hex(0xFFFFFF)  // White background
#define THEME_CALC_OPERATOR         lv_color_hex(0x2196F3)  // Blue operators
#define THEME_CALC_EQUALS           lv_color_hex(0x2196F3)  // Blue equals

// === Drawing app colors ===
#define THEME_DRAW_CANVAS_BG        lv_color_hex(0xFFFFFF)  // White canvas

// === Fonts ===
// These reference LVGL's built-in Montserrat fonts.
// They must be enabled in sdkconfig.defaults (CONFIG_LV_FONT_MONTSERRAT_xx=y).
#define THEME_FONT_TITLE        &lv_font_montserrat_24     // App names, headings
#define THEME_FONT_BODY         &lv_font_montserrat_16     // Descriptions, body text
#define THEME_FONT_SMALL        &lv_font_montserrat_14     // Small labels
#define THEME_FONT_CLOCK        &lv_font_montserrat_20     // Status bar clock
#define THEME_FONT_CALC         &lv_font_montserrat_20     // Calculator keys
#define THEME_FONT_CALC_RESULT  &lv_font_montserrat_28     // Calculator result display
