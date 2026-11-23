#ifndef __UI_INPUT_H__
#define __UI_INPUT_H__

#include "screens.h"

/**
 * Physical button as an LVGL input device.
 *
 *   [BOOT] -> espressif/button (esp_timer task)
 *          -> on_button_click()  -> ui_update_cmd(UI_UPDATE_BUTTON_PRESS)   (no lv_* here)
 *          -> (LVGL context) handle_button_press() -> ui_input_push_enter()
 *          -> keypad read_cb reports LV_KEY_ENTER pressed, then released
 *          -> LVGL sends LV_EVENT_KEY (LV_KEY_ENTER) to the focused object
 *          -> the focused object is the active screen: its controller runs
 *             the contextual action in its LV_EVENT_KEY callback.
 *
 * Home, System and Settings are in the group; the splash is not.
 */

/** Create the group + keypad indev and hook the button. Call once from
 *  lvgl_ui_start(), after ui_screens_prepare(). LVGL context only. */
void ui_input_init(void);

/** Queue one ENTER press for the keypad indev. LVGL context only. */
void ui_input_push_enter(void);

/** Focus the given screen in the input group (called by the screen engine
 *  on every transition). LVGL context only. */
void ui_input_focus_screen(ui_screens_t screen);

#endif
