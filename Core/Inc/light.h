#ifndef LIGHT_H
#define LIGHT_H

#include "main.h"
#include "app_status.h"

typedef struct {
    uint16_t raw;
    float    lux;
} light_data_t;

status_t light_init(ADC_HandleTypeDef *hadc);
status_t light_read(light_data_t *out);

#endif /* LIGHT_H */
