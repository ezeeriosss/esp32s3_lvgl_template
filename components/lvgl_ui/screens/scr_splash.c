#include "scr_splash.h"
#include "screens.h"
#include "ui.h"             /* Squareline widgets: ui_scrSplash, ui_imgLogo, ui_lblSplashStatus */
#include "bsp_display.h"
#include "esp_log.h"

static const char *TAG = "scr_splash";

#define SPLASH_FADE_IN_MS   800

/*
 * Startup note: ui_init() (Squareline) loads ui_scrSplash BEFORE
 * ui_screens_prepare() runs, so the LV_EVENT_SCREEN_LOAD_START / LOADED
 * events of the very first boot have already been sent when our callbacks
 * get registered. That is why the splash start-up (backlight fade-in) is
 * triggered from scr_splash_init(): the screen engine calls it on the first
 * UI tick, when it detects the splash as the active screen. Screen load
 * events are used for the splash -> home transition, which happens with the
 * callbacks already registered.
 */

void scr_splash_prepare(void)
{
}

void scr_splash_init(void)
{
    ESP_LOGD(TAG, "init");
    bsp_display_fade_in(SPLASH_FADE_IN_MS);
}

void scr_splash_uninit(void)
{
    ESP_LOGD(TAG, "uninit");
}

void scr_splash_step(void)
{
}
