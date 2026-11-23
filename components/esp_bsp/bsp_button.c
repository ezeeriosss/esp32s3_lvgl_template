#include "bsp_button.h"

#include "iot_button.h"
#include "button_gpio.h"
#include "esp_check.h"
#include "esp_log.h"

/*
 * espressif/button 4.x API (checked against the resolved version 4.2.1):
 *   iot_button_new_gpio_device(&button_config_t, &button_gpio_config_t, &handle)
 *   iot_button_register_cb(handle, event, event_args, cb, usr_data)
 *   callback: void cb(void *button_handle, void *usr_data)
 */

static const char *TAG = "bsp_button";

static button_handle_t          s_button = NULL;
static volatile bsp_button_cb_t s_click_cb = NULL;
static void *volatile           s_click_arg = NULL;

/* Runs in the esp_timer task of the button component (not LVGL context). */
static void on_single_click(void *button_handle, void *usr_data)
{
    (void)button_handle;
    (void)usr_data;
    bsp_button_cb_t cb = s_click_cb;
    if (cb) {
        cb(s_click_arg);
    }
}

esp_err_t bsp_button_init(void)
{
    if (s_button != NULL) {
        return ESP_OK;
    }

    const button_config_t btn_cfg = { 0 };   /* default long/short press times */
    const button_gpio_config_t gpio_cfg = {
        .gpio_num     = BSP_BUTTON_GPIO,
        .active_level = BSP_BUTTON_ACTIVE_LEVEL,
    };
    ESP_RETURN_ON_ERROR(iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &s_button),
                        TAG, "create GPIO button");
    ESP_RETURN_ON_ERROR(iot_button_register_cb(s_button, BUTTON_SINGLE_CLICK, NULL,
                                               on_single_click, NULL),
                        TAG, "register single click");

    ESP_LOGI(TAG, "Button ready on GPIO%d", (int)BSP_BUTTON_GPIO);
    return ESP_OK;
}

esp_err_t bsp_button_register_click_cb(bsp_button_cb_t cb, void *arg)
{
    if (s_button == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    s_click_arg = arg;      /* argument first, so the callback never sees a stale one */
    s_click_cb = cb;
    return ESP_OK;
}
