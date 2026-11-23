#include "scr_home.h"
#include "screens.h"
#include "ui.h"             /* Squareline widgets: ui_scrHome, ui_arcBattery, ui_lbl* */
#include "bsp_battery.h"
#include "esp_log.h"

static const char *TAG = "scr_home";

/*
 * Lifecycle driven by LVGL screen events (registered once in prepare()):
 *   LV_EVENT_SCREEN_LOAD_START -> refresh_all(), so the widgets never show
 *                                 their Squareline placeholders when entering.
 * The splash -> home transition happens after prepare(), so these events
 * are always delivered (see the startup note in scr_splash.c).
 */

/* ------------------------------------------------------------------ */
/* Widget refresh helpers                                               */
/* ------------------------------------------------------------------ */

static void refresh_battery(void)
{
    float v = -1.0f;
    int pct = bsp_battery_get_percent(&v);

    if (pct >= 0) {
        lv_arc_set_value(ui_arcBattery, pct);
        lv_label_set_text_fmt(ui_lblBatPct, "%d%%", pct);
    } else {
        lv_arc_set_value(ui_arcBattery, 0);
        lv_label_set_text_static(ui_lblBatPct, "--%");
    }
}

static void refresh_all(void)
{
    refresh_battery();
}

/* ------------------------------------------------------------------ */
/* Screen event callbacks                                               */
/* ------------------------------------------------------------------ */

static void on_load_start(lv_event_t *e)
{
    (void)e;
    refresh_all();
}

/* ------------------------------------------------------------------ */

void scr_home_prepare(void)
{
    /* Battery arc in "custom angle" mode, independent of the Squareline setup */
    lv_arc_set_range(ui_arcBattery, 0, 100);
    lv_arc_set_bg_angles(ui_arcBattery, 135, 45);           /* 270 deg, open at the bottom */
    lv_arc_set_rotation(ui_arcBattery, 0);
    lv_arc_set_mode(ui_arcBattery, LV_ARC_MODE_NORMAL);
    lv_obj_remove_style(ui_arcBattery, NULL, LV_PART_KNOB); /* no knob */
    lv_obj_clear_flag(ui_arcBattery, LV_OBJ_FLAG_CLICKABLE); /* indicator only */

    lv_obj_add_event_cb(ui_scrHome, on_load_start, LV_EVENT_SCREEN_LOAD_START, NULL);
}

void scr_home_init(void)
{
    ESP_LOGD(TAG, "init");
}

void scr_home_uninit(void)
{
    ESP_LOGD(TAG, "uninit");
}

void scr_home_step(void)
{
}
