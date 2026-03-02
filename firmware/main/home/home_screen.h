/*
 * home_screen.h - The main home screen that ties everything together
 *
 * This is the "coordinator" - it creates the status bar and app list,
 * arranges them on a screen, and manages the clock timer.
 *
 * Layout (800x1280):
 * ┌──────────────────────────────────────┐
 * │         Status Bar (48px)            │
 * ├──────────────────────────────────────┤
 * │                                      │
 * │         App List (scrollable)        │
 * │                                      │
 * │  [C]  Calculator                     │
 * │       Simple calculator app          │
 * │                                      │
 * │  [D]  Drawing                        │
 * │       Finger drawing canvas          │
 * │                                      │
 * └──────────────────────────────────────┘
 */
#pragma once

#include "lvgl.h"
#include "status_bar.h"
#include "app_list.h"

class AppManager;  // Forward declaration

class HomeScreen {
public:
    HomeScreen();
    ~HomeScreen();

    /*
     * create() - Build the entire home screen.
     * manager: The AppManager that provides app info and handles launching.
     * This creates a new LVGL screen, adds status bar and app list to it,
     * and registers the clock update timer.
     */
    void create(AppManager* manager);

    /*
     * show() - Switch the display to the home screen.
     * Used when returning from an app.
     */
    void show();

    // Get the underlying LVGL screen object
    lv_obj_t* getScreen() const { return _screen; }

private:
    lv_obj_t* _screen;          // The home screen LVGL object
    StatusBar _statusBar;        // Clock and status icons
    AppList _appList;            // Scrollable app launcher list
    AppManager* _manager;
    lv_timer_t* _clockTimer;    // Timer that updates the clock every 30s

    // Timer callback (static because LVGL callbacks are C-style)
    static void clockTimerCb(lv_timer_t* timer);
};
