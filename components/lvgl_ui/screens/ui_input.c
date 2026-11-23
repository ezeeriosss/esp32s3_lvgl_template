#include "ui_input.h"
#include "screens.h"
#include "screens_update.h"
#include "ui.h"
#include "bsp_button.h"
#include "esp_log.h"

static const char *TAG = "ui_input";

static lv_group_t     *s_group = NULL;
static lv_indev_drv_t  s_indev_drv;
static lv_indev_t     *s_indev = NULL;

/* Pending ENTER presses. Only touched from LVGL context: the update-queue
 * handler and the indev read_cb both run in the LVGL task, so no race. */
static uint32_t        s_pending = 0;
static bool            s_last_pressed = false;

/* Screens that take part in the input group (the splash does not). */
static const ui_screens_t s_group_screens[] = { SCREEN_HOME, SCREEN_SYSTEM, SCREEN_SETTINGS };

/* ------------------------------------------------------------------ */

/* Runs in the esp_timer task of the button component: NO lv_* calls here,
 * only hand the event over to the LVGL task through the update queue. */
static void on_button_click(void *arg)
{
    (void)arg;
    ui_update_cmd(UI_UPDATE_BUTTON_PRESS, NULL);
}

/* Keypad read callback (LVGL context): one PRESSED report per queued press,
 * always followed by a RELEASED report. */
static void keypad_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    data->key = LV_KEY_ENTER;
    if (s_last_pressed) {
        data->state = LV_INDEV_STATE_RELEASED;
        s_last_pressed = false;
    } else if (s_pending > 0) {
        s_pending--;
        data->state = LV_INDEV_STATE_PRESSED;
        s_last_pressed = true;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/* ------------------------------------------------------------------ */

void ui_input_init(void)
{
    s_group = lv_group_create();    /* not set as default group on purpose */

    for (size_t i = 0; i < sizeof(s_group_screens) / sizeof(s_group_screens[0]); i++) {
        lv_obj_t *scr = ui_screens_get_obj(s_group_screens[i]);
        if (scr) {
            lv_group_add_obj(s_group, scr);
            /* No focus outline around the whole screen */
            lv_obj_set_style_outline_width(scr, 0, LV_STATE_FOCUS_KEY);
        }
    }

    lv_indev_drv_init(&s_indev_drv);
    s_indev_drv.type    = LV_INDEV_TYPE_KEYPAD;
    s_indev_drv.read_cb = keypad_read_cb;
    s_indev = lv_indev_drv_register(&s_indev_drv);
    lv_indev_set_group(s_indev, s_group);

    esp_err_t err = bsp_button_register_click_cb(on_button_click, NULL);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "button callback not registered: %s", esp_err_to_name(err));
    }
}

void ui_input_push_enter(void)
{
    s_pending++;
}

void ui_input_focus_screen(ui_screens_t screen)
{
    if (s_group == NULL) return;
    for (size_t i = 0; i < sizeof(s_group_screens) / sizeof(s_group_screens[0]); i++) {
        if (s_group_screens[i] == screen) {
            lv_obj_t *scr = ui_screens_get_obj(screen);
            if (scr) {
                lv_group_focus_obj(scr);
            }
            return;
        }
    }
}
