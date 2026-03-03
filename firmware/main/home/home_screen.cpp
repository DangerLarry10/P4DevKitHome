/*
 * home_screen.cpp - Home screen coordinator
 *
 * This creates the home screen's LVGL screen object and populates it
 * with the status bar (top) and app list (below). It also starts a
 * periodic timer to keep the clock updated.
 */
#include "home_screen.h"
#include "app_manager.h"
#include "theme.h"
#include "esp_log.h"

static const char* TAG = "HomeScreen";

HomeScreen::HomeScreen()
    : _screen(nullptr), _manager(nullptr), _clockTimer(nullptr)
{
}

HomeScreen::~HomeScreen()
{
    // Delete the clock timer if it exists
    if (_clockTimer != nullptr) {
        lv_timer_delete(_clockTimer);
        _clockTimer = nullptr;
    }
}

void HomeScreen::create(AppManager* manager)
{
    _manager = manager;

    ESP_LOGI(TAG, "Creating home screen");

    /*
     * Create a new screen object.
     * lv_obj_create(NULL) = new screen (root-level, can be loaded).
     * This is different from lv_obj_create(parent) which creates a child widget.
     */
    _screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(_screen, THEME_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);

    // Tell the AppManager where to return to when apps close
    manager->setHomeScreen(_screen);

    // Build the status bar at the top
    _statusBar.create(_screen);

    // Build the app list below the status bar
    _appList.create(_screen, manager);

    // Do an initial clock update
    _statusBar.updateClock();

    /*
     * Create a periodic timer to update the clock.
     * 30000ms = 30 seconds. We don't need per-second updates since
     * we only show hours:minutes.
     *
     * The timer stores a pointer to `this` so the static callback
     * can access our StatusBar instance.
     */
    _clockTimer = lv_timer_create(clockTimerCb, 30000, this);
    ESP_LOGI(TAG, "Clock timer started (30s interval)");
}

void HomeScreen::show()
{
    if (_screen != nullptr) {
        lv_scr_load(_screen);
    }
}

/*
 * clockTimerCb() - Static callback invoked by LVGL's timer system.
 * Retrieves the HomeScreen pointer from timer->user_data and updates the clock.
 */
void HomeScreen::clockTimerCb(lv_timer_t* timer)
{
    HomeScreen* home = (HomeScreen*)lv_timer_get_user_data(timer);
    if (home != nullptr) {
        home->_statusBar.updateClock();
    }
}
