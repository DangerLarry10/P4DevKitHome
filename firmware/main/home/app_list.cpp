/*
 * app_list.cpp - Scrollable app launcher list
 *
 * DESIGN CHOICE: We use plain LVGL objects styled as "cards" rather than
 * lv_list or lv_menu. This gives us full control over the look and feel,
 * and it's easier to understand what's happening under the hood.
 */
#include "app_list.h"
#include "app_manager.h"
#include "theme.h"
#include "esp_log.h"

static const char* TAG = "AppList";

AppList::AppList()
    : _container(nullptr), _manager(nullptr)
{
}

AppList::~AppList()
{
}

void AppList::create(lv_obj_t* parent, AppManager* manager)
{
    _manager = manager;

    /*
     * Create a scrollable container that fills the area below the status bar.
     * We position it explicitly below STATUS_BAR_HEIGHT and make it fill
     * the remaining height.
     */
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT);
    lv_obj_align(_container, LV_ALIGN_TOP_MID, 0, STATUS_BAR_HEIGHT);

    // Style: transparent background, no border
    lv_obj_set_style_bg_color(_container, THEME_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(_container, 0, 0);
    lv_obj_set_style_border_width(_container, 0, 0);
    lv_obj_set_style_pad_all(_container, APP_LIST_PAD, 0);
    lv_obj_set_style_pad_row(_container, APP_LIST_PAD, 0);

    /*
     * Use LVGL's flex layout - items stack vertically.
     * LV_FLEX_FLOW_COLUMN means children are arranged top-to-bottom.
     * This is similar to CSS flexbox with flex-direction: column.
     */
    lv_obj_set_flex_flow(_container, LV_FLEX_FLOW_COLUMN);

    // Make it scrollable (the default, but explicit is clearer)
    lv_obj_add_flag(_container, LV_OBJ_FLAG_SCROLLABLE);

    /*
     * Build one row for each registered app.
     * We loop through all apps in the manager and create a styled "card" for each.
     */
    for (int i = 0; i < manager->getAppCount(); i++) {
        AppBase* app = manager->getApp(i);
        if (app == nullptr) continue;

        // === Card container (the tappable row) ===
        lv_obj_t* card = lv_obj_create(_container);
        lv_obj_set_size(card, SCREEN_WIDTH - (APP_LIST_PAD * 2), APP_LIST_ITEM_HEIGHT);

        // White card with rounded corners
        lv_obj_set_style_bg_color(card, THEME_CARD_COLOR, 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(card, 12, 0);
        lv_obj_set_style_border_width(card, 0, 0);
        lv_obj_set_style_shadow_width(card, 4, 0);
        lv_obj_set_style_shadow_opa(card, LV_OPA_10, 0);
        lv_obj_set_style_shadow_offset_y(card, 2, 0);
        lv_obj_set_style_pad_all(card, APP_LIST_PAD, 0);

        // Light blue tint when pressed (visual feedback for touch)
        lv_obj_set_style_bg_color(card, THEME_PRESSED_COLOR, LV_STATE_PRESSED);

        // Make it clickable
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        /*
         * Store the app index as user_data on the card object.
         * When the card is tapped, the callback reads this to know which app to launch.
         * We cast int -> void* which is safe for small positive integers.
         */
        lv_obj_add_event_cb(card, onAppClicked, LV_EVENT_CLICKED, this);
        lv_obj_set_user_data(card, (void*)(intptr_t)i);

        // === Icon placeholder ===
        // For now, we show a colored circle with the first letter of the app name.
        // Later this can be replaced with actual icon images.
        lv_obj_t* iconCircle = lv_obj_create(card);
        lv_obj_set_size(iconCircle, APP_ICON_SIZE, APP_ICON_SIZE);
        lv_obj_align(iconCircle, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_bg_color(iconCircle, THEME_ACCENT_COLOR, 0);
        lv_obj_set_style_bg_opa(iconCircle, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(iconCircle, APP_ICON_SIZE / 2, 0);  // Fully round
        lv_obj_set_style_border_width(iconCircle, 0, 0);
        lv_obj_clear_flag(iconCircle, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(iconCircle, LV_OBJ_FLAG_CLICKABLE);

        // First letter of app name as icon text
        char initial[2] = { app->getName()[0], '\0' };
        lv_obj_t* iconLabel = lv_label_create(iconCircle);
        lv_label_set_text(iconLabel, initial);
        lv_obj_set_style_text_color(iconLabel, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(iconLabel, THEME_FONT_TITLE, 0);
        lv_obj_center(iconLabel);

        // === App name (bold, larger font) ===
        lv_obj_t* nameLabel = lv_label_create(card);
        lv_label_set_text(nameLabel, app->getName());
        lv_obj_set_style_text_font(nameLabel, THEME_FONT_BODY, 0);
        lv_obj_set_style_text_color(nameLabel, THEME_TEXT_PRIMARY, 0);
        // Position to the right of the icon, near the top
        lv_obj_align(nameLabel, LV_ALIGN_TOP_LEFT, APP_ICON_SIZE + APP_LIST_PAD, 4);

        // === Description (smaller, gray) ===
        lv_obj_t* descLabel = lv_label_create(card);
        lv_label_set_text(descLabel, app->getDescription());
        lv_obj_set_style_text_font(descLabel, THEME_FONT_SMALL, 0);
        lv_obj_set_style_text_color(descLabel, THEME_TEXT_SECONDARY, 0);
        // Position below the name
        lv_obj_align(descLabel, LV_ALIGN_TOP_LEFT, APP_ICON_SIZE + APP_LIST_PAD, 28);

        ESP_LOGI(TAG, "Added app to list: %s", app->getName());
    }
}

/*
 * onAppClicked() - Static callback when an app row is tapped.
 *
 * HOW WE KNOW WHICH APP: Each card stores its index via lv_obj_set_user_data().
 * The AppList pointer comes in via the event's user_data parameter.
 */
void AppList::onAppClicked(lv_event_t* e)
{
    AppList* self = (AppList*)lv_event_get_user_data(e);
    if (self == nullptr || self->_manager == nullptr) return;

    // Get the card that was clicked
    lv_obj_t* target = lv_event_get_target(e);

    // Retrieve the app index stored on this card
    int appIndex = (int)(intptr_t)lv_obj_get_user_data(target);

    ESP_LOGI(TAG, "App tapped at index %d", appIndex);
    self->_manager->launchApp(appIndex);
}
