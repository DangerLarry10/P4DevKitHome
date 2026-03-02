/*
 * drawing_app.h - Finger drawing canvas app
 *
 * Our first CUSTOM app (not ported from brookesia). This validates that
 * the AppBase framework works correctly for building new apps from scratch.
 *
 * Features:
 * - Full-screen drawing canvas using lv_canvas
 * - Color palette bar at the bottom (5 colors)
 * - Brush size selector (small/medium/large)
 * - Clear button
 * - Touch draws lines between consecutive touch points
 */
#pragma once

#include "lvgl.h"
#include "apps/app_base.h"

class DrawingApp : public AppBase {
public:
    DrawingApp();
    ~DrawingApp();

    // === AppBase lifecycle ===
    bool onRun() override;
    bool onClose() override;

private:
    // Canvas and its pixel buffer
    lv_obj_t* _canvas;
    uint8_t* _canvasBuf;         // Pixel buffer allocated in PSRAM

    // Drawing state
    lv_color_t _brushColor;      // Current brush color
    int _brushSize;               // Current brush radius in pixels
    int _lastX;                   // Previous touch X (for line drawing)
    int _lastY;                   // Previous touch Y
    bool _isDrawing;              // True while finger is on canvas

    // Canvas dimensions (calculated at runtime)
    int _canvasWidth;
    int _canvasHeight;

    // Toolbar widgets
    lv_obj_t* _toolbar;

    // Create the bottom toolbar with color palette and controls
    void createToolbar(lv_obj_t* parent);

    // Clear the canvas to white
    void clearCanvas();

    // Draw a filled circle at position (for brush dots)
    void drawDot(int x, int y);

    // Draw a line between two points (Bresenham-style with dots)
    void drawLine(int x0, int y0, int x1, int y1);

    // === Static LVGL callbacks ===
    static void canvasEventCb(lv_event_t* e);
    static void colorBtnCb(lv_event_t* e);
    static void sizeBtnCb(lv_event_t* e);
    static void clearBtnCb(lv_event_t* e);
};
