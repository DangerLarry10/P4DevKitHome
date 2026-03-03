/*
 * calculator_app.cpp - Calculator implementation
 *
 * Adapted from esp-brookesia's calculator. Main changes:
 * - Uses our AppBase instead of brookesia's App class
 * - Removed round-screen adaptation (we have 800x1280)
 * - Uses light theme colors from theme.h
 * - Sized for our large portrait display
 *
 * HOW THE CALCULATOR WORKS:
 * - A button matrix (lv_btnmatrix) provides the keypad
 * - Typing builds a formula string: "12+34x5"
 * - The calculate() function parses and evaluates using operator precedence
 *   (multiplication/division before addition/subtraction)
 * - The "=" button commits the result and adds to history
 */
#include "calculator_app.h"
#include "home/theme.h"
#include "esp_log.h"
#include <math.h>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cctype>

static const char* TAG = "CalcApp";

// Keyboard height as a percentage of available height
#define KEYBOARD_H_PERCENT      60

// Maximum formula string length
#define FORMULA_LEN_MAX         256

/*
 * The button matrix map.
 * Each string is a button label. "\n" starts a new row.
 * The empty string "" marks the end of the map.
 * Last row has "0" spanning wider (controlled by lv_btnmatrix_set_btn_width).
 */
static const char* KEYBOARD_MAP[] = {
    "C", "/", "x", LV_SYMBOL_BACKSPACE, "\n",
    "7", "8", "9", "-", "\n",
    "4", "5", "6", "+", "\n",
    "1", "2", "3", "%", "\n",
    "0", ".", "=", ""
};

CalculatorApp::CalculatorApp()
    : AppBase("Calculator", "Simple calculator app"),
      _keyboard(nullptr), _historyLabel(nullptr),
      _formulaLabel(nullptr), _resultLabel(nullptr),
      _formulaLen(1), _dispWidth(0), _dispHeight(0)
{
}

CalculatorApp::~CalculatorApp()
{
}

