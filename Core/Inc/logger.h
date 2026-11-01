#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include "app_status.h"

#define LOGGER_INTERVAL_MIN_S   1U
#define LOGGER_INTERVAL_MAX_S   3600U
#define LOGGER_INTERVAL_DEF_S   5U

void logger_init(void);
void logger_on_tick(void);
void logger_task(void);

void     logger_on_tick(void);

status_t logger_set_interval(uint32_t seconds);
uint32_t logger_get_interval(void);
uint32_t logger_get_err_cnt(void);
void     logger_clear_errors(void);

#endif /* LOGGER_H */
