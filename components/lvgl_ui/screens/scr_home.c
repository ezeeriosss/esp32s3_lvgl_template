#include "scr_home.h"
#include "screens.h"
#include "ui.h"             /* Squareline widgets: ui_scrHome, ui_arcBattery, ui_lbl* */
#include "bsp_battery.h"
#include "bsp_temp.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "scr_home";

#define HOME_REFRESH_PERIOD_MS 1000

static lv_timer_t *s_timer = NULL;

/* Battery arc color bands */
typedef enum {
    ARC_BAND_UNSET = -1,
    ARC_BAND_LOW,       /* < 20 % or no reading -> red   */
    ARC_BAND_MID,       /* 20 .. 50 %           -> amber */
    ARC_BAND_HIGH,      /* > 50 %               -> green */
} arc_band_t;

static arc_band_t s_arc_band = ARC_BAND_UNSET;

/*
 * Lifecycle driven by LVGL screen events (registered once in prepare()):
 *   LV_EVENT_SCREEN_LOAD_START -> refresh_all(), so the widgets never show
 *                                 their Squareline placeholders when entering.
 *   LV_EVENT_SCREEN_LOADED     -> create the 1 s refresh lv_timer.
 *   LV_EVENT_SCREEN_UNLOADED   -> delete the refresh lv_timer.
 * The splash -> home transition happens after prepare(), so these events
 * are always delivered (see the startup note in scr_splash.c).
 */

/* ------------------------------------------------------------------ */
/* Widget refresh helpers                                               */
/* ------------------------------------------------------------------ */

/** Color the arc indicator by battery level; only touches the style when the band changes. */
static void update_arc_color(int pct)
{
    arc_band_t band;
    if (pct > 50) {
        band = ARC_BAND_HIGH;
    } else if (pct >= 20) {
        band = ARC_BAND_MID;
    } else {
        band = ARC_BAND_LOW;    /* includes pct < 0 (no reading) */
    }
    if (band == s_arc_band) return;
    s_arc_band = band;

    lv_palette_t palette = (band == ARC_BAND_HIGH) ? LV_PALETTE_GREEN
                         : (band == ARC_BAND_MID)  ? LV_PALETTE_AMBER
                                                   : LV_PALETTE_RED;
    lv_obj_set_style_arc_color(ui_arcBattery, lv_palette_main(palette), LV_PART_INDICATOR);
}

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
    update_arc_color(pct);
}

static void refresh_temperature(void)
{
    float c;
    if (bsp_temp_get_celsius(&c) == ESP_OK) {
        /* LV_SPRINTF_USE_FLOAT is off: format the float with snprintf.
         * \xC2\xB0 = UTF-8 degree sign (included in the Montserrat fonts). */
        char buf[24];
        snprintf(buf, sizeof buf, "%.1f \xC2\xB0" "C", c);
        lv_label_set_text(ui_lblTemp, buf);
    } else {
        lv_label_set_text_static(ui_lblTemp, "--.- \xC2\xB0" "C");
    }
}

static void refresh_uptime(void)
{
    uint64_t s = (uint64_t)esp_timer_get_time() / 1000000ULL;    /* us -> s */
    /* Hours may go past 99, that is fine (HHH:MM:SS) */
    lv_label_set_text_fmt(ui_lblUptime, "%02lu:%02lu:%02lu",
                          (unsigned long)(s / 3600ULL),
                          (unsigned long)((s / 60ULL) % 60ULL),
                          (unsigned long)(s % 60ULL));
}

static void refresh_all(void)
{
    refresh_battery();
    refresh_temperature();
    refresh_uptime();
}

static void home_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    refresh_all();
}

/* ------------------------------------------------------------------ */
/* Screen event callbacks                                               */
/* ------------------------------------------------------------------ */

static void on_load_start(lv_event_t *e)
{
    (void)e;
    refresh_all();
}

static void on_loaded(lv_event_t *e)
{
    (void)e;
    if (s_timer == NULL) {
        s_timer = lv_timer_create(home_timer_cb, HOME_REFRESH_PERIOD_MS, NULL);
    }
}

static void on_unloaded(lv_event_t *e)
{
    (void)e;
    if (s_timer != NULL) {
        lv_timer_del(s_timer);
        s_timer = NULL;
    }
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
    lv_obj_add_event_cb(ui_scrHome, on_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_scrHome, on_unloaded, LV_EVENT_SCREEN_UNLOADED, NULL);
    lv_obj_add_event_cb(ui_scrHome, ui_screens_gesture_cb, LV_EVENT_GESTURE, NULL);
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
