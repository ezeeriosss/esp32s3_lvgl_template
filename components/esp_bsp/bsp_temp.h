#ifndef __BSP_TEMP_H__
#define __BSP_TEMP_H__

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Install and enable the ESP32-S3 internal temperature sensor.
 *
 * Measurement range is configured for -10 .. 80 °C. Safe to call more than
 * once (subsequent calls return ESP_OK without reinstalling).
 *
 * @return ESP_OK on success, or the error returned by the driver.
 */
esp_err_t bsp_temp_init(void);

/**
 * @brief Read the chip temperature in degrees Celsius.
 *
 * @param[out] celsius Where to store the reading.
 * @return ESP_OK on success,
 *         ESP_ERR_INVALID_STATE if bsp_temp_init() was not called (or failed),
 *         ESP_ERR_INVALID_ARG if celsius is NULL,
 *         or the error returned by the driver.
 */
esp_err_t bsp_temp_get_celsius(float *celsius);

#ifdef __cplusplus
}
#endif

#endif
