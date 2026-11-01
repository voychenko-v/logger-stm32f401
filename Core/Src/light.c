#include "light.h"
#include <math.h>

#define LIGHT_SAMPLES        16U
#define ADC_MAX              4095U

#define ADC_FAULT_LOW        5U
#define ADC_FAULT_HIGH       4090U

#define LIGHT_R_FIXED_OHM    10000.0f
#define LIGHT_R10_OHM        8000.0f
#define LIGHT_GAMMA          0.6f
#define LIGHT_LUX_REF        10.0f
#define LIGHT_LUX_MAX        100000.0f

static ADC_HandleTypeDef *s_hadc = NULL;
static uint16_t s_dma_buf[LIGHT_SAMPLES];

status_t light_init(ADC_HandleTypeDef *hadc)
{
    if (HAL_ADC_Start_DMA(hadc, (uint32_t *)s_dma_buf, LIGHT_SAMPLES) != HAL_OK) {
        s_hadc = NULL;
        return STATUS_ERR_ADC;
    }

    __HAL_DMA_DISABLE_IT(hadc->DMA_Handle, DMA_IT_TC | DMA_IT_HT);

    s_hadc = hadc;
    return STATUS_OK;
}

status_t light_read(light_data_t *out)
{
    if (s_hadc == NULL) {
        return STATUS_ERR_NOT_READY;
    }

    uint32_t sum = 0;
    for (uint32_t i = 0; i < LIGHT_SAMPLES; i++) {
        sum += ((volatile uint16_t *)s_dma_buf)[i];
    }
    uint16_t raw = (uint16_t)(sum / LIGHT_SAMPLES);

    if (raw < ADC_FAULT_LOW || raw > ADC_FAULT_HIGH) {
        return STATUS_ERR_INVALID;
    }

    float r_ldr = LIGHT_R_FIXED_OHM * (float)(ADC_MAX - raw) / (float)raw;
    float lux   = LIGHT_LUX_REF * powf(LIGHT_R10_OHM / r_ldr, 1.0f / LIGHT_GAMMA);

    if (lux > LIGHT_LUX_MAX) {
        lux = LIGHT_LUX_MAX;
    }

    out->raw = raw;
    out->lux = lux;
    return STATUS_OK;
}