bool CalculatorApp::onRun()
{
    ESP_LOGI(TAG, "Starting calculator");

    /*
     * Get the usable screen dimensions.
     * We leave room at the top for the back button (STATUS_BAR_HEIGHT area).
     */
    _dispWidth = SCREEN_WIDTH;
    _dispHeight = SCREEN_HEIGHT - STATUS_BAR_HEIGHT;
    _formulaLen = 1;

    lv_obj_t* screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, THEME_CALC_BG, 0);

    // Calculate layout sizes
    int keyboardH = (int)(_dispHeight * KEYBOARD_H_PERCENT / 100.0f);
    int labelH = _dispHeight - keyboardH;
    int textH = labelH - 6;  // Small padding

    // === Keyboard (bottom portion of screen) ===
    _keyboard = lv_btnmatrix_create(screen);
    lv_btnmatrix_set_map(_keyboard, KEYBOARD_MAP);

    // Make the "0" button wider (btn index 16 in the last row)
    // The "." and "=" share the remaining space equally
    lv_btnmatrix_set_btn_width(_keyboard, 16, 2);  // "0" gets 2x width

    lv_obj_set_size(_keyboard, _dispWidth, keyboardH);
    lv_obj_align(_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Keyboard styling
    lv_obj_set_style_text_font(_keyboard, THEME_FONT_CALC, 0);
    lv_obj_set_style_bg_color(_keyboard, THEME_CALC_BG, 0);
    lv_obj_set_style_border_width(_keyboard, 0, 0);
    lv_obj_set_style_radius(_keyboard, 0, 0);
    lv_obj_set_style_pad_all(_keyboard, 4, 0);
    lv_obj_set_style_pad_gap(_keyboard, 4, 0);

    // Style the individual buttons
    lv_obj_set_style_bg_color(_keyboard, lv_color_hex(0xF5F5F5), LV_PART_ITEMS);
    lv_obj_set_style_radius(_keyboard, 12, LV_PART_ITEMS);
    lv_obj_set_style_text_color(_keyboard, THEME_TEXT_PRIMARY, LV_PART_ITEMS);

    // Operator buttons get accent color text
    // Button indices: 0=C, 1=/, 2=x, 3=backspace, 7=-, 11=+, 15=%
    lv_obj_set_style_text_color(_keyboard, THEME_CALC_OPERATOR,
                                 (lv_style_selector_t)LV_PART_ITEMS | (lv_style_selector_t)LV_STATE_CHECKED);
    lv_btnmatrix_set_btn_ctrl(_keyboard, 0, LV_BTNMATRIX_CTRL_CHECKED);   // C
    lv_btnmatrix_set_btn_ctrl(_keyboard, 1, LV_BTNMATRIX_CTRL_CHECKED);   // /
    lv_btnmatrix_set_btn_ctrl(_keyboard, 2, LV_BTNMATRIX_CTRL_CHECKED);   // x
    lv_btnmatrix_set_btn_ctrl(_keyboard, 3, LV_BTNMATRIX_CTRL_CHECKED);   // backspace
    lv_btnmatrix_set_btn_ctrl(_keyboard, 7, LV_BTNMATRIX_CTRL_CHECKED);   // -
    lv_btnmatrix_set_btn_ctrl(_keyboard, 11, LV_BTNMATRIX_CTRL_CHECKED);  // +
    lv_btnmatrix_set_btn_ctrl(_keyboard, 15, LV_BTNMATRIX_CTRL_CHECKED);  // %

    // "=" button gets special styling
    lv_btnmatrix_set_btn_ctrl(_keyboard, 18, LV_BTNMATRIX_CTRL_CHECKED);

    /*
     * Register the keyboard event callback.
     * `this` is passed as user_data so the static callback can access our member variables.
     */
    lv_obj_add_event_cb(_keyboard, keyboardEventCb, LV_EVENT_VALUE_CHANGED, this);

    // === Display area (top portion) ===
    lv_obj_t* labelContainer = lv_obj_create(screen);
    lv_obj_set_size(labelContainer, _dispWidth, labelH);
    lv_obj_align(labelContainer, LV_ALIGN_TOP_MID, 0, STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_color(labelContainer, THEME_CALC_BG, 0);
    lv_obj_set_style_bg_opa(labelContainer, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(labelContainer, 0, 0);
    lv_obj_set_style_border_width(labelContainer, 0, 0);
    lv_obj_set_style_pad_all(labelContainer, 8, 0);

    /*
     * Flex layout: column direction, aligned to the bottom-right.
     * This makes the result label "stick" to the bottom of the display area,
     * like a real calculator where the most recent result is most visible.
     */
    lv_obj_set_flex_flow(labelContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(labelContainer, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_row(labelContainer, 4, 0);

    // === History textarea (scrollable list of past calculations) ===
    _historyLabel = lv_textarea_create(labelContainer);
    lv_obj_set_size(_historyLabel, _dispWidth - 16, textH / 3);
    lv_obj_set_style_radius(_historyLabel, 0, 0);
    lv_obj_set_style_border_width(_historyLabel, 0, 0);
    lv_obj_set_style_pad_all(_historyLabel, 0, 0);
    lv_obj_set_style_bg_opa(_historyLabel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_align(_historyLabel, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(_historyLabel, THEME_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(_historyLabel, THEME_FONT_SMALL, 0);
    // Hide the cursor (this is display-only, not editable)
    lv_obj_set_style_opa(_historyLabel, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_textarea_set_text(_historyLabel, "");
    lv_obj_add_flag(_historyLabel, LV_OBJ_FLAG_SCROLLABLE);

    // === Formula label (current expression) ===
    lv_obj_t* formulaContainer = lv_obj_create(labelContainer);
    lv_obj_set_size(formulaContainer, _dispWidth - 16, textH / 3);
    lv_obj_set_style_radius(formulaContainer, 0, 0);
    lv_obj_set_style_border_width(formulaContainer, 0, 0);
    lv_obj_set_style_pad_all(formulaContainer, 0, 0);
    lv_obj_set_style_bg_opa(formulaContainer, LV_OPA_TRANSP, 0);

    _formulaLabel = lv_label_create(formulaContainer);
    lv_label_set_text(_formulaLabel, "0");
    lv_obj_set_style_text_font(_formulaLabel, THEME_FONT_CALC_RESULT, 0);
    lv_obj_set_style_text_color(_formulaLabel, THEME_TEXT_PRIMARY, 0);
    lv_obj_align(_formulaLabel, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_align(_formulaLabel, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_size(_formulaLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    // === Result label (running total preview) ===
    lv_obj_t* resultContainer = lv_obj_create(labelContainer);
    lv_obj_set_size(resultContainer, _dispWidth - 16, textH / 3);
    lv_obj_set_style_radius(resultContainer, 0, 0);
    lv_obj_set_style_border_width(resultContainer, 0, 0);
    lv_obj_set_style_pad_all(resultContainer, 0, 0);
    lv_obj_set_style_bg_opa(resultContainer, LV_OPA_TRANSP, 0);

    _resultLabel = lv_label_create(resultContainer);
    lv_label_set_text(_resultLabel, "= 0");
    lv_obj_set_style_text_font(_resultLabel, THEME_FONT_BODY, 0);
    lv_obj_set_style_text_color(_resultLabel, THEME_ACCENT_COLOR, 0);
    lv_obj_align(_resultLabel, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_align(_resultLabel, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_size(_resultLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    return true;
}

bool CalculatorApp::onClose()
{
    ESP_LOGI(TAG, "Closing calculator");
    // LVGL objects are deleted when the screen is deleted by AppManager,
    // so we just clear our pointers to avoid dangling references.
    _keyboard = nullptr;
    _historyLabel = nullptr;
    _formulaLabel = nullptr;
    _resultLabel = nullptr;
    return true;
}

// === Calculator logic (ported directly from brookesia) ===

bool CalculatorApp::isStartZero()
{
    const char* text = lv_label_get_text(_formulaLabel);
    int len = strlen(text);

    if (len == 1 && text[0] == '0') return true;
    // Check if trailing "0" is the start of a new number segment
    // (i.e., the character before it is an operator, not a digit)
    if (text[len - 1] == '0' && len >= 2 &&
        (text[len - 2] > '9' || text[len - 2] < '0')) return true;
    return false;
}

bool CalculatorApp::isStartNum()
{
    const char* text = lv_label_get_text(_formulaLabel);
    int len = strlen(text);
    return (text[len - 1] >= '0' && text[len - 1] <= '9');
}

bool CalculatorApp::isStartPercent()
{
    const char* text = lv_label_get_text(_formulaLabel);
    int len = strlen(text);
    return (text[len - 1] == '%');
}

bool CalculatorApp::isLegalDot()
{
    const char* text = lv_label_get_text(_formulaLabel);
    int len = strlen(text);

    // Walk backwards: if we find a dot before an operator, it's illegal
    while (len-- > 0) {
        if (text[len] == '.') return false;
        if (text[len] < '0' || text[len] > '9') return true;
    }
    return true;
}

/*
 * calculate() - Parse and evaluate a math expression string.
 *
 * This is a simple stack-based calculator that respects operator precedence:
 * - Multiplication (x) and division (/) happen before + and -
 * - It processes operators left-to-right within the same precedence
 *
 * Algorithm:
 * 1. Read digits to build a number
 * 2. When we hit an operator, apply the PREVIOUS operator to the stack
 * 3. + and - push to stack (deferred), x and / modify the stack top (immediate)
 * 4. Sum the stack at the end
 */
double CalculatorApp::calculate(const char* input)
{
    std::vector<double> stk;
    int inputLen = strlen(input);
    double num = 0;
    bool dotFlag = false;
    int dotLen = 0;
    char preSign = '+';

    for (int i = 0; i < inputLen; i++) {
        if (input[i] == '.') {
            dotFlag = true;
            dotLen = 0;
        } else if (isdigit(input[i])) {
            if (!dotFlag) {
                num = num * 10 + input[i] - '0';
            } else {
                num += (input[i] - '0') / pow(10.0, ++dotLen);
            }
        } else if (input[i] == '%') {
            num /= 100.0;
        } else if (i != inputLen - 1) {
            dotFlag = false;
            dotLen = 0;
            switch (preSign) {
                case '+': stk.push_back(num); break;
                case '-': stk.push_back(-num); break;
                case 'x': stk.back() *= num; break;
                default:
                    if (num != 0) stk.back() /= num;
                    else return 0;  // Division by zero
            }
            num = 0;
            preSign = input[i];
        }

        // Handle the last number in the expression
        if (i == inputLen - 1) {
            switch (preSign) {
                case '+': stk.push_back(num); break;
                case '-': stk.push_back(-num); break;
                case 'x': stk.back() *= num; break;
                default:
                    if (num != 0) stk.back() /= num;
                    else return 0;
            }
        }
    }

    // Sum all values in the stack to get the final result
    double result = 0;
    for (size_t i = 0; i < stk.size(); i++) {
        result += stk[i];
    }
    return result;
}

/*
 * keyboardEventCb() - Handles all button presses on the calculator keypad.
 *
 * This is where the magic happens. Each button press either:
 * - Appends a digit/operator to the formula
 * - Clears or backspaces
 * - Triggers evaluation (= or automatic preview)
 *
 * Button index mapping (matches KEYBOARD_MAP order):
 *   0=C  1=/  2=x  3=backspace
 *   4=7  5=8  6=9  7=-
 *   8=4  9=5  10=6 11=+
 *  12=1 13=2  14=3 15=%
 *  16=0 17=.  18==
 */
void CalculatorApp::keyboardEventCb(lv_event_t* e)
{
    CalculatorApp* app = (CalculatorApp*)lv_event_get_user_data(e);
    if (app == nullptr) return;

    int btnId = lv_btnmatrix_get_selected_btn(app->_keyboard);
    bool calculateFlag = false;
    bool equalFlag = false;
    double resNum;
    char resStr[32];
    char historyStr[64];

    // If we just pressed "=", the formula is small and result is big.
    // Any new input should make formula big and result small again.
    if (lv_obj_get_style_text_font(app->_formulaLabel, LV_PART_MAIN) == THEME_FONT_BODY) {
        lv_obj_set_style_text_font(app->_formulaLabel, THEME_FONT_CALC_RESULT, LV_PART_MAIN);
        lv_obj_set_style_text_font(app->_resultLabel, THEME_FONT_BODY, LV_PART_MAIN);
    }

    switch (btnId) {
        case 0:  // "C" - Clear everything
            lv_label_set_text(app->_formulaLabel, "0");
            app->_formulaLen = 1;
            calculateFlag = true;
            break;

        case 3:  // Backspace
            if (app->_formulaLen == 1 && app->isStartZero()) break;
            lv_label_cut_text(app->_formulaLabel, --(app->_formulaLen), 1);
            if (app->_formulaLen == 0) {
                lv_label_set_text(app->_formulaLabel, "0");
                app->_formulaLen = 1;
            }
            calculateFlag = true;
            break;

        case 18:  // "=" - Evaluate and commit
            calculateFlag = true;
            equalFlag = true;
            break;

        // Operators: / x - + %
        case 1: case 2: case 7: case 11: case 15:
            if (app->_formulaLen >= FORMULA_LEN_MAX) break;
            if (app->isStartPercent() || app->isStartNum()) {
                // Special case: if starting with "0" and pressing +/-, replace the 0
                if ((btnId == 7 || btnId == 11) && app->isStartZero()) {
                    lv_label_cut_text(app->_formulaLabel, --(app->_formulaLen), 1);
                }
                lv_label_ins_text(app->_formulaLabel, app->_formulaLen++,
                                  lv_btnmatrix_get_btn_text(app->_keyboard, btnId));
                if (btnId == 15) calculateFlag = true;  // % triggers recalc
            }
            break;

        // Digits: 0-9
        case 4: case 5: case 6: case 8: case 9: case 10:
        case 12: case 13: case 14: case 16:
            if (app->_formulaLen >= FORMULA_LEN_MAX) break;
            if (app->isStartZero()) {
                lv_label_cut_text(app->_formulaLabel, --(app->_formulaLen), 1);
            }
            if (!app->isStartPercent()) {
                lv_label_ins_text(app->_formulaLabel, app->_formulaLen++,
                                  lv_btnmatrix_get_btn_text(app->_keyboard, btnId));
                calculateFlag = true;
            }
            break;

        case 17:  // "." - Decimal point
            if (app->_formulaLen >= FORMULA_LEN_MAX) break;
            if (app->isLegalDot() && app->isStartNum()) {
                lv_label_ins_text(app->_formulaLabel, app->_formulaLen++, ".");
            }
            break;

        default:
            break;
    }

    // Update the running result preview
    if (calculateFlag) {
        resNum = app->calculate(lv_label_get_text(app->_formulaLabel));
        if ((long)resNum == resNum) {
            snprintf(resStr, sizeof(resStr) - 1, "%ld", (long)resNum);
        } else {
            snprintf(resStr, sizeof(resStr) - 1, "%.3f", resNum);
            // Remove trailing zeros after decimal point
            char* end = resStr + strlen(resStr) - 1;
            while (end > resStr && *end == '0') *end-- = '\0';
            if (end > resStr && *end == '.') *end = '\0';
        }
        lv_label_set_text_fmt(app->_resultLabel, "= %s", resStr);
        lv_obj_set_style_text_font(app->_resultLabel, THEME_FONT_BODY, 0);
    }

    // "=" commits: swap font sizes and add to history
    if (equalFlag) {
        // Make result big, formula small (visual emphasis shift)
        lv_obj_set_style_text_font(app->_resultLabel, THEME_FONT_CALC_RESULT, 0);

        // Add to history
        snprintf(historyStr, sizeof(historyStr) - 1, "\n%s = %s ",
                 lv_label_get_text(app->_formulaLabel), resStr);
        lv_textarea_set_cursor_pos(app->_historyLabel,
                                    strlen(lv_textarea_get_text(app->_historyLabel)));
        lv_textarea_add_text(app->_historyLabel, historyStr);

        // Replace formula with result for next calculation
        lv_label_set_text_fmt(app->_formulaLabel, "%s", resStr);
        lv_obj_set_style_text_font(app->_formulaLabel, THEME_FONT_BODY, 0);
        app->_formulaLen = strlen(resStr);
    }
}
