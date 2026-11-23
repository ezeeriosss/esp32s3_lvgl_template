#include "scr_settings.h"
#include "screens.h"
#include "ui.h"             /* Squareline widgets: ui_scrSettings and its children */
#include "bsp_display.h"
#include "screens_timeout.h"
#include "esp_log.h"

static const char *TAG = "scr_settings";

/* Defaults */
#define SETTINGS_DEFAULT_BRIGHTNESS  100
#define SETTINGS_MIN_BRIGHTNESS      10     /* slider minimum: never a black screen */
#define SETTINGS_MAX_BRIGHTNESS      100
#define SETTINGS_DEFAULT_TIMEOUT_EN  false
#define SETTINGS_TIMEOUT_MS          30000  /* display off after 30 s of inactivity */

/* ------------------------------------------------------------------ */

/** Apply a brightness value: LEDC backlight + percentage label. */
static void apply_brightness(int32_t value)
{
    if (value < SETTINGS_MIN_BRIGHTNESS) value = SETTINGS_MIN_BRIGHTNESS;
    if (value > SETTINGS_MAX_BRIGHTNESS) value = SETTINGS_MAX_BRIGHTNESS;
    bsp_display_set_brightness((uint8_t)value);
    lv_label_set_text_fmt(ui_lblBrightnessPct, "%d%%", (int)value);
}

/** Enable / disable the display-off timeout. */
static void apply_timeout(bool enabled)
{
    disp_timeout_set_off_ms(enabled ? SETTINGS_TIMEOUT_MS : TIMEOUT_NEVER);
    ESP_LOGI(TAG, "display timeout %s", enabled ? "on (30 s)" : "off");
}

/**
 * Apply a full settings set to the hardware AND the widgets.
 * Used by the physical button (restore defaults): lv_slider_set_value() and
 * lv_obj_add/clear_state() do not emit VALUE_CHANGED, so everything is
 * applied by hand here.
 */
static void apply_settings(int32_t brightness, bool timeout_en)
{
    lv_slider_set_value(ui_sldBrightness, brightness, LV_ANIM_ON);
    apply_brightness(brightness);

    if (timeout_en) {
        lv_obj_add_state(ui_swTimeout, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(ui_swTimeout, LV_STATE_CHECKED);
    }
    apply_timeout(timeout_en);
}

/* Slider moved: change the physical brightness in real time. */
static void on_brightness_changed(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    apply_brightness(lv_slider_get_value(slider));
}

/* Switch toggled: enable / disable the display-off timeout. */
static void on_timeout_changed(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    apply_timeout(lv_obj_has_state(sw, LV_STATE_CHECKED));
}

/* Physical button (ENTER key): restore the default settings. */
static void on_key(lv_event_t *e)
{
    if (lv_event_get_key(e) != LV_KEY_ENTER) return;
    ESP_LOGI(TAG, "restore defaults");
    apply_settings(SETTINGS_DEFAULT_BRIGHTNESS, SETTINGS_DEFAULT_TIMEOUT_EN);
}

/* ------------------------------------------------------------------ */

void scr_settings_prepare(void)
{
    lv_slider_set_range(ui_sldBrightness, SETTINGS_MIN_BRIGHTNESS, SETTINGS_MAX_BRIGHTNESS);
    lv_obj_add_event_cb(ui_sldBrightness, on_brightness_changed, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui_swTimeout, on_timeout_changed, LV_EVENT_VALUE_CHANGED, NULL);

    lv_label_set_text_fmt(ui_lblBuild, "Compilado: %s %s", __DATE__, __TIME__);

    /* Dragging the slider / switch horizontally must not change screen:
     * keep their gestures from bubbling up to the screen. */
    lv_obj_clear_flag(ui_sldBrightness, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_clear_flag(ui_swTimeout, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_add_event_cb(ui_scrSettings, on_key, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(ui_scrSettings, ui_screens_gesture_cb, LV_EVENT_GESTURE, NULL);
}

void scr_settings_init(void)
{
    ESP_LOGD(TAG, "init");
    /* Sync the slider with the real backlight (100 after the splash fade-in).
     * lv_slider_set_value() does not emit VALUE_CHANGED, so set the label too. */
    int32_t current = bsp_display_get_brightness();
    if (current < SETTINGS_MIN_BRIGHTNESS) current = SETTINGS_MIN_BRIGHTNESS;
    lv_slider_set_value(ui_sldBrightness, current, LV_ANIM_OFF);
    lv_label_set_text_fmt(ui_lblBrightnessPct, "%d%%", (int)current);
}

void scr_settings_uninit(void)
{
    ESP_LOGD(TAG, "uninit");
}

void scr_settings_step(void)
{
}
