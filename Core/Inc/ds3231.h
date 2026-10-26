#ifndef DS3231_H
#define DS3231_H

#include "main.h"
#include <stdbool.h>
#include "app_status.h"

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
} rtc_time_t;

status_t ds3231_init(I2C_HandleTypeDef *hi2c, bool *time_lost);
status_t ds3231_get_time(rtc_time_t *t);
status_t ds3231_set_time(const rtc_time_t *t);

void ds3231_build_time(rtc_time_t *t);

#endif /* DS3231_H */
