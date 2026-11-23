#include "scr_splash.h"
#include "screens.h"
#include "ui.h"             /* Squareline widgets: ui_scrSplash, ui_imgLogo, ui_lblSplashStatus */
#include "bsp_display.h"
#include "esp_log.h"

static const char *TAG = "scr_splash";

#define SPLASH_FADE_IN_MS   800
#define SPLASH_DOTS_STEPS   4       /* "", ".", "..", "..." */
#define SPLASH_DOTS_STEP_MS 400
#define SPLASH_PRESSED_OPA  LV_OPA_70   /* logo opacity while the screen is pressed */

/*
 * Startup note: ui_init() (Squareline) loads ui_scrSplash BEFORE
 * ui_screens_prepare() runs, so the LV_EVENT_SCREEN_LOAD_START / LOADED
 * events of the very first boot have already been sent when our callbacks
 * get registered. That is why the splash start-up (backlight fade-in + dot
 * animation) is triggered from scr_splash_init(): the screen engine calls it
 * on the first UI tick, when it detects the splash as the active screen.
 * Screen load events are used for the splash -> home transition, which
 * happens with the callbacks already registered.
 */

/* ------------------------------------------------------------------ */
/* "Iniciando..." dot animation (lv_anim_t)                            */
/* ------------------------------------------------------------------ */

static int32_t s_dots_last = -1;    /* set to -1 before every lv_anim_start */
static bool    s_navigating = false; /* avoids a double navigation to home */

static void dots_exec_cb(void *var, int32_t v)
{
    int32_t n = v % SPLASH_DOTS_STEPS;          /* 0..3 dots */
    if (n == s_dots_last) return;               /* don't redraw if unchanged */
    s_dots_last = n;
    static const char *txt[SPLASH_DOTS_STEPS] = {
        "Iniciando", "Iniciando.", "Iniciando..", "Iniciando..."
    };
    lv_label_set_text_static((lv_obj_t *)var, txt[n]);
}

static void dots_start(void)
{
    lv_anim_del(ui_lblSplashStatus, dots_exec_cb);
    s_dots_last = -1;                           /* first frame always draws */

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui_lblSplashStatus);
    lv_anim_set_exec_cb(&a, dots_exec_cb);
    lv_anim_set_values(&a, 0, SPLASH_DOTS_STEPS);   /* 4 => last instant, 4 % 4 = 0 */
    lv_anim_set_time(&a, SPLASH_DOTS_STEPS * SPLASH_DOTS_STEP_MS);  /* 1600 ms */
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

/** Stop the splash animations and restore widgets. Idempotent. */
static void splash_stop(void)
{
    lv_anim_del(ui_lblSplashStatus, dots_exec_cb);
    lv_obj_set_style_opa(ui_imgLogo, LV_OPA_COVER, 0);
}

/* ------------------------------------------------------------------ */
/* Event callbacks                                                      */
/* ------------------------------------------------------------------ */

/* Touch anywhere: the children are not clickable, so every press lands on
 * the screen object itself. */
static void on_pressed(lv_event_t *e)
{
    lv_obj_set_style_opa(ui_imgLogo, SPLASH_PRESSED_OPA, 0);   /* visual feedback */

    lv_indev_t *indev = lv_event_get_indev(e);
    if (indev) {
        lv_point_t p;
        lv_indev_get_point(indev, &p);
        ESP_LOGI(TAG, "pressed at x=%d y=%d", (int)p.x, (int)p.y);
    }
}

/* Finger slid away before releasing: no click, so undo the feedback. */
static void on_press_lost(lv_event_t *e)
{
    (void)e;
    lv_obj_set_style_opa(ui_imgLogo, LV_OPA_COVER, 0);
}

static void on_clicked(lv_event_t *e)
{
    (void)e;
    if (s_navigating) return;
    s_navigating = true;
    ESP_LOGI(TAG, "touch -> home");
    ui_screens_navigate(SCREEN_HOME);
}

static void on_unload_start(lv_event_t *e)
{
    (void)e;
    splash_stop();
}

/* ------------------------------------------------------------------ */

void scr_splash_prepare(void)
{
    /* The whole screen is the touch target; children must not steal it
     * (in case they were left clickable in Squareline). */
    lv_obj_add_flag(ui_scrSplash, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui_imgLogo, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui_lblSplashStatus, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui_lblHint, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(ui_scrSplash, on_pressed, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(ui_scrSplash, on_press_lost, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_add_event_cb(ui_scrSplash, on_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_scrSplash, on_unload_start, LV_EVENT_SCREEN_UNLOAD_START, NULL);
}

void scr_splash_init(void)
{
    ESP_LOGD(TAG, "init");
    s_navigating = false;
    bsp_display_fade_in(SPLASH_FADE_IN_MS);
    dots_start();
}

void scr_splash_uninit(void)
{
    ESP_LOGD(TAG, "uninit");
    splash_stop();
}

void scr_splash_step(void)
{
}
