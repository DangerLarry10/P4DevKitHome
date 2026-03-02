/*
 * calculator_app.h - Simple calculator app
 *
 * Adapted from the esp-brookesia calculator example, but simplified
 * to work with our AppBase framework instead of the brookesia system.
 *
 * Features:
 * - Basic math: +, -, x, /, %
 * - Backspace and clear
 * - History of previous calculations
 * - Sized for 800x1280 portrait display
 */
#pragma once

#include "lvgl.h"
#include "apps/app_base.h"

class CalculatorApp : public AppBase {
public:
    CalculatorApp();
    ~CalculatorApp();

    // === AppBase lifecycle ===
    bool onRun() override;       // Build the calculator UI
    bool onClose() override;     // Clean up

private:
    // LVGL widget pointers (set during onRun, cleared during onClose)
    lv_obj_t* _keyboard;         // Button matrix for number/operator keys
    lv_obj_t* _historyLabel;     // Shows previous calculations
    lv_obj_t* _formulaLabel;     // Current expression being typed
    lv_obj_t* _resultLabel;      // Running result preview

    int _formulaLen;              // Current length of formula text
    uint16_t _dispWidth;          // Available display width
    uint16_t _dispHeight;         // Available display height

    // === Calculator logic helpers ===
    bool isStartZero();           // Is the current number segment just "0"?
    bool isStartNum();            // Does the formula end with a digit?
    bool isStartPercent();        // Does the formula end with "%"?
    bool isLegalDot();            // Can we add a decimal point here?
    double calculate(const char* input);  // Evaluate the formula string

    // LVGL event callback for key presses (static = C-compatible function pointer)
    static void keyboardEventCb(lv_event_t* e);
};
