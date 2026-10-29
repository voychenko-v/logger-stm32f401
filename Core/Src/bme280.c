#include "bme280.h"

#define BME280_ADDR         (BME280_I2C_ADDR << 1)
#define BME280_TIMEOUT_MS   20U
#define BME280_MEAS_TIMEOUT_MS 30U

#define BME280_CHIP_ID      0x60U

#define REG_ID              0xD0U
#define REG_RESET           0xE0U
#define REG_CTRL_HUM        0xF2U
#define REG_STATUS          0xF3U
#define REG_CTRL_MEAS       0xF4U
#define REG_CONFIG          0xF5U
#define REG_DATA            0xF7U
#define REG_CALIB_1         0x88U
#define REG_CALIB_2         0xE1U

#define RESET_CMD           0xB6U
#define STATUS_MEASURING    0x08U
#define CTRL_HUM_OSRS_X1    0x01U
#define CTRL_MEAS_FORCED_X1 0x25U

#define TEMP_MIN_C   (-40.0f)
#define TEMP_MAX_C   (85.0f)
#define HUM_MIN_PCT  0.0f
#define HUM_MAX_PCT  100.0f
#define PRESS_MIN_HPA 300.0f
#define PRESS_MAX_HPA 1100.0f

#define ADC_SKIPPED_20BIT  0x80000
#define ADC_SKIPPED_16BIT  0x8000

typedef struct {
    uint16_t T1; int16_t T2, T3;
    uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
    uint8_t  H1, H3; int16_t H2, H4, H5; int8_t H6;
} calib_t;

static I2C_HandleTypeDef *s_hi2c = NULL;
static calib_t s_cal;

static status_t map_hal(HAL_StatusTypeDef r)
{
    if (r == HAL_OK)      return STATUS_OK;
    if (r == HAL_TIMEOUT) return STATUS_ERR_TIMEOUT;
    return STATUS_ERR_I2C;
}

static status_t reg_read(uint8_t reg, uint8_t *buf, uint16_t len)
{
    return map_hal(HAL_I2C_Mem_Read(s_hi2c, BME280_ADDR, reg,
                   I2C_MEMADD_SIZE_8BIT, buf, len, BME280_TIMEOUT_MS));
}

static status_t reg_write8(uint8_t reg, uint8_t val)
{
    return map_hal(HAL_I2C_Mem_Write(s_hi2c, BME280_ADDR, reg,
                   I2C_MEMADD_SIZE_8BIT, &val, 1, BME280_TIMEOUT_MS));
}

