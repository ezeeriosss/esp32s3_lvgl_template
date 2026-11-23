#ifndef __BSP_BATTERY_H__
#define __BSP_BATTERY_H__
#include <stdio.h>

#define BSP_BATTERY_ADC_CHANNEL   ADC_CHANNEL_4
#define BSP_BATTERY_ADC_ATTEN     ADC_ATTEN_DB_12
#define BSP_BATTERY_ADC_UNIT      ADC_UNIT_1

#ifdef __cplusplus
extern "C" {
#endif

void bsp_battery_init(void);
void bsp_battery_get_voltage(float *voltage, uint16_t *adc_value);

/**
 * @brief Estimate the battery charge in percent (Li-ion discharge curve).
 *
 * Averages 8 consecutive ADC readings and maps the voltage with a piecewise
 * linear Li-ion curve (4.20 V = 100 %, 3.30 V = 0 %), clamped to 0..100.
 *
 * @param[out] out_voltage Optional (may be NULL). Receives the averaged
 *                         voltage, or -1.0f when there is no valid reading.
 * @return 0..100, or -1 if there is no valid reading (battery not
 *         initialized or ADC calibration unavailable).
 */
int bsp_battery_get_percent(float *out_voltage);

#ifdef __cplusplus
}
#endif


#endif