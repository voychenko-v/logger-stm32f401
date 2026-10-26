#ifndef BME280_H
#define BME280_H

#include "main.h"
#include "app_status.h"

#define BME280_I2C_ADDR   0x76U

typedef struct {
    float temp_c;
    float hum_pct;
    float press_hpa;
} bme280_data_t;

status_t bme280_init(I2C_HandleTypeDef *hi2c, uint8_t *chip_id);
status_t bme280_read(bme280_data_t *out);

#endif /* BME280_H */