static uint16_t u16le(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static int16_t  s16le(const uint8_t *p) { return (int16_t)u16le(p); }

static status_t load_calibration(void)
{
    uint8_t a[26], b[7];
    status_t r = reg_read(REG_CALIB_1, a, sizeof a);
    if (r != STATUS_OK) return r;
    r = reg_read(REG_CALIB_2, b, sizeof b);
    if (r != STATUS_OK) return r;

    s_cal.T1 = u16le(&a[0]);  s_cal.T2 = s16le(&a[2]);  s_cal.T3 = s16le(&a[4]);
    s_cal.P1 = u16le(&a[6]);  s_cal.P2 = s16le(&a[8]);  s_cal.P3 = s16le(&a[10]);
    s_cal.P4 = s16le(&a[12]); s_cal.P5 = s16le(&a[14]); s_cal.P6 = s16le(&a[16]);
    s_cal.P7 = s16le(&a[18]); s_cal.P8 = s16le(&a[20]); s_cal.P9 = s16le(&a[22]);
    s_cal.H1 = a[25];

    s_cal.H2 = s16le(&b[0]);
    s_cal.H3 = b[2];
    s_cal.H4 = (int16_t)((int16_t)(int8_t)b[3] * 16 | (b[4] & 0x0F));
    s_cal.H5 = (int16_t)((int16_t)(int8_t)b[5] * 16 | (b[4] >> 4));
    s_cal.H6 = (int8_t)b[6];
    return STATUS_OK;
}

static int32_t comp_temp(int32_t adc_T, int32_t *t_fine)
{
    int32_t v1 = ((((adc_T >> 3) - ((int32_t)s_cal.T1 << 1))) * (int32_t)s_cal.T2) >> 11;
    int32_t v2 = (((((adc_T >> 4) - (int32_t)s_cal.T1) *
                    ((adc_T >> 4) - (int32_t)s_cal.T1)) >> 12) * (int32_t)s_cal.T3) >> 14;
    *t_fine = v1 + v2;
    return (*t_fine * 5 + 128) >> 8;                 /* 0.01 °C */
}

static uint32_t comp_press(int32_t adc_P, int32_t t_fine)
{
    int32_t v1 = (t_fine >> 1) - 64000;
    int32_t v2 = (((v1 >> 2) * (v1 >> 2)) >> 11) * (int32_t)s_cal.P6;
    v2 = v2 + ((v1 * (int32_t)s_cal.P5) << 1);
    v2 = (v2 >> 2) + ((int32_t)s_cal.P4 << 16);
    v1 = (((s_cal.P3 * (((v1 >> 2) * (v1 >> 2)) >> 13)) >> 3) +
          (((int32_t)s_cal.P2 * v1) >> 1)) >> 18;
    v1 = ((32768 + v1) * (int32_t)s_cal.P1) >> 15;
    if (v1 == 0) return 0;

    uint32_t p = (((uint32_t)(1048576 - adc_P) - (uint32_t)(v2 >> 12))) * 3125U;
    p = (p < 0x80000000U) ? ((p << 1) / (uint32_t)v1) : ((p / (uint32_t)v1) * 2U);
    v1 = ((int32_t)s_cal.P9 * (int32_t)(((p >> 3) * (p >> 3)) >> 13)) >> 12;
    v2 = ((int32_t)(p >> 2) * (int32_t)s_cal.P8) >> 13;
    return (uint32_t)((int32_t)p + ((v1 + v2 + (int32_t)s_cal.P7) >> 4));   /* Па */
}

static uint32_t comp_hum(int32_t adc_H, int32_t t_fine)
{
    int32_t v = t_fine - 76800;
    v = (((((adc_H << 14) - ((int32_t)s_cal.H4 << 20) - ((int32_t)s_cal.H5 * v)) +
           16384) >> 15) *
         (((((((v * (int32_t)s_cal.H6) >> 10) *
              (((v * (int32_t)s_cal.H3) >> 11) + 32768)) >> 10) + 2097152) *
           (int32_t)s_cal.H2 + 8192) >> 14));
    v = v - (((((v >> 15) * (v >> 15)) >> 7) * (int32_t)s_cal.H1) >> 4);
    if (v < 0) v = 0;
    if (v > 419430400) v = 419430400;
    return (uint32_t)(v >> 12);                      /* %RH * 1024 */
}

status_t bme280_init(I2C_HandleTypeDef *hi2c, uint8_t *chip_id)
{
    s_hi2c = hi2c;

    status_t r = reg_read(REG_ID, chip_id, 1);
    if (r != STATUS_OK) { s_hi2c = NULL; return r; }

    if (*chip_id != BME280_CHIP_ID) { s_hi2c = NULL; return STATUS_ERR_INVALID; }

    r = reg_write8(REG_RESET, RESET_CMD);
    if (r != STATUS_OK) { s_hi2c = NULL; return r; }
    HAL_Delay(5);

    r = load_calibration();
    if (r != STATUS_OK) { s_hi2c = NULL; }
    return r;
}

status_t bme280_start(void)
{
    if (s_hi2c == NULL) return STATUS_ERR_NOT_READY;

    status_t r = reg_write8(REG_CTRL_HUM, CTRL_HUM_OSRS_X1);
    if (r != STATUS_OK) return r;
    return reg_write8(REG_CTRL_MEAS, CTRL_MEAS_FORCED_X1);
}

status_t bme280_fetch(bme280_data_t *out)
{
    if (s_hi2c == NULL) return STATUS_ERR_NOT_READY;

    uint8_t d[8];
    status_t r = reg_read(REG_DATA, d, sizeof d);
    if (r != STATUS_OK) return r;

    int32_t adc_P = (int32_t)(((uint32_t)d[0] << 12) | ((uint32_t)d[1] << 4) | (d[2] >> 4));
    int32_t adc_T = (int32_t)(((uint32_t)d[3] << 12) | ((uint32_t)d[4] << 4) | (d[5] >> 4));
    int32_t adc_H = (int32_t)(((uint32_t)d[6] << 8)  |  d[7]);

    if (adc_P == ADC_SKIPPED_20BIT || adc_T == ADC_SKIPPED_20BIT ||
        adc_H == ADC_SKIPPED_16BIT) {
        return STATUS_ERR_INVALID;
    }

    int32_t t_fine;
    out->temp_c    = (float)comp_temp(adc_T, &t_fine) / 100.0f;
    out->press_hpa = (float)comp_press(adc_P, t_fine) / 100.0f;
    out->hum_pct   = (float)comp_hum(adc_H, t_fine) / 1024.0f;

    if (out->temp_c < TEMP_MIN_C || out->temp_c > TEMP_MAX_C ||
        out->hum_pct < HUM_MIN_PCT || out->hum_pct > HUM_MAX_PCT ||
        out->press_hpa < PRESS_MIN_HPA || out->press_hpa > PRESS_MAX_HPA) {
        return STATUS_ERR_INVALID;
    }
    return STATUS_OK;
}
