#include "scr_system.h"
#include "screens.h"
#include "ui.h"             /* Squareline widgets: ui_scrSystem and its children */
#include "esp_log.h"

static const char *TAG = "scr_system";

void scr_system_prepare(void)
{
}

void scr_system_init(void)
{
    ESP_LOGD(TAG, "init");
}

void scr_system_uninit(void)
{
    ESP_LOGD(TAG, "uninit");
}

void scr_system_step(void)
{
}
