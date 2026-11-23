#ifndef __BSP_BUTTON_H__
#define __BSP_BUTTON_H__

#include "esp_err.h"
#include "driver/gpio.h"

/* Physical button: BOOT key. Change here if the board schematic differs.
 * GPIO0 is a strapping pin: reading it after boot is fine, but holding it
 * pressed while resetting enters download mode. RESET cannot be used. */
#define BSP_BUTTON_GPIO          GPIO_NUM_0     /* BOOT */
#define BSP_BUTTON_ACTIVE_LEVEL  0              /* active low, internal pull-up */

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*bsp_button_cb_t)(void *arg);

/**
 * @brief Create the GPIO button (espressif/button component).
 * @return ESP_OK, or the error returned by the button component.
 */
esp_err_t bsp_button_init(void);

/**
 * @brief Register the single-click callback (BUTTON_SINGLE_CLICK).
 *
 * @warning The callback runs in the esp_timer task of the espressif/button
 *          component, NOT in LVGL context: it must never call lv_* functions.
 *          Forward the event to the UI with ui_update_cmd() instead.
 *
 * @param cb  Callback (NULL to unregister).
 * @param arg User argument passed to the callback.
 * @return ESP_OK, or ESP_ERR_INVALID_STATE if bsp_button_init() was not called.
 */
esp_err_t bsp_button_register_click_cb(bsp_button_cb_t cb, void *arg);

#ifdef __cplusplus
}
#endif

#endif
