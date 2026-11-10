#include "ds3231.h"
#include <stdlib.h>
#include <string.h>

#define DS3231_ADDR        (0x68U << 1)
#define DS3231_TIMEOUT_MS  20U

#define REG_TIME           0x00U
#define REG_STATUS         0x0FU
#define STATUS_OSF_BIT     0x80U
#define TIME_REG_COUNT     7U

static I2C_HandleTypeDef *s_hi2c = NULL;

static uint8_t bcd2bin(uint8_t v) { return (uint8_t)(((v >> 4) * 10U) + (v & 0x0FU)); }
static uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10U) << 4) | (v % 10U)); }

static status_t map_hal(HAL_StatusTypeDef r)
{
    if (r == HAL_OK)      return STATUS_OK;
    if (r == HAL_TIMEOUT) return STATUS_ERR_TIMEOUT;
    return STATUS_ERR_I2C;
}

static status_t reg_read(uint8_t reg, uint8_t *buf, uint16_t len)
{
    return map_hal(HAL_I2C_Mem_Read(s_hi2c, DS3231_ADDR, reg,
                   I2C_MEMADD_SIZE_8BIT, buf, len, DS3231_TIMEOUT_MS));
}

static status_t reg_write(uint8_t reg, uint8_t *buf, uint16_t len)
{
    return map_hal(HAL_I2C_Mem_Write(s_hi2c, DS3231_ADDR, reg,
                   I2C_MEMADD_SIZE_8BIT, buf, len, DS3231_TIMEOUT_MS));
}

static bool time_is_valid(const rtc_time_t *t)
{
    return (t->year >= 2000U && t->year <= 2099U) &&
           (t->month >= 1U && t->month <= 12U) &&
           (t->day   >= 1U && t->day   <= 31U) &&
           (t->hour <= 23U) && (t->min <= 59U) && (t->sec <= 59U);
}

status_t ds3231_init(I2C_HandleTypeDef *hi2c, bool *time_lost)
{
    uint8_t st;
    s_hi2c = hi2c;

    status_t r = reg_read(REG_STATUS, &st, 1);
    if (r != STATUS_OK) {
        s_hi2c = NULL;
        return r;
    }
    *time_lost = (st & STATUS_OSF_BIT) != 0U;
    return STATUS_OK;
}

status_t ds3231_get_time(rtc_time_t *t)
{
    uint8_t raw[TIME_REG_COUNT];

    if (s_hi2c == NULL) return STATUS_ERR_NOT_READY;

    status_t r = reg_read(REG_TIME, raw, TIME_REG_COUNT);
    if (r != STATUS_OK) return r;

    t->sec   = bcd2bin(raw[0] & 0x7FU);
    t->min   = bcd2bin(raw[1] & 0x7FU);
    t->hour  = bcd2bin(raw[2] & 0x3FU);
    /* raw[3] day of the week - not use*/
    t->day   = bcd2bin(raw[4] & 0x3FU);
    t->month = bcd2bin(raw[5] & 0x1FU);
    t->year  = (uint16_t)(2000U + bcd2bin(raw[6]));

    return time_is_valid(t) ? STATUS_OK : STATUS_ERR_INVALID;
}

status_t ds3231_set_time(const rtc_time_t *t)
{
    if (s_hi2c == NULL)      return STATUS_ERR_NOT_READY;
    if (!time_is_valid(t))   return STATUS_ERR_INVALID;

    uint8_t raw[TIME_REG_COUNT] = {
        bin2bcd(t->sec), bin2bcd(t->min), bin2bcd(t->hour),
        1U, /* day of the week */
        bin2bcd(t->day), bin2bcd(t->month),
        bin2bcd((uint8_t)(t->year - 2000U))
    };

    status_t r = reg_write(REG_TIME, raw, TIME_REG_COUNT);
    if (r != STATUS_OK) return r;

    uint8_t st;
    r = reg_read(REG_STATUS, &st, 1);
    if (r != STATUS_OK) return r;
    st &= (uint8_t)~STATUS_OSF_BIT;
    return reg_write(REG_STATUS, &st, 1);
}

void ds3231_build_time(rtc_time_t *t)
{
    static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char d[] = __DATE__;   /* "Mmm dd yyyy" */
    const char s[] = __TIME__;   /* "hh:mm:ss"   */
    char m[4] = { d[0], d[1], d[2], '\0' };
    const char *p = strstr(months, m);

    t->month = (uint8_t)(p ? ((p - months) / 3 + 1) : 1);
    t->day   = (uint8_t)((d[4] == ' ' ? 0 : d[4] - '0') * 10 + (d[5] - '0'));
    t->year  = (uint16_t)atoi(&d[7]);
    t->hour  = (uint8_t)((s[0] - '0') * 10 + (s[1] - '0'));
    t->min   = (uint8_t)((s[3] - '0') * 10 + (s[4] - '0'));
    t->sec   = (uint8_t)((s[6] - '0') * 10 + (s[7] - '0'));
}
