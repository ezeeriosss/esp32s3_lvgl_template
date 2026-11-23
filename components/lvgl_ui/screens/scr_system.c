#include "scr_system.h"
#include "screens.h"
#include "ui.h"             /* Squareline widgets: ui_scrSystem and its children */
#include "esp_system.h"     /* esp_get_free_heap_size(), esp_get_idf_version() */
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_private/esp_clk.h"
#include "esp_log.h"

static const char *TAG = "scr_system";

#define SYSTEM_REFRESH_PERIOD_MS 1000

/*
 * Static data (IDF version, chip, flash) is written once in prepare().
 * A 1 s lv_timer created in prepare() keeps running even when the screen is
 * not active; it only touches the labels while System is the active screen.
 */
static lv_timer_t *s_timer = NULL;

/* ------------------------------------------------------------------ */

static const char *chip_model_name(esp_chip_model_t model)
{
    switch (model) {
        case CHIP_ESP32S3: return "ESP32-S3";
        case CHIP_ESP32S2: return "ESP32-S2";
        case CHIP_ESP32C3: return "ESP32-C3";
        case CHIP_ESP32:   return "ESP32";
        default:           return "ESP32-?";
    }
}

static void fill_static_info(void)
{
    lv_label_set_text_fmt(ui_lblIdfVer, "IDF: %s", esp_get_idf_version());

    esp_chip_info_t info;
    esp_chip_info(&info);
    /* revision format MXX: M = major, XX = minor */
    lv_label_set_text_fmt(ui_lblChip, "Chip: %s rev v%d.%d, %d nucleos",
                          chip_model_name(info.model),
                          info.revision / 100, info.revision % 100,
                          (int)info.cores);

    uint32_t flash_size = 0;
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        lv_label_set_text_fmt(ui_lblFlash, "Flash: %lu MB",
                              (unsigned long)(flash_size / (1024UL * 1024UL)));
    } else {
        lv_label_set_text_static(ui_lblFlash, "Flash: --");
    }
}

static void refresh_dynamic(void)
{
    lv_label_set_text_fmt(ui_lblCpuFreq, "CPU: %d MHz", esp_clk_cpu_freq() / 1000000);
    lv_label_set_text_fmt(ui_lblHeapFree, "Heap libre: %lu KB",
                          (unsigned long)(esp_get_free_heap_size() / 1024UL));
    lv_label_set_text_fmt(ui_lblHeapMin, "Heap minimo: %lu KB",
                          (unsigned long)(esp_get_minimum_free_heap_size() / 1024UL));
}

static void system_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (ui_screens_get_current() == SCREEN_SYSTEM) {
        refresh_dynamic();
    }
}

/* ------------------------------------------------------------------ */

void scr_system_prepare(void)
{
    fill_static_info();
    if (s_timer == NULL) {
        s_timer = lv_timer_create(system_timer_cb, SYSTEM_REFRESH_PERIOD_MS, NULL);
    }

    lv_obj_add_event_cb(ui_scrSystem, ui_screens_gesture_cb, LV_EVENT_GESTURE, NULL);
}

void scr_system_init(void)
{
    ESP_LOGD(TAG, "init");
    refresh_dynamic();      /* no "--" while waiting for the first timer tick */
}

void scr_system_uninit(void)
{
    ESP_LOGD(TAG, "uninit");
}

void scr_system_step(void)
{
}
