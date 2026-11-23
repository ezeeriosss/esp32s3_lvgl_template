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

/* Chip temperature status thresholds (deg C) */
#define HOME_TEMP_WARN_C       50.0f    /* >= warn -> Alerta  */
#define HOME_TEMP_CRIT_C       65.0f    /* >= crit -> Critico */

#define HOME_STATUS_WIDTH      130      /* panel width (plan size; export has 100) */
#define HOME_BLINK_MS          200      /* button feedback on the status panel */

static lv_timer_t *s_timer = NULL;

/* Battery arc color bands */
typedef enum {
    ARC_BAND_UNSET = -1,
    ARC_BAND_LOW,       /* < 20 % or no reading -> red   */
    ARC_BAND_MID,       /* 20 .. 50 %           -> amber */
    ARC_BAND_HIGH,      /* > 50 %               -> green */
} arc_band_t;

static arc_band_t s_arc_band = ARC_BAND_UNSET;

/* Status indicator (pnlStatus) states */
typedef enum {
    STATUS_UNSET = -1,
    STATUS_NO_DATA,     /* no temperature reading */
    STATUS_NORMAL,      /* < 50 C       */
    STATUS_WARNING,     /* 50 .. 64.9 C */
    STATUS_CRITICAL,    /* >= 65 C      */
} home_status_t;

static home_status_t s_status = STATUS_UNSET;

/*
 * Lifecycle driven by LVGL screen events (registered once in prepare()):
 *   LV_EVENT_SCREEN_LOAD_START -> refresh_all(), so the widgets never show
 *                                 their Squareline placeholders when entering.
 *   LV_EVENT_SCREEN_LOADED     -> create the 1 s refresh lv_timer.
 *   LV_EVENT_SCREEN_UNLOADED   -> delete the refresh lv_timer.
 *   LV_EVENT_KEY (LV_KEY_ENTER)-> physical button: refresh now + blink.
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

/** Color-coded status by chip temperature; only restyles when the state changes. */
static void update_status(bool valid, float celsius)
{
    home_status_t st;
    if (!valid) {
        st = STATUS_NO_DATA;
    } else if (celsius >= HOME_TEMP_CRIT_C) {
        st = STATUS_CRITICAL;
    } else if (celsius >= HOME_TEMP_WARN_C) {
        st = STATUS_WARNING;
    } else {
        st = STATUS_NORMAL;
    }
    if (st == s_status) return;
    s_status = st;

    const char *icon;
    const char *text;
    lv_palette_t palette;
    switch (st) {
        case STATUS_NORMAL:
            icon = LV_SYMBOL_OK;      text = "Normal";   palette = LV_PALETTE_GREEN; break;
        case STATUS_WARNING:
            icon = LV_SYMBOL_WARNING; text = "Alerta";   palette = LV_PALETTE_AMBER; break;
        case STATUS_CRITICAL:
            icon = LV_SYMBOL_WARNING; text = "Critico";  palette = LV_PALETTE_RED;   break;
        case STATUS_NO_DATA:
        default:
            icon = LV_SYMBOL_CLOSE;   text = "Sin dato"; palette = LV_PALETTE_GREY;  break;
    }
    lv_color_t color = lv_palette_main(palette);
    lv_label_set_text_static(ui_lblStatusIcon, icon);
    lv_label_set_text_static(ui_lblStatusText, text);
    lv_obj_set_style_text_color(ui_lblStatusIcon, color, 0);
    lv_obj_set_style_border_color(ui_pnlStatus, color, 0);
}

static void refresh_temperature(void)
{
    float c = 0.0f;
    bool valid = (bsp_temp_get_celsius(&c) == ESP_OK);
    update_status(valid, c);
    if (valid) {
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

static void blink_exec_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

/** Short opacity blink (50 % -> 100 %) as feedback for the button. */
static void status_blink(void)
{
    lv_anim_del(ui_pnlStatus, blink_exec_cb);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui_pnlStatus);
    lv_anim_set_exec_cb(&a, blink_exec_cb);
    lv_anim_set_values(&a, LV_OPA_50, LV_OPA_COVER);
    lv_anim_set_time(&a, HOME_BLINK_MS);
    lv_anim_start(&a);
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

/* Physical button (ENTER key from the keypad indev): immediate refresh. */
static void on_key(lv_event_t *e)
{
    if (lv_event_get_key(e) != LV_KEY_ENTER) return;
    refresh_all();
    if (s_timer != NULL) {
        lv_timer_reset(s_timer);    /* next periodic refresh a full period later */
    }
    status_blink();
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

    /* Status panel: border shows the state color; keep it compact */
    lv_obj_set_width(ui_pnlStatus, HOME_STATUS_WIDTH);
    lv_obj_set_style_border_width(ui_pnlStatus, 2, 0);
    lv_obj_set_style_pad_all(ui_pnlStatus, 4, 0);
    lv_obj_set_style_pad_column(ui_pnlStatus, 6, 0);
    lv_obj_clear_flag(ui_pnlStatus, LV_OBJ_FLAG_CLICKABLE);
    update_status(false, 0.0f);     /* "Sin dato" until the first reading */

    lv_obj_add_event_cb(ui_scrHome, on_load_start, LV_EVENT_SCREEN_LOAD_START, NULL);
    lv_obj_add_event_cb(ui_scrHome, on_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_scrHome, on_unloaded, LV_EVENT_SCREEN_UNLOADED, NULL);
    lv_obj_add_event_cb(ui_scrHome, on_key, LV_EVENT_KEY, NULL);
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
