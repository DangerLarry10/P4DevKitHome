/*
 * drawing_app.cpp - Finger drawing canvas
 *
 * HOW DRAWING WORKS:
 * 1. lv_canvas is an LVGL widget that wraps a raw pixel buffer
 * 2. We allocate this buffer in PSRAM (external RAM) because it's huge
 *    (800 x 1100+ pixels x 4 bytes per pixel = ~3.5MB!)
 * 3. On touch press: record the starting point, draw a dot
 * 4. On touch move: draw a line from the last point to current point
 * 5. On touch release: stop drawing
 *
 * The line drawing uses a simple approach: step along the line from
 * point A to B, placing dots at each step. This creates smooth lines
 * even with fast finger movements.
 */
#include "drawing_app.h"
#include "home/theme.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <cstring>
#include <cstdlib>

static const char* TAG = "DrawApp";

// Toolbar height at the bottom
#define TOOLBAR_HEIGHT      56

// Available brush colors
static const uint32_t PALETTE_COLORS[] = {
    0x1A1A1A,  // Black
    0xF44336,  // Red
    0x2196F3,  // Blue
    0x4CAF50,  // Green
    0xFF9800,  // Orange
};
#define PALETTE_COUNT  (sizeof(PALETTE_COLORS) / sizeof(PALETTE_COLORS[0]))

// Brush size options (radius in pixels)
static const int BRUSH_SIZES[] = { 2, 5, 10 };
#define BRUSH_SIZE_COUNT  (sizeof(BRUSH_SIZES) / sizeof(BRUSH_SIZES[0]))

DrawingApp::DrawingApp()
    : AppBase("Drawing", "Finger drawing canvas"),
      _canvas(nullptr), _canvasBuf(nullptr),
      _brushColor(lv_color_hex(0x1A1A1A)), _brushSize(5),
      _lastX(0), _lastY(0), _isDrawing(false),
      _canvasWidth(0), _canvasHeight(0), _toolbar(nullptr)
{
}

DrawingApp::~DrawingApp()
{
}

bool DrawingApp::onRun()
{
    ESP_LOGI(TAG, "Starting drawing app");

    lv_obj_t* screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, THEME_BG_COLOR, 0);

    /*
     * Calculate canvas size.
     * We need to fit below the back button area (STATUS_BAR_HEIGHT)
     * and above the toolbar (TOOLBAR_HEIGHT).
     */
    _canvasWidth = SCREEN_WIDTH;
    _canvasHeight = SCREEN_HEIGHT - STATUS_BAR_HEIGHT - TOOLBAR_HEIGHT;

    /*
     * Allocate the pixel buffer in PSRAM.
     * LVGL v9 canvases use LV_COLOR_FORMAT_ARGB8888 = 4 bytes per pixel.
     * For 800 x ~1176 pixels = ~3.7 MB. This MUST be in PSRAM (external RAM)
     * because internal SRAM is only ~768KB total on ESP32-P4.
     *
     * heap_caps_calloc() is like calloc() but lets you specify WHERE to allocate:
     * MALLOC_CAP_SPIRAM = allocate in PSRAM (the 32MB external RAM chip).
     */
    size_t bufSize = _canvasWidth * _canvasHeight * 4;  // 4 bytes per ARGB8888 pixel
    _canvasBuf = (uint8_t*)heap_caps_calloc(1, bufSize, MALLOC_CAP_SPIRAM);
    if (_canvasBuf == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate canvas buffer (%d bytes)", (int)bufSize);
        return false;
    }
    ESP_LOGI(TAG, "Canvas buffer: %dx%d = %d bytes (PSRAM)", _canvasWidth, _canvasHeight, (int)bufSize);

    // === Create the canvas widget ===
    _canvas = lv_canvas_create(screen);
    lv_canvas_set_buffer(_canvas, _canvasBuf, _canvasWidth, _canvasHeight, LV_COLOR_FORMAT_ARGB8888);
    lv_obj_align(_canvas, LV_ALIGN_TOP_MID, 0, STATUS_BAR_HEIGHT);

    // Fill canvas with white
    clearCanvas();

    /*
     * Register touch event handlers on the canvas.
     * LV_EVENT_PRESSED = finger down
     * LV_EVENT_PRESSING = finger moving while down
     * LV_EVENT_RELEASED = finger lifted
     *
     * We make the canvas clickable so it receives touch events.
     */
    lv_obj_add_flag(_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_canvas, canvasEventCb, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(_canvas, canvasEventCb, LV_EVENT_PRESSING, this);
    lv_obj_add_event_cb(_canvas, canvasEventCb, LV_EVENT_RELEASED, this);

    // Build the toolbar at the bottom
    createToolbar(screen);

    return true;
}

