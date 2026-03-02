/*
 * app_manager.cpp - App lifecycle management
 *
 * KEY CONCEPT: Screen-based isolation
 * Each app gets its own lv_obj_create(NULL) screen. LVGL can only show one screen
 * at a time. When we launch an app, we switch to its screen. When we close it,
 * we switch back to the home screen and delete the app's screen.
 *
 * This means apps can't interfere with each other's UI - they each have
 * a completely separate widget tree.
 */
#include "app_manager.h"
#include "theme.h"
#include "esp_log.h"

static const char* TAG = "AppManager";

AppManager::AppManager()
    : _activeApp(nullptr), _homeScreen(nullptr)
{
}

AppManager::~AppManager()
{
    // We don't delete the apps themselves - the caller owns them
}

void AppManager::registerApp(AppBase* app)
{
    if (app == nullptr) {
        ESP_LOGW(TAG, "Tried to register null app");
        return;
    }
    _apps.push_back(app);
    ESP_LOGI(TAG, "Registered app: %s (%d total)", app->getName(), (int)_apps.size());
}

void AppManager::launchApp(int appIndex)
{
    // Safety check: is the index valid?
    if (appIndex < 0 || appIndex >= (int)_apps.size()) {
        ESP_LOGE(TAG, "Invalid app index: %d", appIndex);
        return;
    }

    // Don't launch if an app is already running
    if (_activeApp != nullptr) {
        ESP_LOGW(TAG, "App already active, closing first");
        closeActiveApp();
    }

    AppBase* app = _apps[appIndex];
    ESP_LOGI(TAG, "Launching app: %s", app->getName());

    /*
     * Create a brand new LVGL screen for this app.
     * lv_obj_create(NULL) creates a "screen" object - a root-level container
     * that can be loaded with lv_scr_load(). This is different from creating
     * a normal widget (which needs a parent).
     */
    lv_obj_t* appScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(appScreen, THEME_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(appScreen, LV_OPA_COVER, 0);

    // Give the app a reference to its screen
    app->setScreen(appScreen);

    /*
     * Switch to the app's screen.
     * lv_scr_load() makes this screen visible. The previous screen (home)
     * is NOT deleted - it stays in memory so we can switch back quickly.
     */
    lv_scr_load(appScreen);

    // Now call the app's onRun() - it builds its UI on lv_scr_act()
    _activeApp = app;
    if (!app->onRun()) {
        ESP_LOGE(TAG, "App %s failed to run", app->getName());
        closeActiveApp();
        return;
    }

    // Add a back button on top of everything the app created
    createBackButton(appScreen);
}

void AppManager::closeActiveApp()
{
    if (_activeApp == nullptr) {
        ESP_LOGW(TAG, "No active app to close");
        return;
    }

    ESP_LOGI(TAG, "Closing app: %s", _activeApp->getName());

    // Let the app clean up its resources
    _activeApp->onClose();

    // Get reference to the app's screen before we clear the pointer
    lv_obj_t* appScreen = _activeApp->getScreen();
    _activeApp->setScreen(nullptr);
    _activeApp = nullptr;

    // Switch back to the home screen
    if (_homeScreen != nullptr) {
        lv_scr_load(_homeScreen);
    }

    /*
     * Delete the app's screen AFTER switching away from it.
     * This frees all LVGL objects that were children of that screen.
     * We use lv_obj_delete() (LVGL v9) instead of lv_obj_del() (v8).
     */
    if (appScreen != nullptr) {
        lv_obj_delete(appScreen);
    }
}

int AppManager::getAppCount() const
{
    return (int)_apps.size();
}

AppBase* AppManager::getApp(int index) const
{
    if (index < 0 || index >= (int)_apps.size()) {
        return nullptr;
    }
    return _apps[index];
}

AppBase* AppManager::getActiveApp() const
{
    return _activeApp;
}

void AppManager::setHomeScreen(lv_obj_t* homeScreen)
{
    _homeScreen = homeScreen;
}

/*
 * createBackButton() - Adds a small "X" button in the top-left corner
 *
 * This gives users a way to get back to the home screen from any app.
 * It's positioned in the status bar area and floats above all other content.
 * Later we can replace this with swipe gestures.
 */
void AppManager::createBackButton(lv_obj_t* screen)
{
    lv_obj_t* btn = lv_button_create(screen);
    lv_obj_set_size(btn, BACK_BTN_SIZE, BACK_BTN_SIZE);

    // Position in top-left with a small margin
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 8, 6);

    // Style: subtle rounded button
    lv_obj_set_style_bg_color(btn, THEME_BACK_BTN_COLOR, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0);
    lv_obj_set_style_radius(btn, BACK_BTN_SIZE / 2, 0);  // Fully round
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);

    // "X" label centered in the button
    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(label);

    /*
     * Event callback: when tapped, close the active app.
     * We pass `this` (the AppManager pointer) as user_data so the
     * static callback function can call closeActiveApp().
     */
    lv_obj_add_event_cb(btn, onBackBtnClicked, LV_EVENT_CLICKED, this);
}

/*
 * Static callback for the back button.
 * Static because LVGL callbacks are C-style function pointers - they can't be
 * regular member functions. We recover the AppManager pointer from user_data.
 */
void AppManager::onBackBtnClicked(lv_event_t* e)
{
    AppManager* manager = (AppManager*)lv_event_get_user_data(e);
    if (manager == nullptr) return;

    // Ask the app if it's OK to close (it might want to show a "save?" dialog)
    if (manager->_activeApp != nullptr && !manager->_activeApp->onBack()) {
        return;  // App said "not yet"
    }

    manager->closeActiveApp();
}
