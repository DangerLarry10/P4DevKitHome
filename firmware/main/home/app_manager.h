/*
 * app_manager.h - Manages app registration, launching, and closing
 *
 * This is the "traffic controller" of the home screen. It keeps a list of
 * all registered apps, handles launching them (creating screens, calling onRun),
 * and closing them (calling onClose, deleting screens, restoring home).
 *
 * Only ONE app can be active at a time. The home screen is shown when no app is active.
 */
#pragma once

#include <vector>
#include "lvgl.h"
#include "apps/app_base.h"

// Forward declaration - we reference HomeScreen but don't need its full definition here
class HomeScreen;

class AppManager {
public:
    AppManager();
    ~AppManager();

    /*
     * registerApp() - Add an app to the launcher list.
     * Call this during setup for each app you want available.
     * The AppManager does NOT own the apps - don't delete them while registered.
     */
    void registerApp(AppBase* app);

    /*
     * launchApp() - Switch from home screen to the specified app.
     * Creates a new LVGL screen, sets it active, calls app->onRun().
     * appIndex: position in the registered apps list (0-based).
     */
    void launchApp(int appIndex);

    /*
     * closeActiveApp() - Close the currently running app and return to home.
     * Calls app->onClose(), deletes the app's screen, shows home screen.
     */
    void closeActiveApp();

    // === Getters ===
    int getAppCount() const;
    AppBase* getApp(int index) const;
    AppBase* getActiveApp() const;  // nullptr if on home screen

    // The home screen sets this so we know what to restore when closing an app
    void setHomeScreen(lv_obj_t* homeScreen);

private:
    std::vector<AppBase*> _apps;     // All registered apps
    AppBase* _activeApp;             // Currently running app (nullptr = home)
    lv_obj_t* _homeScreen;           // Cached home screen to restore

    // Creates a back button overlaid on the app screen
    void createBackButton(lv_obj_t* screen);
    // Callback when back button is tapped
    static void onBackBtnClicked(lv_event_t* e);
};