bool DrawingApp::onClose()
{
    ESP_LOGI(TAG, "Closing drawing app");

    // Free the PSRAM canvas buffer
    if (_canvasBuf != nullptr) {
        heap_caps_free(_canvasBuf);
        _canvasBuf = nullptr;
    }

    _canvas = nullptr;
    _toolbar = nullptr;
    return true;
}

void DrawingApp::createToolbar(lv_obj_t* parent)
{
    // Toolbar container at the bottom of the screen
    _toolbar = lv_obj_create(parent);
    lv_obj_set_size(_toolbar, SCREEN_WIDTH, TOOLBAR_HEIGHT);
    lv_obj_align(_toolbar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(_toolbar, THEME_CARD_COLOR, 0);
    lv_obj_set_style_bg_opa(_toolbar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(_toolbar, 0, 0);
    lv_obj_set_style_border_width(_toolbar, 0, 0);
    lv_obj_set_style_border_side(_toolbar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(_toolbar, 1, 0);
    lv_obj_set_style_border_color(_toolbar, THEME_DIVIDER_COLOR, 0);
    lv_obj_set_style_pad_all(_toolbar, 4, 0);
    lv_obj_clear_flag(_toolbar, LV_OBJ_FLAG_SCROLLABLE);

    // Use horizontal flex layout
    lv_obj_set_flex_flow(_toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(_toolbar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // === Color palette buttons ===
    for (size_t i = 0; i < PALETTE_COUNT; i++) {
        lv_obj_t* btn = lv_button_create(_toolbar);
        lv_obj_set_size(btn, 40, 40);
        lv_obj_set_style_bg_color(btn, lv_color_hex(PALETTE_COLORS[i]), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn, 20, 0);  // Fully round
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_border_color(btn, THEME_DIVIDER_COLOR, 0);

        // Store the color index as user data
        lv_obj_set_user_data(btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(btn, colorBtnCb, LV_EVENT_CLICKED, this);
    }

    // === Separator ===
    lv_obj_t* sep = lv_obj_create(_toolbar);
    lv_obj_set_size(sep, 2, 32);
    lv_obj_set_style_bg_color(sep, THEME_DIVIDER_COLOR, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_radius(sep, 1, 0);

    // === Brush size buttons ===
    for (size_t i = 0; i < BRUSH_SIZE_COUNT; i++) {
        lv_obj_t* btn = lv_button_create(_toolbar);
        lv_obj_set_size(btn, 40, 40);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xF0F0F0), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 0, 0);

        // Show a dot in the center that represents the brush size
        lv_obj_t* dot = lv_obj_create(btn);
        int dotSize = BRUSH_SIZES[i] * 2 + 2;
        if (dotSize > 24) dotSize = 24;
        lv_obj_set_size(dot, dotSize, dotSize);
        lv_obj_set_style_bg_color(dot, lv_color_hex(0x1A1A1A), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(dot, dotSize / 2, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_center(dot);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_set_user_data(btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(btn, sizeBtnCb, LV_EVENT_CLICKED, this);
    }

    // === Separator ===
    lv_obj_t* sep2 = lv_obj_create(_toolbar);
    lv_obj_set_size(sep2, 2, 32);
    lv_obj_set_style_bg_color(sep2, THEME_DIVIDER_COLOR, 0);
    lv_obj_set_style_bg_opa(sep2, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep2, 0, 0);
    lv_obj_set_style_radius(sep2, 1, 0);

    // === Clear button ===
    lv_obj_t* clearBtn = lv_button_create(_toolbar);
    lv_obj_set_size(clearBtn, 60, 40);
    lv_obj_set_style_bg_color(clearBtn, lv_color_hex(0xF44336), 0);
    lv_obj_set_style_bg_opa(clearBtn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(clearBtn, 8, 0);
    lv_obj_set_style_shadow_width(clearBtn, 0, 0);

    lv_obj_t* clearLabel = lv_label_create(clearBtn);
    lv_label_set_text(clearLabel, LV_SYMBOL_TRASH);
    lv_obj_set_style_text_color(clearLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(clearLabel);

    lv_obj_add_event_cb(clearBtn, clearBtnCb, LV_EVENT_CLICKED, this);
}

void DrawingApp::clearCanvas()
{
    if (_canvas == nullptr) return;
    // Fill the entire canvas with white
    lv_canvas_fill_bg(_canvas, THEME_DRAW_CANVAS_BG, LV_OPA_COVER);
    // Invalidate so LVGL redraws it
    lv_obj_invalidate(_canvas);
}

void DrawingApp::drawDot(int x, int y)
{
    if (_canvas == nullptr) return;

    /*
     * Draw a filled circle on the canvas using LVGL's draw layer.
     * We open a "draw layer", use LVGL's drawing primitives on it,
     * then close it. This is the LVGL v9 way of drawing on canvases.
     */
    lv_layer_t layer;
    lv_canvas_init_layer(_canvas, &layer);

    lv_draw_fill_dsc_t fillDsc;
    lv_draw_fill_dsc_init(&fillDsc);
    fillDsc.color = _brushColor;
    fillDsc.opa = LV_OPA_COVER;
    fillDsc.radius = _brushSize;  // Makes the rectangle a circle

    lv_area_t area;
    area.x1 = x - _brushSize;
    area.y1 = y - _brushSize;
    area.x2 = x + _brushSize;
    area.y2 = y + _brushSize;

    lv_draw_fill(&layer, &fillDsc, &area);
    lv_canvas_finish_layer(_canvas, &layer);
}

void DrawingApp::drawLine(int x0, int y0, int x1, int y1)
{
    /*
     * Draw a thick line by placing dots along it.
     * We calculate the distance between points and step along,
     * placing a dot at each step. Step size = 1 pixel for smooth lines.
     *
     * This is simpler than true Bresenham but works well for finger drawing
     * where points are usually close together.
     */
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int steps = (dx > dy) ? dx : dy;

    if (steps == 0) {
        drawDot(x0, y0);
        return;
    }

    float xInc = (float)(x1 - x0) / steps;
    float yInc = (float)(y1 - y0) / steps;
    float x = (float)x0;
    float y = (float)y0;

    for (int i = 0; i <= steps; i++) {
        drawDot((int)x, (int)y);
        x += xInc;
        y += yInc;
    }
}

// === Static callbacks ===

void DrawingApp::canvasEventCb(lv_event_t* e)
{
    DrawingApp* app = (DrawingApp*)lv_event_get_user_data(e);
    if (app == nullptr || app->_canvas == nullptr) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t* indev = lv_indev_active();
    if (indev == nullptr) return;

    /*
     * Get the touch point position.
     * lv_indev_get_point() gives coordinates relative to the SCREEN.
     * We need to subtract the canvas position to get canvas-local coordinates.
     */
    lv_point_t point;
    lv_indev_get_point(indev, &point);

    // Convert screen coords to canvas-local coords
    int canvasX = point.x;  // Canvas is at x=0
    int canvasY = point.y - STATUS_BAR_HEIGHT;  // Canvas starts below status bar

    // Clamp to canvas bounds
    if (canvasX < 0) canvasX = 0;
    if (canvasY < 0) canvasY = 0;
    if (canvasX >= app->_canvasWidth) canvasX = app->_canvasWidth - 1;
    if (canvasY >= app->_canvasHeight) canvasY = app->_canvasHeight - 1;

    if (code == LV_EVENT_PRESSED) {
        // Finger just touched down - start a new stroke
        app->_isDrawing = true;
        app->_lastX = canvasX;
        app->_lastY = canvasY;
        app->drawDot(canvasX, canvasY);
    } else if (code == LV_EVENT_PRESSING && app->_isDrawing) {
        // Finger is moving - draw line from last point to current
        app->drawLine(app->_lastX, app->_lastY, canvasX, canvasY);
        app->_lastX = canvasX;
        app->_lastY = canvasY;
    } else if (code == LV_EVENT_RELEASED) {
        // Finger lifted
        app->_isDrawing = false;
    }
}

void DrawingApp::colorBtnCb(lv_event_t* e)
{
    DrawingApp* app = (DrawingApp*)lv_event_get_user_data(e);
    if (app == nullptr) return;

    lv_obj_t* target = lv_event_get_target(e);
    int colorIdx = (int)(intptr_t)lv_obj_get_user_data(target);

    if (colorIdx >= 0 && colorIdx < (int)PALETTE_COUNT) {
        app->_brushColor = lv_color_hex(PALETTE_COLORS[colorIdx]);
        ESP_LOGI(TAG, "Color changed to #%06lX", (unsigned long)PALETTE_COLORS[colorIdx]);
    }
}

void DrawingApp::sizeBtnCb(lv_event_t* e)
{
    DrawingApp* app = (DrawingApp*)lv_event_get_user_data(e);
    if (app == nullptr) return;

    lv_obj_t* target = lv_event_get_target(e);
    int sizeIdx = (int)(intptr_t)lv_obj_get_user_data(target);

    if (sizeIdx >= 0 && sizeIdx < (int)BRUSH_SIZE_COUNT) {
        app->_brushSize = BRUSH_SIZES[sizeIdx];
        ESP_LOGI(TAG, "Brush size changed to %d", app->_brushSize);
    }
}

void DrawingApp::clearBtnCb(lv_event_t* e)
{
    DrawingApp* app = (DrawingApp*)lv_event_get_user_data(e);
    if (app == nullptr) return;

    app->clearCanvas();
    ESP_LOGI(TAG, "Canvas cleared");
}
