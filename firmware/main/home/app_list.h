/*
 * app_list.h - Scrollable list of installed apps
 *
 * This creates the main launcher view - a vertical list of tappable rows,
 * each showing an app's icon, name, and description.
 *
 * Layout per row (80px tall):
 * ┌──────────────────────────────────────┐
 * │  [icon]  App Name                    │
 * │          Short description text      │
 * └──────────────────────────────────────┘
 *
 * Tapping a row launches that app via AppManager.
 */
#pragma once

#include "lvgl.h"

class AppManager;  // Forward declaration

class AppList {
public:
    AppList();
    ~AppList();

    /*
     * create() - Build the app list below the status bar.
     * parent:  The home screen object to attach to
     * manager: Pointer to AppManager (used to get app info and launch apps)
     */
    void create(lv_obj_t* parent, AppManager* manager);

private:
    lv_obj_t* _container;     // Scrollable list container
    AppManager* _manager;      // Needed to launch apps on tap

    // Called when an app row is tapped
    static void onAppClicked(lv_event_t* e);
};
