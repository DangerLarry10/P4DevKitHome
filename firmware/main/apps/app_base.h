/*
 * app_base.h - Base class that ALL apps must inherit from
 *
 * This defines the "contract" between the home screen framework and individual apps.
 * Every app must implement onRun() at minimum. The framework handles screen creation,
 * switching, and cleanup.
 *
 * HOW IT WORKS:
 * 1. AppManager creates a new LVGL screen for the app
 * 2. AppManager sets that screen as active, then calls onRun()
 * 3. The app builds its UI on lv_scr_act() (which is now its screen)
 * 4. When the user taps "back", onBack() is called
 * 5. AppManager deletes the app's screen and restores the home screen
 *
 * Each app gets its own screen object - this provides COMPLETE isolation.
 * One app can't accidentally mess up another app's UI.
 */
#pragma once

#include "lvgl.h"

class AppBase {
public:
    /*
     * Constructor - store metadata about this app.
     * name:        Display name shown in the app list (e.g., "Calculator")
     * description: One-line summary shown below the name
     * icon:        Pointer to an LVGL image descriptor (nullptr = no icon)
     */
    AppBase(const char* name, const char* description, const lv_image_dsc_t* icon = nullptr)
        : _name(name), _description(description), _icon(icon), _screen(nullptr) {}

    // Virtual destructor - required because we delete apps through base class pointers.
    // Without this, the derived class's destructor wouldn't run = memory leak.
    virtual ~AppBase() {}

    // === Lifecycle methods (apps override these) ===

    /*
     * onRun() - Called when the app is launched.
     * BUILD YOUR ENTIRE UI HERE. Use lv_scr_act() as the parent.
     * Return true on success, false if something went wrong.
     */
    virtual bool onRun() = 0;  // "= 0" means PURE VIRTUAL: every app MUST implement this

    /*
     * onClose() - Called when the app is being closed.
     * Free any resources you allocated in onRun() (timers, buffers, etc.).
     * The screen itself is deleted by the framework - you don't need to delete LVGL objects.
     * Default implementation does nothing - override only if you need cleanup.
     */
    virtual bool onClose() { return true; }

    /*
     * onBack() - Called when the back button is pressed.
     * Return true to allow closing, false to prevent it (e.g., "save changes?" dialog).
     * Default: just close the app.
     */
    virtual bool onBack() { return true; }

    // === Getters (used by AppManager and AppList to display info) ===
    const char* getName() const { return _name; }
    const char* getDescription() const { return _description; }
    const lv_image_dsc_t* getIcon() const { return _icon; }

    // The framework sets this when launching the app
    void setScreen(lv_obj_t* screen) { _screen = screen; }
    lv_obj_t* getScreen() const { return _screen; }

protected:
    lv_obj_t* _screen;  // The app's dedicated LVGL screen (set by framework)

private:
    const char* _name;
    const char* _description;
    const lv_image_dsc_t* _icon;
};
