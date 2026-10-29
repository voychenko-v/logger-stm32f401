#ifndef LOG_ENTRY_H
#define LOG_ENTRY_H

#include <stdint.h>
#include "ds3231.h"

typedef struct {
    rtc_time_t ts;
    float      temp_c;
    float      hum_pct;
    float      press_hpa;
    uint32_t   err_cnt;
} log_entry_t;

#endif /* LOG_ENTRY_H */
